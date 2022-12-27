#pragma once

#include "VarTypes.hpp"

namespace fer
{

class FailStack
{
	Vector<Deque<Var *>> stack;

public:
	FailStack();
	~FailStack();

	inline void pushBlk() { stack.push_back({}); }
	inline void popBlk()
	{
		for(auto &e : stack.back()) decref(e);
		stack.pop_back();
	}

	void push(Var *var, bool iref = true);
	Var *pop(bool dref = true);

	inline size_t size() { return stack.size(); }
	inline bool empty() { return stack.empty(); }
	inline bool emptyTop() { return stack.back().empty(); }
};

} // namespace fer