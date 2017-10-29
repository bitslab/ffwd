/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libtest
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <libtest/common.h>
#include <iostream>

using namespace libtest;

static test_return_t _default_callback(void *p)
{
  (void)p;

  return TEST_SUCCESS;
}

static Runner defualt_runners;

Framework::Framework() :
  collections(NULL),
  _create(NULL),
  _destroy(NULL),
  collection_startup(_default_callback),
  collection_shutdown(_default_callback),
  _on_error(NULL),
  _runner(NULL),
  _socket(false),
  _creators_ptr(NULL)
{
}

Framework::~Framework()
{
  if (_destroy and _destroy(_creators_ptr))
  {
    Error << "Failure in _destroy(), some resources may not have been cleaned up.";
  }

  _servers.shutdown();
}

test_return_t Framework::Item::pre(void *arg)
{
  if (pre_run)
  {
    return pre_run(arg);
  }

  return TEST_SUCCESS;
}

test_return_t Framework::Item::post(void *arg)
{
  if (post_run)
  {
    return post_run(arg);
  }

  return TEST_SUCCESS;
}

test_return_t Framework::Item::flush(void* arg, test_st* run)
{
  if (run->requires_flush and _flush)
  {
    return _flush(arg);
  }

  return TEST_SUCCESS;
}

test_return_t Framework::on_error(const test_return_t rc, void* arg)
{
  if (_on_error and test_failed(_on_error(rc, arg)))
  {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

test_return_t Framework::startup(void* arg)
{
  if (collection_startup)
  {
    return collection_startup(arg);
  }

  return TEST_SUCCESS;
}

test_return_t Framework::Item::startup(void* arg)
{
  if (_startup)
  {
    return _startup(arg);
  }

  return TEST_SUCCESS;
}

libtest::Runner *Framework::runner()
{
  return _runner ? _runner : &defualt_runners;
}

void* Framework::create(test_return_t& arg)
{
  arg= TEST_SUCCESS;
  if (_create)
  {
    return _creators_ptr= _create(_servers, arg);
  }

  return NULL;
}
