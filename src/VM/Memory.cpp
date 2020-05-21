/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "VM/Memory.hpp"

namespace mem
{
size_t mult8_roundup( size_t sz )
{
	return ( sz > 512 ) ? sz : ( sz + 7 ) & ~7;
}
}

#ifdef MEM_PROFILE
static size_t tot_alloc = 0;
static size_t tot_alloc_nopool = 0;
static size_t tot_alloc_req = 0;
static size_t tot_manual_alloc = 0;
#endif
void mem_mgr_t::alloc_pool()
{
	u8 * alloc = new u8[ POOL_SIZE ];
#ifdef MEM_PROFILE
	tot_alloc += POOL_SIZE;
#endif
	m_pools.push_back( { alloc, alloc } );
}

mem_mgr_t::mem_mgr_t() { alloc_pool(); }
mem_mgr_t::~mem_mgr_t()
{
	for( auto & c : m_free_chunks ) {
		if( c.first > POOL_SIZE ) {
			for( auto & blk : c.second ) {
				delete[] blk;
			}
		}
		c.second.clear();
	}
	m_free_chunks.clear();
	for( auto & p : m_pools ) {
		delete[] p.mem;
	}
#ifdef MEM_PROFILE
	fprintf( stdout, "Total allocated: %zu bytes, without mempool: %zu, requests: %zu, manually allocated: %zu bytes\n",
		 tot_alloc, tot_alloc_nopool, tot_alloc_req, tot_manual_alloc );
#endif
}

mem_mgr_t & mem_mgr_t::instance()
{
	static mem_mgr_t mem;
	return mem;
}

void * mem_mgr_t::alloc( size_t sz )
{
	if( sz == 0 ) return nullptr;
#ifdef MEM_PROFILE
	tot_alloc_nopool += sz;
	++tot_alloc_req;
#endif
	sz = mem::mult8_roundup( sz );

	if( sz > POOL_SIZE ) {
#if defined(MEM_PROFILE)
#if defined(DEBUG_MODE)
		fprintf( stdout, "Allocating manually ... %zu\n", sz );
#endif
		tot_manual_alloc += sz;
#endif
		return new u8[ sz ];
	}

	if( m_free_chunks[ sz ].size() == 0 ) {
		for( auto & p : m_pools ) {
			size_t free_space = POOL_SIZE - ( p.head - p.mem );
			if( free_space >= sz ) {
				u8 * loc = p.head;
				p.head += sz;
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
				fprintf( stdout, "Allocating from pool ... %zu\n", sz );
#endif
				return loc;
			}
		}
		alloc_pool();
		auto & p = m_pools.back();
		u8 * loc = p.head;
		p.head += sz;
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
		fprintf( stdout, "Allocating from NEW pool ... %zu\n", sz );
#endif
		return loc;
	}

	u8 * loc = m_free_chunks[ sz ].front();
	m_free_chunks[ sz ].pop_front();
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
	fprintf( stdout, "Using previously allocated ... %zu\n", sz );
#endif
	return loc;
}

void mem_mgr_t::free( void * ptr, size_t sz )
{
	if( ptr == nullptr || sz == 0 ) return;
	if( sz > POOL_SIZE ) {
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
		fprintf( stdout, "Deleting manually ... %zu\n", sz );
#endif
		delete[] ( u8 * )ptr;
		return;
	}
#if defined(MEM_PROFILE) && defined(DEBUG_MODE)
	fprintf( stdout, "Giving back to pool ... %zu\n", sz );
#endif
	m_free_chunks[ sz ].push_front( ( u8 * )ptr );
}
