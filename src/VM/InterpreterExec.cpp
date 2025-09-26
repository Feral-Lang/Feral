#include "VM/Interpreter.hpp"

namespace fer
{

int VirtualMachine::execute(bool addFunc, bool addBlk, size_t begin, size_t end)
{
	++recurseCount;
	VarModule *varmod  = getCurrModule();
	Vars &vars	   = getVars();
	const Bytecode &bc = varmod->getBytecode();
	size_t bcsz	   = end == 0 ? bc.size() : end;

	Vars::ScopedModScope _(vars, varmod->getVarStack());

	Vector<FeralFnBody> bodies;
	Vector<Var *> args;
	StringMap<AssnArgData> assn_args;
	size_t currBlkSize = 0;

	if(addFunc) vars.pushFn();
	else currBlkSize = vars.getBlkSize();
	if(addBlk) vars.pushBlk(1);

	for(size_t i = begin; i < bcsz; ++i) {
		const Instruction &ins = bc.getInstrAt(i);
		// std::cout << "[" << i << ": " << getCurrModule()->getPath() << "] ";
		// ins.dump(std::cout);
		// std::cout << " :: ";
		// dumpExecStack(std::cout);
		// std::cout << "\n";

		if(ip.shouldStopExecution()) goto fail;

		if(addFunc && recurseCount >= getRecurseMax()) {
			fail(ins.getLoc(), "stack overflow, current max: ", getRecurseMax());
			recurseExceeded = true;
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
				pushExecStack(res);
			} else {
				Var *res = vars.get(ins.getDataStr());
				if(!res) {
					res = getGlobal(ins.getDataStr());
					if(!res) {
						fail(ins.getLoc(), "variable '", ins.getDataStr(),
						     "' does not exist");
						goto handle_err;
					}
				}
				pushExecStack(res);
			}
			break;
		}
		case Opcode::UNLOAD: {
			for(size_t i = 0; i < ins.getDataInt(); ++i) popExecStack();
			break;
		}
		case Opcode::CREATE: {
			StringRef name = ins.getDataStr();
			Var *val       = popExecStack(false);
			if(!val) {
				fail(ins.getLoc(),
				     "expected a value in stack for creating variable: ", name,
				     ", but found none");
				goto handle_err;
			}
			// only copy if reference count > 1 (no point in copying unique values)
			if(val->getRef() == 1) {
				vars.add(name, val, true);
			} else {
				vars.add(name, copyVar(ins.getLoc(), val), false);
			}
			decVarRef(val);
			break;
		}
		case Opcode::CREATE_IN: {
			// back() and pop() are not combined because pop can cause the Var* to be
			// destroyed
			StringRef name = ins.getDataStr();
			Var *in	       = popExecStack(false);
			Var *val       = popExecStack(false);
			if(in->isAttrBased()) {
				// only copy if reference count > 1 (no point in copying unique
				// values) or if loadAsRef() of value is false
				if(val->getRef() == 1) {
					in->setAttr(getMemoryManager(), name, val, true);
				} else {
					in->setAttr(getMemoryManager(), name,
						    copyVar(ins.getLoc(), val), false);
				}
			} else {
				if(!val->isCallable()) {
					fail(ins.getLoc(), "only callables can be added to non "
							   "attribute based types");
					goto create_fail;
				}
				addTypeFn(in->is<VarTypeID>() ? as<VarTypeID>(in)->getVal()
							      : in->getType(),
					  name, val, true);
			}
			decVarRef(in);
			decVarRef(val);
			break;
		create_fail:
			decVarRef(in);
			decVarRef(val);
			goto handle_err;
		}
		case Opcode::STORE: {
			if(execstack.size() < 2) {
				fail(ins.getLoc(), "execution stack has ", execstack.size(),
				     " item(s), required 2 for store operation");
				goto handle_err;
			}
			Var *var = popExecStack(false);
			Var *val = popExecStack(false);
			// TODO: check if this works for assigning one struct instance of type X to
			// another struct instance of type Y
			if(var->getType() != val->getType()) {
				fail(var->getLoc(),
				     "type mismatch for assignment: ", getTypeName(val),
				     " cannot be assigned to variable of type: ", getTypeName(var));
				decVarRef(val);
				decVarRef(var);
				goto handle_err;
			}
			setVar(var, val);
			pushExecStack(var, false);
			decVarRef(val);
			break;
		}
		case Opcode::PUSH_BLOCK: {
			vars.pushBlk(ins.getDataInt());
			break;
		}
		case Opcode::POP_BLOCK: {
			vars.popBlk(ins.getDataInt());
			break;
		}
		case Opcode::JMP: {
			i = ins.getDataInt() - 1;
			break;
		}
		case Opcode::JMP_NIL: {
			if(execstack.back()->is<VarNil>()) {
				popExecStack();
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
				res = as<VarInt>(var)->getVal();
			} else if(var->is<VarFlt>()) {
				res = as<VarFlt>(var)->getVal();
			} else if(var->is<VarBool>()) {
				res = as<VarBool>(var)->getVal();
			} else if(var->is<VarNil>()) {
				res = false;
			} else {
				fail(ins.getLoc(),
				     "conditional jump requires boolean"
				     " (or int/float) data, found: ",
				     getTypeName(var));
				popExecStack();
				goto handle_err;
			}
			if(ins.getOpcode() == Opcode::JMP_TRUE_POP ||
			   ins.getOpcode() == Opcode::JMP_TRUE)
			{
				if(res) i = ins.getDataInt() - 1;
				if(!res || ins.getOpcode() == Opcode::JMP_TRUE_POP) popExecStack();
			} else {
				if(!res) i = ins.getDataInt() - 1;
				if(res || ins.getOpcode() == Opcode::JMP_FALSE_POP) popExecStack();
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
				kw = as<VarStr>(execstack.back())->getVal();
				popExecStack();
			}
			if(arginfo[1] == '1') {
				va = as<VarStr>(execstack.back())->getVal();
				popExecStack();
			}
			size_t argcount = 0, assnarg_count = 0;
			for(size_t i = 0; i < arginfo.size(); ++i) {
				if(arginfo[i] == '1') ++assnarg_count;
				else ++argcount;
			}
			VarFn *fn =
			makeVarWithRef<VarFn>(ins.getLoc(), varmod->getModuleId(), kw, va, argcount,
					      assnarg_count, FnBody{.feral = bodies.back()}, false);
			bodies.pop_back();
			for(size_t i = 2; i < arginfo.size(); ++i) {
				String name = as<VarStr>(execstack.back())->getVal();
				popExecStack();
				if(arginfo[i] == '1') {
					Var *data = execstack.back();
					popExecStack(false);
					fn->insertAssnParam(name, data);
				}
				fn->pushParam(name);
			}
			pushExecStack(fn, false);
			break;
		}
		case Opcode::MEM_CALL: // fallthrough
		case Opcode::CALL: {
			// self is not decVarRef()'d manually at the end because it becomes a
			// part of args anyway
			Var *self   = nullptr; // only for memcall
			Var *fnbase = nullptr;
			Var *res    = nullptr;
			String fnname;
			// setup call args
			args.clear();
			assn_args.clear();
			bool memcall	  = ins.getOpcode() == Opcode::MEM_CALL;
			StringRef arginfo = ins.getDataStr();
			size_t kwargpos	  = 0;
			for(size_t i = 0; i < arginfo.size(); ++i) {
				if(arginfo[i] == '2') { // unpack
					Var *a = popExecStack(false);
					if(!a->is<VarVec>() && !a->is<VarMap>()) {
						fail(
						ins.getLoc(),
						"expected a vector or kwarg to unpack, found: ",
						getTypeName(a));
						decVarRef(a);
						goto fncall_fail;
					}
					if(a->is<VarVec>()) {
						for(auto &va : as<VarVec>(a)->getVal()) {
							incVarRef(va);
							args.push_back(va);
						}
					} else if(a->is<VarMap>()) {
						for(auto &k : as<VarMap>(a)->getPositions()) {
							Var *v = as<VarMap>(a)->getAttr(k);
							incVarRef(v);
							auto loc = assn_args.find(k);
							if(loc != assn_args.end())
								decVarRef(loc->second.val);
							assn_args[k] = {kwargpos++, v};
						}
					}
					decVarRef(a);
				} else if(arginfo[i] == '1') {
					String name = as<VarStr>(execstack.back())->getVal();
					popExecStack();
					auto loc   = assn_args.find(name);
					size_t pos = 0;
					if(loc != assn_args.end()) {
						pos = loc->second.pos;
						decVarRef(loc->second.val);
					} else {
						pos = kwargpos++;
					}
					assn_args[name] = {pos, popExecStack(false)};
				} else if(arginfo[i] == '0') {
					args.push_back(popExecStack(false));
				}
			}

			// fetch the function
			if(memcall) {
				fnname = as<VarStr>(execstack.back())->getVal();
				popExecStack();
				self = popExecStack(false);
				if(self->isAttrBased()) fnbase = self->getAttr(fnname);
				if(!fnbase) fnbase = getTypeFn(self, fnname);
			} else {
				fnbase = popExecStack(false);
			}
			if(!fnbase) {
				if(memcall) {
					fail(ins.getLoc(), "callable '", fnname,
					     "' does not exist for type: ", getTypeName(self));
				} else {
					fail(ins.getLoc(), "this function does not exist");
				}
				decVarRef(self);
				goto fncall_fail;
			}
			if(!fnbase->isCallable()) {
				fail(ins.getLoc(), "'", getTypeName(fnbase),
				     "' is not a callable type");
				decVarRef(self);
				goto fncall_fail;
			}
			args.insert(args.begin(), self);

			// call the function
			res = fnbase->call(*this, ins.getLoc(), args, assn_args);
			// don't show the following failure when exec stack count is exceeded or
			// there'll be a GIANT stack trace
			if(!res) {
				if(!recurseExceeded) {
					warn(ins.getLoc(),
					     "function call failed, check the error above");
				}
				goto fncall_fail;
			}
			if(!res->is<VarNil>()) pushExecStack(res);

			// cleanup
			for(auto &a : args) decVarRef(a);
			for(auto &aa : assn_args) decVarRef(aa.second.val);
			if(!memcall) decVarRef(fnbase);
			if(isExitCalled()) goto done;
			break;
		fncall_fail:
			for(auto &a : args) decVarRef(a);
			for(auto &aa : assn_args) decVarRef(aa.second.val);
			if(!memcall) decVarRef(fnbase);
			goto handle_err;
		}
		case Opcode::ATTR: {
			StringRef attr = ins.getDataStr();
			Var *inbase    = popExecStack(false);
			Var *val       = nullptr;
			if(inbase->isAttrBased()) val = inbase->getAttr(attr);
			if(!val) val = getTypeFn(inbase, attr);
			if(!val) {
				fail(ins.getLoc(), "type ", getTypeName(inbase),
				     " does not contain attribute: ", attr);
				decVarRef(inbase);
				goto handle_err;
			}
			pushExecStack(val);
			decVarRef(inbase);
			break;
		}
		case Opcode::RETURN: {
			if(!ins.getDataBool()) pushExecStack(ip.nil);
			goto done;
		}
		case Opcode::PUSH_LOOP: {
			vars.pushLoop();
			break;
		}
		case Opcode::POP_LOOP: {
			vars.popLoop();
			break;
		}
		case Opcode::CONTINUE: {
			vars.continueLoop();
			i = ins.getDataInt() - 1;
			break;
		}
		case Opcode::BREAK: {
			// jumps to pop_loop instr
			i = ins.getDataInt() - 1;
			break;
		}
		case Opcode::PUSH_JMP: {
			// Decode the data from string.
			// It contains blkBegin (size_t bytes) + blkEnd (size_t bytes) + var name.
			StringRef data	= ins.getDataStr();
			size_t blkBegin = *(size_t *)data.data();
			size_t blkEnd	= *(((size_t *)data.data()) + 1);
			data		= data.substr(sizeof(size_t) * 2);
			failstack.pushScope();
			failstack.initFrame(recurseCount, data, blkBegin, blkEnd);
			break;
		}
		case Opcode::PUSH_JMP_NAME: {
			break;
		}
		case Opcode::POP_JMP: {
			failstack.popScope();
			break;
		}
		case Opcode::LAST: {
			assert(false);
		handle_err:
			if(!failstack.hasErr() || recurseCount != failstack.getRecurseLevel())
				goto fail;
			if(recurseExceeded) {
				if(recurseCount != failstack.getRecurseLevel()) goto fail;
				recurseExceeded = false;
			}
			StringRef varName = failstack.getVarName();
			size_t blkBegin	  = failstack.getBlkBegin();
			size_t blkEnd	  = failstack.getBlkEnd();
			Var *err	  = failstack.getErr();
			if(!varName.empty()) {
				if(!err) err = makeVar<VarStr>(ins.getLoc(), "unknown failure");
				vars.stash(varName, err, true);
			}
			pushModule(getCurrModule()->getModuleId());
			if(execute(false, false, blkBegin, blkEnd) && !isExitCalled()) {
				if(!varName.empty()) vars.unstash();
				popModule();
				// Must pop failstack scope because this is a failure in the the
				// scope itself - it can't recover using the POP_JMP instruction.
				failstack.popScope();
				goto handle_err;
			}
			// POP_JMP instr will take care of popping from jmps and failstack.
			i = blkEnd - 1;
			popModule();
			if(isExitCalled()) {
				// Will never reach POP_JMP, so pop the failstack here.
				failstack.popScope();
				goto done;
			}
			break;
		}
		}
	}
done:
	if(addBlk) vars.popBlk(1);
	if(addFunc) vars.popFn();
	else vars.resizeBlkTo(currBlkSize);
	--recurseCount;
	return exitcode;
fail:
	if(addBlk) vars.popBlk(1);
	if(addFunc) vars.popFn();
	else vars.resizeBlkTo(currBlkSize);
	--recurseCount;
	return 1;
}

void VirtualMachine::dumpExecStack(OStream &os)
{
	for(auto &e : execstack.get()) {
		e->dump(std::cout, this);
		std::cout << " -- ";
	}
}

} // namespace fer