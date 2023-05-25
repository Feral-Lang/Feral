#include "VM/Interpreter.hpp"

namespace fer
{

struct JumpData
{
	StringRef name;
	size_t pos;
};

int Interpreter::execute(Bytecode *custombc, size_t begin, size_t end)
{
	++recurse_count;
	VarModule *varmod = getCurrModule();
	Vars *vars	  = varmod->getVars();
	Module *mod	  = varmod->getMod();
	Bytecode &bc	  = custombc ? *custombc : mod->getBytecode();
	size_t bcsz	  = end == 0 ? bc.size() : end;

	Vector<JumpData> jmps;
	Vector<FeralFnBody> bodies;
	Vector<Var *> args;
	Map<String, AssnArgData> assn_args;

	if(!custombc) vars->pushFn();

	for(size_t i = begin; i < bcsz; ++i) {
		Instruction &ins = bc.getInstrAt(i);
		// std::cout << "[" << i << ": " << ins.getLoc()->getMod()->getPath() << "] ";
		// ins.dump(std::cout);
		// std::cout << " :: ";
		// dumpExecStack(std::cout);
		// std::cout << "\n";

		if(recurse_count >= getMaxRecurseCount()) {
			fail(ins.getLoc(), "stack overflow, current max: ", getMaxRecurseCount());
			recurse_count_exceeded = true;
			goto handle_err;
		}
		switch(ins.getOpcode()) {
		case Opcode::LOAD_DATA: {
			if(!ins.isDataIden()) {
				Var *res = getConst(ins.getLoc(), ins.getData(), ins.getDataType());
				if(res == nullptr) {
					fail(ins.getLoc(), "invalid data received as const");
					goto handle_err;
				}
				execstack.push(res);
			} else {
				Var *res = vars->get(ins.getDataStr());
				if(!res) {
					res = getGlobal(ins.getDataStr());
					if(!res) {
						fail(ins.getLoc(), "variable '", ins.getDataStr(),
						     "' does not exist");
						goto handle_err;
					}
				}
				execstack.push(res);
			}
			break;
		}
		case Opcode::UNLOAD: {
			for(size_t i = 0; i < ins.getDataInt(); ++i) execstack.pop();
			break;
		}
		case Opcode::CREATE: {
			StringRef name = ins.getDataStr();
			Var *val       = execstack.pop(false);
			// only copy if reference count > 1 (no point in copying unique values)
			// or if loadAsRef() of value is false
			if(val->isLoadAsRef() || val->getRef() == 1) {
				vars->add(name, val, true);
				val->unsetLoadAsRef();
			} else {
				vars->add(name, val->copy(ins.getLoc()), false);
			}
			decref(val);
			break;
		}
		case Opcode::CREATE_IN: {
			// back() and pop() are not combined because pop can cause the Var* to be
			// destroyed
			StringRef name = ins.getDataStr();
			Var *in	       = execstack.pop(false);
			Var *val       = execstack.pop(false);
			if(in->isAttrBased()) {
				// only copy if reference count > 1 (no point in copying unique
				// values) or if loadAsRef() of value is false
				if(val->isLoadAsRef() || val->getRef() == 1) {
					in->setAttr(name, val, true);
					val->unsetLoadAsRef();
				} else {
					in->setAttr(name, val->copy(ins.getLoc()), false);
				}
			} else {
				if(!val->isCallable()) {
					fail(ins.getLoc(), "only callables can be added to non "
							   "attribute based types");
					goto create_fail;
				}
				addTypeFn(in->is<VarTypeID>() ? as<VarTypeID>(in)->get()
							      : in->getType(),
					  name, val, true);
			}
			decref(in);
			decref(val);
			break;
		create_fail:
			decref(in);
			decref(val);
			goto handle_err;
		}
		case Opcode::STORE: {
			if(execstack.size() < 2) {
				fail(ins.getLoc(), "execution stack has ", execstack.size(),
				     " item(s); required 2 for store operation");
				goto handle_err;
			}
			Var *var = execstack.pop(false);
			Var *val = execstack.pop(false);
			// TODO: check if this works for assigning one struct instance of type X to
			// another struct instance of type Y
			if(var->getType() != val->getType()) {
				fail(var->getLoc(),
				     "type mismatch for assignment: ", getTypeName(val),
				     " cannot be assigned to variable of type: ", getTypeName(var));
				decref(val);
				decref(var);
				goto handle_err;
			}
			var->set(val);
			execstack.push(var, false);
			decref(val);
			break;
		}
		case Opcode::PUSH_BLOCK: {
			vars->pushBlk(ins.getDataInt());
			break;
		}
		case Opcode::POP_BLOCK: {
			vars->popBlk(ins.getDataInt());
			break;
		}
		case Opcode::JMP: {
			i = ins.getDataInt() - 1;
			break;
		}
		case Opcode::JMP_NIL: {
			if(execstack.back()->is<VarNil>()) {
				execstack.pop();
				i = ins.getDataInt() - 1;
			}
			break;
		}
		case Opcode::JMP_TRUE_POP:  // fallthrough
		case Opcode::JMP_TRUE:	    // fallthrough
		case Opcode::JMP_FALSE_POP: // fallthrough
		case Opcode::JMP_FALSE: {
			assert(!execstack.empty());
			Var *var = execstack.back();
			bool res = false;
			if(var->is<VarInt>()) {
				res = mpz_get_si(as<VarInt>(var)->get());
			} else if(var->is<VarFlt>()) {
				res = mpfr_get_si(as<VarFlt>(var)->get(),
						  mpfr_get_default_rounding_mode());
			} else if(var->is<VarBool>()) {
				res = as<VarBool>(var)->get();
			} else if(var->is<VarNil>()) {
				res = false;
			} else {
				fail(ins.getLoc(),
				     "conditional jump requires boolean"
				     " (or int/float) data, found: ",
				     getTypeName(var));
				execstack.pop();
				goto handle_err;
			}
			if(ins.getOpcode() == Opcode::JMP_TRUE_POP ||
			   ins.getOpcode() == Opcode::JMP_TRUE)
			{
				if(res) i = ins.getDataInt() - 1;
				if(!res || ins.getOpcode() == Opcode::JMP_TRUE_POP) execstack.pop();
			} else {
				if(!res) i = ins.getDataInt() - 1;
				if(res || ins.getOpcode() == Opcode::JMP_FALSE_POP) execstack.pop();
			}
			break;
		}
		case Opcode::BLOCK_TILL: {
			bodies.push_back({i + 1, (size_t)ins.getDataInt()});
			i = ins.getDataInt();
			break;
		}
		case Opcode::CREATE_FN: {
			StringRef arginfo = ins.getDataStr();
			String kw, va;
			if(arginfo[0] == '1') {
				kw = as<VarStr>(execstack.back())->get();
				execstack.pop();
			}
			if(arginfo[1] == '1') {
				va = as<VarStr>(execstack.back())->get();
				execstack.pop();
			}
			size_t argcount = 0, assnarg_count = 0;
			for(size_t i = 0; i < arginfo.size(); ++i) {
				if(arginfo[i] == '1') ++assnarg_count;
				else ++argcount;
			}
			VarFn *fn =
			makeVarWithRef<VarFn>(ins.getLoc(), mod->getPath(), kw, va, argcount,
					      assnarg_count, FnBody{.feral = bodies.back()}, false);
			bodies.pop_back();
			for(size_t i = 2; i < arginfo.size(); ++i) {
				String name = as<VarStr>(execstack.back())->get();
				execstack.pop();
				if(arginfo[i] == '1') {
					fn->insertAssnParam(name,
							    execstack.back()->copy(ins.getLoc()));
					execstack.pop();
				}
				fn->pushParam(name);
			}
			execstack.push(fn, false);
			break;
		}
		case Opcode::MEM_CALL: // fallthrough
		case Opcode::CALL: {
			// self is not decref()'d manually at the end because it becomes a part of
			// args anyway
			Var *self   = nullptr; // only for memcall
			Var *fnbase = nullptr;
			Var *res    = nullptr;
			String fnname;
			// setup call args
			args.clear();
			assn_args.clear();
			bool memcall	  = ins.getOpcode() == Opcode::MEM_CALL;
			StringRef arginfo = ins.getDataStr();
			for(size_t i = 0; i < arginfo.size(); ++i) {
				if(arginfo[i] == '2') { // unpack
					Var *a = execstack.pop(false);
					if(!a->is<VarVec>()) {
						fail(ins.getLoc(),
						     "expected a vector to unpack, found: ",
						     getTypeName(a));
						decref(a);
						goto fncall_fail;
					}
					for(auto &va : as<VarVec>(a)->get()) {
						incref(va);
						args.push_back(va);
					}
					decref(a);
				} else if(arginfo[i] == '1') {
					String name = as<VarStr>(execstack.back())->get();
					execstack.pop();
					assn_args[name] = {i, execstack.pop(false)};
				} else if(arginfo[i] == '0') {
					args.push_back(execstack.pop(false));
				}
			}

			// fetch the function
			if(memcall) {
				fnname = as<VarStr>(execstack.back())->get();
				execstack.pop();
				self = execstack.pop(false);
				if(self->isAttrBased()) fnbase = self->getAttr(fnname);
				if(!fnbase) fnbase = getTypeFn(self, fnname);
			} else {
				fnbase = execstack.pop(false);
			}
			if(!fnbase) {
				if(memcall) {
					fail(ins.getLoc(), "callable '", fnname,
					     "' does not exist for type: ", getTypeName(self));
				} else {
					fail(ins.getLoc(), "this function does not exist");
				}
				decref(self);
				goto fncall_fail;
			}
			if(!fnbase->isCallable()) {
				fail(ins.getLoc(), "'", getTypeName(fnbase),
				     "' is not a callable type");
				decref(self);
				goto fncall_fail;
			}
			args.insert(args.begin(), self);

			// call the function
			res = fnbase->call(*this, ins.getLoc(), args, assn_args);
			// don't show the following failure when exec stack count is exceeded or
			// there'll be a GIANT stack trace
			if(!res) {
				if(!recurse_count_exceeded) {
					fail(ins.getLoc(),
					     "function call failed, check the error above");
				}
				goto fncall_fail;
			}
			if(!res->is<VarNil>()) execstack.push(res);

			// cleanup
			for(auto &a : args) decref(a);
			for(auto &aa : assn_args) decref(aa.second.val);
			if(!memcall) decref(fnbase);
			if(exitcalled) goto done;
			break;
		fncall_fail:
			for(auto &a : args) decref(a);
			for(auto &aa : assn_args) decref(aa.second.val);
			if(!memcall) decref(fnbase);
			goto handle_err;
		}
		case Opcode::ATTR: {
			StringRef attr = ins.getDataStr();
			Var *inbase    = execstack.pop(false);
			Var *val       = nullptr;
			if(inbase->isAttrBased()) val = inbase->getAttr(attr);
			if(!val) val = getTypeFn(inbase, attr);
			if(!val) {
				fail(ins.getLoc(), "type ", getTypeName(inbase),
				     " does not contain attribute: ", attr);
				decref(inbase);
				goto handle_err;
			}
			execstack.push(val);
			decref(inbase);
			break;
		}
		case Opcode::RETURN: {
			if(!ins.getDataBool()) execstack.push(nil);
			goto done;
		}
		case Opcode::PUSH_LOOP: {
			vars->pushLoop();
			break;
		}
		case Opcode::POP_LOOP: {
			vars->popLoop();
			break;
		}
		case Opcode::CONTINUE: {
			vars->continueLoop();
			i = ins.getDataInt() - 1;
			break;
		}
		case Opcode::BREAK: {
			// jumps to pop_loop instr
			i = ins.getDataInt() - 1;
			break;
		}
		case Opcode::PUSH_JMP: {
			// name is set in the PUSH_JMP_NAME instr
			jmps.push_back({"", (size_t)ins.getDataInt()});
			failstack.pushBlk();
			break;
		}
		case Opcode::PUSH_JMP_NAME: {
			jmps.back().name = ins.getDataStr();
			break;
		}
		case Opcode::POP_JMP: {
			jmps.pop_back();
			failstack.popBlk();
			break;
		}
		case Opcode::LAST: {
			assert(false);
		handle_err:
			if(!jmps.empty() && !exitcalled) {
				i = jmps.back().pos - 1;
				if(!jmps.back().name.empty()) {
					Var *dat =
					!failstack.emptyTop()
					? failstack.pop(false)
					: makeVarWithRef<VarStr>(ins.getLoc(), "unknown failure");
					vars->stash(jmps.back().name, dat, false);
				}
				jmps.pop_back();
				failstack.popBlk();
				recurse_count_exceeded = false;
				break;
			}
			goto fail;
		}
		}
	}
done:
	assert(jmps.empty());
	if(!custombc) vars->popFn();
	--recurse_count;
	return exitcode;
fail:
	if(!custombc) vars->popFn();
	--recurse_count;
	return 1;
}

void Interpreter::dumpExecStack(OStream &os)
{
	for(auto &e : execstack.get()) {
		if(e->is<VarInt>()) {
			os << "int";
		} else if(e->is<VarFlt>()) {
			os << "flt";
		} else if(e->is<VarStr>()) {
			os << "Str:" << as<VarStr>(e)->get();
		} else if(e->is<VarNil>()) {
			os << "nil";
		} else if(e->is<VarBool>()) {
			os << "bool:" << (as<VarBool>(e)->get() ? "true" : "false");
		} else {
			os << "unknown";
		}
		std::cout << " ";
	}
}

} // namespace fer