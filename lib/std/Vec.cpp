#include "std/VecType.hpp"
#include "VM/Interpreter.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *vecNew(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	auto refloc    = assn_args.find("refs");
	auto caploc    = assn_args.find("cap");
	size_t resvcap = args.size() - 1;
	bool refs      = false;
	if(refloc != assn_args.end()) {
		Var *refsv = refloc->second.val;
		if(!refsv->is<VarBool>()) {
			vm.fail(loc,
				"expected 'refs' named argument to be"
				" of type bool for vec.new(), found: ",
				vm.getTypeName(refsv));
			return nullptr;
		}
		refs = as<VarBool>(refsv)->get();
	}
	if(caploc != assn_args.end()) {
		Var *capv = caploc->second.val;
		if(!capv->is<VarBool>()) {
			vm.fail(loc,
				"expected 'cap' named argument to be"
				" of type int for vec.new(), found: ",
				vm.getTypeName(capv));
			return nullptr;
		}
		resvcap = mpz_get_ui(as<VarInt>(capv)->getSrc());
	}
	VarVec *res = vm.makeVar<VarVec>(loc, resvcap, refs);
	if(refs) {
		for(size_t i = 1; i < args.size(); ++i) {
			incref(args[i]);
			res->push(args[i]);
		}
	} else {
		for(size_t i = 1; i < args.size(); ++i) {
			res->push(args[i]->copy(loc));
		}
	}
	return res;
}

Var *vecSize(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarVec>(args[0])->get().size());
}

Var *vecCapacity(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarVec>(args[0])->get().capacity());
}

Var *vecIsRef(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	return as<VarVec>(args[0])->isRefVec() ? vm.getTrue() : vm.getFalse();
}

Var *vecEmpty(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	return as<VarVec>(args[0])->get().empty() ? vm.getTrue() : vm.getFalse();
}

Var *vecFront(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	return as<VarVec>(args[0])->get().empty() ? vm.getNil()
						  : as<VarVec>(args[0])->get().front();
}

Var *vecBack(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	return as<VarVec>(args[0])->get().empty() ? vm.getNil() : as<VarVec>(args[0])->get().back();
}

Var *vecPush(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	VarVec *res = as<VarVec>(args[0]);
	if(res->isRefVec()) {
		incref(args[1]);

		res->push(args[1]);
	} else {
		res->push(args[1]->copy(loc));
	}
	return args[0];
}

Var *vecPop(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	VarVec *res = as<VarVec>(args[0]);
	if(res->get().empty()) {
		vm.fail(loc, "called pop() on an empty vector");
		return nullptr;
	}
	decref(res->get().back());
	res->get().pop_back();
	return args[0];
}

Var *vecSetAt(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected first argument to be of "
			"type integer for vec.set(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	VarVec *res = as<VarVec>(args[0]);
	size_t pos  = mpz_get_ui(as<VarInt>(args[1])->getSrc());
	if(pos >= res->get().size()) {
		vm.fail(loc, "position ", std::to_string(pos), " is not within vector of length ",
			std::to_string(res->get().size()));
		return nullptr;
	}
	decref(res->get()[pos]);
	if(res->isRefVec()) {
		incref(args[2]);
		res->get()[pos] = args[2];
	} else {
		res->get()[pos] = args[2]->copy(loc);
	}
	return args[0];
}

Var *vecInsert(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected first argument to be of"
			" type integer for vec.insert(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	VarVec *res = as<VarVec>(args[0]);
	size_t pos  = mpz_get_ui(as<VarInt>(args[1])->getSrc());
	if(pos >= res->get().size()) {
		vm.fail(loc, "position ", std::to_string(pos), " is not within vector of length ",
			std::to_string(res->get().size()));
		return nullptr;
	}
	if(res->isRefVec()) {
		incref(args[2]);
		res->get().insert(res->get().begin() + pos, args[2]);
	} else {
		res->get().insert(res->get().begin() + pos, args[2]->copy(loc));
	}
	return args[0];
}

Var *vecErase(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected argument to be of type integer for vec.erase(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	VarVec *res = as<VarVec>(args[0]);
	size_t pos  = mpz_get_ui(as<VarInt>(args[1])->getSrc());
	if(pos >= res->get().size()) {
		vm.fail(loc, "position ", std::to_string(pos), " is not within vector of length ",
			std::to_string(res->get().size()));
		return nullptr;
	}
	decref(res->get()[pos]);
	res->get().erase(res->get().begin() + pos);
	return args[0];
}

Var *vecLast(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarVec>(args[0])->get().size() - 1);
}

Var *vecAt(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected argument to be of type integer for vec.at(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	VarVec *res = as<VarVec>(args[0]);
	size_t pos  = mpz_get_ui(as<VarInt>(args[1])->getSrc());
	if(pos >= res->get().size()) return vm.getNil();
	return res->get()[pos];
}

Var *vecSub(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected starting index to be of"
			" type integer for vec.sub(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarInt>()) {
		vm.fail(loc,
			"expected ending index to be of"
			" type integer for vec.sub(), found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}

	VarVec *v  = as<VarVec>(args[0]);
	size_t beg = mpz_get_ui(as<VarInt>(args[1])->getSrc());
	size_t end = mpz_get_ui(as<VarInt>(args[2])->getSrc());

	if(beg >= v->get().size()) {
		vm.fail(loc, "starting index is greater than vector size");
		return nullptr;
	}
	if(end > v->get().size()) {
		vm.fail(loc, "ending index is greater than vector size");
		return nullptr;
	}
	VarVec *res = vm.makeVar<VarVec>(loc, end - beg > 0 ? end - beg : 0, false);
	if(end <= beg) return res;
	for(size_t i = beg; i < end; ++i) {
		res->push(v->get()[i]->copy(loc));
	}
	return res;
}

Var *vecSlice(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected starting index to be of type integer for vec.slice(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarInt>()) {
		vm.fail(loc, "expected ending index to be of type integer for vec.slice(), found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}

	VarVec *v  = as<VarVec>(args[0]);
	size_t beg = mpz_get_ui(as<VarInt>(args[1])->getSrc());
	size_t end = mpz_get_ui(as<VarInt>(args[2])->getSrc());

	if(beg >= v->get().size()) {
		vm.fail(loc, "starting index is greater than vector size");
		return nullptr;
	}
	if(end > v->get().size()) {
		vm.fail(loc, "ending index is greater than vector size");
		return nullptr;
	}
	VarVec *res = vm.makeVar<VarVec>(loc, end - beg > 0 ? end - beg : 0, true);
	if(end <= beg) return res;
	for(size_t i = beg; i < end; ++i) {
		Var *e = v->get()[i];
		incref(e);
		res->push(e);
	}
	return res;
}

Var *vecEach(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarVecIterator>(loc, as<VarVec>(args[0]));
}

Var *vecIteratorNext(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		     const Map<String, AssnArgData> &assn_args)
{
	VarVecIterator *it = as<VarVecIterator>(args[0]);
	Var *res	   = nullptr;
	if(!it->next(res)) return vm.getNil();
	res->setLoadAsRef();
	return res;
}

INIT_MODULE(Vec)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn("new", vecNew, 0, true);

	vm.addNativeTypeFn<VarVec>(loc, "len", vecSize, 0);
	vm.addNativeTypeFn<VarVec>(loc, "capacity", vecCapacity, 0);
	vm.addNativeTypeFn<VarVec>(loc, "isRef", vecIsRef, 0);
	vm.addNativeTypeFn<VarVec>(loc, "empty", vecEmpty, 0);
	vm.addNativeTypeFn<VarVec>(loc, "front", vecFront, 0);
	vm.addNativeTypeFn<VarVec>(loc, "back", vecBack, 0);
	vm.addNativeTypeFn<VarVec>(loc, "push", vecPush, 1);
	vm.addNativeTypeFn<VarVec>(loc, "pop", vecPop, 0);
	vm.addNativeTypeFn<VarVec>(loc, "insert", vecInsert, 2);
	vm.addNativeTypeFn<VarVec>(loc, "erase", vecErase, 1);
	vm.addNativeTypeFn<VarVec>(loc, "lastIdx", vecLast, 0);
	vm.addNativeTypeFn<VarVec>(loc, "set", vecSetAt, 2);
	vm.addNativeTypeFn<VarVec>(loc, "at", vecAt, 1);
	vm.addNativeTypeFn<VarVec>(loc, "[]", vecAt, 1);

	vm.addNativeTypeFn<VarVec>(loc, "subNative", vecSub, 2);
	vm.addNativeTypeFn<VarVec>(loc, "sliceNative", vecSlice, 2);

	vm.addNativeTypeFn<VarVec>(loc, "each", vecEach, 0);
	vm.addNativeTypeFn<VarVecIterator>(loc, "next", vecIteratorNext, 0);

	return true;
}
