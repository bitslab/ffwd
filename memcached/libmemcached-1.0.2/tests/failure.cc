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

#include <config.h>

/*
  C++ interface test
*/
#include <libmemcached-1.0/memcached.hpp>
#include <libmemcached/server_instance.h>
#include <libtest/test.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>

#include <string>
#include <iostream>

using namespace std;
using namespace memcache;
using namespace libtest;

Framework *global_framework= NULL;

static test_return_t shutdown_servers(memcached_st *memc)
{
  test_compare(memcached_server_count(memc), 1U);

  // Disable a single server, just the first
  global_framework->servers().shutdown(0);

  return TEST_SUCCESS;
}

static test_return_t add_shutdown_servers(memcached_st *memc)
{
  while (memcached_server_count(memc) < 2)
  {
    const char *argv[1]= { "add_shutdown_server" };
    in_port_t port= max_port() +1;
    test_true(global_framework->servers().start_socket_server("memcached", port, 1, argv));
    test_compare(MEMCACHED_SUCCESS, memcached_server_add(memc, "localhost", port));
  }

  // Disable a single server, just the first
  global_framework->servers().shutdown(0);

  return TEST_SUCCESS;
}

static test_return_t restart_servers(memcached_st *)
{
  // Restart the servers
  global_framework->servers().restart();

  return TEST_SUCCESS;
}

static test_return_t cull_TEST(memcached_st *memc)
{
  uint32_t count= memcached_server_count(memc);

  // Do not do this in your code, it is not supported.
  memc->servers[0].options.is_dead= true;
  memc->state.is_time_for_rebuild= true;

  uint32_t new_count= memcached_server_count(memc);
  test_compare(count, new_count);

  return TEST_SUCCESS;
}

static test_return_t MEMCACHED_SERVER_TEMPORARILY_DISABLED_TEST(memcached_st *memc)
{
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 30));
  test_compare_got(MEMCACHED_CONNECTION_FAILURE,
                   memcached_set(memc,
                                 test_literal_param("foo"),
                                 NULL, 0, time_t(0), uint32_t(0)),
                   memcached_last_error_message(memc));

  /*
    Setting server_failure_counter==0 should not influence the timeout that we set above,
    since we check the timeout that is created by the failure before we check how many times
    a server has failed.
  */
  test_compare(MEMCACHED_SERVER_TEMPORARILY_DISABLED,
               memcached_set(memc, test_literal_param("foo"), NULL, 0, time_t(0), uint32_t(0)));

  return TEST_SUCCESS;
}

static test_return_t MEMCACHED_SERVER_TEMPORARILY_DISABLED_to_success_TEST(memcached_st *memc)
{
  test_compare_got(MEMCACHED_CONNECTION_FAILURE,
                   memcached_set(memc,
                                 test_literal_param("foo"),
                                 NULL, 0, time_t(0), uint32_t(0)),
                   memcached_last_error_message(memc));

  /*
    Setting server_failure_counter==0 should not influence the timeout that we set above,
    since we check the timeout that is created by the failure before we check how many times
    a server has failed.
  */
  test_compare(MEMCACHED_SERVER_TEMPORARILY_DISABLED,
               memcached_set(memc, test_literal_param("foo"), NULL, 0, time_t(0), uint32_t(0)));

  global_framework->servers().restart();

  memcached_return_t ret;
  do {
    sleep(3);
    ret= memcached_set(memc, test_literal_param("foo"), NULL, 0, time_t(0), uint32_t(0));
  } while (ret == MEMCACHED_SERVER_TEMPORARILY_DISABLED);

  test_compare_got(MEMCACHED_SUCCESS, ret, memcached_last_error_message(memc));

  return TEST_SUCCESS;
}

static test_return_t MEMCACHED_SERVER_MARKED_DEAD_TEST(memcached_st *memc)
{
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 30));
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS, true));

  memcached_return_t ret;
  do {
    ret= memcached_set(memc,
                       test_literal_param("foo"),
                       NULL, 0, time_t(0), uint32_t(0));
  } while (ret == MEMCACHED_SUCCESS or ret == MEMCACHED_CONNECTION_FAILURE);
  test_compare(MEMCACHED_SERVER_TEMPORARILY_DISABLED, ret);

  do {
    sleep(3);
    ret= memcached_set(memc, test_literal_param("foo"), NULL, 0, time_t(0), uint32_t(0));
  } while (ret == MEMCACHED_SERVER_TEMPORARILY_DISABLED or ret == MEMCACHED_SUCCESS);

  test_compare_got(MEMCACHED_SERVER_MARKED_DEAD, ret, memcached_last_error_message(memc));

  return TEST_SUCCESS;
}

test_st cull_TESTS[] ={
  { "cull servers", true, (test_callback_fn*)cull_TEST },
  { 0, 0, 0 }
};

test_st server_temporarily_disabled_TESTS[] ={
  { "memcached_set(MEMCACHED_SERVER_TEMPORARILY_DISABLED)", true, (test_callback_fn*)MEMCACHED_SERVER_TEMPORARILY_DISABLED_TEST },
  { "memcached_set(MEMCACHED_SERVER_TEMPORARILY_DISABLED -> MEMCACHED_SUCCESS)", true, (test_callback_fn*)MEMCACHED_SERVER_TEMPORARILY_DISABLED_to_success_TEST },
  { 0, 0, 0 }
};

test_st server_permanently_disabled_TESTS[] ={
  { "memcached_set(MEMCACHED_SERVER_MARKED_DEAD)", true, (test_callback_fn*)MEMCACHED_SERVER_MARKED_DEAD_TEST },
  { 0, 0, 0 }
};

collection_st collection[] ={
  { "cull", (test_callback_fn*)shutdown_servers, (test_callback_fn*)restart_servers, cull_TESTS },
  { "server failed", (test_callback_fn*)shutdown_servers, (test_callback_fn*)restart_servers, server_temporarily_disabled_TESTS },
  { "server eject", (test_callback_fn*)add_shutdown_servers, (test_callback_fn*)restart_servers, server_permanently_disabled_TESTS },
  { 0, 0, 0, 0 }
};

#define TEST_PORT_BASE MEMCACHED_DEFAULT_PORT +10
#include "libmemcached_world.h"

void get_world(Framework *world)
{
  world->servers().set_count(1);

  world->collections= collection;

  world->_create= (test_callback_create_fn*)world_create;
  world->_destroy= (test_callback_destroy_fn*)world_destroy;

  world->item._startup= (test_callback_fn*)world_test_startup;
  world->item.set_pre((test_callback_fn*)world_pre_run);
  world->item.set_flush((test_callback_fn*)world_flush);
  world->item.set_post((test_callback_fn*)world_post_run);
  world->_on_error= (test_callback_error_fn*)world_on_error;

  world->collection_startup= (test_callback_fn*)world_container_startup;
  world->collection_shutdown= (test_callback_fn*)world_container_shutdown;

  world->set_runner(&defualt_libmemcached_runner);

  global_framework= world;
}
