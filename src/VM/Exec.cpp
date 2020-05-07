/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <cstring>

#include "Consts.hpp"
#include "Vars.hpp"

namespace vm
{

// declared in VM.hpp
int exec( vm_state_t & vm, const size_t & begin, const size_t & end )
{
	var_src_t * src = vm.current_source();
	vars_t * vars = src->vars();
	srcfile_t * src_file = src->src();
	size_t src_id = src_file->id();
	vm_stack_t * vms = vm.vm_stack;
	const auto & bc = src_file->bcode().get();
	size_t bc_sz = end == 0 ? bc.size() : end;

	std::vector< fn_body_span_t > bodies;

	vars->push_fn();

	for( size_t i = begin; i < bc_sz; ++i ) {
		const op_t & op = bc[ i ];
#ifdef DEBUG_MODE
		fprintf( stdout, "%s [%zu]: %*s: ", src_file->path().c_str(), i, 12, OpCodeStrs[ op.op ] );
		for( auto & e : vms->get() ) {
			fprintf( stdout, "%s ", vm.type_name( e->type() ).c_str() );
		}
		fprintf( stdout, "\n" );
#endif // DEBUG_MODE
		switch( op.op ) {
		case OP_LOAD: {
			if( op.dtype != ODT_IDEN ) {
				var_base_t * res = consts::get( vm, op.dtype, op.data, op.idx );
				if( res == nullptr ) {
					vm.fail( op.idx, "invalid data received as const" );
					goto fail;
				}
				vms->push( res );
			} else {
				var_base_t * res = vars->get( op.data.s );
				if( res == nullptr ) {
					res = vm.gget( op.data.s );
					if( res == nullptr ) {
						vm.fail( op.idx, "variable '%s' does not exist", op.data.s );
						goto fail;
					}
				}
				vms->push( res, true );
			}
			break;
		}
		case OP_ULOAD: {
			vms->pop();
			break;
		}
		case OP_CREATE: {
			const std::string name = STR( vms->back() )->get();
			vms->pop();
			var_base_t * in = nullptr;
			if( op.data.b ) {
				in = vms->pop( false );
			}
			var_base_t * val = vms->pop( false );
			if( !in ) {
				// only copy if reference count > 1 (no point in copying unique values)
				if( val->ref() == 1 ) {
					vars->add( name, val, true );
				} else {
					vars->add( name, val->copy( src_id, op.idx ), false );
				}
				goto create_done;
			}
			// for creation with 'in' parameter
			// if it's a function, add that to vm.typefuncs
			if( val->callable() ) {
				// since typefn_id() is virtual, it automatically accomodates
				// struct_def, typeid, as well as simple type()
				vm.add_typefn( in->typefn_id(), name, val, true );
			} else { // else add to attribute if type >= _VT_LAST
				if( !in->attr_based() ) {
					vm.fail( op.idx, "attributes can be added only to structure objects" );
					goto create_fail;
				}
				// only copy if reference count > 1 (no point in copying unique values)
				if( val->ref() == 1 ) {
					in->attr_set( name, val, true );
				} else {
					in->attr_set( name, val->copy( src_id, op.idx ), false );
				}
			}
		create_done:
			var_dref( in );
			var_dref( val );
			break;
		create_fail:
			var_dref( in );
			var_dref( val );
			goto fail;
		}
		case OP_STORE: {
			var_base_t * var = vms->pop( false );
			var_base_t * val = vms->pop( false );
			if( var->type() != val->type() ) {
				vm.fail( op.idx, "assignment requires type of lhs and rhs to be same, found lhs: %s, rhs: %s"
					 "; to redeclare a variable using another type, use the 'let' statement",
					 vm.type_name( var->type() ).c_str(), vm.type_name( val->type() ).c_str() );
				var_dref( val );
				var_dref( var );
				goto fail;
			}
			var->set( val );
			vms->push( var, false );
			var_dref( val );
			break;
		}
		case OP_BLKA: {
			vars->blk_add( op.data.sz );
			break;
		}
		case OP_BLKR: {
			vars->blk_rem( op.data.sz );
			break;
		}
		case OP_JMP: {
			i = op.data.sz - 1;
			break;
		}
		case OP_JMPTPOP: // fallthrough
		case OP_JMPT: {
			assert( vms->back()->istype< var_bool_t >() );
			bool res = static_cast< var_bool_t * >( vms->back() )->get();
			if( res ) i = op.data.sz - 1;
			if( !res || op.op == OP_JMPTPOP ) vms->pop();
			break;
		}
		case OP_JMPFPOP: // fallthrough
		case OP_JMPF: {
			assert( vms->back()->istype< var_bool_t >() );
			bool res = static_cast< var_bool_t * >( vms->back() )->get();
			if( !res ) i = op.data.sz - 1;
			if( res || op.op == OP_JMPFPOP ) vms->pop();
			break;
		}
		case OP_JMPN: {
			if( vms->back()->istype< var_nil_t >() ) {
				vms->pop();
				i = op.data.sz - 1;
			}
			break;
		}
		case OP_BODY_TILL: {
			bodies.push_back( { i + 1, op.data.sz } );
			i = op.data.sz - 1;
			break;
		}
		case OP_MKFN: {
			std::string kw_arg;
			std::string var_arg;
			std::vector< std::string > args;
			std::unordered_map< std::string, var_base_t * > assn_args;
			if( op.data.s[ 0 ] == '1' ) {
				kw_arg = STR( vms->back() )->get();
				vms->pop();
			}
			if( op.data.s[ 1 ] == '1' ) {
				var_arg = STR( vms->back() )->get();
				vms->pop();
			}

			size_t arg_sz = strlen( op.data.s );
			for( size_t i = 2; i < arg_sz; ++i ) {
				std::string name = STR( vms->back() )->get();
				vms->pop();
				if( op.data.s[ i ] == '1' ) {
					// name is guaranteed to be unique, thanks to parser
					assn_args[ name ] = vms->back()->copy( src_id, op.idx );
					vms->pop();
				}
				args.push_back( name );
			}

			fn_body_span_t body = bodies.back();
			bodies.pop_back();

			vms->push( new var_fn_t( src_file->path(), kw_arg, var_arg,
						 args, assn_args, fn_body_t{ .feral = body },
						 false, src_id, op.idx ),
				   false );
			break;
		}
		case OP_MEM_FNCL: // fallthrough
		case OP_FNCL: {
			size_t len = strlen( op.data.s );
			bool mem_call = op.op == OP_MEM_FNCL;
			std::vector< var_base_t * > args;
			std::vector< fn_assn_arg_t > assn_args;
			std::unordered_map< std::string, size_t > assn_args_loc;
			for( size_t i = 0; i < len; ++i ) {
				if( op.data.s[ i ] == '0' ) {
					args.push_back( vms->pop( false ) );
				} else {
					const size_t idx = vms->back()->idx();
					const std::string name = STR( vms->back() )->get();
					vms->pop();
					var_base_t * val = vms->pop( false );
					assn_args.push_back( { idx, name, val } );
					assn_args_loc[ name ] = assn_args.size() - 1;
				}
			}
			var_base_t * in_base = nullptr; // only for mem_call
			var_base_t * fn_base = nullptr;
			var_base_t * res = nullptr;
			std::string fn_name;
			if( mem_call ) {
				fn_name = STR( vms->back() )->get();
				vms->pop();
				in_base = vms->pop( false );
				if( in_base->attr_based() ) {
					fn_base = in_base->attr_get( fn_name );
				}
				if( !fn_base ) {
					fn_base = vm.get_typefn( in_base->typefn_id(), fn_name );
				}
			} else {
				fn_base = vms->pop( false );
			}
			if( !fn_base ) {
				if( mem_call ) vm.fail( op.idx, "function '%s' does not exist for type: %s",
							fn_name.c_str(), vm.type_name( in_base->type() ).c_str() );
				else vm.fail( op.idx, "this function does not exist" );
				var_dref( in_base );
				goto fncall_fail;
			}
			if( !fn_base->callable() ) {
				vm.fail( op.idx, "'%s' is not a function or struct definition",
					 vm.type_name( fn_base->type() ).c_str() );
				var_dref( in_base );
				goto fncall_fail;
			}
			args.insert( args.begin(), in_base );
			res = fn_base->call( vm, args, assn_args, assn_args_loc, src_id, op.idx );
			if( !res ) {
				vm.fail( op.idx, "'%s' call failed, look at error above", vm.type_name( fn_base ).c_str() );
				goto fncall_fail;
			}
			if( !res->istype< var_nil_t >() ) {
				vms->push( res, false );
			}
			for( auto & arg : args ) var_dref( arg );
			for( auto & arg : assn_args ) var_dref( arg.val );
			if( !mem_call ) var_dref( fn_base );
			if( vm.exit_called ) goto done;
			break;
		fncall_fail:
			for( auto & arg : args ) var_dref( arg );
			for( auto & arg : assn_args ) var_dref( arg.val );
			if( !mem_call ) var_dref( fn_base );
			goto fail;
		}
		case OP_ATTR: {
			const std::string attr = op.data.s;
			var_base_t * in_base = vms->pop( false );
			var_base_t * val = nullptr;
			if( in_base->attr_based() ) {
				val = in_base->attr_get( attr );
			}
			if( !val ) val = vm.get_typefn( in_base->typefn_id(), attr );
			if( val == nullptr ) {
				vm.fail( op.idx, "type %s does not contain attribute: '%s'",
					 vm.type_name( in_base->type() ).c_str(), attr.c_str() );
				goto attr_fail;
			}
			var_dref( in_base );
			vms->push( val );
			break;
		attr_fail:
			var_dref( in_base );
			goto fail;
		}
		case OP_RET: {
			if( !op.data.b ) {
				vms->push( vm.nil );
			}
			goto done;
		}
		case OP_PUSH_LOOP: {
			vars->push_loop();
			break;
		}
		case OP_POP_LOOP: {
			vars->pop_loop();
			break;
		}
		case OP_CONTINUE: {
			vars->loop_continue();
			i = op.data.sz - 1;
			break;
		}
		case OP_BREAK: {
			// jumps to pop_loop instruction
			i = op.data.sz - 1;
			break;
		}
		// NOOP
		case _OP_LAST: {
			break;
		}
		}
	}

done:
	vars->pop_fn();
	return vm.exit_code;
fail:
	vars->pop_fn();
	return E_EXEC_FAIL;
}

}
