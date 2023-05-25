#include "VM/Interpreter.hpp"

using namespace fer;

gmp_randstate_t rngstate;

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
	gmp_randseed(rngstate, as<VarInt>(args[1])->get());
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
	VarInt *res = vm.makeVar<VarInt>(loc, 0);
	mpz_urandomm(res->get(), rngstate, as<VarInt>(args[1])->get());
	return res;
}

INIT_MODULE(RNG)
{
	gmp_randinit_default(rngstate);

	VarModule *mod = vm.getCurrModule();
	mod->addNativeFn("seed", rngSeed, 1);
	mod->addNativeFn("getNative", rngGet, 1);

	return true;
}

DEINIT_MODULE(RNG) { gmp_randclear(rngstate); }