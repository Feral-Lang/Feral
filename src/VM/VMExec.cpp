#include "VM/VM.hpp"

namespace fer
{

int VirtualMachine::execute(Var *&ret, bool addFunc, bool addBlk, size_t begin, size_t end)
{
    ++recurseCount;
    VarModule *varmod  = getCurrModule();
    VarVars *vars      = getVars();
    const Bytecode &bc = varmod->getBytecode();
    size_t bcsz        = end == 0 ? bc.size() : end;

    VarVars::ScopedModScope _(*this, vars, varmod->getVarStack());

    Vector<FeralFnBody> bodies;
    Vector<Var *> args;
    StringMap<AssnArgData> assnArgs;
    size_t currBlkSize = 0;

    if(addFunc) vars->pushFn(*this, {});
    else currBlkSize = vars->getBlkSize();
    if(addBlk) vars->pushBlk(*this, {}, 1);

    for(size_t i = begin; i < bcsz; ++i) {
        const Instruction &ins = bc.getInstrAt(i);
#if defined(CORE_BUILD_DEBUG)
        logger.trace("[", i, ": ", getCurrModule()->getPath(), "] ", ins.dump(),
                     " :: ", execstack->dump(this));
#endif

        if(shouldStopExecution()) goto fail;
        if(exitCalled) goto done;

        if(addFunc && recurseCount >= getRecurseMax()) {
            fail(ins.getLoc(), "stack overflow, current max: ", getRecurseMax());
            recurseExceeded = true;
            goto handleErr;
        }
        switch(ins.getOpcode()) {
        case Opcode::LOAD_DATA: {
            if(!ins.isDataIden()) {
                Var *res = getConst(ins.getLoc(), ins.getData(), ins.getDataType());
                if(res == nullptr) {
                    fail(ins.getLoc(), "invalid data received as const");
                    goto handleErr;
                }
                pushExecStack(res);
            } else {
                Var *res = vars->getAttr(ins.getDataStr());
                if(!res) {
                    res = getGlobal(ins.getDataStr());
                    if(!res) {
                        fail(ins.getLoc(), "variable '", ins.getDataStr(), "' does not exist");
                        goto handleErr;
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
                fail(ins.getLoc(), "expected a value in stack for creating variable: ", name,
                     ", but found none");
                goto handleErr;
            }
            if(ins.hasComment()) { val->setDoc(*this, ins.getLoc(), ins.getComment()); }
            // only copy if reference count > 1 (no point in copying unique values)
            if(val->getRef() == 1) {
                vars->setAttr(*this, name, val, true);
            } else {
                Var *cp = copyVar(ins.getLoc(), val);
                if(!cp) goto handleErr;
                vars->setAttr(*this, name, cp, false);
            }
            decVarRef(val);
            break;
        }
        case Opcode::CREATE_IN: {
            StringRef name = ins.getDataStr();
            Var *in        = popExecStack(false);
            Var *val       = popExecStack(false);
            if(val->isCallable()) {
                addTypeFn(in->is<VarTypeID>() ? as<VarTypeID>(in)->getVal() : in->getSubType(),
                          name, val, true);
            } else if(in->isAttrBased()) {
                // only copy if reference count > 1 (no point in copying unique
                // values) or if loadAsRef() of value is false
                if(val->getRef() == 1) {
                    in->setAttr(*this, name, val, true);
                } else {
                    Var *cp = copyVar(ins.getLoc(), val);
                    if(!cp) goto createFail;
                    in->setAttr(*this, name, cp, false);
                }
            } else {
                fail(ins.getLoc(),
                     "cannot add a non-callable to a non attribute based type: ", getTypeName(in));
                goto createFail;
            }
            decVarRef(in);
            decVarRef(val);
            break;
        createFail:
            decVarRef(in);
            decVarRef(val);
            goto handleErr;
        }
        case Opcode::STORE: {
            if(execstack->size() < 2) {
                fail(ins.getLoc(), "execution stack has ", execstack->size(),
                     " item(s), required 2 for store operation");
                goto handleErr;
            }
            Var *var = popExecStack(false);
            Var *val = popExecStack(false);
            // TODO: check if this works for assigning one struct instance of type X to
            // another struct instance of type Y
            if(var->getType() != val->getType()) {
                fail(ins.getLoc(), "type mismatch for assignment: ", getTypeName(val),
                     " cannot be assigned to variable of type: ", getTypeName(var));
                decVarRef(val);
                decVarRef(var);
                goto handleErr;
            }
            if(var->isConst()) {
                fail(ins.getLoc(),
                     "cannot assign to a const marked variable of type: ", getTypeName(var));
                decVarRef(val);
                decVarRef(var);
                goto handleErr;
            }
            if(!var->set(*this, val)) {
                fail(ins.getLoc(), "failed to assign: ", getTypeName(val),
                     " to: ", getTypeName(var));
                decVarRef(val);
                decVarRef(var);
                goto handleErr;
            }
            pushExecStack(var, false);
            decVarRef(val);
            break;
        }
        case Opcode::PUSH_BLOCK: {
            vars->pushBlk(*this, {}, ins.getDataInt());
            break;
        }
        case Opcode::POP_BLOCK: {
            vars->popBlk(*this, ins.getDataInt());
            break;
        }
        case Opcode::JMP: {
            i = ins.getDataInt() - 1;
            break;
        }
        case Opcode::JMP_NIL: {
            if(execstack->back()->is<VarNil>()) {
                popExecStack();
                i = ins.getDataInt() - 1;
            }
            break;
        }
        case Opcode::JMP_TRUE_POP:  // fallthrough
        case Opcode::JMP_TRUE:      // fallthrough
        case Opcode::JMP_FALSE_POP: // fallthrough
        case Opcode::JMP_FALSE: {
            assert(!execstack->empty());
            Var *var = execstack->back();
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
                goto handleErr;
            }
            if(ins.getOpcode() == Opcode::JMP_TRUE_POP || ins.getOpcode() == Opcode::JMP_TRUE) {
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
                kw = as<VarStr>(execstack->back())->getVal();
                popExecStack();
            }
            if(arginfo[1] == '1') {
                va = as<VarStr>(execstack->back())->getVal();
                popExecStack();
            }
            size_t argCount = 0, assnArgCount = 0;
            for(size_t i = 0; i < arginfo.size(); ++i) {
                if(arginfo[i] == '1') ++assnArgCount;
                else ++argCount;
            }
            VarFn *fn = makeVar<VarFn>(ins.getLoc(), varmod->getModuleId(), kw, va, argCount,
                                       assnArgCount, FnBody{.feral = bodies.back()}, false);
            bodies.pop_back();
            for(size_t i = 2; i < arginfo.size(); ++i) {
                String name = as<VarStr>(execstack->back())->getVal();
                popExecStack();
                if(arginfo[i] == '1') {
                    Var *data = execstack->back();
                    popExecStack(false);
                    fn->insertAssnParam(name, data);
                }
                fn->pushParam(name);
            }
            pushExecStack(fn);
            break;
        }
        case Opcode::MEM_CALL: // fallthrough
        case Opcode::CALL: {
            // self is not decVarRef()'d manually at the end because it becomes a
            // part of args anyway
            Var *self   = nullptr; // only for memcall
            Var *fnbase = nullptr;
            String fnname;
            // setup call args
            args.clear();
            assnArgs.clear();
            bool memcall      = ins.getOpcode() == Opcode::MEM_CALL;
            StringRef arginfo = ins.getDataStr();
            size_t kwargpos   = 0;
            Var *res          = nullptr;
            for(size_t i = 0; i < arginfo.size(); ++i) {
                if(arginfo[i] == '2') { // unpack
                    Var *a = popExecStack(false);
                    if(!a->is<VarVec>() && !a->is<VarMap>()) {
                        fail(ins.getLoc(),
                             "expected a vector or kwarg to unpack, found: ", getTypeName(a));
                        decVarRef(a);
                        goto callFail;
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
                            auto loc = assnArgs.find(k);
                            if(loc != assnArgs.end()) decVarRef(loc->second.val);
                            assnArgs[k] = {kwargpos++, v};
                        }
                    }
                    decVarRef(a);
                } else if(arginfo[i] == '1') {
                    String name = as<VarStr>(execstack->back())->getVal();
                    popExecStack();
                    auto loc   = assnArgs.find(name);
                    size_t pos = 0;
                    if(loc != assnArgs.end()) {
                        pos = loc->second.pos;
                        decVarRef(loc->second.val);
                    } else {
                        pos = kwargpos++;
                    }
                    assnArgs[name] = {pos, popExecStack(false)};
                } else if(arginfo[i] == '0') {
                    args.push_back(popExecStack(false));
                }
            }

            // fetch the function
            if(memcall) {
                fnname = as<VarStr>(execstack->back())->getVal();
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
                goto callFail;
            }
            if(!fnbase->isCallable()) {
                fail(ins.getLoc(), "'", getTypeName(fnbase), "' is not a callable type");
                decVarRef(self);
                goto callFail;
            }
            args.insert(args.begin(), self);

            // call the function
            if(!(res = fnbase->call(*this, ins.getLoc(), args, assnArgs))) {
                // don't show the following failure when exec stack count is
                // exceeded or there'll be a GIANT stack trace
                if(!recurseExceeded) {
                    fail(ins.getLoc(), "function call failed, check the error above");
                }
                goto callFail;
            }
            pushExecStack(res, false);

            // cleanup
            for(auto &a : args) decVarRef(a);
            for(auto &aa : assnArgs) decVarRef(aa.second.val);
            if(!memcall) decVarRef(fnbase);
            if(isExitCalled()) {
                ret = popExecStack(false);
                goto done;
            }
            break;
        callFail:
            for(auto &a : args) decVarRef(a);
            for(auto &aa : assnArgs) decVarRef(aa.second.val);
            if(!memcall) decVarRef(fnbase);
            goto handleErr;
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
                goto handleErr;
            }
            pushExecStack(val);
            decVarRef(inbase);
            break;
        }
        case Opcode::RETURN: {
            ret = ins.getDataBool() ? popExecStack(false) : incVarRef(gs->nil);
            goto done;
        }
        case Opcode::PUSH_LOOP: {
            vars->pushLoop(*this, {});
            break;
        }
        case Opcode::POP_LOOP: {
            vars->popLoop(*this);
            break;
        }
        case Opcode::CONTINUE: {
            vars->continueLoop(*this);
            i = ins.getDataInt() - 1;
            break;
        }
        case Opcode::BREAK: {
            // jumps to pop_loop instr
            i = ins.getDataInt() - 1;
            break;
        }
        case Opcode::PUSH_TRY: {
            VarFn *handler = as<VarFn>(execstack->pop(false));
            failstack->pushHandler(handler, ins.getDataInt(), recurseCount);
            decVarRef(handler);
            break;
        }
        case Opcode::POP_TRY: {
            failstack->popHandler();
            break;
        }
        case Opcode::LAST: {
            assert(false);
        handleErr:
            if(recurseCount > failstack->getLastRecurseCount()) goto fail;
            if(recurseExceeded) recurseExceeded = false;
            size_t popLoc = i + 1;
            Var *res      = failstack->handle(*this, ins.getLoc(), popLoc);
            if(!res) goto fail;
            i = popLoc - 1;
            pushExecStack(res, false);
            break;
        }
        }
    }
done:
    if(addBlk) vars->popBlk(*this, 1);
    if(addFunc) vars->popFn(*this);
    else vars->resizeBlkTo(*this, currBlkSize);
    --recurseCount;
    return exitcode;
fail:
    if(ret) decVarRef(ret);
    if(addBlk) vars->popBlk(*this, 1);
    if(addFunc) vars->popFn(*this);
    else vars->resizeBlkTo(*this, currBlkSize);
    --recurseCount;
    return 1;
}

} // namespace fer
