#include "Allocator.hpp"

#include "Logger.hpp"

namespace fer
{

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

static size_t totalAllocReq = 0, totalManualAlloc = 0, totalPoolAlloc = 0, reuseCount = 0;
constexpr size_t SIZE_BYTES = sizeof(size_t);

MemoryManager::MemoryManager(StringRef name) : name(name) { allocPool(); }
MemoryManager::~MemoryManager()
{
	// clear out the allocations that are larger than MAX_ROUNDUP
	for(auto &c : freechunks) {
		if(c.first > POOL_SIZE) {
			for(auto &blk : c.second) {
				delete[] blk;
			}
		}
		c.second.clear();
	}
	freechunks.clear();
	for(auto &p : pools) delete[] p.mem;
	logger.trace("=============== ", name, " memory manager stats: ===============");
	logger.trace("-- Total allocated bytes (pools + otherwise): ", totalManualAlloc);
	logger.trace("--             Total allocated bytes (pools): ", totalPoolAlloc);
	logger.trace("--                          Requests (count): ", totalAllocReq);
	logger.trace("--                            Reuses (count): ", reuseCount);
}

size_t MemoryManager::mult8Roundup(size_t sz)
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
	char *alloc = new char[POOL_SIZE];
	totalManualAlloc += POOL_SIZE;
	pools.push_back({alloc, alloc});
}

void *MemoryManager::alloc(size_t sz)
{
	if(sz == 0) return nullptr;

	sz += SIZE_BYTES; // for storing the size in the first size_t bytes.
	sz = mult8Roundup(sz);

	++totalAllocReq;

	char *loc = nullptr;

	if(sz > POOL_SIZE) {
		totalManualAlloc += sz;
		loc = new char[sz];
	} else {
		totalPoolAlloc += sz;

		LockGuard<Mutex> mtxlock(mtx);
		// there is a free chunk available in the chunk list
		auto &freechunkloc = freechunks[sz];
		if(!freechunkloc.empty()) {
			loc = freechunkloc.front();
			freechunkloc.pop_front();
			++reuseCount;
			// No need to size size bytes here because they would have already been set
			// when they were taken from the pool.
			return loc;
		}

		// fetch a chunk from the pool
		for(auto &p : pools) {
			size_t freespace = POOL_SIZE - (p.head - p.mem);
			if(freespace >= sz) {
				loc = p.head;
				p.head += sz;
				break;
			}
		}
		if(!loc) {
			allocPool();
			auto &p = pools.back();
			loc	= p.head;
			p.head += sz;
		}
	}
	*((size_t *)loc) = sz;
	return loc + SIZE_BYTES;
}

void MemoryManager::free(void *data)
{
	if(data == nullptr) return;
	char *loc = (char *)data;
	size_t sz = *((size_t *)(loc - SIZE_BYTES));
	if(sz > POOL_SIZE) {
		delete[](loc - SIZE_BYTES);
		return;
	}
	LockGuard<Mutex> mtxlock(mtx);
	freechunks[sz].push_front(loc);
}

} // namespace fer