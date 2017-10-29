/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/*
  Test that we are cycling the servers we are creating during testing.
*/

#include <config.h>
#include <libtest/test.hpp>

#include <libmemcached/memcached.h>
#include <libmemcached/is.h>
#include <libmemcached/util.h>

#include <iostream>


#include <libtest/server.h>

using namespace libtest;

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

static test_return_t alive(memcached_st *memc)
{
  test_true(memc);
  test_true(memcached_is_allocated(memc));
  for (uint32_t x= 0; x < memcached_server_count(memc); ++x)
  {
    memcached_server_instance_st instance= memcached_server_instance_by_position(memc, x);
    test_true(instance);

    test_true(libmemcached_util_ping(memcached_server_name(instance),
                                     memcached_server_port(instance), NULL));
  }

  return TEST_SUCCESS;
}

static test_return_t valid(memcached_st *memc)
{
  test_true(memc);
  test_true(memcached_is_allocated(memc));

  for (uint32_t x= 0; x < memcached_server_count(memc); ++x)
  {
    memcached_server_instance_st instance= memcached_server_instance_by_position(memc, x);
    test_true(instance);

    pid_t pid= libmemcached_util_getpid(memcached_server_name(instance),
                                        memcached_server_port(instance), NULL);
    test_true(pid != -1);
  }

  return TEST_SUCCESS;
}

static test_return_t kill_test(memcached_st *)
{
  static struct timespec global_sleep_value= { 2, 0 };

#ifdef WIN32
  sleep(1);
#else
  nanosleep(&global_sleep_value, NULL);
#endif

  return TEST_SUCCESS;
}

test_st ping_tests[] ={
  {"alive", true, (test_callback_fn*)alive },
  {0, 0, 0}
};

test_st getpid_tests[] ={
  {"valid", true, (test_callback_fn*)valid },
  {0, 0, 0}
};

test_st kill_tests[] ={
  {"kill", true, (test_callback_fn*)kill_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"libmemcached_util_ping()", 0, 0, ping_tests},
  {"libmemcached_util_getpid()", 0, 0, getpid_tests},
  {"kill", 0, 0, kill_tests},
  {0, 0, 0, 0}
};


#define TEST_PORT_BASE MEMCACHED_DEFAULT_PORT +10
#include "tests/libmemcached_world.h"

void get_world(Framework *world)
{
  world->collections= collection;

  world->_create= (test_callback_create_fn*)world_create;
  world->_destroy= (test_callback_destroy_fn*)world_destroy;

  world->item.set_startup((test_callback_fn*)world_test_startup);
  world->item.set_pre((test_callback_fn*)world_pre_run);
  world->item.set_post((test_callback_fn*)world_post_run);

  world->set_on_error((test_callback_error_fn*)world_on_error);

  world->collection_startup= (test_callback_fn*)world_container_startup;
  world->collection_shutdown= (test_callback_fn*)world_container_shutdown;

  world->set_runner(&defualt_libmemcached_runner);
  world->set_socket();
}

