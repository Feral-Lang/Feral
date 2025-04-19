#pragma once

#include "Core.hpp"

namespace fer
{

struct AllocDetail
{
	// Allocation size
	size_t memSz;
	// Address of next free allocation of the same size
	// The address points to the usable location, so to get AllocDetail from there,
	// you must do: (char*)loc - sizeof(AllocDetail)
	size_t next;
};

constexpr size_t MAX_ROUNDUP	    = 2048;
constexpr size_t POOL_SIZE	    = 8 * 1024;
constexpr size_t MAX_ALIGNMENT	    = alignof(std::max_align_t);
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

class MemoryManager
{
	// The size_t at freechunks[sz] is an address which holds an allocation.
	// This address is after ALLOC_DETAIL_BYTES bytes.
	Map<size_t, size_t> freechunks;
	Vector<MemPool> pools;
	// TODO: have multiple arenas and then use mutexes individual to those arenas
	// then, in a mutithreaded environment, the manager should be able to choose one of the
	// available arenas and allocate memory from that (therefore increasing speed, compared to a
	// global mutex).
	RecursiveMutex mtx;
	String name;

	// works upto MAX_ROUNDUP
	size_t nextPow2(size_t sz);
	void allocPool();

public:
	MemoryManager(StringRef name);
	~MemoryManager();

	void *alloc(size_t size, size_t align);
	void free(void *data);

	template<typename T, typename... Args> T *alloc(Args &&...args)
	{
		void *m = alloc(sizeof(T), alignof(T));
		return new(m) T(std::forward<Args>(args)...);
	}

	// Helper function - only use if seeing memory issues.
	void dumpMem(char *pool);
};

// Base class for anything that uses the allocator
class IAllocated
{
public:
	IAllocated();
	virtual ~IAllocated();
};

// Cannot be a static object - as it uses the static variable `logger` in destructor.
// RAII based - does not allow freeing of the memory unless it goes out of scope.
class Allocator
{
	MemoryManager &mem;
	UniList<IAllocated *> allocs;
	String name;

public:
	Allocator(MemoryManager &mem, StringRef name);
	~Allocator();

	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<IAllocated, T>::value, T *>::type
	alloc(Args... args)
	{
		T *res = mem.alloc<T>(std::forward<Args>(args)...);
		allocs.push_front(res);
		return res;
	}
};

} // namespace fer