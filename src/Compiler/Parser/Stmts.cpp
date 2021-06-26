/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Compiler/Parser/Stmts.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// BASE //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_base_t::stmt_base_t(const GramType type, const size_t &idx) : m_idx(idx), m_type(type) {}
stmt_base_t::~stmt_base_t() {}

size_t stmt_base_t::idx() const
{
	return m_idx;
}
GramType stmt_base_t::type() const
{
	return m_type;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// SIMPLE /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_simple_t::stmt_simple_t(const lex::tok_t *val) : stmt_base_t(GT_SIMPLE, val->pos), m_val(val)
{}

void stmt_simple_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Simple at: %p\n", this);
	io::tadd(false);
	if(m_val) {
		io::print(false, "Value: %s (type: %s)\n",
			  !m_val->data.empty() ? m_val->data.c_str() : TokStrs[m_val->type],
			  TokStrs[m_val->type]);
	}
	io::trem(2);
}

const lex::tok_t *stmt_simple_t::val() const
{
	return m_val;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// BLOCK /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_block_t::stmt_block_t(const std::vector<const stmt_base_t *> &stmts, const size_t &idx)
	: stmt_base_t(GT_BLOCK, idx), m_stmts(stmts), m_no_brace(false)
{}
stmt_block_t::~stmt_block_t()
{
	for(auto &s : m_stmts) delete s;
}

void stmt_block_t::set_no_brace(const bool &no_brace)
{
	m_no_brace = no_brace;
}

void stmt_block_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Block at: %p (top level: %s)\n", this, m_no_brace ? "yes" : "no");
	for(size_t i = 0; i < m_stmts.size(); ++i) {
		m_stmts[i]->disp(i != m_stmts.size() - 1);
	}
	io::trem();
}

const std::vector<const stmt_base_t *> &stmt_block_t::stmts() const
{
	return m_stmts;
}
const bool &stmt_block_t::no_brace() const
{
	return m_no_brace;
}

void stmt_block_t::clear_stmts()
{
	m_stmts.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// EXPR //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_expr_t::stmt_expr_t(const stmt_base_t *lhs, const lex::tok_t *oper, const stmt_base_t *rhs,
			 const size_t &idx)
	: stmt_base_t(GT_EXPR, idx), m_lhs(lhs), m_rhs(rhs), m_oper(oper), m_or_blk(nullptr),
	  m_commas(0), m_with_cols(false)
{}

stmt_expr_t::~stmt_expr_t()
{
	if(m_lhs) delete m_lhs;
	if(m_rhs) delete m_rhs;
	if(m_or_blk) delete m_or_blk;
}

void stmt_expr_t::set_or_blk(stmt_base_t *or_blk, const lex::tok_t *or_blk_var)
{
	m_or_blk     = or_blk;
	m_or_blk_var = or_blk_var;
}
void stmt_expr_t::set_with_cols(const bool &with_cols)
{
	m_with_cols = with_cols;
}

const stmt_base_t *stmt_expr_t::lhs() const
{
	return m_lhs;
}
const stmt_base_t *stmt_expr_t::rhs() const
{
	return m_rhs;
}
const lex::tok_t *stmt_expr_t::oper() const
{
	return m_oper;
}
const stmt_base_t *stmt_expr_t::or_blk() const
{
	return m_or_blk;
}
const lex::tok_t *stmt_expr_t::or_blk_var() const
{
	return m_or_blk_var;
}
size_t stmt_expr_t::commas() const
{
	return m_commas;
}
const bool &stmt_expr_t::with_cols() const
{
	return m_with_cols;
}
void stmt_expr_t::commas_set(const size_t &commas)
{
	m_commas = commas;
}

void stmt_expr_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Expression at: %p (commas: %zu) (with semicolon: %s)\n", this,
		  m_commas, m_with_cols ? "yes" : "no");

	io::tadd(m_lhs != nullptr || m_rhs != nullptr);
	io::print(m_lhs != nullptr || m_rhs != nullptr, "Operator: %s\n",
		  m_oper != nullptr ? TokStrs[m_oper->type] : "(null)");
	io::trem();
	if(m_lhs != nullptr) {
		io::tadd(m_rhs != nullptr);
		io::print(m_rhs != nullptr, "LHS:\n");
		m_lhs->disp(false);
		io::trem();
	}
	if(m_rhs != nullptr) {
		io::tadd(m_or_blk != nullptr);
		io::print(m_or_blk != nullptr, "RHS:\n");
		m_rhs->disp(false);
		io::trem();
	}
	io::tadd(false);
	if(m_or_blk != nullptr) {
		io::print(false, "Or Block (var: %s):\n",
			  m_or_blk_var ? m_or_blk_var->data.c_str() : "<none>");
		m_or_blk->disp(false);
	}
	io::trem(2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VAR_DECL_BASE ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_var_decl_base_t::stmt_var_decl_base_t(const stmt_simple_t *lhs, const stmt_base_t *in,
					   const stmt_base_t *rhs)
	: stmt_base_t(GT_VAR_DECL_BASE, lhs->val()->pos), m_lhs(lhs), m_in(in), m_rhs(rhs)
{}

stmt_var_decl_base_t::~stmt_var_decl_base_t()
{
	delete m_lhs;
	if(m_in) delete m_in;
	delete m_rhs;
}

void stmt_var_decl_base_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Var Decl Base at: %p\n", this);
	io::tadd(true);
	io::print(true, "LHS:\n");
	m_lhs->disp(false);
	io::trem();
	if(m_in) {
		io::tadd(true);
		io::print(true, "In:\n");
		m_in->disp(false);
		io::trem();
	}
	io::tadd(false);
	io::print(false, "RHS:\n");
	m_rhs->disp(false);
	io::trem(2);
}

const stmt_simple_t *stmt_var_decl_base_t::lhs() const
{
	return m_lhs;
}
const stmt_base_t *stmt_var_decl_base_t::in() const
{
	return m_in;
}
const stmt_base_t *stmt_var_decl_base_t::rhs() const
{
	return m_rhs;
}

bool stmt_var_decl_base_t::has_in() const
{
	return m_in != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VAR_DECL ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_var_decl_t::stmt_var_decl_t(const std::vector<const stmt_var_decl_base_t *> &decls,
				 const size_t &idx)
	: stmt_base_t(GT_VAR_DECL, idx), m_decls(decls)
{}

stmt_var_decl_t::~stmt_var_decl_t()
{
	for(auto &decl : m_decls) delete decl;
}

void stmt_var_decl_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Var Decl(s) at: %p\n", this);
	for(size_t i = 0; i < m_decls.size(); ++i) {
		m_decls[i]->disp(i != m_decls.size() - 1);
	}
	io::trem();
}

const std::vector<const stmt_var_decl_base_t *> &stmt_var_decl_t::decls() const
{
	return m_decls;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// FUNC_ASSN_ARG ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_fn_assn_arg_t::stmt_fn_assn_arg_t(const stmt_simple_t *lhs, const stmt_base_t *rhs)
	: stmt_base_t(GT_FN_ASSN_ARG, lhs->val()->pos), m_lhs(lhs), m_rhs(rhs)
{}
stmt_fn_assn_arg_t::~stmt_fn_assn_arg_t()
{
	delete m_lhs;
	delete m_rhs;
}

void stmt_fn_assn_arg_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Assigned Argument at: %p\n", this);
	io::tadd(true);
	io::print(m_rhs, "Parameter:\n");
	m_lhs->disp(false);
	io::trem();
	if(m_rhs) {
		io::tadd(false);
		io::print(false, "Value:\n");
		m_rhs->disp(false);
		io::trem();
	}
	io::trem();
}

const stmt_simple_t *stmt_fn_assn_arg_t::lhs() const
{
	return m_lhs;
}
const stmt_base_t *stmt_fn_assn_arg_t::rhs() const
{
	return m_rhs;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// FUNC_DEF_ARGS ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_fn_def_args_t::stmt_fn_def_args_t(const std::vector<const stmt_base_t *> &args,
				       const stmt_simple_t *kwarg, const stmt_simple_t *vaarg,
				       const size_t &idx)
	: stmt_base_t(GT_FN_ARGS, idx), m_args(args), m_kwarg(kwarg), m_vaarg(vaarg)
{}
stmt_fn_def_args_t::~stmt_fn_def_args_t()
{
	for(auto &a : m_args) delete a;
	if(m_kwarg) delete m_kwarg;
	if(m_vaarg) delete m_vaarg;
}

void stmt_fn_def_args_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Arguments at: %p\n", this);
	if(m_kwarg) {
		io::tadd(true);
		io::print(m_vaarg || m_args.size() > 0, "Keyword Argument:\n");
		m_kwarg->disp(m_vaarg || m_args.size() > 0);
		io::trem();
	}
	if(m_vaarg) {
		io::tadd(true);
		io::print(m_args.size() > 0, "Variadic Argument:\n");
		m_vaarg->disp(m_args.size() > 0);
		io::trem();
	}
	for(size_t i = 0; i < m_args.size(); ++i) {
		m_args[i]->disp(i != m_args.size() - 1);
	}
	io::trem();
}
const std::vector<const stmt_base_t *> &stmt_fn_def_args_t::args() const
{
	return m_args;
}
const stmt_simple_t *stmt_fn_def_args_t::kwarg() const
{
	return m_kwarg;
}
const stmt_simple_t *stmt_fn_def_args_t::vaarg() const
{
	return m_vaarg;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// FUNC_DEF ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_fn_def_t::stmt_fn_def_t(const stmt_fn_def_args_t *args, const stmt_block_t *body,
			     const size_t &idx)
	: stmt_base_t(GT_FN_DEF, idx), m_args(args), m_body(body)
{}
stmt_fn_def_t::~stmt_fn_def_t()
{
	if(m_args) delete m_args;
	delete m_body;
}

void stmt_fn_def_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Function definition at: %p\n", this);
	if(m_args) {
		m_args->disp(true);
	}
	m_body->disp(false);
	io::trem();
}
const stmt_fn_def_args_t *stmt_fn_def_t::args() const
{
	return m_args;
}
const stmt_block_t *stmt_fn_def_t::body() const
{
	return m_body;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// FUNC_CALL_ARGS ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_fn_call_args_t::stmt_fn_call_args_t(const std::vector<const stmt_base_t *> &args,
					 const std::vector<const stmt_fn_assn_arg_t *> &assn_args,
					 const bool &va_unpack, const size_t &idx)
	: stmt_base_t(GT_FN_ARGS, idx), m_args(args), m_assn_args(assn_args), m_va_unpack(va_unpack)
{}
stmt_fn_call_args_t::~stmt_fn_call_args_t()
{
	for(auto &a : m_args) delete a;
	for(auto &a : m_assn_args) delete a;
}

void stmt_fn_call_args_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Arguments at: %p (va_unpack: %s)\n", this, m_va_unpack ? "yes" : "no");
	for(size_t i = 0; i < m_args.size(); ++i) {
		m_args[i]->disp(i != m_args.size() - 1 || m_assn_args.size() > 0);
	}
	for(size_t i = 0; i < m_assn_args.size(); ++i) {
		m_assn_args[i]->disp(i != m_assn_args.size() - 1);
	}
	io::trem();
}
const std::vector<const stmt_base_t *> &stmt_fn_call_args_t::args() const
{
	return m_args;
}
const std::vector<const stmt_fn_assn_arg_t *> &stmt_fn_call_args_t::assn_args() const
{
	return m_assn_args;
}
const bool &stmt_fn_call_args_t::va_unpack() const
{
	return m_va_unpack;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// SINGLE_EXPR_STMT //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_single_operand_stmt_t::stmt_single_operand_stmt_t(const lex::tok_t *sost,
						       const stmt_base_t *operand)
	: stmt_base_t(GT_SINGLE_OPERAND_STMT, sost->pos), m_sost(sost), m_operand(operand)
{}
stmt_single_operand_stmt_t::~stmt_single_operand_stmt_t()
{
	if(m_operand) delete m_operand;
}

void stmt_single_operand_stmt_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "%s at: %p\n", TokStrs[m_sost->type], this);
	if(m_operand) {
		io::tadd(false);
		io::print(false, "Operand:\n");
		m_operand->disp(false);
		io::trem();
	}
	io::trem();
}

const lex::tok_t *stmt_single_operand_stmt_t::sost() const
{
	return m_sost;
}
const stmt_base_t *stmt_single_operand_stmt_t::operand() const
{
	return m_operand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// CONDITIONAL_STMT //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_conditional_t::stmt_conditional_t(const std::vector<conditional_t> &conds, const size_t &idx)
	: stmt_base_t(GT_CONDITIONAL, idx), m_conds(conds)
{}
stmt_conditional_t::~stmt_conditional_t()
{
	for(auto &c : m_conds) {
		if(c.condition) delete c.condition;
		delete c.body;
	}
}

void stmt_conditional_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "Conditional at: %p\n", this);
	for(size_t i = 0; i < m_conds.size(); ++i) {
		io::tadd(i != m_conds.size() - 1);
		if(i == 0) {
			io::print(i != m_conds.size() - 1, "If:\n");
		} else if(m_conds[i].condition == nullptr) {
			io::print(i != m_conds.size() - 1, "Else:\n");
		} else {
			io::print(i != m_conds.size() - 1, "Elif:\n");
		}
		if(m_conds[i].condition) {
			m_conds[i].condition->disp(true);
		}
		m_conds[i].body->disp(false);
		io::trem();
	}
	io::trem();
}

const std::vector<conditional_t> &stmt_conditional_t::conds() const
{
	return m_conds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// FOR_STMT ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_for_t::stmt_for_t(const stmt_base_t *init, const stmt_base_t *cond, const stmt_base_t *incr,
		       const stmt_base_t *body, const size_t &idx)
	: stmt_base_t(GT_FOR, idx), m_init(init), m_cond(cond), m_incr(incr), m_body(body)
{}
stmt_for_t::~stmt_for_t()
{
	if(m_init) delete m_init;
	if(m_cond) delete m_cond;
	if(m_incr) delete m_incr;
	delete m_body;
}

void stmt_for_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "For loop at: %p\n", this);

	if(m_init) {
		io::tadd(true);
		io::print(true, "Initialization:\n");
		m_init->disp(false);
		io::trem();
	}

	if(m_cond) {
		io::tadd(true);
		io::print(true, "Condition:\n");
		m_cond->disp(false);
		io::trem();
	}

	if(m_incr) {
		io::tadd(true);
		io::print(true, "Increment:\n");
		m_incr->disp(false);
		io::trem();
	}

	io::tadd(false);

	io::print(false, "Body:\n");
	m_body->disp(false);

	io::trem(2);
}

const stmt_base_t *stmt_for_t::init() const
{
	return m_init;
}
const stmt_base_t *stmt_for_t::cond() const
{
	return m_cond;
}
const stmt_base_t *stmt_for_t::incr() const
{
	return m_incr;
}
const stmt_base_t *stmt_for_t::body() const
{
	return m_body;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// FOREACH_STMT //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_foreach_t::stmt_foreach_t(const lex::tok_t *loop_var, const stmt_base_t *expr,
			       const stmt_base_t *body, const size_t &idx)
	: stmt_base_t(GT_FOREACH, idx), m_loop_var(loop_var), m_expr(expr), m_body(body)
{}
stmt_foreach_t::~stmt_foreach_t()
{
	delete m_expr;
	delete m_body;
}

void stmt_foreach_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "For loop at: %p\n", this);

	io::tadd(true);
	io::print(true, "Loop var: %s\n", m_loop_var->data.c_str());
	io::trem();

	io::tadd(true);
	io::print(true, "Expression:\n");
	m_expr->disp(false);
	io::trem();

	io::tadd(false);

	io::print(false, "Body:\n");
	m_body->disp(false);

	io::trem(2);
}

const lex::tok_t *stmt_foreach_t::loop_var() const
{
	return m_loop_var;
}
const stmt_base_t *stmt_foreach_t::expr() const
{
	return m_expr;
}
const stmt_base_t *stmt_foreach_t::body() const
{
	return m_body;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// WHILE_STMT ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

stmt_while_t::stmt_while_t(const stmt_base_t *expr, const stmt_base_t *body, const size_t &idx)
	: stmt_base_t(GT_WHILE, idx), m_expr(expr), m_body(body)
{}
stmt_while_t::~stmt_while_t()
{
	delete m_expr;
	delete m_body;
}

void stmt_while_t::disp(const bool has_next) const
{
	io::tadd(has_next);
	io::print(has_next, "While loop at: %p\n", this);

	io::tadd(true);
	io::print(true, "Condition:\n");
	m_expr->disp(false);
	io::trem();

	io::tadd(false);
	io::print(false, "Body:\n");
	m_body->disp(false);
	io::trem(2);
}

const stmt_base_t *stmt_while_t::expr() const
{
	return m_expr;
}
const stmt_base_t *stmt_while_t::body() const
{
	return m_body;
}
