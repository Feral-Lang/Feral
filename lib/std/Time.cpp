#include <chrono>

#include "VM/Interpreter.hpp"

using namespace fer;

Var *sysclkNow(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	VarInt *res = vm.makeVar<VarInt>(loc, 0);
	mpz_set_ui(res->get(), std::chrono::duration_cast<std::chrono::nanoseconds>(
			       std::chrono::system_clock::now().time_since_epoch())
			       .count());
	return res;
}

Var *formatTime(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected integer argument as time for formatting, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(loc, "expected string argument as format for time formatting, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	unsigned long val = mpz_get_ui(as<VarInt>(args[1])->get());
	std::chrono::nanoseconds nsval(val);
	std::chrono::system_clock::time_point tp(
	std::chrono::duration_cast<std::chrono::system_clock::duration>(nsval));
	std::time_t time = std::chrono::system_clock::to_time_t(tp);
	std::tm *t	 = std::localtime(&time);
	char fmt[1024]	 = {0};
	if(std::strftime(fmt, sizeof(fmt), as<VarStr>(args[2])->get().c_str(), t)) {
		return vm.makeVar<VarStr>(loc, fmt);
	}
	return vm.getNil();
}

INIT_MODULE(Time)
{
	VarModule *mod = vm.getCurrModule();
	mod->addNativeFn("systemClockNowNative", sysclkNow);
	mod->addNativeFn("formatNative", formatTime, 2);
	return true;
}