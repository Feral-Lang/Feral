#include <random>

#include "VM/Interpreter.hpp"

namespace fer
{

static std::default_random_engine rng;

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *rngSeed(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected seed value to be integer, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	rng.seed(as<VarInt>(args[1])->get());
	return vm.getNil();
}

// [0, to)
Var *rngGet(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected upper bound to be an integer, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	std::uniform_int_distribution<int64_t> dist(0, as<VarInt>(args[1])->get());
	return vm.makeVar<VarInt>(loc, dist(rng));
}

INIT_MODULE(RNG)
{
	VarModule *mod = vm.getCurrModule();
	mod->addNativeFn("seed", rngSeed, 1);
	mod->addNativeFn("getNative", rngGet, 1);
	return true;
}

} // namespace fer