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

static Atomic<size_t> totalAllocRequests = 0, totalAllocBytes = 0, totalPoolAlloc = 0,
                      chunkReuseCount = 0;

MemoryManager::MemoryManager(StringRef name) : freechunks({}), name(name)
{
    for(auto &sz : freechunks) sz = 0;
}
MemoryManager::~MemoryManager()
{
    if(DEFAULT_LOGGER.isLevelLoggable(LogLevels::INFO)) {
        // count free chunks at each index/size - by end of memory manager, this contains the true
        // allocation count ie, allocations - reuses
        LOG_INFO("=================== Freechunk Stats (real allocation count) ===================");
        for(size_t i = 0; i < freechunks.size(); ++i) {
            auto &sz = freechunks[i];
            if(sz == 0) continue;
            size_t allocAddr = sz;
            size_t count     = 0;
            while(allocAddr > 0) {
                ++count;
                allocAddr = getAllocDetail(allocAddr, AllocDetails::NEXT);
            }
            LOG_INFO("-- of bytes ", (1 << size_t(i + 1)), ": ", count, " allocations");
        }
    }
    for(auto &p : pools) AlignedFree(p.mem);
    LOG_INFO("======================== ", name, " memory manager stats: =======================");
    LOG_INFO("-- Total allocated bytes (pools + otherwise): ", totalAllocBytes.load());
    LOG_INFO("--                Allocated bytes from pools: ", totalPoolAlloc.load());
    LOG_INFO("--                                Pool count: ", pools.size());
    LOG_INFO("--                             Request count: ", totalAllocRequests.load());
    LOG_INFO("--                         Chunk Reuse count: ", chunkReuseCount.load());
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
    size_t poolSize = DEFAULT_POOL_SIZE;
    if(pools.empty()) pools.reserve(20);
    else poolSize = pools.back().sz;
    char *alloc = (char *)AlignedAlloc(MAX_ALIGNMENT, poolSize);
    totalAllocBytes += poolSize;
    pools.emplace_back(poolSize, alloc, alloc);
}

void *MemoryManager::allocRaw(size_t size)
{
    // align is unused for now.
    if(size == 0) return nullptr;

    // Add ALLOC_DETAIL_BYTES to the size since it is guaranteed
    // (static_assert) to be a multiple of MAX_ALIGNMENT.
    size_t requiredSz = size + ALLOC_DETAIL_BYTES;
    size_t allocSz    = nextPow2(requiredSz);

    LOG_TRACE("Allocating: ", allocSz, " (reqd size: ", requiredSz, ") (orig size: ", size, ")");

    char *loc = nullptr;

    ++totalAllocRequests;
    if(allocSz > MAX_ROUNDUP) {
        totalAllocBytes += allocSz;
        loc = (char *)AlignedAlloc(MAX_ALIGNMENT, allocSz);
        LOG_TRACE("Allocated ", allocSz,
                  " using malloc as it exceeds pool allocation size: ", MAX_ROUNDUP);
    } else {
        totalPoolAlloc += allocSz;
        size_t poolIndex = getIndexForAllocSize(allocSz);
        LockGuard<RecursiveMutex> mtxlock(mtx);
        // there is a free chunk available in the chunk list
        size_t &addrSz = freechunks[poolIndex];
        if(addrSz != 0) {
            loc            = (char *)addrSz;
            size_t nextTmp = getAllocDetail(addrSz, AllocDetails::NEXT);
            setAllocDetail(addrSz, AllocDetails::NEXT, 0);
            addrSz = nextTmp;
            ++chunkReuseCount;
            LOG_TRACE("Allocated ", allocSz, " using chunk list");
            // No need to size size bytes here because they would have already been set
            // when they were taken from the pool.
            return loc;
        }

        // fetch a chunk from the pool
        for(auto &p : pools) {
            size_t freespace = p.sz - (p.head - p.mem);
            if(freespace >= allocSz) {
                loc = p.head;
                p.head += allocSz;
                LOG_TRACE("Allocated ", allocSz, " using existing pool");
                break;
            }
        }
        if(!loc) {
            allocPool();
            auto &p = pools.back();
            loc     = p.head;
            p.head += allocSz;
            LOG_TRACE("Allocated ", allocSz, " using a newly generated pool");
        }
    }
    loc += ALLOC_DETAIL_BYTES;
    setAllocDetail((size_t)loc, AllocDetails::SIZE, allocSz);
    setAllocDetail((size_t)loc, AllocDetails::NEXT, 0);
    return loc;
}

void MemoryManager::freeRaw(void *data)
{
    if(data == nullptr) return;
    char *loc = (char *)data;
    size_t sz = getAllocDetail((size_t)loc, AllocDetails::SIZE);
    if(sz > MAX_ROUNDUP) {
        AlignedFree(loc - ALLOC_DETAIL_BYTES);
        return;
    }
    LockGuard<RecursiveMutex> mtxlock(mtx);
    size_t idx     = getIndexForAllocSize(sz);
    size_t &addrSz = freechunks[idx];
    setAllocDetail((size_t)loc, AllocDetails::NEXT, addrSz);
    addrSz = (size_t)loc;
}

void MemoryManager::dumpMem(MemPool &pool)
{
    constexpr size_t charSize     = 2; // in bytes
    constexpr size_t charsPerLine = 64 * charSize;
    for(size_t i = 0; i < pool.sz; i += charSize) {
        if(i % charsPerLine == 0) std::cout << "\n" << (void *)(pool.mem + i) << " :: ";
        std::cout << std::hex << (*(uint16_t *)(pool.mem + i)) << " ";
    }
    std::cout << std::dec << "\n";
}

IAllocated::IAllocated() {}
IAllocated::~IAllocated() {}

IAllocatedList::IAllocatedList(MemoryManager &mem, String &&name)
    : mem(mem), name(std::move(name)), count(0)
{}
IAllocatedList::IAllocatedList(MemoryManager &mem, const char *name)
    : mem(mem), name(name), count(0)
{}
IAllocatedList::~IAllocatedList() {}

void *IAllocatedList::addAlloc(void *newAlloc, void *&start, void *&end)
{
    mem.setAllocDetail((size_t)newAlloc, AllocDetails::NEXT, 0);
    mem.setAllocDetail((size_t)newAlloc, AllocDetails::PREV, (size_t)end);
    if(!start) {
        start = newAlloc;
        end   = start;
    } else {
        mem.setAllocDetail((size_t)end, AllocDetails::NEXT, (size_t)newAlloc);
        end = newAlloc;
    }
    ++count;
    return newAlloc;
}

void *IAllocatedList::removeAlloc(void *alloc, void *&start, void *&end)
{
    if(!alloc) return nullptr;
    if(alloc == start) {
        start = (void *)mem.getAllocDetail((size_t)alloc, AllocDetails::NEXT);
        mem.setAllocDetail((size_t)alloc, AllocDetails::NEXT, 0);
        if(start) mem.setAllocDetail((size_t)start, AllocDetails::PREV, 0);
    } else if(alloc == end) {
        end = (void *)mem.getAllocDetail((size_t)alloc, AllocDetails::PREV);
        mem.setAllocDetail((size_t)alloc, AllocDetails::PREV, 0);
        if(end) mem.setAllocDetail((size_t)end, AllocDetails::NEXT, 0);
    } else {
        void *prev = (void *)mem.getAllocDetail((size_t)alloc, AllocDetails::PREV);
        void *next = (void *)mem.getAllocDetail((size_t)alloc, AllocDetails::NEXT);
        mem.setAllocDetail((size_t)alloc, AllocDetails::PREV, 0);
        mem.setAllocDetail((size_t)alloc, AllocDetails::NEXT, 0);
        if(prev) mem.setAllocDetail((size_t)prev, AllocDetails::NEXT, (size_t)next);
        if(next) mem.setAllocDetail((size_t)next, AllocDetails::PREV, (size_t)prev);
    }
    --count;
    return alloc;
}

void *IAllocatedList::removeAlloc(size_t allocIndex, void *&start, void *&end)
{
    void *iter = nullptr;
    size_t i   = 0;
    while(i++ <= allocIndex && (iter = getNext(iter, start)));
    return removeAlloc(iter, start, end);
}

void *IAllocatedList::getAt(size_t index, void *start, void *end) const
{
    size_t i   = 0;
    void *iter = nullptr;
    while(i <= index && (iter = getNext(iter, start))) { ++i; }
    return iter;
}

ManagedList::ManagedList(MemoryManager &mem, String &&name)
    : IAllocatedList(mem, std::move(name)), start(0), end(0)
{}
ManagedList::ManagedList(MemoryManager &mem, const char *name)
    : IAllocatedList(mem, name), start(0), end(0)
{}
ManagedList::~ManagedList()
{
    size_t count = clear();
    LOG_DEBUG(getName(), " allocator had ", count, " allocations");
}

bool ManagedList::free(IAllocated *alloc)
{
    removeAlloc(alloc, (void *&)start, (void *&)end);
    mem.freeDeinit(alloc);
    return true;
}
bool ManagedList::free(size_t index)
{
    IAllocated *alloc = (IAllocated *)removeAlloc(index, (void *&)start, (void *&)end);
    if(!alloc) return false;
    mem.freeDeinit(alloc);
    return true;
}

size_t ManagedList::clear()
{
    size_t count = 0;
    while(start) {
        free(start);
        ++count;
    }
    end = start;
    return count;
}

ManagedRawList::ManagedRawList(MemoryManager &mem, String &&name)
    : IAllocatedList(mem, std::move(name)), start(0), end(0)
{}
ManagedRawList::ManagedRawList(MemoryManager &mem, const char *name)
    : IAllocatedList(mem, name), start(0), end(0)
{}
ManagedRawList::~ManagedRawList()
{
    size_t count = clear();
    LOG_DEBUG(getName(), " allocator had ", count, " allocations");
}

bool ManagedRawList::free(void *alloc)
{
    removeAlloc(alloc, start, end);
    mem.freeRaw(alloc);
    return true;
}
bool ManagedRawList::free(size_t index)
{
    void *alloc = removeAlloc(index, start, end);
    if(!alloc) return false;
    mem.freeRaw(alloc);
    return true;
}

size_t ManagedRawList::clear()
{
    size_t count = 0;
    while(start) {
        free(start);
        ++count;
    }
    end = start;
    return count;
}

} // namespace fer