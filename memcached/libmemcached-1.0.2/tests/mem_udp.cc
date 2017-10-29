/* libMemcached Functions Test
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/*
  Sample test application.
*/

#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;

#include <libmemcached-1.0/memcached.h>
#include <libmemcached/server_instance.h>
#include <libmemcached/io.h>
#include <libmemcachedutil-1.0/util.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <libtest/server.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

/**
  @note This should be testing to see if the server really supports the binary protocol.
*/
static test_return_t pre_binary(memcached_st *memc)
{
  memcached_st *memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);

  // The memcached_version needs to be done on a clone, because the server
  // will not toggle protocol on an connection.
  memcached_version(memc_clone);

  test_compare(MEMCACHED_SUCCESS, memcached_version(memc));
  test_compare(true, libmemcached_util_version_check(memc, 1, 2, 1));
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, true));
  test_compare(true, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL));

  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

typedef std::vector<uint16_t> Expected;

static void increment_request_id(uint16_t *id)
{
  (*id)++;
  if ((*id & UDP_REQUEST_ID_THREAD_MASK) != 0)
  {
    *id= 0;
  }
}

static void get_udp_request_ids(memcached_st *memc, Expected &ids)
{
  for (uint32_t x= 0; x < memcached_server_count(memc); x++)
  {
    memcached_server_instance_st instance= memcached_server_instance_by_position(memc, x);

    ids.push_back(get_udp_datagram_request_id((struct udp_datagram_header_st *) ((memcached_server_instance_st )instance)->write_buffer));
  }
}

static test_return_t post_udp_op_check(memcached_st *memc, Expected& expected_req_ids)
{
  (void)memc;
  (void)expected_req_ids;
#if 0
  memcached_server_st *cur_server = memcached_server_list(memc);
  uint16_t *cur_req_ids = get_udp_request_ids(memc);

  for (size_t x= 0; x < memcached_server_count(memc); x++)
  {
    test_true(cur_server[x].cursor_active == 0);
    test_true(cur_req_ids[x] == expected_req_ids[x]);
  }
  free(expected_req_ids);
  free(cur_req_ids);

#endif
  return TEST_SUCCESS;
}

/*
** There is a little bit of a hack here, instead of removing
** the servers, I just set num host to 0 and them add then new udp servers
**/
static test_return_t init_udp(memcached_st *memc)
{
  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_USE_UDP, true));

  return TEST_SUCCESS;
}

static test_return_t binary_init_udp(memcached_st *memc)
{
  test_skip(TEST_SUCCESS, pre_binary(memc));

  return init_udp(memc);
}

/* Make sure that I cant add a tcp server to a udp client */
static test_return_t add_tcp_server_udp_client_test(memcached_st *memc)
{
  (void)memc;
#if 0
  memcached_server_st server;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);
  memcached_server_clone(&server, &memc->hosts[0]);
  test_true(memcached_server_remove(&(memc->hosts[0])) == MEMCACHED_SUCCESS);
  test_true(memcached_server_add(memc, server.hostname, server.port) == MEMCACHED_INVALID_HOST_PROTOCOL);
#endif
  return TEST_SUCCESS;
}

/* Make sure that I cant add a udp server to a tcp client */
static test_return_t add_udp_server_tcp_client_test(memcached_st *memc)
{
  (void)memc;
#if 0
  memcached_server_st server;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);
  memcached_server_clone(&server, &memc->hosts[0]);
  test_true(memcached_server_remove(&(memc->hosts[0])) == MEMCACHED_SUCCESS);

  memcached_st tcp_client;
  memcached_create(&tcp_client);
  test_true(memcached_server_add_udp(&tcp_client, server.hostname, server.port) == MEMCACHED_INVALID_HOST_PROTOCOL);
#endif

  return TEST_SUCCESS;
}

static test_return_t set_udp_behavior_test(memcached_st *memc)
{
  memcached_quit(memc);

  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION, memc->distribution));
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_USE_UDP, true));
  test_true(memc->flags.use_udp);
  test_true(memc->flags.no_reply);

  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_USE_UDP, false));
  test_false(memc->flags.use_udp);
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, false));
  test_false(memc->flags.no_reply);

  return TEST_SUCCESS;
}

static test_return_t udp_set_test(memcached_st *memc)
{
  unsigned int num_iters= 1025; //request id rolls over at 1024

  for (size_t x= 0; x < num_iters;x++)
  {
    Expected expected_ids;
    get_udp_request_ids(memc, expected_ids);
    unsigned int server_key= memcached_generate_hash(memc, test_literal_param("foo"));
    memcached_server_instance_st instance= memcached_server_instance_by_position(memc, server_key);
    size_t init_offset= instance->write_buffer_offset;

    memcached_return_t rc= memcached_set(memc, test_literal_param("foo"),
                                         test_literal_param("when we sanitize"),
                                         (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);
    /** NB, the check below assumes that if new write_ptr is less than
     *  the original write_ptr that we have flushed. For large payloads, this
     *  maybe an invalid assumption, but for the small payload we have it is OK
     */
    if (rc == MEMCACHED_SUCCESS or instance->write_buffer_offset < init_offset)
    {
      increment_request_id(&expected_ids[server_key]);
    }

    if (rc == MEMCACHED_SUCCESS)
    {
      test_true(instance->write_buffer_offset == UDP_DATAGRAM_HEADER_LENGTH);
    }
    else
    {
      test_true(instance->write_buffer_offset != UDP_DATAGRAM_HEADER_LENGTH);
      test_true(instance->write_buffer_offset <= MAX_UDP_DATAGRAM_LENGTH);
    }

    test_compare(TEST_SUCCESS, post_udp_op_check(memc, expected_ids));
  }

  return TEST_SUCCESS;
}

static test_return_t udp_buffered_set_test(memcached_st *memc)
{
  test_compare(MEMCACHED_INVALID_ARGUMENTS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, true));
  return TEST_SUCCESS;
}

static test_return_t udp_set_too_big_test(memcached_st *memc)
{
  char value[MAX_UDP_DATAGRAM_LENGTH];
  Expected expected_ids;
  get_udp_request_ids(memc, expected_ids);

  memset(value, int('f'), sizeof(value));

  test_compare_hint(MEMCACHED_WRITE_FAILURE, memcached_set(memc, test_literal_param("bar"), value, sizeof(value), time_t(0), uint32_t(0)),
                    memcached_last_error_message(memc));

  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_delete_test(memcached_st *memc)
{
  //request id rolls over at 1024
  for (size_t x= 0; x < 1025; x++)
  {
    Expected expected_ids;
    get_udp_request_ids(memc, expected_ids);

    unsigned int server_key= memcached_generate_hash(memc, test_literal_param("foo"));
    memcached_server_instance_st instance= memcached_server_instance_by_position(memc, server_key);
    size_t init_offset= instance->write_buffer_offset;

    memcached_return_t rc= memcached_delete(memc, test_literal_param("foo"), 0);
    test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);

    if (rc == MEMCACHED_SUCCESS or instance->write_buffer_offset < init_offset)
    {
      increment_request_id(&expected_ids[server_key]);
    }

    if (rc == MEMCACHED_SUCCESS)
    {
      test_true(instance->write_buffer_offset == UDP_DATAGRAM_HEADER_LENGTH);
    }
    else
    {
      test_true(instance->write_buffer_offset != UDP_DATAGRAM_HEADER_LENGTH);
      test_true(instance->write_buffer_offset <= MAX_UDP_DATAGRAM_LENGTH);
    }
    test_compare(TEST_SUCCESS, post_udp_op_check(memc, expected_ids));
  }

  return TEST_SUCCESS;
}

static test_return_t udp_buffered_delete_test(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1);
  return udp_delete_test(memc);
}

static test_return_t udp_verbosity_test(memcached_st *memc)
{
  Expected expected_ids;
  get_udp_request_ids(memc, expected_ids);

  for (size_t x= 0; x < memcached_server_count(memc); x++)
  {
    increment_request_id(&expected_ids[x]);
  }

  test_compare(MEMCACHED_SUCCESS, memcached_verbosity(memc, 3));

  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_quit_test(memcached_st *memc)
{
  Expected expected_ids;
  memcached_quit(memc);

  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_flush_test(memcached_st *memc)
{
  Expected expected_ids;
  get_udp_request_ids(memc, expected_ids);

  for (size_t x= 0; x < memcached_server_count(memc); x++)
  {
    increment_request_id(&expected_ids[x]);
  }
  test_compare_hint(MEMCACHED_SUCCESS, memcached_flush(memc, 0), memcached_last_error_message(memc));

  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_incr_test(memcached_st *memc)
{
  test_compare(MEMCACHED_SUCCESS, memcached_set(memc, test_literal_param("incr"), 
                                                test_literal_param("1"),
                                                (time_t)0, (uint32_t)0));

  Expected expected_ids;
  get_udp_request_ids(memc, expected_ids);

  unsigned int server_key= memcached_generate_hash(memc, test_literal_param("incr"));
  increment_request_id(&expected_ids[server_key]);

  uint64_t newvalue;
  test_compare(MEMCACHED_SUCCESS, memcached_increment(memc, test_literal_param("incr"), 1, &newvalue));

  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_decr_test(memcached_st *memc)
{
  test_compare(MEMCACHED_SUCCESS, memcached_set(memc, 
                                                test_literal_param("decr"),
                                                test_literal_param("1"),
                                                (time_t)0, (uint32_t)0));

  Expected expected_ids;
  get_udp_request_ids(memc, expected_ids);

  unsigned int server_key= memcached_generate_hash(memc, test_literal_param("decr"));
  increment_request_id(&expected_ids[server_key]);

  uint64_t newvalue;
  test_compare(MEMCACHED_SUCCESS, memcached_decrement(memc, test_literal_param("decr"), 1, &newvalue));

  return post_udp_op_check(memc, expected_ids);
}


static test_return_t udp_stat_test(memcached_st *memc)
{
  memcached_return_t rc;
  char args[]= "";
  Expected expected_ids;
  get_udp_request_ids(memc, expected_ids);
  memcached_stat_st *rv= memcached_stat(memc, args, &rc);
  memcached_stat_free(memc, rv);
  test_compare(MEMCACHED_NOT_SUPPORTED, rc);

  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_version_test(memcached_st *memc)
{
  Expected expected_ids;
  get_udp_request_ids(memc, expected_ids);

  test_compare(MEMCACHED_NOT_SUPPORTED, memcached_version(memc));

  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_get_test(memcached_st *memc)
{
  memcached_return_t rc;
  size_t vlen;
  Expected expected_ids;
  get_udp_request_ids(memc, expected_ids);
  test_null(memcached_get(memc, test_literal_param("foo"), &vlen, (uint32_t)0, &rc));
  test_compare(MEMCACHED_NOT_SUPPORTED, rc);

  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_mixed_io_test(memcached_st *memc)
{
  test_st mixed_io_ops [] ={
    {"udp_set_test", 0,
      (test_callback_fn*)udp_set_test},
    {"udp_set_too_big_test", 0,
      (test_callback_fn*)udp_set_too_big_test},
    {"udp_delete_test", 0,
      (test_callback_fn*)udp_delete_test},
    {"udp_verbosity_test", 0,
      (test_callback_fn*)udp_verbosity_test},
    {"udp_quit_test", 0,
      (test_callback_fn*)udp_quit_test},
#if 0
    {"udp_flush_test", 0,
      (test_callback_fn*)udp_flush_test},
#endif
    {"udp_incr_test", 0,
      (test_callback_fn*)udp_incr_test},
    {"udp_decr_test", 0,
      (test_callback_fn*)udp_decr_test},
    {"udp_version_test", 0,
      (test_callback_fn*)udp_version_test}
  };

  for (size_t x= 0; x < 500; x++)
  {
    test_st current_op= mixed_io_ops[(random() % 8)];
    test_compare(TEST_SUCCESS, current_op.test_fn(memc));
  }
  return TEST_SUCCESS;
}

test_st udp_setup_server_tests[] ={
  {"set_udp_behavior_test", 0, (test_callback_fn*)set_udp_behavior_test},
  {"add_tcp_server_udp_client_test", 0, (test_callback_fn*)add_tcp_server_udp_client_test},
  {"add_udp_server_tcp_client_test", 0, (test_callback_fn*)add_udp_server_tcp_client_test},
  {0, 0, 0}
};

test_st upd_io_tests[] ={
  {"udp_set_test", 0, (test_callback_fn*)udp_set_test},
  {"udp_buffered_set_test", 0, (test_callback_fn*)udp_buffered_set_test},
  {"udp_set_too_big_test", 0, (test_callback_fn*)udp_set_too_big_test},
  {"udp_delete_test", 0, (test_callback_fn*)udp_delete_test},
  {"udp_buffered_delete_test", 0, (test_callback_fn*)udp_buffered_delete_test},
  {"udp_verbosity_test", 0, (test_callback_fn*)udp_verbosity_test},
  {"udp_quit_test", 0, (test_callback_fn*)udp_quit_test},
  {"udp_flush_test", 0, (test_callback_fn*)udp_flush_test},
  {"udp_incr_test", 0, (test_callback_fn*)udp_incr_test},
  {"udp_decr_test", 0, (test_callback_fn*)udp_decr_test},
  {"udp_stat_test", 0, (test_callback_fn*)udp_stat_test},
  {"udp_version_test", 0, (test_callback_fn*)udp_version_test},
  {"udp_get_test", 0, (test_callback_fn*)udp_get_test},
  {"udp_mixed_io_test", 0, (test_callback_fn*)udp_mixed_io_test},
  {0, 0, 0}
};

collection_st collection[] ={
  {"udp_setup", (test_callback_fn*)init_udp, 0, udp_setup_server_tests},
  {"udp_io", (test_callback_fn*)init_udp, 0, upd_io_tests},
  {"udp_binary_io", (test_callback_fn*)binary_init_udp, 0, upd_io_tests},
  {0, 0, 0, 0}
};

#define TEST_PORT_BASE MEMCACHED_DEFAULT_PORT +10
#include "libmemcached_world.h"

void get_world(Framework *world)
{
  world->collections= collection;

  world->_create= (test_callback_create_fn*)world_create;
  world->_destroy= (test_callback_destroy_fn*)world_destroy;

  world->item._startup= (test_callback_fn*)world_test_startup;
  world->item._flush= (test_callback_fn*)world_flush;
  world->item.set_pre((test_callback_fn*)world_pre_run);
  world->item.set_post((test_callback_fn*)world_post_run);
  world->_on_error= (test_callback_error_fn*)world_on_error;

  world->collection_startup= (test_callback_fn*)world_container_startup;
  world->collection_shutdown= (test_callback_fn*)world_container_shutdown;

  world->set_runner(&defualt_libmemcached_runner);
}
