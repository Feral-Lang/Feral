#include <random>

#include "VM/Interpreter.hpp"

namespace fer
{

static std::default_random_engine rng;

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(rngSeed, 1, false,
           "  fn(seed) -> Nil\n"
           "Provides a `seed` number to the random number generator.")
{
    if(!args[1]->is<VarInt>()) {
        vm.fail(loc, "expected seed value to be integer, found: ", vm.getTypeName(args[1]));
        return nullptr;
    }
    rng.seed(as<VarInt>(args[1])->getVal());
    return vm.getNil();
}

FERAL_FUNC(rngGet, 1, false,
           "  fn(upto) -> Int\n"
           "Returns a random number between [0, `upto`).")
{
    if(!args[1]->is<VarInt>()) {
        vm.fail(loc, "expected upper bound to be an integer, found: ", vm.getTypeName(args[1]));
        return nullptr;
    }
    std::uniform_int_distribution<int64_t> dist(0, as<VarInt>(args[1])->getVal());
    return vm.makeVar<VarInt>(loc, dist(rng));
}

INIT_MODULE(RNG)
{
    VarModule *mod = vm.getCurrModule();
    mod->addNativeFn(vm, "seed", rngSeed);
    mod->addNativeFn(vm, "getNative", rngGet);
    return true;
}

} // namespace fer