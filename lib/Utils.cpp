#include "VM/Interpreter.hpp"

namespace fer
{

class VarIntIterator : public Var
{
	int64_t begin, end, step, curr;
	bool started;
	bool reversed;

public:
	VarIntIterator(const ModuleLoc *loc);
	VarIntIterator(const ModuleLoc *loc, int64_t _begin, int64_t _end, int64_t _step);
	~VarIntIterator();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	bool next(int64_t &val);

	inline void setReversed(int64_t step) { reversed = step < 0; }
	inline int64_t getBegin() { return begin; }
	inline int64_t getEnd() { return end; }
	inline int64_t getStep() { return step; }
	inline int64_t getCurr() { return curr; }
};

VarIntIterator::VarIntIterator(const ModuleLoc *loc)
	: Var(loc, false, false), started(false), reversed(false), begin(0), end(0), step(0),
	  curr(0)
{}
VarIntIterator::VarIntIterator(const ModuleLoc *loc, int64_t _begin, int64_t _end, int64_t _step)
	: Var(loc, false, false), started(false), reversed(_step < 0), begin(_begin), end(_end),
	  step(_step), curr(_begin)
{}
VarIntIterator::~VarIntIterator() {}

Var *VarIntIterator::copy(const ModuleLoc *loc)
{
	return new VarIntIterator(loc, begin, end, step);
}
void VarIntIterator::set(Var *from)
{
	VarIntIterator *f = as<VarIntIterator>(from);

	begin	 = f->begin;
	end	 = f->end;
	step	 = f->step;
	curr	 = f->curr;
	started	 = f->started;
	reversed = f->reversed;
}

bool VarIntIterator::next(int64_t &val)
{
	if(reversed) {
		if(curr <= end) return false;
	} else {
		if(curr >= end) return false;
	}
	if(!started) {
		val	= curr;
		started = true;
		return true;
	}
	int64_t tmp = curr + step;
	if(reversed) {
		if(tmp <= end) return false;
	} else {
		if(tmp >= end) return false;
	}
	curr = tmp;
	val  = curr;
	return true;
}

Var *intRange(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	Var *lhs_base  = args[1];
	Var *rhs_base  = args.size() > 2 ? args[2] : nullptr;
	Var *step_base = args.size() > 3 ? args[3] : nullptr;

	if(!lhs_base->is<VarInt>()) {
		vm.fail(lhs_base->getLoc(),
			"expected argument 1 to be of type int, found: ", vm.getTypeName(lhs_base));
		return nullptr;
	}
	if(rhs_base && !rhs_base->is<VarInt>()) {
		vm.fail(rhs_base->getLoc(),
			"expected argument 2 to be of type int, found: ", vm.getTypeName(rhs_base));
		return nullptr;
	}
	if(step_base && !step_base->is<VarInt>()) {
		vm.fail(step_base->getLoc(), "expected argument 3 to be of type int, found: ",
			vm.getTypeName(step_base));
		return nullptr;
	}

	int64_t begin, end, step;
	if(args.size() > 2) begin = as<VarInt>(lhs_base)->get();
	else begin = 0;
	if(rhs_base) end = as<VarInt>(rhs_base)->get();
	else end = as<VarInt>(lhs_base)->get();
	if(step_base) step = as<VarInt>(step_base)->get();
	else step = 1;
	VarIntIterator *res = vm.makeVar<VarIntIterator>(loc, begin, end, step);
	return res;
}

Var *getIntIteratorNext(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			const StringMap<AssnArgData> &assn_args)
{
	VarIntIterator *it = as<VarIntIterator>(args[0]);
	int64_t _res;
	if(!it->next(_res)) return vm.getNil();
	VarInt *res = vm.makeVar<VarInt>(loc, _res);
	res->setLoadAsRef();
	return res;
}

INIT_MODULE(Utils)
{
	vm.addNativeFn(loc, "irange", intRange, 1, true);

	// generate the type ID for int iterator (registerType)
	vm.registerType<VarIntIterator>(loc, "IntIterator");

	vm.addNativeTypeFn<VarIntIterator>(loc, "next", getIntIteratorNext, 0);

	return true;
}

} // namespace fer