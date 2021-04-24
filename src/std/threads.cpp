/*
	MIT License

	Copyright (c) 2021 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "VM/VM.hpp"

#include "std/thread_type.hpp"

thread_res_t thread_exec( vm_state_t * vm, var_fn_t *& fn, std::vector< var_base_t * > args,
			  const size_t src_id, const size_t idx )
{
	thread_res_t res = { nullptr, nullptr };
	if( !fn->call( * vm, args, {}, {}, src_id, idx ) ) {
		vm->fail( src_id, idx, "function call for thread failed" );
		// go to the next part - !vm->fails.empty()
	}
	if( !vm->fails.empty() ) {
		res.err = vm->fails.pop( false );
		if( vm->vm_stack->size() > 0 ) var_dref( vm->vm_stack->back() );
		return res;
	}
	if( vm->vm_stack->size() > 0 ) {
		res.res = vm->vm_stack->pop( false );
	} else {
		var_iref( vm->nil );
		res.res = vm->nil;
	}
	delete vm;
	for( auto & arg : args ) var_dref( arg );
	return res;
}

var_base_t * threads_max( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( std::thread::hardware_concurrency() );
}

var_base_t * threads_new( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_fn_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected function to be executed as first parameter, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	var_fn_t * fn = FN( fd.args[ 1 ] );
	return make< var_thread_t >( nullptr, fn, nullptr );
}

var_base_t * thread_start( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > args;
	args.push_back( nullptr ); // for 'self'
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		var_iref( fd.args[ i ] );
		args.push_back( fd.args[ i ] );
	}
	var_thread_t * t = THREAD( fd.args[ 0 ] );
	var_fn_t *& fn = t->get_fn();
	std::packaged_task<
		thread_res_t( vm_state_t *, var_fn_t *&, std::vector< var_base_t * >, const size_t, const size_t )
	     > task( thread_exec );
	vm_state_t * threadvm = vm.thread_copy( fd.src_id, fd.idx );
	t->get_future() = new std::shared_future< thread_res_t >( task.get_future() );
	t->get_thread() = new std::thread( std::move( task ), threadvm, std::ref( fn ), args, fd.src_id, fd.idx );
	return vm.nil;
}

var_base_t * thread_get_id( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( THREAD( fd.args[ 0 ] )->get_id() );
}

var_base_t * thread_is_done( vm_state_t & vm, const fn_data_t & fd )
{
	std::shared_future< thread_res_t > *& fut = THREAD( fd.args[ 0 ] )->get_future();
	if( !fut->valid() ) return vm.fals;
	return fut->wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready ? vm.tru : vm.fals;
}

var_base_t * thread_join( vm_state_t & vm, const fn_data_t & fd )
{
	std::shared_future< thread_res_t > *& fut = THREAD( fd.args[ 0 ] )->get_future();
	if( !fut->valid() ) fut->wait();
	if( fut->get().err ) {
		vm.fail( fd.src_id, fd.idx, fut->get().err, "thread function failed", false );
		return nullptr;
	}
	return fut->get().res;
}

INIT_MODULE( threads )
{
	var_src_t * src = vm.current_source();

	src->add_native_fn( "max", threads_max, 0 );
	src->add_native_fn( "new", threads_new, 1 );

	vm.add_native_typefn< var_thread_t >( "start",   thread_start, 0, src_id, idx, true );
	vm.add_native_typefn< var_thread_t >( "id",     thread_get_id, 0, src_id, idx );
	vm.add_native_typefn< var_thread_t >( "done",  thread_is_done, 0, src_id, idx );
	vm.add_native_typefn< var_thread_t >( "join",     thread_join, 0, src_id, idx );

	return true;
}