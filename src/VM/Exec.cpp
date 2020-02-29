/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Consts.hpp"
#include "Vars.hpp"

namespace vm
{

// declared in VM.hpp
int exec( vm_state_t & vm, const bcode_t & bcode, const size_t & fn_id )
{
	srcfile_t * src = vm.src_stack.back()->src();
	srcfile_vars_t * vars = vm.src_stack.back()->vars();
	vm_stack_t * vms = vm.vm_stack;
	const auto & bc = bcode.bcode();
	size_t bc_sz = bc.size();

	bool in_fn = fn_id != 0;

	if( in_fn ) vars->push_fn_id( fn_id );
	vm.add_in_fn( in_fn );

	std::vector< fn_body_span_t > bodies;

	for( size_t i = 0; i < bc_sz; ++i ) {
		const op_t & op = bc[ i ];
		// fprintf( stdout, "ID: %zu %*s\n", i, 12, OpCodeStrs[ op.op ] );
		switch( op.op ) {
		case OP_LOAD: {
			if( op.dtype != ODT_IDEN ) {
				var_base_t * res = consts::get( vm, op.dtype, op.data, op.idx );
				if( res == nullptr ) {
					src->fail( op.idx, "invalid data received as const" );
					goto fail;
				}
				vms->push_back( res, false );
			} else {
				var_base_t * res = vars->get( op.data.s );
				if( res == nullptr ) {
					res = vm.gget( op.data.s );
					if( res == nullptr ) {
						src->fail( op.idx, "variable '%s' does not exist", op.data.s );
						goto fail;
					}
				}
				vms->push_back( res, true );
			}
			break;
		}
		case OP_ULOAD: {
			vms->pop_back();
			break;
		}
		case OP_CREATE: {
			const std::string name = STR( vms->back() )->get();
			vms->pop_back();
			var_base_t * in = nullptr;
			if( op.data.b ) {
				in = vms->pop_back( false );
			}
			var_base_t * val = vms->pop_back( false );
			if( in ) {
				var_struct_t * vs = nullptr;
				if( in->type() != VT_STRUCT ) {
					src->fail( op.idx, "attributes can only be added to types" );
					goto create_fail;
				}
				vs = static_cast< var_struct_t * >( in );
				if( vs->id() < VT_STRUCT && val->type() != VT_FUNC ) {
					src->fail( op.idx, "only functions can be added to builtin types" );
					goto create_fail;
				}
				if( !vs->add_attr( name, val, true ) ) {
					src->fail( op.idx, "attribute/function: %s already exists in the type: %zu",
						   name.c_str(), vs->id() );
					goto create_fail;
				}
			} else {
				vars->add( name, val->copy( op.idx ), in_fn, false );
			}
			var_dref( in );
			var_dref( val );
			break;
		create_fail:
			var_dref( in );
			var_dref( val );
			goto fail;
		}
		case OP_STORE: {
			var_base_t * var = vms->pop_back( false );
			var_base_t * val = vms->pop_back( false );
			if( var->type() != val->type() ) {
				src->fail( op.idx, "assignment requires type of lhs and rhs to be same, found lhs: %zu, rhs: %zu"
					   "; to redeclare a variable using another type, use the 'let' statement",
					   var->type(), val->type() );
				var_dref( val );
				var_dref( var );
				goto fail;
			}
			var->set( val );
			vms->push_back( var );
			var_dref( val );
			var_dref( var );
			break;
		}
		case OP_BLKA: {
			vars->blk_add( op.data.sz, in_fn );
			break;
		}
		case OP_BLKR: {
			vars->blk_rem( op.data.sz, in_fn );
			break;
		}
		case OP_JMP: {
			i = op.data.sz - 1;
			break;
		}
		case OP_JMPTNU: // fallthrough
		case OP_JMPT: {
			if( vms->back()->type() != VT_BOOL ) {
				src->fail( op.idx, "expected boolean operand for jump instruction" );
				goto fail;
			}
			bool res = static_cast< var_bool_t * >( vms->back() )->get();
			if( res ) i = op.data.sz - 1;
			if( op.op != OP_JMPTNU ) vms->pop_back();
			break;
		}
		case OP_JMPFNU: // fallthrough
		case OP_JMPF: {
			if( vms->back()->type() != VT_BOOL ) {
				src->fail( op.idx, "expected boolean operand for jump instruction" );
				goto fail;
			}
			bool res = static_cast< var_bool_t * >( vms->back() )->get();
			if( !res ) i = op.data.sz - 1;
			if( op.op != OP_JMPFNU ) vms->pop_back();
			break;
		}
		case OP_JMPN: {
			if( vms->back()->type() == VT_NIL ) {
				vms->pop_back();
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
			std::vector< fn_assn_arg_t > def_args;
			if( op.data.s[ 0 ] == '1' ) {
				kw_arg = STR( vms->back() )->get();
				vms->pop_back();
			}
			if( op.data.s[ 1 ] == '1' ) {
				var_arg = STR( vms->back() )->get();
				vms->pop_back();
			}

			size_t arg_sz = strlen( op.data.s ) - 2;
			for( size_t i = 2; i < arg_sz; ++i ) {
				std::string name = STR( vms->back() )->get();
				vms->pop_back();
				var_base_t * val = nullptr;
				if( op.data.s[ i ] == '1' ) {
					val = vms->pop_back( false );
					def_args.push_back( { name, val } );
				} else {
					args.push_back( name );
				}
			}

			fn_body_span_t body = bodies.back();
			bodies.pop_back();

			vms->push_back( new var_fn_t( vm.src_stack.size() - 1, kw_arg, var_arg,
						      args, def_args, { .feral = body }, false,
						      op.idx ),
					false );
			break;
		}
		case OP_MEM_FNCL: // fallthrough
		case OP_FNCL: {
			size_t len = strlen( op.data.s );
			bool mem_call = op.op == OP_MEM_FNCL;
			std::vector< var_base_t * > args;
			std::vector< fn_assn_arg_t > assn_args;
			for( size_t i = 0; i < len; ++i ) {
				if( op.data.s[ i ] == '0' ) {
					args.push_back( vms->pop_back( false ) );
				} else {
					const std::string name = STR( vms->back() )->get();
					vms->pop_back();
					var_base_t * val = vms->pop_back( false );
					assn_args.push_back( { name, val } );
				}
			}
			var_base_t * in_base = nullptr;
			var_fn_t * fn = nullptr;
			if( mem_call ) {
				const std::string attr = STR( vms->back() )->get();
				vms->pop_back();
				in_base = vms->pop_back( false );
				var_base_t * val = nullptr;
				if( in_base->type() == VT_MOD ) {
					var_module_t * mod = MOD( in_base );
					val = mod->vars()->get( attr );
				}
				if( in_base->type() == VT_STRUCT ) {
					var_struct_t * in = STRUCT( in_base );
					val = in->get_attr( attr );
				}
				if( val == nullptr ) {
					var_struct_t * in = nullptr;
					// get struct of type
					in = vm.sget( in_base );
					if( in == nullptr ) {
						src->fail( op.idx, "could not find struct for type: %zu", in_base->type() );
						goto fncall_fail;
					}
					val = in->get_attr( attr );
				}
				if( val == nullptr ) {
					src->fail( op.idx, "struct %zu does not contain attribute: '%s'",
						   in_base->type(), attr.c_str() );
					goto fncall_fail;
				}
				if( val->type() != VT_FUNC ) {
					src->fail( op.idx, "this attribute is not a function" );
					goto fncall_fail;
				}
				fn = FN( val );
			} else {
				if( vms->back()->type() != VT_FUNC ) {
					src->fail( op.idx, "this variable is not a function" );
					goto fncall_fail;
				}
				fn = FN( vms->pop_back( false ) );
			}
			if( args.size() < fn->args().size() ) {
				src->fail( op.idx, "the function expects %zu arguments, provided: %zu",
					   fn->args().size(), args.size() );
				goto fncall_fail;
			}
			if( args.size() > fn->args().size() + fn->def_args().size() ) {
				if( fn->var_arg().size() == 0 ) {
					src->fail( op.idx, "the function expects [%zu, %zu] arguments, provided: %zu",
						   fn->args().size(), fn->args().size() + fn->def_args().size(),
						   args.size() );
					goto fncall_fail;
				}
			}
			args.insert( args.begin(), in_base );
			if( !fn->call( vm, args, assn_args, op.idx ) ) {
				src->fail( op.idx, "the function call failed, look up for error",
					   fn->args().size(), args.size() );
				goto fncall_fail;
			}
			for( auto & arg : args ) var_dref( arg );
			for( auto & arg : assn_args ) var_dref( arg.val );
			if( !mem_call ) var_dref( fn );
			break;
		fncall_fail:
			for( auto & arg : args ) var_dref( arg );
			for( auto & arg : assn_args ) var_dref( arg.val );
			if( !mem_call ) var_dref( fn );
			goto fail;
		}
		case OP_ATTR: {
			const std::string attr = op.data.s;
			var_base_t * in_base = vms->pop_back( false );
			var_base_t * val = nullptr;
			if( in_base->type() == VT_MOD ) {
				var_module_t * mod = MOD( in_base );
				val = mod->vars()->get( attr );
			}
			if( in_base->type() == VT_STRUCT ) {
				var_struct_t * in = STRUCT( in_base );
				val = in->get_attr( attr );
			}
			if( val == nullptr ) {
				var_struct_t * in = nullptr;
				// get struct of type
				in = vm.sget( in_base );
				if( in == nullptr ) {
					src->fail( op.idx, "could not find struct for type: %zu", in_base->type() );
					goto attr_fail;
				}
				val = in->get_attr( attr );
			}
			if( val == nullptr ) {
				src->fail( op.idx, "struct %zu does not contain attribute: '%s'",
					   in_base->type(), attr.c_str() );
				goto attr_fail;
			}
			var_dref( in_base );
			vms->push_back( val );
			break;
		attr_fail:
			var_dref( in_base );
			goto fail;
		}
		case OP_RET: {
			if( !op.data.b ) {
				vms->push_back( vm.nil );
			}
			goto done;
		}
		}
	}

done:
	vm.rem_in_fn();
	if( in_fn ) vars->pop_fn_id();
	return E_OK;
fail:
	vm.rem_in_fn();
	if( in_fn ) vars->pop_fn_id();
	return E_EXEC_FAIL;
}

}
