#pragma once

#include "VarTypes.hpp"

namespace fer
{

struct SysAlign
{
	char c;
	size_t sz;
};

static constexpr size_t MAX_ROUNDUP = 2048;
static constexpr size_t POOL_SIZE   = 8 * 1024;
static constexpr size_t ALIGNMENT   = sizeof(SysAlign) - sizeof(size_t);

struct MemPool
{
	u8 *head;
	u8 *mem;
};

class VarMemory
{
	Map<size_t, List<u8 *>> freechunks;
	Vector<MemPool> pools;

	// works upto MAX_ROUNDUP
	size_t mult8Roundup(size_t sz);
	void allocPool();

public:
	VarMemory();
	~VarMemory();

	static VarMemory &getInstance();

	void *alloc(size_t sz);
	void free(void *data, size_t sz);
};

} // namespace fer