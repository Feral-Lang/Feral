#pragma once

#include "Core.hpp"

#if defined(FER_OS_WINDOWS)
#include <bit> // required for std::countr_zero()
#endif

namespace fer
{

enum class AllocDetails : uint32_t
{
    // Size in bytes of the allocation
    SIZE,
    // Address of next free allocation of the same size
    // Can be used for something else while the memory is allocated -
    // make sure to reset the value to zero before freeing though
    // The address points to the usable location, so to get AllocDetail from there,
    // you must do: (char*)loc - sizeof(AllocDetail)
    NEXT,
    // <Unused internally as of now>
    // Can be used for something else while the memory is allocated -
    // make sure to reset the value to zero before freeing though
    // The address points to the usable location, so to get AllocDetail from there,
    // you must do: (char*)loc - sizeof(AllocDetail)
    PREV,
    // <Unused internally as of now>
    // Can be used for something else while the memory is allocated -
    // make sure to reset the value to zero before freeing though
    // The address points to the usable location, so to get AllocDetail from there,
    // you must do: (char*)loc - sizeof(AllocDetail)
    DATA,

    _LAST,
};

using AllocDetail = size_t[static_cast<uint32_t>(AllocDetails::_LAST)];

constexpr size_t MAX_ROUNDUP        = 2048;
constexpr size_t DEFAULT_POOL_SIZE  = 8 * 1024;
constexpr size_t MAX_ALIGNMENT      = alignof(std::max_align_t);
constexpr size_t ALLOC_DETAIL_BYTES = sizeof(AllocDetail);

// Extra allocation bytes must be a multiple of MAX_ALIGNMENT.
// That way, when we allocate the memory, and move forward by ALLOC_DETAIL_BYTES,
// we are still on an address that is a multiple of MAX_ALIGNMENT.
static_assert(ALLOC_DETAIL_BYTES % MAX_ALIGNMENT == 0,
              "sizeof(AllocDetail) must be a multiple of max alignment");

struct MemPool
{
    char *head;
    char *mem;
};

// Base class for anything that uses the memory manager / allocator
class IAllocated
{
public:
    IAllocated();
    virtual ~IAllocated();
};

template<typename T> concept IAllocatedDerived = std::is_base_of_v<IAllocated, T>;

class FER_API MemoryManager
{
    // The size_t at freechunks[sz] is an address which holds an allocation.
    // This address is after ALLOC_DETAIL_BYTES bytes.
    Array<size_t, std::countr_zero(MAX_ROUNDUP)> freechunks;
    Vector<MemPool> pools;
    // TODO: have multiple arenas and then use mutexes individual to those arenas
    // then, in a mutithreaded environment, the manager should be able to choose one of the
    // available arenas and allocate memory from that (therefore increasing speed, compared to a
    // global mutex).
    RecursiveMutex mtx;
    String name;
    size_t poolSize;

    inline constexpr size_t getFreeChunkIndex(size_t sz) { return std::countr_zero(sz) - 1; }
    // works upto MAX_ROUNDUP
    size_t nextPow2(size_t sz);
    void allocPool();

public:
    MemoryManager(StringRef name, size_t poolSize = DEFAULT_POOL_SIZE);
    ~MemoryManager();

    void *allocRaw(size_t size, size_t align);
    void freeRaw(void *data);

    // Helper function - only use if seeing memory issues.
    void dumpMem(char *pool);

    template<IAllocatedDerived T, typename... Args> T *allocInit(Args &&...args)
    {
        void *m = allocRaw(sizeof(T), alignof(T));
        return new(m) T(std::forward<Args>(args)...);
    }
    inline void freeDeinit(IAllocated *data)
    {
        data->~IAllocated();
        freeRaw(data);
    }

    // alloc address must be AFTER sizeof(AllocDetail)
    inline void setAllocDetail(size_t alloc, AllocDetails field, size_t value)
    {
        (*(AllocDetail *)((char *)alloc - ALLOC_DETAIL_BYTES))[static_cast<uint32_t>(field)] =
            value;
    }
    // alloc address must be AFTER sizeof(AllocDetail)
    inline size_t getAllocDetail(size_t alloc, AllocDetails field)
    {
        return (*(AllocDetail *)((char *)alloc - ALLOC_DETAIL_BYTES))[static_cast<uint32_t>(field)];
    }

    inline size_t getPoolSize() { return poolSize; }
    inline size_t getPoolCount() { return pools.size(); }
};

class IAllocatedList : public IAllocated
{
    String name;
    size_t count;

protected:
    MemoryManager &mem;
    void *addAlloc(void *newAlloc, void *&start, void *&end);
    void *removeAlloc(void *alloc, void *&start, void *&end);
    void *removeAlloc(size_t allocIndex, void *&start, void *&end);

    void *getAt(size_t index, void *start, void *end) const;

    inline void *getPrev(void *from, void *end) const
    {
        return from ? (void *)mem.getAllocDetail((size_t)from, AllocDetails::PREV) : end;
    }
    inline void *getNext(void *from, void *start) const
    {
        return from ? (void *)mem.getAllocDetail((size_t)from, AllocDetails::NEXT) : start;
    }

    inline size_t getSize() const { return count; }
    inline bool isEmpty(void *start) const { return !start; }

public:
    IAllocatedList(MemoryManager &mem, String &&name);
    IAllocatedList(MemoryManager &mem, const char *name);
    virtual ~IAllocatedList();

    inline StringRef getName() { return name; }
};

// Cannot be a static object - as it uses the static variable `logger` in destructor.
// RAII based - does not allow freeing of the memory unless it goes out of scope.
// Only allocates IAllocated derived objects
class FER_API ManagedList : public IAllocatedList
{
    IAllocated *start, *end;

public:
    ManagedList(MemoryManager &mem, String &&name);
    ManagedList(MemoryManager &mem, const char *name);
    ~ManagedList();

    template<IAllocatedDerived T, typename... Args> T *alloc(Args &&...args)
    {
        T *res = mem.allocInit<T>(std::forward<Args>(args)...);
        addAlloc(res, (void *&)start, (void *&)end);
        return res;
    }

    bool free(IAllocated *alloc);
    bool free(size_t index);

    size_t clear();

    inline IAllocated *add(IAllocated *alloc)
    {
        return (IAllocated *)addAlloc(alloc, (void *&)start, (void *&)end);
    }

    inline IAllocated *remove(IAllocated *alloc)
    {
        return (IAllocated *)removeAlloc(alloc, (void *&)start, (void *&)end);
    }
    inline IAllocated *remove(size_t index)
    {
        return (IAllocated *)removeAlloc(index, (void *&)start, (void *&)end);
    }

    inline IAllocated *getStart() const { return start; }
    inline IAllocated *getEnd() const { return end; }

    inline IAllocated *at(size_t index) const { return (IAllocated *)getAt(index, start, end); }

    inline IAllocated *prev(IAllocated *from = nullptr) const
    {
        return (IAllocated *)getPrev(from, end);
    }
    inline IAllocated *next(IAllocated *from = nullptr) const
    {
        return (IAllocated *)getNext(from, start);
    }

    inline size_t size() const { return getSize(); }
    inline bool empty() const { return isEmpty(start); }
};

// Cannot be a static object - as it uses the static variable `logger` in destructor.
// RAII based - does not allow freeing of the memory unless it goes out of scope.
// Only allocates raw objects - NO constructor / destructor is called
class FER_API ManagedRawList : public IAllocatedList
{
    void *start, *end;

public:
    ManagedRawList(MemoryManager &mem, String &&name);
    ManagedRawList(MemoryManager &mem, const char *name);
    ~ManagedRawList();

    template<typename T> T *alloc(size_t count = 1)
    {
        T *res = (T *)mem.allocRaw(sizeof(T) * count, alignof(T));
        addAlloc(res, start, end);
        return res;
    }
    template<typename T> T *allocInit(const T *value, size_t count = 1)
    {
        T *res = alloc<T>(count);
        memcpy(res, value, sizeof(T) * count);
        return res;
    }

    bool free(void *alloc);
    bool free(size_t index);

    size_t clear();

    inline void *add(void *alloc) { return addAlloc(alloc, start, end); }

    inline void *remove(void *alloc) { return removeAlloc(alloc, start, end); }
    inline void *remove(size_t index) { return removeAlloc(index, start, end); }

    inline void *getStart() const { return start; }
    inline void *getEnd() const { return end; }

    inline void *at(size_t index) const { return getAt(index, start, end); }

    inline void *prev(void *from = nullptr) const { return getPrev(from, end); }
    inline void *next(void *from = nullptr) const { return getNext(from, start); }

    inline size_t size() const { return getSize(); }
    inline bool empty() const { return isEmpty(start); }
};

} // namespace fer