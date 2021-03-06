/*
   Copyright [2019] [IBM Corporation]
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <ccpm/cca.h>

#include "area_top.h"
#include "logging.h"
#include <common/errors.h> // S_OK, E_FAIL
#include <cassert>
#include <ostream>

#define FINE_TRACE 0
#if FINE_TRACE
#include <iostream>
static unsigned i = 0;
#endif

ccpm::cca::cca()
	: _top()
	, _last_top_allocate(0)
	, _last_top_free(0)
{}

ccpm::cca::cca(const region_vector_t &regions, ownership_callback_t resolver)
	: cca()
{
	reconstitute(regions, resolver, false);
}

ccpm::cca::cca(const region_vector_t &regions)
	: cca()
{
	reconstitute(regions, nullptr, true);
}

bool ccpm::cca::reconstitute(
	const region_vector_t &regions_
	, ownership_callback_t resolver_
	, const bool force_init_
)
{
	PLOG(PREFIX "reconsitiute (%s)", LOCATION, force_init_ ? "init" : "recover");
	if ( ! _top.empty() ) { return false; }
	for ( const auto & r : regions_ )
	{
		_top.push_back(
			force_init_
			? area_top::create(r)
			: area_top::restore(r, resolver_)
		);
	}
#if FINE_TRACE
	this->print(std::cerr);
#endif
	return ! _top.empty() && force_init_;
}

void ccpm::cca::add_regions(const region_vector_t &regions_)
{
	for ( const auto & r : regions_ )
	{
		area_top::create(r);
	}
}

bool ccpm::cca::includes(const void *addr) const
{
	for ( const auto &it : _top )
	{
		if ( it->includes(addr) )
		{
			return true;
		}
	}
	return false;
}

auto ccpm::cca::allocate(
	void * & ptr_
	, std::size_t bytes_
	, std::size_t alignment_
) -> status_t
{
	assert(ptr_ == nullptr);
	/* Try all regions, round robin.
	 * When ranges are available, theis can be done by a concactenation
	 * of the ranges [i .. end) and [begin .. i)
	 * Until then, use two loops.
	 */

	auto split = _top.begin() + _last_top_allocate;
#if FINE_TRACE
	PLOG(PREFIX "AL %u", LOCATION, i++);
	this->print(std::cerr);
#endif
	for ( auto it = split; it != _top.end(); ++it )
	{
		(*it)->allocate(ptr_, bytes_, alignment_);
		if ( ptr_ != nullptr )
		{
			_last_top_allocate = it - _top.begin();
#if FINE_TRACE
			PLOG(PREFIX "allocate %p.%zx", LOCATION, ptr_, bytes_);
			this->print(std::cerr);
#endif
			return S_OK;
		}
	}

	for ( auto it = _top.begin(); it != split; ++it )
	{
		(*it)->allocate(ptr_, bytes_, alignment_);
		if ( ptr_ != nullptr )
		{
			_last_top_allocate = it - _top.begin();
#if FINE_TRACE
			PLOG(PREFIX "allocate %p.%zx", LOCATION, ptr_, bytes_);
			this->print(std::cerr);
#endif
			return S_OK;
		}
	}

	return E_FAIL;
}

auto ccpm::cca::free(
	void * & ptr_
	, std::size_t bytes_
) -> status_t
{
#if FINE_TRACE
	PLOG(PREFIX "cca DE %u %p", LOCATION, i++, ptr_);
	this->print(std::cerr);
#endif

	/* Change when ranges appear (see note for allocate) */
	auto split = _top.begin() + _last_top_free;

	for ( auto it = split; it != _top.end(); ++it )
	{
		if ( (*it)->contains(ptr_) )
		{
			(*it)->deallocate(ptr_, bytes_);
			_last_top_free = it - _top.begin();
			return ptr_ == nullptr ? S_OK : E_FAIL;
		}
	}

	for ( auto it = _top.begin(); it != split; ++it )
	{
		if ( (*it)->contains(ptr_) )
		{
			(*it)->deallocate(ptr_, bytes_);
			_last_top_free = it - _top.begin();
			return ptr_ == nullptr ? S_OK : E_FAIL;
		}
	}

	return E_FAIL;
}

auto ccpm::cca::remaining(
	std::size_t & out_size_
) const -> status_t
{
	std::size_t size = 0;
	for ( const auto & t : _top )
	{
		size += t->bytes_free();
	}
	out_size_ = size;
	return _top.empty() ? E_FAIL : S_OK;
}

void ccpm::cca::print(std::ostream &o_, const std::string &title_) const
{
	o_ << title_ << ": " << "\n";
	for ( const auto & t : _top )
	{
		t->print(o_, 1);
	}
	o_ << title_ << " end" << "\n";
}
