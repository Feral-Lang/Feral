#include "Allocator.hpp"

#include "Core.hpp"
#include "Logger.hpp"

// aligned_alloc doesn't exist on Windows, so we use _aligned_malloc and _aligned_free instead.
#if defined(FER_OS_WINDOWS)
#include <malloc.h>
#define AlignedAlloc(align, sz) _aligned_malloc(sz, align)
#define AlignedFree(ptr) _aligned_free(ptr)
#else
#define AlignedAlloc(align, sz) std::aligned_alloc(align, sz)
#define AlignedFree(ptr) std::free(ptr)
#endif

namespace fer
{

static Atomic<size_t> totalAllocRequests = 0, totalAllocBytes = 0, totalPoolAlloc = 0,
		      chunkReuseCount = 0;

// alloc address must be AFTER sizeof(AllocDetail)
inline void setAllocDetail(size_t alloc, size_t memSz, size_t next)
{
	*(AllocDetail *)((char *)alloc - ALLOC_DETAIL_BYTES) = {memSz, next};
}
// alloc address must be AFTER sizeof(AllocDetail)
inline void setAllocDetailNext(size_t alloc, size_t next)
{
	(*(AllocDetail *)((char *)alloc - ALLOC_DETAIL_BYTES)).next = next;
}
// alloc address must be AFTER sizeof(AllocDetail)
inline void setAllocDetailMemSz(size_t alloc, size_t memSz)
{
	(*(AllocDetail *)((char *)alloc - ALLOC_DETAIL_BYTES)).memSz = memSz;
}

// alloc address must be AFTER sizeof(AllocDetail)
inline AllocDetail getAllocDetail(size_t alloc)
{
	return *(AllocDetail *)((char *)alloc - ALLOC_DETAIL_BYTES);
}
// alloc address must be AFTER sizeof(AllocDetail)
inline size_t getAllocDetailNext(size_t alloc)
{
	return (*(AllocDetail *)((char *)alloc - ALLOC_DETAIL_BYTES)).next;
}
// alloc address must be AFTER sizeof(AllocDetail)
inline size_t getAllocDetailMemSz(size_t alloc)
{
	return (*(AllocDetail *)((char *)alloc - ALLOC_DETAIL_BYTES)).memSz;
}

MemoryManager::MemoryManager(StringRef name) : name(name) { allocPool(); }
MemoryManager::~MemoryManager()
{
	// clear out the allocations that are larger than MAX_ROUNDUP
	for(auto &it : freechunks) {
		if(it.first > POOL_SIZE || it.second == 0) continue;
		size_t allocAddr = it.second;
		AllocDetail detail;
		while(allocAddr > 0) {
			detail = getAllocDetail(allocAddr);
			if(detail.memSz <= POOL_SIZE) break;
			AlignedFree((char *)allocAddr - ALLOC_DETAIL_BYTES);
			allocAddr = detail.next;
		}
	}
	freechunks.clear();
	for(auto &p : pools) AlignedFree(p.mem);
	logger.info("=============== ", name, " memory manager stats: ===============");
	logger.info("-- Total allocated bytes (pools + otherwise): ", totalAllocBytes.load());
	logger.info("--                Allocated bytes from pools: ", totalPoolAlloc.load());
	logger.info("--                             Request count: ", totalAllocRequests.load());
	logger.info("--                         Chunk Reuse count: ", chunkReuseCount.load());
}

size_t MemoryManager::nextPow2(size_t sz)
{
	if(sz > MAX_ROUNDUP) return sz;
	--sz;
	sz |= sz >> 1;
	sz |= sz >> 2;
	sz |= sz >> 4;
	sz |= sz >> 8;
	sz |= sz >> 16;
	return ++sz;
}

void MemoryManager::allocPool()
{
	char *alloc = (char *)AlignedAlloc(MAX_ALIGNMENT, POOL_SIZE);
	totalAllocBytes += POOL_SIZE;
	pools.emplace_back(alloc, alloc);
}

void *MemoryManager::alloc(size_t size, size_t align)
{
	// align is unused for now.
	if(size == 0) return nullptr;

	// Add ALLOC_DETAIL_BYTES to the size since it is guaranteed
	// (static_assert) to be a multiple of MAX_ALIGNMENT.
	size_t requiredSz = size + ALLOC_DETAIL_BYTES;
	size_t allocSz	  = nextPow2(requiredSz);

	logger.trace("Allocating: ", allocSz, " (required size: ", requiredSz,
		     ") (original size: ", size, ")\n");

	char *loc = nullptr;

	++totalAllocRequests;
	if(allocSz > POOL_SIZE) {
		totalAllocBytes += allocSz;
		loc = (char *)AlignedAlloc(MAX_ALIGNMENT, allocSz);
	} else {
		totalPoolAlloc += allocSz;
		LockGuard<RecursiveMutex> mtxlock(mtx);
		// there is a free chunk available in the chunk list
		size_t addr = 0;
		auto it	    = freechunks.find(allocSz);
		if(it != freechunks.end()) addr = it->second;
		if(addr != 0) {
			loc	   = (char *)addr;
			it->second = getAllocDetailNext(addr);
			setAllocDetailNext(addr, 0);
			++chunkReuseCount;
			// No need to size size bytes here because they would have already been set
			// when they were taken from the pool.
			return loc;
		}

		// fetch a chunk from the pool
		for(auto &p : pools) {
			size_t freespace = POOL_SIZE - (p.head - p.mem);
			if(freespace >= allocSz) {
				loc = p.head;
				p.head += allocSz;
				break;
			}
		}
		if(!loc) {
			allocPool();
			auto &p = pools.back();
			loc	= p.head;
			p.head += allocSz;
		}
	}
	loc += ALLOC_DETAIL_BYTES;
	setAllocDetail((size_t)loc, allocSz, 0);
	return loc;
}

void MemoryManager::free(void *data)
{
	if(data == nullptr) return;
	char *loc = (char *)data;
	size_t sz = getAllocDetailMemSz((size_t)loc);
	if(sz > POOL_SIZE) {
		AlignedFree(loc - ALLOC_DETAIL_BYTES);
		return;
	}
	LockGuard<RecursiveMutex> mtxlock(mtx);
	auto mapLoc = freechunks.find(sz);
	if(mapLoc == freechunks.end()) {
		freechunks.insert({sz, (size_t)loc});
		return;
	}
	setAllocDetailNext((size_t)loc, mapLoc->second);
	mapLoc->second = (size_t)loc;
}

void MemoryManager::dumpMem(char *pool)
{
	constexpr size_t charSize     = 2; // in bytes
	constexpr size_t charsPerLine = 64 * charSize;
	for(size_t i = 0; i < POOL_SIZE; i += charSize) {
		if(i % charsPerLine == 0) std::cout << "\n" << (void *)(pool + i) << " :: ";
		std::cout << std::hex << (*(uint16_t *)(pool + i)) << " ";
	}
	std::cout << std::dec << "\n";
}

IAllocated::IAllocated() {}
IAllocated::~IAllocated() {}

Allocator::Allocator(MemoryManager &mem, StringRef name) : mem(mem), name(name) {}
Allocator::~Allocator()
{
	size_t count = 0;
	for(auto &m : allocs) {
		++count;
		m->~IAllocated();
		mem.free(m);
	}
	logger.trace(name, " allocator had ", count, " allocations");
}

} // namespace fer