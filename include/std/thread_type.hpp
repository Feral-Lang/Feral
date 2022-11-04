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

#ifndef THREAD_TYPE_HPP
#define THREAD_TYPE_HPP

#include <future>
#include <thread>

#include "../VM/VM.hpp"

struct thread_res_t
{
	var_base_t *err;
	var_base_t *res;
};

class var_thread_t : public var_base_t
{
	std::thread *m_thread;
	var_fn_t *m_fn;
	std::shared_future<thread_res_t> *m_res;
	size_t m_id;
	bool m_owner;

public:
	var_thread_t(std::thread *thread, var_fn_t *fn, std::shared_future<thread_res_t> *res,
		     const bool &owner, const size_t &src_id, const size_t &idx);
	var_thread_t(std::thread *thread, var_fn_t *fn, std::shared_future<thread_res_t> *res,
		     const bool &owner, const size_t &id, const size_t &src_id, const size_t &idx);
	~var_thread_t();

	var_base_t *copy(const size_t &src_id, const size_t &idx);
	void set(var_base_t *from);

	void init_id();

	inline std::thread *&get_thread() { return m_thread; }
	inline var_fn_t *&get_fn() { return m_fn; }
	inline std::shared_future<thread_res_t> *&get_future() { return m_res; }
	inline size_t &get_id() { return m_id; }
};
#define THREAD(x) static_cast<var_thread_t *>(x)

#endif // THREAD_TYPE_HPP