#include <random>

#include "VM/VM.hpp"

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
    EXPECT(VarInt, args[1], "seed value");
    rng.seed(as<VarInt>(args[1])->getVal());
    return vm.getNil();
}

FERAL_FUNC(rngGet, 1, false,
           "  fn(upto) -> Int\n"
           "Returns a random number between [0, `upto`).")
{
    EXPECT(VarInt, args[1], "upper bound");
    std::uniform_int_distribution<int64_t> dist(0, as<VarInt>(args[1])->getVal());
    return vm.makeVar<VarInt>(loc, dist(rng));
}

INIT_DLL(RNG)
{
    vm.addLocal(loc, "seed", rngSeed);
    vm.addLocal(loc, "getNative", rngGet);
    return true;
}

} // namespace fer