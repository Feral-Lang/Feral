#include "Allocator.hpp"

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

static size_t totalAllocRequests = 0, totalAllocBytes = 0, totalPoolAlloc = 0, chunkReuseCount = 0;

MemoryManager::MemoryManager(StringRef name) : name(name) { allocPool(); }
MemoryManager::~MemoryManager()
{
	// clear out the allocations that are larger than MAX_ROUNDUP
	for(auto &c : freechunks) {
		if(c.first > POOL_SIZE) {
			for(auto &blk : c.second) {
				AlignedFree(blk);
			}
		}
		c.second.clear();
	}
	freechunks.clear();
	for(auto &p : pools) AlignedFree(p.mem);
	logger.trace("=============== ", name, " memory manager stats: ===============");
	logger.trace("-- Total allocated bytes (pools + otherwise): ", totalAllocBytes);
	logger.trace("--                Allocated bytes from pools: ", totalPoolAlloc);
	logger.trace("--                             Request count: ", totalAllocRequests);
	logger.trace("--                         Chunk Reuse count: ", chunkReuseCount);
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

	// Add MAX_ALIGNMENT instead of SIZE_BYTES - since MAX_ALIGNMENT is guaranteed
	// (static_assert) to be a multiple of SIZE_BYTES.
	size_t allocSz = size + MAX_ALIGNMENT;
	allocSz	       = nextPow2(allocSz);

	++totalAllocRequests;

	char *loc = nullptr;

	if(allocSz > POOL_SIZE) {
		totalAllocBytes += allocSz;
		loc = (char *)AlignedAlloc(MAX_ALIGNMENT, allocSz);
	} else {
		totalPoolAlloc += allocSz;

		LockGuard<Mutex> mtxlock(mtx);
		// there is a free chunk available in the chunk list
		auto &freechunkloc = freechunks[allocSz];
		if(!freechunkloc.empty()) {
			loc = freechunkloc.front();
			freechunkloc.pop_front();
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
	loc += MAX_ALIGNMENT;
	*((size_t *)(loc - SIZE_BYTES)) = allocSz;
	return loc;
}

void MemoryManager::free(void *data)
{
	if(data == nullptr) return;
	char *loc = (char *)data;
	size_t sz = *((size_t *)(loc - SIZE_BYTES));
	if(sz > POOL_SIZE) {
		AlignedFree(loc - MAX_ALIGNMENT);
		return;
	}
	LockGuard<Mutex> mtxlock(mtx);
	freechunks[sz].push_front(loc);
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