#pragma once

#include "Core.hpp"

namespace fer
{

struct SysAlign
{
	char c;
	size_t sz;
};

constexpr size_t MAX_ROUNDUP = 2048;
constexpr size_t POOL_SIZE   = 8 * 1024;
constexpr size_t ALIGNMENT   = sizeof(SysAlign) - sizeof(size_t);

struct MemPool
{
	char *head;
	char *mem;
};

class MemoryManager
{
	Map<size_t, UniList<char *>> freechunks;
	Vector<MemPool> pools;
	// TODO: have multiple arenas and then use mutexes individual to those arenas
	// then, in a mutithreaded environment, the manager should be able to choose one of the
	// available arenas and allocate memory from that (therefore increasing speed, compared to a
	// global mutex).
	Mutex mtx;
	String name;

	// works upto MAX_ROUNDUP
	size_t mult8Roundup(size_t sz);
	void allocPool();

public:
	MemoryManager(StringRef name);
	~MemoryManager();

	void *alloc(size_t sz);
	void free(void *data);
};

// Base class for anything that uses the allocator
class IAllocated
{
public:
	IAllocated();
	virtual ~IAllocated();
};

// Cannot be a static object - as it uses the static variable `logger` in destructor
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
		void *m = mem.alloc(sizeof(T));
		T *res	= new(m) T(std::forward<Args>(args)...);
		allocs.push_front(res);
		return res;
	}
};

} // namespace fer