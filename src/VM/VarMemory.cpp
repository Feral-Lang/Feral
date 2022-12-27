#include "VM/VarMemory.hpp"

namespace fer
{

// TODO: have multiple arenas and then use mutexes individual to those arenas
// then, in a mutithreaded environment, the manager should be able to choose one of the available
// arenas and allocate memory from that (therefore increasing speed, compared to a global mutex).
static Mutex memmtx;

#ifdef MEM_PROFILE
static size_t tot_alloc	       = 0;
static size_t tot_alloc_nopool = 0;
static size_t tot_alloc_req    = 0;
static size_t tot_manual_alloc = 0;
#endif

VarMemory::VarMemory() { allocPool(); }
VarMemory::~VarMemory()
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
#ifdef MEM_PROFILE
	fprintf(stdout,
		"Total allocated: %zu bytes, without mempool: %zu, requests: %zu, manually "
		"allocated: %zu bytes\n",
		tot_alloc, tot_alloc_nopool, tot_alloc_req, tot_manual_alloc);
#endif
}

size_t VarMemory::mult8Roundup(size_t sz)
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

void VarMemory::allocPool()
{
	u8 *alloc = new u8[POOL_SIZE];
#ifdef MEM_PROFILE
	tot_alloc += POOL_SIZE;
#endif
	pools.push_back({alloc, alloc});
}

VarMemory &VarMemory::getInstance()
{
	static VarMemory varmem;
	return varmem;
}

void *VarMemory::alloc(size_t sz)
{
	if(sz == 0) return nullptr;

#ifdef MEM_PROFILE
	tot_alloc_nopool += sz;
	++tot_alloc_req;
#endif
	sz = mult8Roundup(sz);
	if(sz > POOL_SIZE) {
#if defined(MEM_PROFILE)
	#if defined(DEBUG_MODE)
		fprintf(stdout, "Allocating manually ... %zu\n", sz);
	#endif
		tot_manual_alloc += sz;
#endif
		return new u8[sz];
	}

	LockGuard<Mutex> mtxlock(memmtx);
	// there is a free chunk available in the chunk list
	auto &freechunkloc = freechunks[sz];
	if(!freechunkloc.empty()) {
		u8 *loc = freechunkloc.front();
		freechunkloc.pop_front();
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
		fprintf(stdout, "Using previously allocated ... %zu\n", sz);
#endif
		return loc;
	}

	// fetch a chunk from the pool
	for(auto &p : pools) {
		size_t freespace = POOL_SIZE - (p.head - p.mem);
		if(freespace >= sz) {
			u8 *loc = p.head;
			p.head += sz;
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
			fprintf(stdout, "Allocating from pool ... %zu\n", sz);
#endif
			return loc;
		}
	}
	allocPool();
	auto &p = pools.back();
	u8 *loc = p.head;
	p.head += sz;
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
	fprintf(stdout, "Allocating from NEW pool ... %zu\n", sz);
#endif
	return loc;
}

void VarMemory::free(void *data, size_t sz)
{
	if(data == nullptr || sz == 0) return;
	if(sz > POOL_SIZE) {
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
		fprintf(stdout, "Deleting manually ... %zu\n", sz);
#endif
		delete[](u8 *)data;
		return;
	}
	LockGuard<Mutex> mtxlock(memmtx);
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
	fprintf(stdout, "Giving back to pool ... %zu\n", sz);
#endif
	freechunks[sz].push_front((u8 *)data);
}

} // namespace fer