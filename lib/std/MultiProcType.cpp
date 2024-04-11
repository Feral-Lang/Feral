#include "std/MultiProcType.hpp"

namespace fer
{

static size_t threadId = 0;

VarMultiProc::VarMultiProc(const ModuleLoc *loc, Thread *thread, SharedFuture<int> *res, bool owner)
	: Var(loc, false, false), thread(thread), res(res), id(threadId++), owner(owner)
{}
VarMultiProc::VarMultiProc(const ModuleLoc *loc, Thread *thread, SharedFuture<int> *res, size_t id,
			   bool owner)
	: Var(loc, false, false), thread(thread), res(res), id(id), owner(owner)
{}
VarMultiProc::~VarMultiProc()
{
	if(owner) {
		if(thread != nullptr) {
			thread->join();
			delete thread;
		}
		if(res != nullptr) delete res;
	}
}

Var *VarMultiProc::copy(const ModuleLoc *loc)
{
	return new VarMultiProc(loc, thread, res, id, false);
}
void VarMultiProc::set(Var *from)
{
	VarMultiProc *t = as<VarMultiProc>(from);
	owner		= false;
	thread		= t->thread;
	res		= t->res;
	id		= t->id;
}

} // namespace fer