#pragma once

#include "VarTypes.hpp"

namespace fer
{

struct ErrorHandlingInfo
{
	bool usable;
	size_t recurseLevel;
	StringRef varName;
	size_t blkBegin, blkEnd;
	Var *errMsg;
};

class FailStack
{
	Vector<ErrorHandlingInfo> stack;
	MemoryManager &mem;

public:
	FailStack(MemoryManager &mem);
	~FailStack();

	inline void pushScope() { stack.emplace_back(); }
	inline void popScope()
	{
		reset(); // to decref errMsg if any
		stack.pop_back();
	}

	void initFrame(size_t recurseLevel, StringRef varName, size_t blkBegin, size_t blkEnd);
	void reset();

	inline void setErr(Var *var) { stack.back().errMsg = var; }

	inline bool isUsable() { return !stack.empty() && stack.back().usable; }
	inline size_t getRecurseLevel() { return stack.back().recurseLevel; }
	inline StringRef getVarName() { return stack.back().varName; }
	inline size_t getBlkBegin() { return stack.back().blkBegin; }
	inline size_t getBlkEnd() { return stack.back().blkEnd; }
	inline Var *getErr() { return stack.back().errMsg; }

	inline size_t size() { return stack.size(); }
	inline bool empty() { return stack.empty(); }
};

} // namespace fer