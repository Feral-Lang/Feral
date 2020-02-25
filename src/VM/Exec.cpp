/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Consts.hpp"

namespace vm
{

// declared in VM.hpp
int exec( vm_state_t & vm, const size_t & fn_id )
{
	srcfile_t * src = vm.src_stack.back();
	auto & vars = src->vars();
	vm_stack_t * vms = vm.vm_stack;
	const auto & bc = src->bcode().bcode();
	size_t bc_sz = bc.size();

	bool in_fn = fn_id != 0;

	vars.push_fn_id( fn_id );

	std::vector< fn_body_span_t > bodies;

	for( size_t i = 0; i < bc_sz; ++i ) {
		const op_t & op = bc[ i ];
		//fprintf( stdout, "ID: %zu %*s\n", i, 12, OpCodeStrs[ op.op ] );
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
				var_base_t * res = vars.get( op.data.s );
				if( res == nullptr ) {
					src->fail( op.idx, "variable '%s' does not exist", op.data.s );
					goto fail;
				}
				vms->push_back( res, true );
			}
			break;
		}
		case OP_ULOAD: {
			if( vms->size() > 0 ) vms->pop_back();
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
				if( !in->is_type() ) {
					src->fail( op.idx, "addition of attributes is possible only in type variables" );
					goto create_fail;
				}
				if( in->type() < VT_TYPE ) {
					src->fail( op.idx, "addition of attributes is possible only for custom types (not inbuilt ones)" );
					goto create_fail;
				}
				vartype_type_t * vt_in = static_cast< vartype_type_t * >( in );
				vt_in->add_attr( name, val );
			} else {
				vars.add( name, val->copy( op.idx ), in_fn, false );
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
			vars.blk_add( op.data.sz, in_fn );
			break;
		}
		case OP_BLKR: {
			vars.blk_rem( op.data.sz, in_fn );
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
			std::vector< std::string > args_order;
			std::unordered_map< std::string, var_base_t * > args;
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
					val = vms->back();
					vms->pop_back( false );
				}
				args_order.push_back( name );
				args[ name ] = val;
			}

			fn_body_span_t body = bodies.back();
			bodies.pop_back();

			vms->push_back( new var_fn_t( kw_arg, var_arg, args_order, args, { .feral = body }, false, op.idx ), false );
			break;
		}
		}
	}

	vars.pop_fn_id();
	return E_OK;
fail:
	vars.pop_fn_id();
	return E_EXEC_FAIL;
}

}

//static var_base_t * type_func( var_base_t * var, const std::string & fn_name, const std::vector< size_t > & types )
//{
	//auto type = 
//}