#include "VM/Interpreter.hpp"

using namespace fer;

class VarIntIterator : public Var
{
	mpz_t begin, end, step, curr;
	bool started;
	bool reversed;

public:
	VarIntIterator(const ModuleLoc *loc);
	VarIntIterator(const ModuleLoc *loc, mpz_srcptr _begin, mpz_srcptr _end, mpz_srcptr _step);
	~VarIntIterator();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	bool next(mpz_ptr val);

	inline void setReversed(mpz_srcptr step) { reversed = mpz_cmp_si(step, 0) < 0; }
	inline mpz_ptr getBegin() { return begin; }
	inline mpz_ptr getEnd() { return end; }
	inline mpz_ptr getStep() { return step; }
	inline mpz_ptr getCurr() { return curr; }
};

VarIntIterator::VarIntIterator(const ModuleLoc *loc)
	: Var(loc, typeID<VarIntIterator>(), false, false), started(false), reversed(false)
{
	mpz_init(begin);
	mpz_init(end);
	mpz_init(step);
	mpz_init(curr);
}
VarIntIterator::VarIntIterator(const ModuleLoc *loc, mpz_srcptr _begin, mpz_srcptr _end,
			       mpz_srcptr _step)
	: Var(loc, typeID<VarIntIterator>(), false, false), started(false),
	  reversed(mpz_cmp_si(step, 0) < 0)
{
	mpz_init_set(begin, _begin);
	mpz_init_set(end, _end);
	mpz_init_set(step, _step);
	mpz_init_set(curr, _begin);
}
VarIntIterator::~VarIntIterator() { mpz_clears(begin, end, step, curr, NULL); }

Var *VarIntIterator::copy(const ModuleLoc *loc)
{
	return new VarIntIterator(loc, begin, end, step);
}
void VarIntIterator::set(Var *from)
{
	VarIntIterator *f = as<VarIntIterator>(from);
	mpz_set(begin, f->begin);
	mpz_set(end, f->end);
	mpz_set(step, f->step);
	mpz_set(curr, f->curr);
	started	 = f->started;
	reversed = f->reversed;
}

bool VarIntIterator::next(mpz_ptr val)
{
	if(reversed) {
		if(mpz_cmp(curr, end) <= 0) return false;
	} else {
		if(mpz_cmp(curr, end) >= 0) return false;
	}
	if(!started) {
		mpz_init(val);
		mpz_set(val, curr);
		started = true;
		return true;
	}
	mpz_t tmp;
	mpz_init(tmp);
	mpz_add(tmp, curr, step);
	if(reversed) {
		if(mpz_cmp(tmp, end) <= 0) {
			mpz_clear(tmp);
			return false;
		}
	} else {
		if(mpz_cmp(tmp, end) >= 0) {
			mpz_clear(tmp);
			return false;
		}
	}
	mpz_set(curr, tmp);
	mpz_init(val);
	mpz_set(val, curr);
	mpz_clear(tmp);
	return true;
}

Var *range(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const Map<StringRef, AssnArgData> &assn_args)
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

	VarIntIterator *res = vm.makeVar<VarIntIterator>(loc);
	if(args.size() > 2) mpz_set(res->getBegin(), as<VarInt>(lhs_base)->getSrc());
	else mpz_set_si(res->getBegin(), 0);
	if(rhs_base) mpz_set(res->getEnd(), as<VarInt>(rhs_base)->getSrc());
	else mpz_set(res->getEnd(), as<VarInt>(lhs_base)->getSrc());
	if(step_base) mpz_set(res->getStep(), as<VarInt>(step_base)->getSrc());
	else mpz_set_si(res->getStep(), 1);
	return res;
}

Var *assertion(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<StringRef, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarBool>()) {
		vm.fail(loc,
			"expected boolean argument"
			" for assertion, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}

	if(!as<VarBool>(args[1])->get()) {
		vm.fail(loc, "assertion failed");
		return nullptr;
	}
	return vm.getNil();
}

Var *getIntIteratorNext(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			const Map<StringRef, AssnArgData> &assn_args)
{
	VarIntIterator *it = as<VarIntIterator>(args[0]);
	VarInt *res	   = vm.makeVar<VarInt>(loc, 0);
	res->setLoadAsRef();
	if(!it->next(res->get())) {
		vm.unmakeVar(res);
		return vm.getNil();
	}
	return res;
}

INIT_MODULE(Utils)
{
	vm.addNativeFn(loc, "range", range, 1, true);
	vm.addNativeFn(loc, "assert", assertion, 1);

	// generate the type ID for int iterator (registerType)
	vm.registerType<VarIntIterator>(loc, "IntIterator");

	vm.addNativeTypeFn<VarIntIterator>(loc, "next", getIntIteratorNext, 0);

	return true;
}