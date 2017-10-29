/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2006-2009 Brian Aker All rights reserved.
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
#include <libtest/test.hpp>

/*
  Test cases
*/

#include <libmemcached-1.0/memcached.h>
#include <libmemcached/is.h>
#include <libmemcached/server_instance.h>

#include <libhashkit-1.0/hashkit.h>

#include <cassert>
#include <cerrno>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include <libtest/server.h>

#include "clients/generator.h"
#include "clients/execute.h"

#define SMALL_STRING_LEN 1024

#include <libtest/test.hpp>

#include "tests/basic.h"
#include "tests/debug.h"
#include "tests/deprecated.h"
#include "tests/error_conditions.h"
#include "tests/exist.h"
#include "tests/ketama.h"
#include "tests/namespace.h"
#include "tests/parser.h"
#include "tests/touch.h"
#include "tests/callbacks.h"
#include "tests/pool.h"
#include "tests/print.h"
#include "tests/replication.h"
#include "tests/server_add.h"
#include "tests/virtual_buckets.h"

using namespace libtest;

#include <libmemcached/util.h>

#include "hash_results.h"

#define GLOBAL_COUNT 10000
#define GLOBAL2_COUNT 100
#define SERVERS_TO_CREATE 5
static uint32_t global_count;

static pairs_st *global_pairs;
static const char *global_keys[GLOBAL_COUNT];
static size_t global_keys_length[GLOBAL_COUNT];

/**
  @note This should be testing to see if the server really supports the binary protocol.
*/
static test_return_t pre_binary(memcached_st *memc)
{
  memcached_return_t rc= MEMCACHED_FAILURE;

  if (libmemcached_util_version_check(memc, 1, 4, 4))
  {
    rc = memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) == 1);
  }

  return rc == MEMCACHED_SUCCESS ? TEST_SUCCESS : TEST_SKIPPED;
}


static test_return_t init_test(memcached_st *not_used)
{
  memcached_st memc;
  (void)not_used;

  (void)memcached_create(&memc);
  memcached_free(&memc);

  return TEST_SUCCESS;
}

#define TEST_PORT_COUNT 7
in_port_t test_ports[TEST_PORT_COUNT];

static memcached_return_t server_display_function(const memcached_st *ptr,
                                                  const memcached_server_st *server,
                                                  void *context)
{
  /* Do Nothing */
  size_t bigger= *((size_t *)(context));
  (void)ptr;
  assert(bigger <= memcached_server_port(server));
  *((size_t *)(context))= memcached_server_port(server);

  return MEMCACHED_SUCCESS;
}

static memcached_return_t dump_server_information(const memcached_st *ptr,
                                                  const memcached_server_st *instance,
                                                  void *context)
{
  /* Do Nothing */
  FILE *stream= (FILE *)context;
  (void)ptr;

  fprintf(stream, "Memcached Server: %s %u Version %u.%u.%u\n",
          memcached_server_name(instance),
          memcached_server_port(instance),
          instance->major_version,
          instance->minor_version,
          instance->micro_version);

  return MEMCACHED_SUCCESS;
}

static test_return_t server_sort_test(memcached_st *ptr)
{
  size_t bigger= 0; /* Prime the value for the test_true in server_display_function */

  memcached_return_t rc;
  memcached_server_fn callbacks[1];
  memcached_st *local_memc;
  (void)ptr;

  local_memc= memcached_create(NULL);
  test_true(local_memc);
  memcached_behavior_set(local_memc, MEMCACHED_BEHAVIOR_SORT_HOSTS, 1);

  for (uint32_t x= 0; x < TEST_PORT_COUNT; x++)
  {
    test_ports[x]= (in_port_t)random() % 64000;
    rc= memcached_server_add_with_weight(local_memc, "localhost", test_ports[x], 0);
    test_compare(memcached_server_count(local_memc), x +1);
#if 0 // Rewrite
    test_true(memcached_server_list_count(memcached_server_list(local_memc)) == x+1);
#endif
    test_compare(MEMCACHED_SUCCESS, rc);
  }

  callbacks[0]= server_display_function;
  memcached_server_cursor(local_memc, callbacks, (void *)&bigger,  1);


  memcached_free(local_memc);

  return TEST_SUCCESS;
}

static test_return_t server_sort2_test(memcached_st *ptr)
{
  size_t bigger= 0; /* Prime the value for the test_true in server_display_function */
  memcached_server_fn callbacks[1];
  memcached_st *local_memc;
  memcached_server_instance_st instance;
  (void)ptr;

  local_memc= memcached_create(NULL);
  test_true(local_memc);
  test_compare(MEMCACHED_SUCCESS, 
               memcached_behavior_set(local_memc, MEMCACHED_BEHAVIOR_SORT_HOSTS, 1));

  test_compare(MEMCACHED_SUCCESS,
               memcached_server_add_with_weight(local_memc, "MEMCACHED_BEHAVIOR_SORT_HOSTS", 43043, 0));
  instance= memcached_server_instance_by_position(local_memc, 0);
  test_compare(in_port_t(43043), memcached_server_port(instance));

  test_compare(MEMCACHED_SUCCESS,
               memcached_server_add_with_weight(local_memc, "MEMCACHED_BEHAVIOR_SORT_HOSTS", 43042, 0));

  instance= memcached_server_instance_by_position(local_memc, 0);
  test_compare(in_port_t(43042), memcached_server_port(instance));

  instance= memcached_server_instance_by_position(local_memc, 1);
  test_compare(in_port_t(43043), memcached_server_port(instance));

  callbacks[0]= server_display_function;
  memcached_server_cursor(local_memc, callbacks, (void *)&bigger,  1);


  memcached_free(local_memc);

  return TEST_SUCCESS;
}

static test_return_t memcached_server_remove_test(memcached_st*)
{
  const char *server_string= "--server=localhost:4444 --server=localhost:4445 --server=localhost:4446 --server=localhost:4447 --server=localhost --server=memcache1.memcache.bk.sapo.pt:11211 --server=memcache1.memcache.bk.sapo.pt:11212 --server=memcache1.memcache.bk.sapo.pt:11213 --server=memcache1.memcache.bk.sapo.pt:11214 --server=memcache2.memcache.bk.sapo.pt:11211 --server=memcache2.memcache.bk.sapo.pt:11212 --server=memcache2.memcache.bk.sapo.pt:11213 --server=memcache2.memcache.bk.sapo.pt:11214";
  char buffer[BUFSIZ];

  test_compare(MEMCACHED_SUCCESS,
               libmemcached_check_configuration(server_string, strlen(server_string), buffer, sizeof(buffer)));
  memcached_st *memc= memcached(server_string, strlen(server_string));
  test_true(memc);

  memcached_server_fn callbacks[1];
  callbacks[0]= server_print_callback;
  memcached_server_cursor(memc, callbacks, NULL,  1);

  memcached_free(memc);

  return TEST_SUCCESS;
}

static memcached_return_t server_display_unsort_function(const memcached_st*,
                                                         const memcached_server_st *server,
                                                         void *context)
{
  /* Do Nothing */
  uint32_t x= *((uint32_t *)(context));

  if (! (test_ports[x] == server->port))
  {
    fprintf(stderr, "%lu -> %lu\n", (unsigned long)test_ports[x], (unsigned long)server->port);
    return MEMCACHED_FAILURE;
  }

  *((uint32_t *)(context))= ++x;

  return MEMCACHED_SUCCESS;
}

static test_return_t server_unsort_test(memcached_st *ptr)
{
  size_t counter= 0; /* Prime the value for the test_true in server_display_function */
  size_t bigger= 0; /* Prime the value for the test_true in server_display_function */
  memcached_server_fn callbacks[1];
  memcached_st *local_memc;
  (void)ptr;

  local_memc= memcached_create(NULL);
  test_true(local_memc);

  for (uint32_t x= 0; x < TEST_PORT_COUNT; x++)
  {
    test_ports[x]= (in_port_t)(random() % 64000);
    test_compare(MEMCACHED_SUCCESS,
                 memcached_server_add_with_weight(local_memc, "localhost", test_ports[x], 0));
    test_compare(memcached_server_count(local_memc), x +1);
#if 0 // Rewrite
    test_true(memcached_server_list_count(memcached_server_list(local_memc)) == x+1);
#endif
  }

  callbacks[0]= server_display_unsort_function;
  memcached_server_cursor(local_memc, callbacks, (void *)&counter,  1);

  /* Now we sort old data! */
  memcached_behavior_set(local_memc, MEMCACHED_BEHAVIOR_SORT_HOSTS, 1);
  callbacks[0]= server_display_function;
  memcached_server_cursor(local_memc, callbacks, (void *)&bigger,  1);


  memcached_free(local_memc);

  return TEST_SUCCESS;
}

static test_return_t allocation_test(memcached_st *not_used)
{
  (void)not_used;
  memcached_st *memc;
  memc= memcached_create(NULL);
  test_true(memc);
  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t clone_test(memcached_st *memc)
{
  /* All null? */
  {
    memcached_st *memc_clone;
    memc_clone= memcached_clone(NULL, NULL);
    test_true(memc_clone);
    memcached_free(memc_clone);
  }

  /* Can we init from null? */
  {
    memcached_st *memc_clone;
    memc_clone= memcached_clone(NULL, memc);
    test_true(memc_clone);

    { // Test allocators
      test_true(memc_clone->allocators.free == memc->allocators.free);
      test_true(memc_clone->allocators.malloc == memc->allocators.malloc);
      test_true(memc_clone->allocators.realloc == memc->allocators.realloc);
      test_true(memc_clone->allocators.calloc == memc->allocators.calloc);
    }

    test_true(memc_clone->connect_timeout == memc->connect_timeout);
    test_true(memc_clone->delete_trigger == memc->delete_trigger);
    test_true(memc_clone->distribution == memc->distribution);
    { // Test all of the flags
      test_true(memc_clone->flags.no_block == memc->flags.no_block);
      test_true(memc_clone->flags.tcp_nodelay == memc->flags.tcp_nodelay);
      test_true(memc_clone->flags.support_cas == memc->flags.support_cas);
      test_true(memc_clone->flags.buffer_requests == memc->flags.buffer_requests);
      test_true(memc_clone->flags.use_sort_hosts == memc->flags.use_sort_hosts);
      test_true(memc_clone->flags.verify_key == memc->flags.verify_key);
      test_true(memc_clone->ketama.weighted == memc->ketama.weighted);
      test_true(memc_clone->flags.binary_protocol == memc->flags.binary_protocol);
      test_true(memc_clone->flags.hash_with_namespace == memc->flags.hash_with_namespace);
      test_true(memc_clone->flags.no_reply == memc->flags.no_reply);
      test_true(memc_clone->flags.use_udp == memc->flags.use_udp);
      test_true(memc_clone->flags.auto_eject_hosts == memc->flags.auto_eject_hosts);
      test_true(memc_clone->flags.randomize_replica_read == memc->flags.randomize_replica_read);
    }
    test_true(memc_clone->get_key_failure == memc->get_key_failure);
    test_true(hashkit_compare(&memc_clone->hashkit, &memc->hashkit));
    test_true(memc_clone->io_bytes_watermark == memc->io_bytes_watermark);
    test_true(memc_clone->io_msg_watermark == memc->io_msg_watermark);
    test_true(memc_clone->io_key_prefetch == memc->io_key_prefetch);
    test_true(memc_clone->on_cleanup == memc->on_cleanup);
    test_true(memc_clone->on_clone == memc->on_clone);
    test_true(memc_clone->poll_timeout == memc->poll_timeout);
    test_true(memc_clone->rcv_timeout == memc->rcv_timeout);
    test_true(memc_clone->recv_size == memc->recv_size);
    test_true(memc_clone->retry_timeout == memc->retry_timeout);
    test_true(memc_clone->send_size == memc->send_size);
    test_true(memc_clone->server_failure_limit == memc->server_failure_limit);
    test_true(memc_clone->snd_timeout == memc->snd_timeout);
    test_true(memc_clone->user_data == memc->user_data);

    memcached_free(memc_clone);
  }

  /* Can we init from struct? */
  {
    memcached_st declared_clone;
    memcached_st *memc_clone;
    memset(&declared_clone, 0 , sizeof(memcached_st));
    memc_clone= memcached_clone(&declared_clone, NULL);
    test_true(memc_clone);
    memcached_free(memc_clone);
  }

  /* Can we init from struct? */
  {
    memcached_st declared_clone;
    memcached_st *memc_clone;
    memset(&declared_clone, 0 , sizeof(memcached_st));
    memc_clone= memcached_clone(&declared_clone, memc);
    test_true(memc_clone);
    memcached_free(memc_clone);
  }

  return TEST_SUCCESS;
}

static test_return_t userdata_test(memcached_st *memc)
{
  void* foo= NULL;
  test_false(memcached_set_user_data(memc, foo));
  test_true(memcached_get_user_data(memc) == foo);
  test_true(memcached_set_user_data(memc, NULL) == foo);

  return TEST_SUCCESS;
}

static test_return_t connection_test(memcached_st *memc)
{
  test_compare(MEMCACHED_SUCCESS,
               memcached_server_add_with_weight(memc, "localhost", 0, 0));

  return TEST_SUCCESS;
}

static test_return_t libmemcached_string_behavior_test(memcached_st *)
{
  for (int x= MEMCACHED_BEHAVIOR_NO_BLOCK; x < int(MEMCACHED_BEHAVIOR_MAX); ++x)
  {
    test_true(libmemcached_string_behavior(memcached_behavior_t(x)));
  }
  test_compare(36, int(MEMCACHED_BEHAVIOR_MAX));

  return TEST_SUCCESS;
}

static test_return_t libmemcached_string_distribution_test(memcached_st *)
{
  for (int x= MEMCACHED_DISTRIBUTION_MODULA; x < int(MEMCACHED_DISTRIBUTION_CONSISTENT_MAX); ++x)
  {
    test_true(libmemcached_string_distribution(memcached_server_distribution_t(x)));
  }
  test_compare(7, int(MEMCACHED_DISTRIBUTION_CONSISTENT_MAX));

  return TEST_SUCCESS;
}

static test_return_t memcached_return_t_TEST(memcached_st *memc)
{
  uint32_t values[] = { 851992627U, 2337886783U, 4109241422U, 4001849190U,
                        982370485U, 1263635348U, 4242906218U, 3829656100U,
                        1891735253U, 334139633U, 2257084983U, 3088286104U,
                        13199785U, 2542027183U, 1097051614U, 199566778U,
                        2748246961U, 2465192557U, 1664094137U, 2405439045U,
                        1842224848U, 692413798U, 3479807801U, 919913813U,
                        4269430871U, 610793021U, 527273862U, 1437122909U,
                        2300930706U, 2943759320U, 674306647U, 2400528935U,
                        54481931U, 4186304426U, 1741088401U, 2979625118U,
                        4159057246U, 3425930182U, 2593724503U,  1868899624U,
                        1769812374U, 2302537950U, 1110330676U, 3365377466U, 
                        1336171666U, 3021258493U, 2334992265U, 3861994737U, 
                        3365377466U };

  // You have updated the memcache_error messages but not updated docs/tests.
  for (int rc= int(MEMCACHED_SUCCESS); rc < int(MEMCACHED_MAXIMUM_RETURN); ++rc)
  {
    uint32_t hash_val;
    const char *msg=  memcached_strerror(memc, memcached_return_t(rc));
    hash_val= memcached_generate_hash_value(msg, strlen(msg),
                                            MEMCACHED_HASH_JENKINS);
    if (values[rc] != hash_val)
    {
      fprintf(stderr, "\n\nYou have updated memcached_return_t without updating the memcached_return_t_TEST\n");
      fprintf(stderr, "%u, %s, (%u)\n\n", (uint32_t)rc, memcached_strerror(memc, memcached_return_t(rc)), hash_val);
    }
    test_compare(values[rc], hash_val);
  }
  test_compare(48, int(MEMCACHED_MAXIMUM_RETURN));

  return TEST_SUCCESS;
}

static test_return_t set_test(memcached_st *memc)
{
  memcached_return_t rc= memcached_set(memc,
                                       test_literal_param("foo"),
                                       test_literal_param("when we sanitize"),
                                       time_t(0), (uint32_t)0);
  test_true_got(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED, memcached_strerror(NULL, rc));

  return TEST_SUCCESS;
}

static test_return_t append_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "fig";
  const char *in_value= "we";
  char *out_value= NULL;
  size_t value_length;
  uint32_t flags;

  rc= memcached_flush(memc, 0);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_set(memc, key, strlen(key),
                    in_value, strlen(in_value),
                    (time_t)0, (uint32_t)0);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_append(memc, key, strlen(key),
                       " the", strlen(" the"),
                       (time_t)0, (uint32_t)0);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_append(memc, key, strlen(key),
                       " people", strlen(" people"),
                       (time_t)0, (uint32_t)0);
  test_compare(MEMCACHED_SUCCESS, rc);

  out_value= memcached_get(memc, key, strlen(key),
                           &value_length, &flags, &rc);
  test_memcmp(out_value, "we the people", strlen("we the people"));
  test_compare(strlen("we the people"), value_length);
  test_compare(MEMCACHED_SUCCESS, rc);
  free(out_value);

  return TEST_SUCCESS;
}

static test_return_t append_binary_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "numbers";
  uint32_t store_list[] = { 23, 56, 499, 98, 32847, 0 };
  uint32_t *value;
  size_t value_length;
  uint32_t flags;
  uint32_t x;

  rc= memcached_flush(memc, 0);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_set(memc,
                    key, strlen(key),
                    NULL, 0,
                    (time_t)0, (uint32_t)0);
  test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));

  for (x= 0; store_list[x] ; x++)
  {
    rc= memcached_append(memc,
                         key, strlen(key),
                         (char *)&store_list[x], sizeof(uint32_t),
                         (time_t)0, (uint32_t)0);
    test_compare(MEMCACHED_SUCCESS, rc);
  }

  value= (uint32_t *)memcached_get(memc, key, strlen(key),
                                   &value_length, &flags, &rc);
  test_compare(value_length, sizeof(uint32_t) * x);
  test_compare(MEMCACHED_SUCCESS, rc);

  for (uint32_t counter= x, *ptr= value; counter; counter--)
  {
    test_compare(*ptr, store_list[x - counter]);
    ptr++;
  }
  free(value);

  return TEST_SUCCESS;
}

static test_return_t cas2_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};
  const char *value= "we the people";
  size_t value_length= strlen("we the people");
  memcached_result_st results_obj;
  memcached_result_st *results;
  unsigned int set= 1;

  test_compare(MEMCACHED_SUCCESS, memcached_flush(memc, 0));

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, set);

  for (uint32_t x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_compare(MEMCACHED_SUCCESS, rc);
  }

  test_compare(MEMCACHED_SUCCESS, 
               memcached_mget(memc, keys, key_length, 3));

  results= memcached_result_create(memc, &results_obj);
  test_true(results);

  results= memcached_fetch_result(memc, &results_obj, &rc);
  test_true(results);
  test_true(results->item_cas);
  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(memcached_result_cas(results));

  test_memcmp(value, "we the people", strlen("we the people"));
  test_compare(strlen("we the people"), value_length);
  test_compare(MEMCACHED_SUCCESS, rc);

  memcached_result_free(&results_obj);

  return TEST_SUCCESS;
}

static test_return_t cas_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "fun";
  size_t key_length= strlen(key);
  const char *value= "we the people";
  const char* keys[2] = { key, NULL };
  size_t keylengths[2] = { strlen(key), 0 };
  size_t value_length= strlen(value);
  const char *value2= "change the value";
  size_t value2_length= strlen(value2);

  memcached_result_st results_obj;
  memcached_result_st *results;
  unsigned int set= 1;

  rc= memcached_flush(memc, 0);
  test_compare(MEMCACHED_SUCCESS, rc);

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, set);

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_compare(MEMCACHED_SUCCESS, rc);

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, keylengths, 1));

  results= memcached_result_create(memc, &results_obj);
  test_true(results);

  results= memcached_fetch_result(memc, &results_obj, &rc);
  test_true(results);
  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(memcached_result_cas(results));
  test_memcmp(value, memcached_result_value(results), value_length);
  test_compare(strlen(memcached_result_value(results)), value_length);
  test_compare(MEMCACHED_SUCCESS, rc);
  uint64_t cas = memcached_result_cas(results);

#if 0
  results= memcached_fetch_result(memc, &results_obj, &rc);
  test_true(rc == MEMCACHED_END);
  test_true(results == NULL);
#endif

  rc= memcached_cas(memc, key, key_length, value2, value2_length, 0, 0, cas);
  test_compare(MEMCACHED_SUCCESS, rc);

  /*
   * The item will have a new cas value, so try to set it again with the old
   * value. This should fail!
   */
  rc= memcached_cas(memc, key, key_length, value2, value2_length, 0, 0, cas);
  test_compare(MEMCACHED_DATA_EXISTS, rc);

  memcached_result_free(&results_obj);

  return TEST_SUCCESS;
}

static test_return_t prepend_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "fig";
  const char *value= "people";
  char *out_value= NULL;
  size_t value_length;
  uint32_t flags;

  rc= memcached_flush(memc, 0);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_prepend(memc, key, strlen(key),
                       "the ", strlen("the "),
                       (time_t)0, (uint32_t)0);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_prepend(memc, key, strlen(key),
                       "we ", strlen("we "),
                       (time_t)0, (uint32_t)0);
  test_compare(MEMCACHED_SUCCESS, rc);

  out_value= memcached_get(memc, key, strlen(key),
                       &value_length, &flags, &rc);
  test_memcmp(out_value, "we the people", strlen("we the people"));
  test_compare(strlen("we the people"), value_length);
  test_compare(MEMCACHED_SUCCESS, rc);
  free(out_value);

  return TEST_SUCCESS;
}

/*
  Set the value, then quit to make sure it is flushed.
  Come back in and test that add fails.
*/
static test_return_t add_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  const char *value= "when we sanitize";
  unsigned long long setting_value;

  setting_value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NO_BLOCK);

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  memcached_quit(memc);
  rc= memcached_add(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);

  /* Too many broken OS'es have broken loopback in async, so we can't be sure of the result */
  if (setting_value)
  {
    test_true(rc == MEMCACHED_NOTSTORED || rc == MEMCACHED_STORED);
  }
  else
  {
    test_true(rc == MEMCACHED_NOTSTORED || rc == MEMCACHED_DATA_EXISTS);
  }

  return TEST_SUCCESS;
}

/*
** There was a problem of leaking filedescriptors in the initial release
** of MacOSX 10.5. This test case triggers the problem. On some Solaris
** systems it seems that the kernel is slow on reclaiming the resources
** because the connects starts to time out (the test doesn't do much
** anyway, so just loop 10 iterations)
*/
static test_return_t add_wrapper(memcached_st *memc)
{
  unsigned int max= 10000;
#ifdef __sun
  max= 10;
#endif
#ifdef __APPLE__
  max= 10;
#endif

  for (uint32_t x= 0; x < max; x++)
    add_test(memc);

  return TEST_SUCCESS;
}

static test_return_t replace_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  const char *value= "when we sanitize";
  const char *original= "first we insert some data";

  rc= memcached_set(memc, key, strlen(key),
                    original, strlen(original),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  test_compare(MEMCACHED_SUCCESS,
               memcached_replace(memc, key, strlen(key),
                                 value, strlen(value),
                                 (time_t)0, (uint32_t)0));

  return TEST_SUCCESS;
}

static test_return_t delete_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  const char *value= "when we sanitize";

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  rc= memcached_delete(memc, key, strlen(key), (time_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  return TEST_SUCCESS;
}

static test_return_t flush_test(memcached_st *memc)
{
  uint64_t query_id= memcached_query_id(memc);
  test_compare(MEMCACHED_SUCCESS,
               memcached_flush(memc, 0));
  test_compare(query_id +1, memcached_query_id(memc));

  return TEST_SUCCESS;
}

static memcached_return_t  server_function(const memcached_st *ptr,
                                           const memcached_server_st *server,
                                           void *context)
{
  (void)ptr; (void)server; (void)context;
  /* Do Nothing */

  return MEMCACHED_SUCCESS;
}

static test_return_t memcached_server_cursor_test(memcached_st *memc)
{
  char context[10];
  strncpy(context, "foo bad", sizeof(context));
  memcached_server_fn callbacks[1];

  callbacks[0]= server_function;
  memcached_server_cursor(memc, callbacks, context,  1);
  return TEST_SUCCESS;
}

static test_return_t bad_key_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo bad";
  uint32_t flags;
  memcached_st *memc_clone;

  uint64_t query_id= memcached_query_id(memc);
  
  // Just skip if we are in binary mode.
  test_skip(false, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL));

  test_compare(query_id, memcached_query_id(memc)); // We should not increase the query_id for memcached_behavior_get()

  memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);

  query_id= memcached_query_id(memc_clone);
  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_VERIFY_KEY, true));
  test_compare(query_id, memcached_query_id(memc_clone)); // We should not increase the query_id for memcached_behavior_set()

  /* All keys are valid in the binary protocol (except for length) */
  if (not memcached_behavior_get(memc_clone, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL))
  {
    uint64_t before_query_id= memcached_query_id(memc_clone);
    {
      size_t string_length;
      char *string= memcached_get(memc_clone, key, strlen(key),
                                  &string_length, &flags, &rc);
      test_compare(MEMCACHED_BAD_KEY_PROVIDED, rc);
      test_zero(string_length);
      test_false(string);
    }
    test_compare(before_query_id +1, memcached_query_id(memc_clone));

    query_id= memcached_query_id(memc_clone);
    test_compare(MEMCACHED_SUCCESS,
                 memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_VERIFY_KEY, false));
    test_compare(query_id, memcached_query_id(memc_clone)); // We should not increase the query_id for memcached_behavior_set()
    {
      size_t string_length;
      char *string= memcached_get(memc_clone, key, strlen(key),
                                  &string_length, &flags, &rc);
      test_compare_got(MEMCACHED_NOTFOUND, rc, memcached_strerror(NULL, rc));
      test_zero(string_length);
      test_false(string);
    }

    /* Test multi key for bad keys */
    const char *keys[] = { "GoodKey", "Bad Key", "NotMine" };
    size_t key_lengths[] = { 7, 7, 7 };
    query_id= memcached_query_id(memc_clone);
    test_compare(MEMCACHED_SUCCESS, 
                 memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_VERIFY_KEY, true));
    test_compare(query_id, memcached_query_id(memc_clone));

    query_id= memcached_query_id(memc_clone);
    test_compare(MEMCACHED_BAD_KEY_PROVIDED,
                 memcached_mget(memc_clone, keys, key_lengths, 3));
    test_compare(query_id +1, memcached_query_id(memc_clone));

    query_id= memcached_query_id(memc_clone);
    test_compare(MEMCACHED_BAD_KEY_PROVIDED,
                 memcached_mget_by_key(memc_clone, "foo daddy", 9, keys, key_lengths, 1));
    test_compare(query_id +1, memcached_query_id(memc_clone));

    /* The following test should be moved to the end of this function when the
       memcached server is updated to allow max size length of the keys in the
       binary protocol
    */
    test_compare(MEMCACHED_SUCCESS, 
                 memcached_callback_set(memc_clone, MEMCACHED_CALLBACK_NAMESPACE, NULL));

    std::vector <char> longkey;
    longkey.reserve(MEMCACHED_MAX_KEY);
    longkey.insert(longkey.end(), MEMCACHED_MAX_KEY, 'a');
    test_compare(longkey.size(), size_t(MEMCACHED_MAX_KEY));
    {
      size_t string_length;
      // We subtract 1
      test_null(memcached_get(memc_clone, &longkey[0], longkey.size() -1, &string_length, &flags, &rc));
      test_compare(MEMCACHED_NOTFOUND, rc);
      test_zero(string_length);

      test_null(memcached_get(memc_clone, &longkey[0], longkey.size(), &string_length, &flags, &rc));
      test_compare(MEMCACHED_BAD_KEY_PROVIDED, rc);
      test_zero(string_length);
    }
  }

  /* Make sure zero length keys are marked as bad */
  {
    test_compare(MEMCACHED_SUCCESS,
                 memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_VERIFY_KEY, true));
    size_t string_length;
    char *string= memcached_get(memc_clone, key, 0,
                                &string_length, &flags, &rc);
    test_compare(MEMCACHED_BAD_KEY_PROVIDED, rc);
    test_zero(string_length);
    test_false(string);
  }

  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

#define READ_THROUGH_VALUE "set for me"
static memcached_return_t read_through_trigger(memcached_st *memc,
                                               char *key,
                                               size_t key_length,
                                               memcached_result_st *result)
{
   (void)memc;(void)key;(void)key_length;
  return memcached_result_set_value(result, READ_THROUGH_VALUE, strlen(READ_THROUGH_VALUE));
}

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

static test_return_t read_through(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  char *string;
  size_t string_length;
  uint32_t flags;
  memcached_trigger_key_fn cb= (memcached_trigger_key_fn)read_through_trigger;

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_compare(MEMCACHED_NOTFOUND, rc);
  test_false(string_length);
  test_false(string);

  rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_GET_FAILURE, *(void **)&cb);
  test_compare(MEMCACHED_SUCCESS, rc);

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_compare(MEMCACHED_SUCCESS, rc);
  test_compare(string_length, sizeof(READ_THROUGH_VALUE) -1);
  test_true(string[sizeof(READ_THROUGH_VALUE) -1] == 0);
  test_strcmp(READ_THROUGH_VALUE, string);
  free(string);

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(string);
  test_compare(string_length, sizeof(READ_THROUGH_VALUE) -1);
  test_true(string[sizeof(READ_THROUGH_VALUE) -1] == 0);
  test_strcmp(READ_THROUGH_VALUE, string);
  free(string);

  return TEST_SUCCESS;
}

static test_return_t get_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  char *string;
  size_t string_length;
  uint32_t flags;

  uint64_t query_id= memcached_query_id(memc);
  rc= memcached_delete(memc, key, strlen(key), (time_t)0);
  test_true(rc == MEMCACHED_BUFFERED || rc == MEMCACHED_NOTFOUND);
  test_compare(query_id +1, memcached_query_id(memc));

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_compare_got(MEMCACHED_NOTFOUND, rc, memcached_strerror(NULL, rc));
  test_false(string_length);
  test_false(string);

  return TEST_SUCCESS;
}

static test_return_t get_test2(memcached_st *memc)
{
  const char *key= "foo";
  const char *value= "when we sanitize";

  uint64_t query_id= memcached_query_id(memc);
  memcached_return_t rc= memcached_set(memc, key, strlen(key),
                                       value, strlen(value),
                                       (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);
  test_compare(query_id +1, memcached_query_id(memc));

  query_id= memcached_query_id(memc);
  test_true(query_id);

  uint32_t flags;
  size_t string_length;
  char *string= memcached_get(memc, key, strlen(key),
                              &string_length, &flags, &rc);
  test_compare(query_id +1, memcached_query_id(memc));

  test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));
  test_compare_got(MEMCACHED_SUCCESS, memcached_last_error(memc), memcached_last_error_message(memc));
  test_true(string);
  test_compare(strlen(value), string_length);
  test_memcmp(string, value, string_length);

  free(string);

  return TEST_SUCCESS;
}

static test_return_t set_test2(memcached_st *memc)
{
  for (uint32_t x= 0; x < 10; x++)
  {
    memcached_return_t rc= memcached_set(memc,
                                         test_literal_param("foo"),
                                         test_literal_param("train in the brain"),
                                         time_t(0), uint32_t(0));
    test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);
  }

  return TEST_SUCCESS;
}

static test_return_t set_test3(memcached_st *memc)
{
  size_t value_length= 8191;

  std::vector<char> value;
  value.reserve(value_length);
  for (uint32_t x= 0; x < value_length; x++)
  {
    value.push_back(char(x % 127));
  }

  /* The dump test relies on there being at least 32 items in memcached */
  for (uint32_t x= 0; x < 32; x++)
  {
    char key[16];

    snprintf(key, sizeof(key), "foo%u", x);

    uint64_t query_id= memcached_query_id(memc);
    memcached_return_t rc= memcached_set(memc, key, strlen(key),
                                         &value[0], value.size(),
                                         (time_t)0, (uint32_t)0);
    test_true_got(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED, memcached_strerror(NULL, rc));
    test_compare(query_id +1, memcached_query_id(memc));
  }

  return TEST_SUCCESS;
}

static test_return_t get_test3(memcached_st *memc)
{
  const char *key= "foo";
  size_t value_length= 8191;

  std::vector<char> value;
  value.reserve(value_length);
  for (uint32_t x= 0; x < value_length; x++)
  {
    value.push_back(char(x % 127));
  }

  memcached_return_t rc;
  rc= memcached_set(memc, key, strlen(key),
                    &value[0], value.size(),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);

  size_t string_length;
  uint32_t flags;
  char *string= memcached_get(memc, key, strlen(key),
                              &string_length, &flags, &rc);

  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(string);
  test_compare(string_length, value_length);
  test_memcmp(string, &value[0], string_length);

  free(string);

  return TEST_SUCCESS;
}

static test_return_t get_test4(memcached_st *memc)
{
  const char *key= "foo";
  size_t value_length= 8191;

  std::vector<char> value;
  value.reserve(value_length);
  for (uint32_t x= 0; x < value_length; x++)
  {
    value.push_back(char(x % 127));
  }

  memcached_return_t rc= memcached_set(memc, key, strlen(key),
                                       &value[0], value.size(),
                                       (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);

  for (uint32_t x= 0; x < 10; x++)
  {
    uint32_t flags;
    size_t string_length;
    char *string= memcached_get(memc, key, strlen(key),
                                &string_length, &flags, &rc);

    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(string);
    test_compare(string_length, value_length);
    test_memcmp(string, &value[0], string_length);
    free(string);
  }

  return TEST_SUCCESS;
}

/*
 * This test verifies that memcached_read_one_response doesn't try to
 * dereference a NIL-pointer if you issue a multi-get and don't read out all
 * responses before you execute a storage command.
 */
static test_return_t get_test5(memcached_st *memc)
{
  /*
  ** Request the same key twice, to ensure that we hash to the same server
  ** (so that we have multiple response values queued up) ;-)
  */
  const char *keys[]= { "key", "key" };
  size_t lengths[]= { 3, 3 };
  uint32_t flags;
  size_t rlen;

  memcached_return_t rc= memcached_set(memc, keys[0], lengths[0],
                                     keys[0], lengths[0], 0, 0);
  test_compare(MEMCACHED_SUCCESS, memcached_mget(memc, keys, lengths, test_array_length(keys)));

  memcached_result_st results_obj;
  memcached_result_st *results= memcached_result_create(memc, &results_obj);
  test_true(results);

  results= memcached_fetch_result(memc, &results_obj, &rc);
  test_true(results);

  memcached_result_free(&results_obj);

  /* Don't read out the second result, but issue a set instead.. */
  test_compare(MEMCACHED_SUCCESS, memcached_set(memc, keys[0], lengths[0], keys[0], lengths[0], 0, 0));

  char *val= memcached_get_by_key(memc, keys[0], lengths[0], "yek", 3,
                                  &rlen, &flags, &rc);
  test_false(val);
  test_compare(MEMCACHED_NOTFOUND, rc);
  val= memcached_get(memc, keys[0], lengths[0], &rlen, &flags, &rc);
  test_true(val);
  test_compare(MEMCACHED_SUCCESS, rc);
  free(val);

  return TEST_SUCCESS;
}

static test_return_t mget_end(memcached_st *memc)
{
  const char *keys[]= { "foo", "foo2" };
  size_t lengths[]= { 3, 4 };
  const char *values[]= { "fjord", "41" };

  memcached_return_t rc;

  // Set foo and foo2
  for (size_t x= 0; x < test_array_length(keys); x++)
  {
    test_compare(MEMCACHED_SUCCESS, memcached_set(memc, keys[x], lengths[x], values[x], strlen(values[x]), (time_t)0, (uint32_t)0));
  }

  char *string;
  size_t string_length;
  uint32_t flags;

  // retrieve both via mget
  test_compare(MEMCACHED_SUCCESS, memcached_mget(memc, keys, lengths, test_array_length(keys)));

  char key[MEMCACHED_MAX_KEY];
  size_t key_length;

  // this should get both
  for (size_t x= 0; x < test_array_length(keys); x++)
  {
    string= memcached_fetch(memc, key, &key_length, &string_length,
                            &flags, &rc);
    test_compare(MEMCACHED_SUCCESS, rc);
    int val = 0;
    if (key_length == 4)
    {
      val= 1;
    }

    test_compare(string_length, strlen(values[val]));
    test_true(strncmp(values[val], string, string_length) == 0);
    free(string);
  }

  // this should indicate end
  string= memcached_fetch(memc, key, &key_length, &string_length, &flags, &rc);
  test_compare(MEMCACHED_END, rc);
  test_null(string);

  // now get just one
  rc= memcached_mget(memc, keys, lengths, 1);
  test_compare(MEMCACHED_SUCCESS, rc);

  string= memcached_fetch(memc, key, &key_length, &string_length, &flags, &rc);
  test_compare(key_length, lengths[0]);
  test_true(strncmp(keys[0], key, key_length) == 0);
  test_compare(string_length, strlen(values[0]));
  test_true(strncmp(values[0], string, string_length) == 0);
  test_compare(MEMCACHED_SUCCESS, rc);
  free(string);

  // this should indicate end
  string= memcached_fetch(memc, key, &key_length, &string_length, &flags, &rc);
  test_compare(MEMCACHED_END, rc);
  test_null(string);

  return TEST_SUCCESS;
}

/* Do not copy the style of this code, I just access hosts to testthis function */
static test_return_t stats_servername_test(memcached_st *memc)
{
  memcached_stat_st memc_stat;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  if (LIBMEMCACHED_WITH_SASL_SUPPORT and memcached_get_sasl_callbacks(memc))
  {
    return TEST_SKIPPED;
  }

  test_compare(MEMCACHED_SUCCESS, memcached_stat_servername(&memc_stat, NULL,
                                                            memcached_server_name(instance),
                                                            memcached_server_port(instance)));

  return TEST_SUCCESS;
}

static test_return_t increment_test(memcached_st *memc)
{
  uint64_t new_number;

  test_compare(MEMCACHED_SUCCESS, 
               memcached_set(memc, 
                             test_literal_param("number"),
                             test_literal_param("0"),
                             (time_t)0, (uint32_t)0));

  test_compare(MEMCACHED_SUCCESS, 
               memcached_increment(memc, test_literal_param("number"), 1, &new_number));
  test_compare(uint64_t(1), new_number);

  test_compare(MEMCACHED_SUCCESS, 
               memcached_increment(memc, test_literal_param("number"), 1, &new_number));
  test_compare(uint64_t(2), new_number);

  return TEST_SUCCESS;
}

static test_return_t increment_with_initial_test(memcached_st *memc)
{
  test_skip(true, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL));

  uint64_t new_number;
  uint64_t initial= 0;

  test_compare(MEMCACHED_SUCCESS, memcached_flush_buffers(memc));

  test_compare(MEMCACHED_SUCCESS, 
               memcached_increment_with_initial(memc, test_literal_param("number"), 1, initial, 0, &new_number));
  test_compare(new_number, initial);

  test_compare(MEMCACHED_SUCCESS, 
               memcached_increment_with_initial(memc, test_literal_param("number"), 1, initial, 0, &new_number));
  test_compare(new_number, (initial +1));

  return TEST_SUCCESS;
}

static test_return_t decrement_test(memcached_st *memc)
{
  uint64_t new_number;
  memcached_return_t rc;
  const char *value= "3";

  rc= memcached_set(memc,
                    test_literal_param("number"),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);

  test_compare(MEMCACHED_SUCCESS,
               memcached_decrement(memc,
                                   test_literal_param("number"),
                                   1, &new_number));
  test_compare(uint64_t(2), new_number);

  test_compare(MEMCACHED_SUCCESS,
               memcached_decrement(memc,
                                   test_literal_param("number"),
                                   1, &new_number));
  test_compare(uint64_t(1), new_number);

  return TEST_SUCCESS;
}

static test_return_t decrement_with_initial_test(memcached_st *memc)
{
  test_skip(true, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL));

  uint64_t new_number;
  uint64_t initial= 3;

  test_compare(MEMCACHED_SUCCESS, memcached_flush_buffers(memc));

  test_compare(MEMCACHED_SUCCESS,
               memcached_decrement_with_initial(memc,
                                                test_literal_param("number"),
                                                1, initial, 0, &new_number));
  test_compare(new_number, initial);

  test_compare(MEMCACHED_SUCCESS,
               memcached_decrement_with_initial(memc,
                                                test_literal_param("number"),
                                                1, initial, 0, &new_number));
  test_compare(new_number, (initial - 1));

  return TEST_SUCCESS;
}

static test_return_t increment_by_key_test(memcached_st *memc)
{
  uint64_t new_number;
  memcached_return_t rc;
  const char *master_key= "foo";
  const char *key= "number";
  const char *value= "0";

  rc= memcached_set_by_key(memc, master_key, strlen(master_key),
                           key, strlen(key),
                           value, strlen(value),
                           (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);

  test_compare(MEMCACHED_SUCCESS,
               memcached_increment_by_key(memc, master_key, strlen(master_key), key, strlen(key), 1, &new_number));
  test_compare(uint64_t(1), new_number);

  test_compare(MEMCACHED_SUCCESS,
               memcached_increment_by_key(memc, master_key, strlen(master_key), key, strlen(key), 1, &new_number));
  test_compare(uint64_t(2), new_number);

  return TEST_SUCCESS;
}

static test_return_t increment_with_initial_by_key_test(memcached_st *memc)
{
  test_skip(true, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL));

  uint64_t new_number;
  memcached_return_t rc;
  const char *master_key= "foo";
  const char *key= "number";
  uint64_t initial= 0;

  rc= memcached_increment_with_initial_by_key(memc, master_key, strlen(master_key),
                                              key, strlen(key),
                                              1, initial, 0, &new_number);
  test_compare(MEMCACHED_SUCCESS, rc);
  test_compare(new_number, initial);

  rc= memcached_increment_with_initial_by_key(memc, master_key, strlen(master_key),
                                              key, strlen(key),
                                              1, initial, 0, &new_number);
  test_compare(MEMCACHED_SUCCESS, rc);
  test_compare(new_number, (initial +1));

  return TEST_SUCCESS;
}

static test_return_t decrement_by_key_test(memcached_st *memc)
{
  uint64_t new_number;
  memcached_return_t rc;
  const char *value= "3";

  rc= memcached_set_by_key(memc,
                           test_literal_param("foo"),
                           test_literal_param("number"),
                           value, strlen(value),
                           (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);

  test_compare(MEMCACHED_SUCCESS,
               memcached_decrement_by_key(memc,
                                          test_literal_param("foo"),
                                          test_literal_param("number"),
                                          1, &new_number));
  test_compare(uint64_t(2), new_number);

  test_compare(MEMCACHED_SUCCESS,
               memcached_decrement_by_key(memc,
                                          test_literal_param("foo"),
                                          test_literal_param("number"),
                                          1, &new_number));
  test_compare(uint64_t(1), new_number);

  return TEST_SUCCESS;
}

static test_return_t decrement_with_initial_by_key_test(memcached_st *memc)
{
  test_skip(true, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL));

  uint64_t new_number;
  uint64_t initial= 3;

  test_compare(MEMCACHED_SUCCESS,
               memcached_decrement_with_initial_by_key(memc,
                                                       test_literal_param("foo"),
                                                       test_literal_param("number"),
                                                       1, initial, 0, &new_number));
  test_compare(new_number, initial);

  test_compare(MEMCACHED_SUCCESS,
               memcached_decrement_with_initial_by_key(memc,
                                                       test_literal_param("foo"),
                                                       test_literal_param("number"),
                                                       1, initial, 0, &new_number));
  test_compare(new_number, (initial - 1));

  return TEST_SUCCESS;
}
static test_return_t binary_increment_with_prefix_test(memcached_st *orig_memc)
{
  memcached_st *memc= memcached_clone(NULL, orig_memc);

  test_skip(TEST_SUCCESS, pre_binary(memc));

  test_compare(MEMCACHED_SUCCESS, memcached_callback_set(memc, MEMCACHED_CALLBACK_PREFIX_KEY, (void *)"namespace:"));

  memcached_return_t rc;
  rc= memcached_set(memc,
                    test_literal_param("number"),
                    test_literal_param("0"),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  uint64_t new_number;
  test_compare(MEMCACHED_SUCCESS, memcached_increment(memc, 
                                                      test_literal_param("number"), 
                                                      1, &new_number));
  test_compare(uint64_t(1), new_number);

  test_compare(MEMCACHED_SUCCESS, memcached_increment(memc,
                                                      test_literal_param("number"),
                                                      1, &new_number));
  test_compare(uint64_t(2), new_number);
  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t quit_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "fudge";
  const char *value= "sanford and sun";

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)10, (uint32_t)3);
  test_true_got(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED, memcached_strerror(NULL, rc));
  memcached_quit(memc);

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)50, (uint32_t)9);
  test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);

  return TEST_SUCCESS;
}

static test_return_t mget_result_test(memcached_st *memc)
{
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};

  memcached_result_st results_obj;
  memcached_result_st *results;

  results= memcached_result_create(memc, &results_obj);
  test_true(results);
  test_true(&results_obj == results);

  /* We need to empty the server before continueing test */
  test_compare(MEMCACHED_SUCCESS,
               memcached_flush(memc, 0));

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, key_length, 3));

  memcached_return_t rc;
  while ((results= memcached_fetch_result(memc, &results_obj, &rc)))
  {
    test_true(results);
  }

  while ((results= memcached_fetch_result(memc, &results_obj, &rc))) { test_true(false); /* We should never see a value returned */ };
  test_false(results);
  test_compare_got(MEMCACHED_NOTFOUND, rc, memcached_strerror(NULL, rc));

  for (uint32_t x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true_got(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED, memcached_strerror(NULL, rc));
  }

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, key_length, 3));

  while ((results= memcached_fetch_result(memc, &results_obj, &rc)))
  {
    test_true(results);
    test_true(&results_obj == results);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_memcmp(memcached_result_key_value(results),
                memcached_result_value(results),
                memcached_result_length(results));
    test_compare(memcached_result_key_length(results), memcached_result_length(results));
  }

  memcached_result_free(&results_obj);

  return TEST_SUCCESS;
}

static test_return_t mget_result_alloc_test(memcached_st *memc)
{
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};

  memcached_result_st *results;

  /* We need to empty the server before continueing test */
  test_compare(MEMCACHED_SUCCESS,
               memcached_flush(memc, 0));

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, key_length, 3));

  memcached_return_t rc;
  while ((results= memcached_fetch_result(memc, NULL, &rc)))
  {
    test_true(results);
  }
  test_false(results);
  test_compare_got(MEMCACHED_NOTFOUND, rc, memcached_strerror(NULL, rc));

  for (uint32_t x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true_got(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED, memcached_strerror(NULL, rc));
  }

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, key_length, 3));

  uint32_t x= 0;
  while ((results= memcached_fetch_result(memc, NULL, &rc)))
  {
    test_true(results);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_compare(memcached_result_key_length(results), memcached_result_length(results));
    test_memcmp(memcached_result_key_value(results),
                memcached_result_value(results),
                memcached_result_length(results));
    memcached_result_free(results);
    x++;
  }

  return TEST_SUCCESS;
}

/* Count the results */
static memcached_return_t callback_counter(const memcached_st*, memcached_result_st*, void *context)
{
  size_t *counter= (size_t *)context;

  *counter= *counter + 1;

  return MEMCACHED_SUCCESS;
}

static test_return_t mget_result_function(memcached_st *memc)
{
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};
  size_t counter;
  memcached_execute_fn callbacks[1];

  /* We need to empty the server before continueing test */
  test_compare(MEMCACHED_SUCCESS,
               memcached_flush(memc, 0));
  for (uint32_t x= 0; x < 3; x++)
  {
    memcached_return_t rc= memcached_set(memc, keys[x], key_length[x],
                                         keys[x], key_length[x],
                                         (time_t)50, (uint32_t)9);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, key_length, 3));

  callbacks[0]= &callback_counter;
  counter= 0;

  test_compare(MEMCACHED_SUCCESS, 
               memcached_fetch_execute(memc, callbacks, (void *)&counter, 1));

  test_compare(size_t(3), counter);

  return TEST_SUCCESS;
}

static test_return_t mget_test(memcached_st *memc)
{
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};

  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *return_value;
  size_t return_value_length;

  /* We need to empty the server before continueing test */
  test_compare(MEMCACHED_SUCCESS,
               memcached_flush(memc, 0));

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, key_length, 3));

  uint32_t flags;
  memcached_return_t rc;
  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                        &return_value_length, &flags, &rc)))
  {
    test_true(return_value);
  }
  test_false(return_value);
  test_zero(return_value_length);
  test_compare(MEMCACHED_NOTFOUND, rc);

  for (uint32_t x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true_got(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED, memcached_strerror(NULL, rc));
  }
  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, key_length, 3));

  uint32_t x= 0;
  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                        &return_value_length, &flags, &rc)))
  {
    test_true(return_value);
    test_compare(MEMCACHED_SUCCESS, rc);
    if (not memc->_namespace)
    {
      test_compare(return_key_length, return_value_length);
      test_memcmp(return_value, return_key, return_value_length);
    }
    free(return_value);
    x++;
  }

  return TEST_SUCCESS;
}

static test_return_t mget_execute(memcached_st *memc)
{
  bool binary= false;

  if (memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) != 0)
    binary= true;

  /*
   * I only want to hit _one_ server so I know the number of requests I'm
   * sending in the pipeline.
   */
  uint32_t number_of_hosts= memc->number_of_hosts;
  memc->number_of_hosts= 1;

  size_t max_keys= 20480;


  char **keys= static_cast<char **>(calloc(max_keys, sizeof(char*)));
  size_t *key_length=static_cast<size_t *>(calloc(max_keys, sizeof(size_t)));

  /* First add all of the items.. */
  char blob[1024] = {0};
  memcached_return_t rc;

  for (size_t x= 0; x < max_keys; ++x)
  {
    char k[251];

    key_length[x]= (size_t)snprintf(k, sizeof(k), "0200%lu", (unsigned long)x);
    keys[x]= strdup(k);
    test_true(keys[x] != NULL);
    uint64_t query_id= memcached_query_id(memc);
    rc= memcached_add(memc, keys[x], key_length[x], blob, sizeof(blob), 0, 0);
    test_true_got(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED, memcached_strerror(NULL, rc));
    test_compare(query_id +1, memcached_query_id(memc));
  }

  /* Try to get all of them with a large multiget */
  size_t counter= 0;
  memcached_execute_fn callbacks[]= { &callback_counter };
  rc= memcached_mget_execute(memc, (const char**)keys, key_length,
                             max_keys, callbacks, &counter, 1);

  if (memcached_success(rc))
  {
    test_true(binary);
    uint64_t query_id= memcached_query_id(memc);
    test_compare(MEMCACHED_SUCCESS, 
                 memcached_fetch_execute(memc, callbacks, (void *)&counter, 1));
    test_compare(query_id, memcached_query_id(memc));

    /* Verify that we got all of the items */
    test_true(counter == max_keys);
  }
  else if (rc == MEMCACHED_NOT_SUPPORTED)
  {
    test_true(counter == 0);
  }
  else
  {
    test_fail("note: this test functions differently when in binary mode");
  }

  /* Release all allocated resources */
  for (size_t x= 0; x < max_keys; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);

  memc->number_of_hosts= number_of_hosts;
  return TEST_SUCCESS;
}

#define REGRESSION_BINARY_VS_BLOCK_COUNT  20480

static test_return_t key_setup(memcached_st *memc)
{
  test_skip(TEST_SUCCESS, pre_binary(memc));

  global_pairs= pairs_generate(REGRESSION_BINARY_VS_BLOCK_COUNT, 0);

  return TEST_SUCCESS;
}

static test_return_t key_teardown(memcached_st *memc)
{
  (void)memc;
  pairs_free(global_pairs);

  return TEST_SUCCESS;
}

static test_return_t block_add_regression(memcached_st *memc)
{
  /* First add all of the items.. */
  for (size_t x= 0; x < REGRESSION_BINARY_VS_BLOCK_COUNT; ++x)
  {
    memcached_return_t rc;
    char blob[1024] = {0};

    rc= memcached_add_by_key(memc, "bob", 3, global_pairs[x].key, global_pairs[x].key_length, blob, sizeof(blob), 0, 0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  return TEST_SUCCESS;
}

static test_return_t binary_add_regression(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
  test_return_t rc= block_add_regression(memc);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
  return rc;
}

static test_return_t get_stats_keys(memcached_st *memc)
{
 char **stat_list;
 char **ptr;
 memcached_stat_st memc_stat;
 memcached_return_t rc;

 stat_list= memcached_stat_get_keys(memc, &memc_stat, &rc);
 test_compare(MEMCACHED_SUCCESS, rc);
 for (ptr= stat_list; *ptr; ptr++)
   test_true(*ptr);

 free(stat_list);

 return TEST_SUCCESS;
}

static test_return_t version_string_test(memcached_st *)
{
  test_strcmp(LIBMEMCACHED_VERSION_STRING, memcached_lib_version());

  return TEST_SUCCESS;
}

static test_return_t get_stats(memcached_st *memc)
{
 memcached_return_t rc;

 memcached_stat_st *memc_stat= memcached_stat(memc, NULL, &rc);
 test_compare(MEMCACHED_SUCCESS, rc);
 test_true(memc_stat);

 for (uint32_t x= 0; x < memcached_server_count(memc); x++)
 {
   char **stat_list= memcached_stat_get_keys(memc, memc_stat+x, &rc);
   test_compare(MEMCACHED_SUCCESS, rc);
   for (char **ptr= stat_list; *ptr; ptr++) {};

   free(stat_list);
 }

 memcached_stat_free(NULL, memc_stat);

  return TEST_SUCCESS;
}

static test_return_t add_host_test(memcached_st *memc)
{
  char servername[]= "0.example.com";

  memcached_return_t rc;
  memcached_server_st *servers= memcached_server_list_append_with_weight(NULL, servername, 400, 0, &rc);
  test_compare(1U, memcached_server_list_count(servers));

  for (unsigned int x= 2; x < 20; x++)
  {
    char buffer[SMALL_STRING_LEN];

    snprintf(buffer, SMALL_STRING_LEN, "%u.example.com", 400+x);
    servers= memcached_server_list_append_with_weight(servers, buffer, 401, 0,
                                     &rc);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_compare(x, memcached_server_list_count(servers));
  }

  test_compare(MEMCACHED_SUCCESS, memcached_server_push(memc, servers));
  test_compare(MEMCACHED_SUCCESS, memcached_server_push(memc, servers));

  memcached_server_list_free(servers);

  return TEST_SUCCESS;
}

static test_return_t memcached_fetch_result_NOT_FOUND(memcached_st *memc)
{
  memcached_return_t rc;

  const char *key= "not_found";
  size_t key_length= test_literal_param_size("not_found");

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, &key, &key_length, 1));

  memcached_result_st *result= memcached_fetch_result(memc, NULL, &rc);
  test_null(result);
  test_compare_got(MEMCACHED_NOTFOUND, rc, memcached_strerror(NULL, rc));

  memcached_result_free(result);

  return TEST_SUCCESS;
}

static memcached_return_t  clone_test_callback(memcached_st *, memcached_st *)
{
  return MEMCACHED_SUCCESS;
}

static memcached_return_t  cleanup_test_callback(memcached_st *)
{
  return MEMCACHED_SUCCESS;
}

static test_return_t callback_test(memcached_st *memc)
{
  /* Test User Data */
  {
    int x= 5;
    int *test_ptr;
    memcached_return_t rc;

    test_compare(MEMCACHED_SUCCESS, memcached_callback_set(memc, MEMCACHED_CALLBACK_USER_DATA, &x));
    test_ptr= (int *)memcached_callback_get(memc, MEMCACHED_CALLBACK_USER_DATA, &rc);
    test_true(*test_ptr == x);
  }

  /* Test Clone Callback */
  {
    memcached_clone_fn clone_cb= (memcached_clone_fn)clone_test_callback;
    void *clone_cb_ptr= *(void **)&clone_cb;
    void *temp_function= NULL;

    test_compare(MEMCACHED_SUCCESS, memcached_callback_set(memc, MEMCACHED_CALLBACK_CLONE_FUNCTION, clone_cb_ptr));
    memcached_return_t rc;
    temp_function= memcached_callback_get(memc, MEMCACHED_CALLBACK_CLONE_FUNCTION, &rc);
    test_true(temp_function == clone_cb_ptr);
    test_compare(MEMCACHED_SUCCESS, rc);
  }

  /* Test Cleanup Callback */
  {
    memcached_cleanup_fn cleanup_cb= (memcached_cleanup_fn)cleanup_test_callback;
    void *cleanup_cb_ptr= *(void **)&cleanup_cb;
    void *temp_function= NULL;
    memcached_return_t rc;

    test_compare(MEMCACHED_SUCCESS, memcached_callback_set(memc, MEMCACHED_CALLBACK_CLONE_FUNCTION, cleanup_cb_ptr));
    temp_function= memcached_callback_get(memc, MEMCACHED_CALLBACK_CLONE_FUNCTION, &rc);
    test_true(temp_function == cleanup_cb_ptr);
  }

  return TEST_SUCCESS;
}

/* We don't test the behavior itself, we test the switches */
static test_return_t behavior_test(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
  test_compare(true, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NO_BLOCK));

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1);
  test_compare(true, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY));

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, MEMCACHED_HASH_MD5);
  test_compare(uint64_t(MEMCACHED_HASH_MD5), memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH));

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 0);
  test_zero(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NO_BLOCK));

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, 0);
  test_zero(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY));

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, MEMCACHED_HASH_DEFAULT);
  test_compare(uint64_t(MEMCACHED_HASH_DEFAULT), memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH));

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, MEMCACHED_HASH_CRC);
  test_compare(uint64_t(MEMCACHED_HASH_CRC), memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH));

  test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE));

  test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE));

  uint64_t value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS, value +1);
  test_compare((value +1),  memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS));

  return TEST_SUCCESS;
}

static test_return_t MEMCACHED_BEHAVIOR_CORK_test(memcached_st *memc)
{
  test_compare(MEMCACHED_DEPRECATED, 
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CORK, true));

  // Platform dependent
#if 0
  bool value= (bool)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_CORK);
  test_false(value);
#endif

  return TEST_SUCCESS;
}


static test_return_t MEMCACHED_BEHAVIOR_TCP_KEEPALIVE_test(memcached_st *memc)
{
  memcached_return_t rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_KEEPALIVE, true);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_NOT_SUPPORTED);

  bool value= (bool)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_TCP_KEEPALIVE);

  if (memcached_success(rc))
  {
    test_true(value);
  }
  else
  {
    test_false(value);
  }

  return TEST_SUCCESS;
}


static test_return_t MEMCACHED_BEHAVIOR_TCP_KEEPIDLE_test(memcached_st *memc)
{
  memcached_return_t rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_KEEPIDLE, true);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_NOT_SUPPORTED);

  bool value= (bool)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_TCP_KEEPIDLE);

  if (memcached_success(rc))
  {
    test_true(value);
  }
  else
  {
    test_false(value);
  }

  return TEST_SUCCESS;
}

static test_return_t fetch_all_results(memcached_st *memc, unsigned int &keys_returned, const memcached_return_t expect)
{
  memcached_return_t rc;
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *return_value;
  size_t return_value_length;
  uint32_t flags;

  keys_returned= 0;
  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                        &return_value_length, &flags, &rc)))
  {
    test_true(return_value);
    test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));
    free(return_value);
    keys_returned+= 1;
  }

  if (memcached_success(expect) and memcached_success(rc))
  {
    return TEST_SUCCESS;
  }
  else if (expect == rc)
  {
    return TEST_SUCCESS;
  }
  fprintf(stderr, "\n%s:%u %s(#%u)\n", __FILE__, __LINE__, memcached_strerror(NULL, rc), keys_returned);

  return TEST_FAILURE;
}

/* Test case provided by Cal Haldenbrand */
#define HALDENBRAND_KEY_COUNT 3000U // * 1024576
#define HALDENBRAND_FLAG_KEY 99 // * 1024576
static test_return_t user_supplied_bug1(memcached_st *memc)
{
  /* We just keep looking at the same values over and over */
  srandom(10);

  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, true));
  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, true));


  /* add key */
  unsigned long long total= 0;
  for (uint32_t x= 0 ; total < 20 * 1024576 ; x++ )
  {
    uint32_t size= (uint32_t)(rand() % ( 5 * 1024 ) ) + 400;
    char randomstuff[6 * 1024];
    memset(randomstuff, 0, 6 * 1024);
    test_true(size < 6 * 1024); /* Being safe here */

    for (uint32_t j= 0 ; j < size ;j++)
    {
      randomstuff[j] = (signed char) ((rand() % 26) + 97);
    }

    total+= size;
    char key[22];
    int key_length= snprintf(key, sizeof(key), "%u", x);
    test_compare(MEMCACHED_SUCCESS,
                 memcached_set(memc, key, key_length, randomstuff, strlen(randomstuff), time_t(0), HALDENBRAND_FLAG_KEY));
  }
  test_true(total > HALDENBRAND_KEY_COUNT);

  return TEST_SUCCESS;
}

/* Test case provided by Cal Haldenbrand */
static test_return_t user_supplied_bug2(memcached_st *memc)
{
  test_compare(MEMCACHED_SUCCESS, 
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, true));

  test_compare(MEMCACHED_SUCCESS, 
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, true));

#ifdef NOT_YET
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE, 20 * 1024576));
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE, 20 * 1024576));
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE);

  for (x= 0, errors= 0; total < 20 * 1024576 ; x++)
#endif

  size_t total_value_length= 0;
  for (uint32_t x= 0, errors= 0; total_value_length < 24576 ; x++)
  {
    uint32_t flags= 0;
    size_t val_len= 0;

    char key[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
    int key_length= snprintf(key, sizeof(key), "%u", x);

    memcached_return_t rc;
    char *getval= memcached_get(memc, key, key_length, &val_len, &flags, &rc);
    if (memcached_failed(rc))
    {
      if (rc == MEMCACHED_NOTFOUND)
      {
        errors++;
      }
      else
      {
        test_true(rc);
      }

      continue;
    }
    test_compare(uint32_t(HALDENBRAND_FLAG_KEY), flags);

    total_value_length+= val_len;
    errors= 0;
    free(getval);
  }

  return TEST_SUCCESS;
}

/* Do a large mget() over all the keys we think exist */
static test_return_t user_supplied_bug3(memcached_st *memc)
{
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1));
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1));

#ifdef NOT_YET
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE, 20 * 1024576);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE, 20 * 1024576);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE);
#endif

  size_t key_lengths[HALDENBRAND_KEY_COUNT];
  char **keys= static_cast<char **>(calloc(HALDENBRAND_KEY_COUNT, sizeof(char *)));
  test_true(keys);
  for (uint32_t x= 0; x < HALDENBRAND_KEY_COUNT; x++)
  {
    char key[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
    int key_length= snprintf(key, sizeof(key), "%u", x);
    test_true(key_length);
    keys[x]= strdup(key);
    test_true(keys[x]);
    key_lengths[x]= key_length;
    test_compare(size_t(key_length), strlen(keys[x]));
  }

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, (const char **)keys, key_lengths, HALDENBRAND_KEY_COUNT));

  unsigned int keys_returned;
  test_compare(TEST_SUCCESS, fetch_all_results(memc, keys_returned, MEMCACHED_SUCCESS));
  test_compare(HALDENBRAND_KEY_COUNT, keys_returned);

  for (uint32_t x= 0; x < HALDENBRAND_KEY_COUNT; x++)
  {
    free(keys[x]);
  }
  free(keys);

  return TEST_SUCCESS;
}

/* Make sure we behave properly if server list has no values */
static test_return_t user_supplied_bug4(memcached_st *memc)
{
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};

  /* Here we free everything before running a bunch of mget tests */
  memcached_servers_reset(memc);


  /* We need to empty the server before continueing test */
  test_compare(MEMCACHED_NO_SERVERS,
               memcached_flush(memc, 0));

  test_compare(MEMCACHED_NO_SERVERS,
               memcached_mget(memc, keys, key_length, 3));

  unsigned int keys_returned;
  test_compare(TEST_SUCCESS, fetch_all_results(memc, keys_returned, MEMCACHED_NOTFOUND));
  test_zero(keys_returned);

  for (uint32_t x= 0; x < 3; x++)
  {
    test_compare(MEMCACHED_NO_SERVERS,
                 memcached_set(memc, keys[x], key_length[x],
                               keys[x], key_length[x],
                               (time_t)50, (uint32_t)9));
  }

  test_compare(MEMCACHED_NO_SERVERS, 
               memcached_mget(memc, keys, key_length, 3));

  {
    char *return_value;
    char return_key[MEMCACHED_MAX_KEY];
    memcached_return_t rc;
    size_t return_key_length;
    size_t return_value_length;
    uint32_t flags;
    uint32_t x= 0;
    while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                          &return_value_length, &flags, &rc)))
    {
      test_true(return_value);
      test_compare(MEMCACHED_SUCCESS, rc);
      test_true(return_key_length == return_value_length);
      test_memcmp(return_value, return_key, return_value_length);
      free(return_value);
      x++;
    }
  }

  return TEST_SUCCESS;
}

#define VALUE_SIZE_BUG5 1048064
static test_return_t user_supplied_bug5(memcached_st *memc)
{
  const char *keys[]= {"036790384900", "036790384902", "036790384904", "036790384906"};
  size_t key_length[]=  {strlen("036790384900"), strlen("036790384902"), strlen("036790384904"), strlen("036790384906")};
  char *value;
  size_t value_length;
  uint32_t flags;
  char *insert_data= new (std::nothrow) char[VALUE_SIZE_BUG5];

  for (uint32_t x= 0; x < VALUE_SIZE_BUG5; x++)
  {
    insert_data[x]= (signed char)rand();
  }

  test_compare(MEMCACHED_SUCCESS,
               memcached_flush(memc, 0));

  memcached_return_t rc;
  test_null(memcached_get(memc, keys[0], key_length[0], &value_length, &flags, &rc));
  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, key_length, 4));

  unsigned int count;
  test_compare(TEST_SUCCESS, fetch_all_results(memc, count, MEMCACHED_NOTFOUND));
  test_zero(count);

  for (uint32_t x= 0; x < 4; x++)
  {
    test_compare(MEMCACHED_SUCCESS,
                 memcached_set(memc, keys[x], key_length[x],
                               insert_data, VALUE_SIZE_BUG5,
                               (time_t)0, (uint32_t)0));
  }

  for (uint32_t x= 0; x < 10; x++)
  {
    value= memcached_get(memc, keys[0], key_length[0],
                         &value_length, &flags, &rc);
    test_compare(rc, MEMCACHED_SUCCESS);
    test_true(value);
    free(value);

    test_compare(MEMCACHED_SUCCESS,
                 memcached_mget(memc, keys, key_length, 4));

    test_compare(TEST_SUCCESS, fetch_all_results(memc, count, MEMCACHED_SUCCESS));
    test_compare(4U, count);
  }
  delete [] insert_data;

  return TEST_SUCCESS;
}

static test_return_t user_supplied_bug6(memcached_st *memc)
{
  const char *keys[]= {"036790384900", "036790384902", "036790384904", "036790384906"};
  size_t key_length[]=  {strlen("036790384900"), strlen("036790384902"), strlen("036790384904"), strlen("036790384906")};
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *value;
  size_t value_length;
  uint32_t flags;
  char *insert_data= new (std::nothrow) char[VALUE_SIZE_BUG5];

  for (uint32_t x= 0; x < VALUE_SIZE_BUG5; x++)
  {
    insert_data[x]= (signed char)rand();
  }

  test_compare(MEMCACHED_SUCCESS, memcached_flush(memc, 0));

  test_compare(TEST_SUCCESS, confirm_keys_dont_exist(memc, keys, test_array_length(keys)));

  // We will now confirm that memcached_mget() returns success, but we will
  // then check to make sure that no actual keys are returned.
  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, keys, key_length, 4));

  memcached_return_t rc;
  uint32_t count= 0;
  while ((value= memcached_fetch(memc, return_key, &return_key_length,
                                 &value_length, &flags, &rc)))
  {
    count++;
  }
  test_zero(count);
  test_compare_got(MEMCACHED_NOTFOUND, rc, memcached_strerror(NULL, rc));

  for (uint32_t x= 0; x < test_array_length(keys); x++)
  {
    test_compare(MEMCACHED_SUCCESS,
                 memcached_set(memc, keys[x], key_length[x],
                               insert_data, VALUE_SIZE_BUG5,
                               (time_t)0, (uint32_t)0));
  }
  test_compare(TEST_SUCCESS, confirm_keys_exist(memc, keys, test_array_length(keys)));

  for (uint32_t x= 0; x < 2; x++)
  {
    value= memcached_get(memc, keys[0], key_length[0],
                         &value_length, &flags, &rc);
    test_true(value);
    free(value);

    test_compare(MEMCACHED_SUCCESS,
                 memcached_mget(memc, keys, key_length, 4));
    /* We test for purge of partial complete fetches */
    for (count= 3; count; count--)
    {
      value= memcached_fetch(memc, return_key, &return_key_length,
                             &value_length, &flags, &rc);
      test_compare(MEMCACHED_SUCCESS, rc);
      test_memcmp(value, insert_data, value_length);
      test_true(value_length);
      free(value);
    }
  }
  delete [] insert_data;

  return TEST_SUCCESS;
}

static test_return_t user_supplied_bug8(memcached_st *)
{
  memcached_return_t rc;
  memcached_st *mine;
  memcached_st *memc_clone;

  memcached_server_st *servers;
  const char *server_list= "memcache1.memcache.bk.sapo.pt:11211, memcache1.memcache.bk.sapo.pt:11212, memcache1.memcache.bk.sapo.pt:11213, memcache1.memcache.bk.sapo.pt:11214, memcache2.memcache.bk.sapo.pt:11211, memcache2.memcache.bk.sapo.pt:11212, memcache2.memcache.bk.sapo.pt:11213, memcache2.memcache.bk.sapo.pt:11214";

  servers= memcached_servers_parse(server_list);
  test_true(servers);

  mine= memcached_create(NULL);
  rc= memcached_server_push(mine, servers);
  test_compare(MEMCACHED_SUCCESS, rc);
  memcached_server_list_free(servers);

  test_true(mine);
  memc_clone= memcached_clone(NULL, mine);

  memcached_quit(mine);
  memcached_quit(memc_clone);


  memcached_free(mine);
  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

/* Test flag store/retrieve */
static test_return_t user_supplied_bug7(memcached_st *memc)
{
  const char *keys= "036790384900";
  size_t key_length=  strlen(keys);
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *value;
  size_t value_length;
  uint32_t flags;
  char *insert_data= new (std::nothrow) char[VALUE_SIZE_BUG5];

  for (unsigned int x= 0; x < VALUE_SIZE_BUG5; x++)
  {
    insert_data[x]= (signed char)rand();
  }

  memcached_flush(memc, 0);

  flags= 245;
  memcached_return_t rc= memcached_set(memc, keys, key_length,
                                       insert_data, VALUE_SIZE_BUG5,
                                       (time_t)0, flags);
  test_compare(MEMCACHED_SUCCESS, rc);

  flags= 0;
  value= memcached_get(memc, keys, key_length,
                       &value_length, &flags, &rc);
  test_compare(245U, flags);
  test_true(value);
  free(value);

  test_compare(MEMCACHED_SUCCESS, memcached_mget(memc, &keys, &key_length, 1));

  flags= 0;
  value= memcached_fetch(memc, return_key, &return_key_length,
                         &value_length, &flags, &rc);
  test_compare(uint32_t(245), flags);
  test_true(value);
  free(value);
  delete [] insert_data;


  return TEST_SUCCESS;
}

static test_return_t user_supplied_bug9(memcached_st *memc)
{
  const char *keys[]= {"UDATA:edevil@sapo.pt", "fudge&*@#", "for^#@&$not"};
  size_t key_length[3];
  uint32_t flags;
  unsigned count= 0;

  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *return_value;
  size_t return_value_length;


  key_length[0]= strlen("UDATA:edevil@sapo.pt");
  key_length[1]= strlen("fudge&*@#");
  key_length[2]= strlen("for^#@&$not");


  for (unsigned int x= 0; x < 3; x++)
  {
    memcached_return_t rc= memcached_set(memc, keys[x], key_length[x],
                                         keys[x], key_length[x],
                                         (time_t)50, (uint32_t)9);
    test_compare(MEMCACHED_SUCCESS, rc);
  }

  memcached_return_t rc= memcached_mget(memc, keys, key_length, 3);
  test_compare(MEMCACHED_SUCCESS, rc);

  /* We need to empty the server before continueing test */
  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                        &return_value_length, &flags, &rc)) != NULL)
  {
    test_true(return_value);
    free(return_value);
    count++;
  }
  test_compare(3U, count);

  return TEST_SUCCESS;
}

/* We are testing with aggressive timeout to get failures */
static test_return_t user_supplied_bug10(memcached_st *memc)
{
  size_t value_length= 512;
  unsigned int set= 1;
  memcached_st *mclone= memcached_clone(NULL, memc);

  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_NO_BLOCK, set);
  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_TCP_NODELAY, set);
  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, uint64_t(0));

  std::vector<char> value;
  value.reserve(value_length);
  for (uint32_t x= 0; x < value_length; x++)
  {
    value.push_back(char(x % 127));
  }

  for (unsigned int x= 1; x <= 100000; ++x)
  {
    memcached_return_t rc= memcached_set(mclone, 
                                         test_literal_param("foo"),
                                         &value[0], value.size(),
                                         0, 0);

    test_true_got((rc == MEMCACHED_SUCCESS or rc == MEMCACHED_WRITE_FAILURE or rc == MEMCACHED_BUFFERED or rc == MEMCACHED_TIMEOUT or rc == MEMCACHED_CONNECTION_FAILURE 
                   or rc == MEMCACHED_SERVER_TEMPORARILY_DISABLED), 
                  memcached_strerror(NULL, rc));

    if (rc == MEMCACHED_WRITE_FAILURE or rc == MEMCACHED_TIMEOUT)
    {
      x--;
    }
  }

  memcached_free(mclone);

  return TEST_SUCCESS;
}

/*
  We are looking failures in the async protocol
*/
static test_return_t user_supplied_bug11(memcached_st *memc)
{
  memcached_st *mclone= memcached_clone(NULL, memc);

  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_NO_BLOCK, true);
  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_TCP_NODELAY, true);
  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, size_t(-1));

  test_compare(-1, int32_t(memcached_behavior_get(mclone, MEMCACHED_BEHAVIOR_POLL_TIMEOUT)));


  std::vector<char> value;
  value.reserve(512);
  for (unsigned int x= 0; x < 512; x++)
  {
    value.push_back(char(x % 127));
  }

  for (unsigned int x= 1; x <= 100000; ++x)
  {
    memcached_return_t rc= memcached_set(mclone, test_literal_param("foo"), &value[0], value.size(), 0, 0);
    (void)rc;
  }

  memcached_free(mclone);

  return TEST_SUCCESS;
}

/*
  Bug found where incr was not returning MEMCACHED_NOTFOUND when object did not exist.
*/
static test_return_t user_supplied_bug12(memcached_st *memc)
{
  memcached_return_t rc;
  uint32_t flags;
  size_t value_length;
  char *value;
  uint64_t number_value;

  value= memcached_get(memc, "autoincrement", strlen("autoincrement"),
                       &value_length, &flags, &rc);
  test_true(value == NULL);
  test_compare(MEMCACHED_NOTFOUND, rc);

  rc= memcached_increment(memc, "autoincrement", strlen("autoincrement"),
                          1, &number_value);
  test_true(value == NULL);
  /* The binary protocol will set the key if it doesn't exist */
  if (memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) == 1)
  {
    test_compare(MEMCACHED_SUCCESS, rc);
  }
  else
  {
    test_compare(MEMCACHED_NOTFOUND, rc);
  }

  rc= memcached_set(memc, "autoincrement", strlen("autoincrement"), "1", 1, 0, 0);

  value= memcached_get(memc, "autoincrement", strlen("autoincrement"),
                       &value_length, &flags, &rc);
  test_true(value);
  test_compare(MEMCACHED_SUCCESS, rc);
  free(value);

  rc= memcached_increment(memc, "autoincrement", strlen("autoincrement"),
                          1, &number_value);
  test_true(number_value == 2);
  test_compare(MEMCACHED_SUCCESS, rc);

  return TEST_SUCCESS;
}

/*
  Bug found where command total one more than MEMCACHED_MAX_BUFFER
  set key34567890 0 0 8169 \r\n is sent followed by buffer of size 8169, followed by 8169
*/
static test_return_t user_supplied_bug13(memcached_st *memc)
{
  char key[] = "key34567890";
  memcached_return_t rc;
  size_t overflowSize;

  char commandFirst[]= "set key34567890 0 0 ";
  char commandLast[] = " \r\n"; /* first line of command sent to server */
  size_t commandLength;
  size_t testSize;

  commandLength = strlen(commandFirst) + strlen(commandLast) + 4; /* 4 is number of characters in size, probably 8196 */

  overflowSize = MEMCACHED_MAX_BUFFER - commandLength;

  for (testSize= overflowSize - 1; testSize < overflowSize + 1; testSize++)
  {
    char *overflow= new (std::nothrow) char[testSize];
    test_true(overflow);

    memset(overflow, 'x', testSize);
    rc= memcached_set(memc, key, strlen(key),
                      overflow, testSize, 0, 0);
    test_compare(MEMCACHED_SUCCESS, rc);
    delete [] overflow;
  }

  return TEST_SUCCESS;
}


/*
  Test values of many different sizes
  Bug found where command total one more than MEMCACHED_MAX_BUFFER
  set key34567890 0 0 8169 \r\n
  is sent followed by buffer of size 8169, followed by 8169
*/
static test_return_t user_supplied_bug14(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, true);

  std::vector<char> value;
  value.reserve(18000);
  for (size_t x= 0; x < 18000; x++)
  {
    value.push_back((char) (x % 127));
  }

  for (size_t current_length= 0; current_length < value.size(); current_length++)
  {
    memcached_return_t rc= memcached_set(memc, test_literal_param("foo"),
                                         &value[0], current_length,
                                         (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);

    size_t string_length;
    uint32_t flags;
    char *string= memcached_get(memc, test_literal_param("foo"),
                                &string_length, &flags, &rc);

    test_compare(MEMCACHED_SUCCESS, rc);
    test_compare(string_length, current_length);
    test_memcmp(string, &value[0], string_length);

    free(string);
  }

  return TEST_SUCCESS;
}

/*
  Look for zero length value problems
*/
static test_return_t user_supplied_bug15(memcached_st *memc)
{
  for (uint32_t x= 0; x < 2; x++)
  {
    memcached_return_t rc= memcached_set(memc, test_literal_param("mykey"),
                                         NULL, 0,
                                         (time_t)0, (uint32_t)0);

    test_compare(MEMCACHED_SUCCESS, rc);

    size_t length;
    uint32_t flags;
    char *value= memcached_get(memc, test_literal_param("mykey"),
                               &length, &flags, &rc);

    test_compare(MEMCACHED_SUCCESS, rc);
    test_false(value);
    test_zero(length);
    test_zero(flags);

    value= memcached_get(memc, test_literal_param("mykey"),
                         &length, &flags, &rc);

    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(value == NULL);
    test_zero(length);
    test_zero(flags);
  }

  return TEST_SUCCESS;
}

/* Check the return sizes on FLAGS to make sure it stores 32bit unsigned values correctly */
static test_return_t user_supplied_bug16(memcached_st *memc)
{
  memcached_return_t rc= memcached_set(memc, test_literal_param("mykey"),
                                       NULL, 0,
                                       (time_t)0, UINT32_MAX);

  test_compare(MEMCACHED_SUCCESS, rc);

  size_t length;
  uint32_t flags;
  char *value= memcached_get(memc, test_literal_param("mykey"),
                             &length, &flags, &rc);

  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(value == NULL);
  test_zero(length);
  test_compare(flags, UINT32_MAX);

  return TEST_SUCCESS;
}

#if !defined(__sun) && !defined(__OpenBSD__)
/* Check the validity of chinese key*/
static test_return_t user_supplied_bug17(memcached_st *memc)
{
  const char *key= "豆瓣";
  const char *value="我们在炎热抑郁的夏天无法停止豆瓣";
  memcached_return_t rc= memcached_set(memc, key, strlen(key),
                                       value, strlen(value),
                                       (time_t)0, 0);

  test_compare(MEMCACHED_SUCCESS, rc);

  size_t length;
  uint32_t flags;
  char *value2= memcached_get(memc, key, strlen(key),
                              &length, &flags, &rc);

  test_true(length==strlen(value));
  test_compare(MEMCACHED_SUCCESS, rc);
  test_memcmp(value, value2, length);
  free(value2);

  return TEST_SUCCESS;
}
#endif

/*
  From Andrei on IRC
*/

static test_return_t user_supplied_bug19(memcached_st *)
{
  memcached_return_t res;

  memcached_st *memc= memcached(test_literal_param("--server=localhost:11311/?100 --server=localhost:11312/?100"));

  const memcached_server_st *server= memcached_server_by_key(memc, "a", 1, &res);
  test_true(server);

  memcached_free(memc);

  return TEST_SUCCESS;
}

/* CAS test from Andei */
static test_return_t user_supplied_bug20(memcached_st *memc)
{
  const char *key= "abc";
  size_t key_len= strlen("abc");

  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, true));

  test_compare(MEMCACHED_SUCCESS,
               memcached_set(memc,
                             test_literal_param("abc"),
                             test_literal_param("foobar"),
                             (time_t)0, (uint32_t)0));

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, &key, &key_len, 1));

  memcached_result_st result_obj;
  memcached_result_st *result= memcached_result_create(memc, &result_obj);
  test_true(result);

  memcached_result_create(memc, &result_obj);
  memcached_return_t status;
  result= memcached_fetch_result(memc, &result_obj, &status);

  test_true(result);
  test_compare(MEMCACHED_SUCCESS, status);

  memcached_result_free(result);

  return TEST_SUCCESS;
}

/* Large mget() of missing keys with binary proto
 *
 * If many binary quiet commands (such as getq's in an mget) fill the output
 * buffer and the server chooses not to respond, memcached_flush hangs. See
 * http://lists.tangent.org/pipermail/libmemcached/2009-August/000918.html
 */

/* sighandler_t function that always asserts false */
static void fail(int)
{
  assert(0);
}


static test_return_t _user_supplied_bug21(memcached_st* memc, size_t key_count)
{
#ifdef WIN32
  (void)memc;
  (void)key_count;
  return TEST_SKIPPED;
#else
  void (*oldalarm)(int);

  memcached_st *memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);

  /* only binproto uses getq for mget */
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, true));

  /* empty the cache to ensure misses (hence non-responses) */
  test_compare(MEMCACHED_SUCCESS, memcached_flush(memc_clone, 0));

  size_t* key_lengths= new (std::nothrow) size_t[key_count];
  test_true(key_lengths);
  char **keys= static_cast<char **>(calloc(key_count, sizeof(char *)));
  test_true(keys);
  for (unsigned int x= 0; x < key_count; x++)
  {
    char buffer[30];

    snprintf(buffer, 30, "%u", x);
    keys[x]= strdup(buffer);
    test_true(keys[x]);
    key_lengths[x]= strlen(keys[x]);
  }

  oldalarm= signal(SIGALRM, fail);
  alarm(5);

  test_compare_got(MEMCACHED_SUCCESS,
                   memcached_mget(memc_clone, (const char **)keys, key_lengths, key_count), memcached_last_error_message(memc_clone));

  alarm(0);
  signal(SIGALRM, oldalarm);

  memcached_return_t rc;
  uint32_t flags;
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *return_value;
  size_t return_value_length;
  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                        &return_value_length, &flags, &rc)))
  {
    test_false(return_value); // There are no keys to fetch, so the value should never be returned
  }
  test_compare(MEMCACHED_NOTFOUND, rc);
  test_zero(return_value_length);
  test_zero(return_key_length);
  test_false(return_key[0]);
  test_false(return_value);

  for (unsigned int x= 0; x < key_count; x++)
  {
    free(keys[x]);
  }
  free(keys);
  delete [] key_lengths;

  memcached_free(memc_clone);

  return TEST_SUCCESS;
#endif
}

static test_return_t user_supplied_bug21(memcached_st *memc)
{
  test_skip(TEST_SUCCESS, pre_binary(memc));

  /* should work as of r580 */
  test_compare(TEST_SUCCESS,
               _user_supplied_bug21(memc, 10));

  /* should fail as of r580 */
  test_compare(TEST_SUCCESS,
               _user_supplied_bug21(memc, 1000));

  return TEST_SUCCESS;
}

static test_return_t output_ketama_weighted_keys(memcached_st *)
{
  memcached_st *memc= memcached_create(NULL);
  test_true(memc);


  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, true));

  uint64_t value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
  test_compare(value, uint64_t(1));

  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH, MEMCACHED_HASH_MD5));

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH);
  test_true(value == MEMCACHED_HASH_MD5);


  test_true(memcached_behavior_set_distribution(memc, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY) == MEMCACHED_SUCCESS);

  memcached_server_st *server_pool;
  server_pool = memcached_servers_parse("10.0.1.1:11211,10.0.1.2:11211,10.0.1.3:11211,10.0.1.4:11211,10.0.1.5:11211,10.0.1.6:11211,10.0.1.7:11211,10.0.1.8:11211,192.168.1.1:11211,192.168.100.1:11211");
  memcached_server_push(memc, server_pool);

  // @todo this needs to be refactored to actually test something.
#if 0
  FILE *fp;
  if ((fp = fopen("ketama_keys.txt", "w")))
  {
    // noop
  } else {
    printf("cannot write to file ketama_keys.txt");
    return TEST_FAILURE;
  }

  for (int x= 0; x < 10000; x++)
  {
    char key[10];
    snprintf(key, sizeof(key), "%d", x);

    uint32_t server_idx = memcached_generate_hash(memc, key, strlen(key));
    char *hostname = memc->hosts[server_idx].hostname;
    in_port_t port = memc->hosts[server_idx].port;
    fprintf(fp, "key %s is on host /%s:%u\n", key, hostname, port);
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc, host_index);
  }
  fclose(fp);
#endif
  memcached_server_list_free(server_pool);
  memcached_free(memc);

  return TEST_SUCCESS;
}


static test_return_t result_static(memcached_st *memc)
{
  memcached_result_st result;
  memcached_result_st *result_ptr= memcached_result_create(memc, &result);
  test_false(result.options.is_allocated);
  test_true(memcached_is_initialized(&result));
  test_true(result_ptr);
  test_true(result_ptr == &result);

  memcached_result_free(&result);

  test_false(result.options.is_allocated);
  test_false(memcached_is_initialized(&result));

  return TEST_SUCCESS;
}

static test_return_t result_alloc(memcached_st *memc)
{
  memcached_result_st *result_ptr= memcached_result_create(memc, NULL);
  test_true(result_ptr);
  test_true(result_ptr->options.is_allocated);
  test_true(memcached_is_initialized(result_ptr));
  memcached_result_free(result_ptr);

  return TEST_SUCCESS;
}

static test_return_t cleanup_pairs(memcached_st *memc)
{
  (void)memc;
  pairs_free(global_pairs);

  return TEST_SUCCESS;
}

static test_return_t generate_pairs(memcached_st *)
{
  global_pairs= pairs_generate(GLOBAL_COUNT, 400);
  global_count= GLOBAL_COUNT;

  for (size_t x= 0; x < global_count; x++)
  {
    global_keys[x]= global_pairs[x].key;
    global_keys_length[x]=  global_pairs[x].key_length;
  }

  return TEST_SUCCESS;
}

static test_return_t generate_large_pairs(memcached_st *)
{
  global_pairs= pairs_generate(GLOBAL2_COUNT, MEMCACHED_MAX_BUFFER+10);
  global_count= GLOBAL2_COUNT;

  for (size_t x= 0; x < global_count; x++)
  {
    global_keys[x]= global_pairs[x].key;
    global_keys_length[x]=  global_pairs[x].key_length;
  }

  return TEST_SUCCESS;
}

static test_return_t generate_data(memcached_st *memc)
{
  unsigned int check_execute= execute_set(memc, global_pairs, global_count);

  test_compare(check_execute, global_count);

  return TEST_SUCCESS;
}

static test_return_t generate_data_with_stats(memcached_st *memc)
{
  uint32_t host_index= 0;
  unsigned int check_execute= execute_set(memc, global_pairs, global_count);

  test_true(check_execute == global_count);

  // @todo hosts used size stats
  memcached_return_t rc;
  memcached_stat_st *stat_p= memcached_stat(memc, NULL, &rc);
  test_true(stat_p);

  for (host_index= 0; host_index < SERVERS_TO_CREATE; host_index++)
  {
    /* This test was changes so that "make test" would work properlly */
    if (DEBUG)
    {
      memcached_server_instance_st instance=
        memcached_server_instance_by_position(memc, host_index);

      printf("\nserver %u|%s|%u bytes: %llu\n", host_index, instance->hostname, instance->port, (unsigned long long)(stat_p + host_index)->bytes);
    }
    test_true((unsigned long long)(stat_p + host_index)->bytes);
  }

  memcached_stat_free(NULL, stat_p);

  return TEST_SUCCESS;
}
static test_return_t generate_buffer_data(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, true);
  generate_data(memc);

  return TEST_SUCCESS;
}

static test_return_t get_read_count(memcached_st *memc)
{
  memcached_st *memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);

  memcached_server_add_with_weight(memc_clone, "localhost", 6666, 0);

  {
    char *return_value;
    size_t return_value_length;
    uint32_t flags;
    uint32_t count;

    for (size_t x= count= 0; x < global_count; x++)
    {
      memcached_return_t rc;
      return_value= memcached_get(memc_clone, global_keys[x], global_keys_length[x],
                                  &return_value_length, &flags, &rc);
      if (rc == MEMCACHED_SUCCESS)
      {
        count++;
        if (return_value)
        {
          free(return_value);
        }
      }
    }
  }

  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

static test_return_t get_read(memcached_st *memc)
{
  for (size_t x= 0; x < global_count; x++)
  {
    size_t return_value_length;
    uint32_t flags;
    memcached_return_t rc;
    char *return_value= memcached_get(memc, global_keys[x], global_keys_length[x],
                                      &return_value_length, &flags, &rc);
    /*
      test_true(return_value);
      test_compare(MEMCACHED_SUCCESS, rc);
    */
    if (rc == MEMCACHED_SUCCESS && return_value)
    {
      free(return_value);
    }
  }

  return TEST_SUCCESS;
}

static test_return_t mget_read(memcached_st *memc)
{

  test_skip(true, bool(libmemcached_util_version_check(memc, 1, 4, 4)));

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, global_keys, global_keys_length, global_count));

  // Go fetch the keys and test to see if all of them were returned
  {
    unsigned int keys_returned;
    test_compare(TEST_SUCCESS, fetch_all_results(memc, keys_returned, MEMCACHED_SUCCESS));
    test_true(keys_returned > 0);
    test_compare_warn_hint(global_count, keys_returned, "Possible false, positive, memcached may have ejected key/value based on memory needs");
  }

  return TEST_SUCCESS;
}

static test_return_t mget_read_result(memcached_st *memc)
{

  test_skip(true, bool(libmemcached_util_version_check(memc, 1, 4, 4)));

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, global_keys, global_keys_length, global_count));

  /* Turn this into a help function */
  {
    memcached_result_st results_obj;
    memcached_result_st *results= memcached_result_create(memc, &results_obj);
    test_true(results);

    memcached_return_t rc;
    while ((results= memcached_fetch_result(memc, &results_obj, &rc)))
    {
      if (rc == MEMCACHED_IN_PROGRESS)
      {
        continue;
      }

      test_true(results);
      test_compare(MEMCACHED_SUCCESS, rc);
    }
    test_compare(MEMCACHED_END, rc);

    memcached_result_free(&results_obj);
  }

  return TEST_SUCCESS;
}

static test_return_t mget_read_internal_result(memcached_st *memc)
{

  test_skip(true, bool(libmemcached_util_version_check(memc, 1, 4, 4)));

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, global_keys, global_keys_length, global_count));
  {
    memcached_result_st *results= NULL;
    memcached_return_t rc;
    while ((results= memcached_fetch_result(memc, results, &rc)))
    {
      test_true(results);
      test_compare(MEMCACHED_SUCCESS, rc);
    }
    test_compare(MEMCACHED_END, rc);

    memcached_result_free(results);
  }

  return TEST_SUCCESS;
}

static test_return_t mget_read_partial_result(memcached_st *memc)
{

  test_skip(true, bool(libmemcached_util_version_check(memc, 1, 4, 4)));

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, global_keys, global_keys_length, global_count));

  // We will scan for just one key
  {
    memcached_result_st results_obj;
    memcached_result_st *results= memcached_result_create(memc, &results_obj);

    memcached_return_t rc;
    results= memcached_fetch_result(memc, results, &rc);
    test_true(results);
    test_compare(MEMCACHED_SUCCESS, rc);

    memcached_result_free(&results_obj);
  }

  // We already have a read happening, lets start up another one.
  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, global_keys, global_keys_length, global_count));
  {
    memcached_result_st results_obj;
    memcached_result_st *results= memcached_result_create(memc, &results_obj);
    test_true(results);
    test_false(memcached_is_allocated(results));

    memcached_return_t rc;
    while ((results= memcached_fetch_result(memc, &results_obj, &rc)))
    {
      test_true(results);
      test_compare(MEMCACHED_SUCCESS, rc);
    }
    test_compare(MEMCACHED_END, rc);

    memcached_result_free(&results_obj);
  }

  return TEST_SUCCESS;
}

static test_return_t mget_read_function(memcached_st *memc)
{
  test_skip(true, bool(libmemcached_util_version_check(memc, 1, 4, 4)));

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, global_keys, global_keys_length, global_count));

  memcached_execute_fn callbacks[]= { &callback_counter };
  size_t counter= 0;
  test_compare(MEMCACHED_SUCCESS, 
               memcached_fetch_execute(memc, callbacks, (void *)&counter, 1));

  return TEST_SUCCESS;
}

static test_return_t delete_generate(memcached_st *memc)
{
  for (size_t x= 0; x < global_count; x++)
  {
    (void)memcached_delete(memc, global_keys[x], global_keys_length[x], (time_t)0);
  }

  return TEST_SUCCESS;
}

static test_return_t delete_buffer_generate(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, true);

  for (size_t x= 0; x < global_count; x++)
  {
    (void)memcached_delete(memc, global_keys[x], global_keys_length[x], (time_t)0);
  }

  return TEST_SUCCESS;
}

static test_return_t add_host_test1(memcached_st *memc)
{
  memcached_return_t rc;
  char servername[]= "0.example.com";

  memcached_server_st *servers= memcached_server_list_append_with_weight(NULL, servername, 400, 0, &rc);
  test_true(servers);
  test_compare(1U, memcached_server_list_count(servers));

  for (uint32_t x= 2; x < 20; x++)
  {
    char buffer[SMALL_STRING_LEN];

    snprintf(buffer, SMALL_STRING_LEN, "%lu.example.com", (unsigned long)(400 +x));
    servers= memcached_server_list_append_with_weight(servers, buffer, 401, 0,
                                                      &rc);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_compare(x, memcached_server_list_count(servers));
  }

  test_compare(MEMCACHED_SUCCESS, memcached_server_push(memc, servers));
  test_compare(MEMCACHED_SUCCESS, memcached_server_push(memc, servers));

  memcached_server_list_free(servers);

  return TEST_SUCCESS;
}

static test_return_t pre_nonblock(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 0);

  return TEST_SUCCESS;
}

static test_return_t pre_cork(memcached_st *memc)
{
#ifdef __APPLE__
  return TEST_SKIPPED;
#endif
  bool set= true;
  if (memcached_success(memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CORK, set)))
    return TEST_SUCCESS;

  return TEST_SKIPPED;
}

static test_return_t pre_cork_and_nonblock(memcached_st *memc)
{
#ifdef __APPLE__
  return TEST_SKIPPED;
#endif
  test_return_t test_rc;
  if ((test_rc= pre_cork(memc)) != TEST_SUCCESS)
    return test_rc;

  return pre_nonblock(memc);
}

static test_return_t pre_nonblock_binary(memcached_st *memc)
{
  memcached_st *memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);

  // The memcached_version needs to be done on a clone, because the server
  // will not toggle protocol on an connection.
  memcached_version(memc_clone);

  memcached_return_t rc= MEMCACHED_FAILURE;
  if (libmemcached_util_version_check(memc_clone, 1, 4, 4))
  {
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 0);
    test_compare(MEMCACHED_SUCCESS,
                 memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1));
    test_compare(uint64_t(1), memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL));
  }
  else
  {
    memcached_free(memc_clone);
    return TEST_SKIPPED;
  }

  memcached_free(memc_clone);

  return rc == MEMCACHED_SUCCESS ? TEST_SUCCESS : TEST_SKIPPED;
}

static test_return_t pre_murmur(memcached_st *memc)
{
  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_MURMUR));
  return TEST_SUCCESS;
}

static test_return_t pre_jenkins(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_JENKINS);

  return TEST_SUCCESS;
}


static test_return_t pre_md5(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_MD5);

  return TEST_SUCCESS;
}

static test_return_t pre_crc(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_CRC);

  return TEST_SUCCESS;
}

static test_return_t pre_hsieh(memcached_st *memc)
{
  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_HSIEH));
  return TEST_SUCCESS;
}

static test_return_t pre_hash_fnv1_64(memcached_st *memc)
{
  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_MURMUR));

  return TEST_SUCCESS;
}

static test_return_t pre_hash_fnv1a_64(memcached_st *memc)
{
  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_FNV1A_64));

  return TEST_SUCCESS;
}

static test_return_t pre_hash_fnv1_32(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_FNV1_32);

  return TEST_SUCCESS;
}

static test_return_t pre_hash_fnv1a_32(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_FNV1A_32);

  return TEST_SUCCESS;
}

static test_return_t pre_behavior_ketama(memcached_st *memc)
{
  memcached_return_t rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA, 1);
  test_compare(MEMCACHED_SUCCESS, rc);

  uint64_t value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA);
  test_compare(value, uint64_t(1));

  return TEST_SUCCESS;
}

static test_return_t pre_behavior_ketama_weighted(memcached_st *memc)
{
  memcached_return_t rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
  test_compare(MEMCACHED_SUCCESS, rc);

  uint64_t value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
  test_compare(value, uint64_t(1));

  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH, MEMCACHED_HASH_MD5));

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH);
  test_compare(MEMCACHED_HASH_MD5, memcached_hash_t(value));

  return TEST_SUCCESS;
}

static test_return_t pre_replication(memcached_st *memc)
{
  test_skip(TEST_SUCCESS, pre_binary(memc));

  /*
   * Make sure that we store the item on all servers
   * (master + replicas == number of servers)
 */
  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS, memcached_server_count(memc) - 1));
  test_compare(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS), uint64_t(memcached_server_count(memc) - 1));

  return TEST_SUCCESS;
}


static test_return_t pre_replication_noblock(memcached_st *memc)
{
  test_skip(TEST_SUCCESS, pre_replication(memc));

  return pre_nonblock(memc);
}


static void my_free(const memcached_st *ptr, void *mem, void *context)
{
  (void)context;
  (void)ptr;
#ifdef HARD_MALLOC_TESTS
  void *real_ptr= (mem == NULL) ? mem : (void*)((caddr_t)mem - 8);
  free(real_ptr);
#else
  free(mem);
#endif
}


static void *my_malloc(const memcached_st *ptr, const size_t size, void *context)
{
  (void)context;
  (void)ptr;
#ifdef HARD_MALLOC_TESTS
  void *ret= malloc(size + 8);
  if (ret != NULL)
  {
    ret= (void*)((caddr_t)ret + 8);
  }
#else
  void *ret= malloc(size);
#endif

  if (ret != NULL)
  {
    memset(ret, 0xff, size);
  }

  return ret;
}


static void *my_realloc(const memcached_st *ptr, void *mem, const size_t size, void *)
{
#ifdef HARD_MALLOC_TESTS
  void *real_ptr= (mem == NULL) ? NULL : (void*)((caddr_t)mem - 8);
  void *nmem= realloc(real_ptr, size + 8);

  void *ret= NULL;
  if (nmem != NULL)
  {
    ret= (void*)((caddr_t)nmem + 8);
  }

  return ret;
#else
  (void)ptr;
  return realloc(mem, size);
#endif
}


static void *my_calloc(const memcached_st *ptr, size_t nelem, const size_t size, void *)
{
#ifdef HARD_MALLOC_TESTS
  void *mem= my_malloc(ptr, nelem * size);
  if (mem)
  {
    memset(mem, 0, nelem * size);
  }

  return mem;
#else
  (void)ptr;
  return calloc(nelem, size);
#endif
}

static test_return_t selection_of_namespace_tests(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "mine";
  char *value;

  /* Make sure be default none exists */
  value= (char*)memcached_callback_get(memc, MEMCACHED_CALLBACK_NAMESPACE, &rc);
  test_null(value);
  test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));

  /* Test a clean set */
  test_compare(MEMCACHED_SUCCESS,
               memcached_callback_set(memc, MEMCACHED_CALLBACK_NAMESPACE, (void *)key));

  value= (char*)memcached_callback_get(memc, MEMCACHED_CALLBACK_NAMESPACE, &rc);
  test_true(value);
  test_memcmp(value, key, 4);
  test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));

  /* Test that we can turn it off */
  test_compare(MEMCACHED_SUCCESS,
               memcached_callback_set(memc, MEMCACHED_CALLBACK_NAMESPACE, NULL));

  value= (char*)memcached_callback_get(memc, MEMCACHED_CALLBACK_NAMESPACE, &rc);
  test_null(value);
  test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));

  /* Now setup for main test */
  test_compare(MEMCACHED_SUCCESS,
               memcached_callback_set(memc, MEMCACHED_CALLBACK_NAMESPACE, (void *)key));

  value= (char *)memcached_callback_get(memc, MEMCACHED_CALLBACK_NAMESPACE, &rc);
  test_true(value);
  test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));
  test_memcmp(value, key, 4);

  /* Set to Zero, and then Set to something too large */
  {
    char long_key[255];
    memset(long_key, 0, 255);

    test_compare(MEMCACHED_SUCCESS,
                 memcached_callback_set(memc, MEMCACHED_CALLBACK_NAMESPACE, NULL));

    value= (char*)memcached_callback_get(memc, MEMCACHED_CALLBACK_NAMESPACE, &rc);
    test_null(value);
    test_compare(MEMCACHED_SUCCESS, rc);

    /* Test a long key for failure */
    /* TODO, extend test to determine based on setting, what result should be */
    strncpy(long_key, "Thisismorethentheallottednumberofcharacters", sizeof(long_key));
    test_compare(MEMCACHED_SUCCESS, 
                 memcached_callback_set(memc, MEMCACHED_CALLBACK_NAMESPACE, long_key));

    /* Now test a key with spaces (which will fail from long key, since bad key is not set) */
    strncpy(long_key, "This is more then the allotted number of characters", sizeof(long_key));
    test_compare(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) ? MEMCACHED_SUCCESS : MEMCACHED_BAD_KEY_PROVIDED,
                 memcached_callback_set(memc, MEMCACHED_CALLBACK_NAMESPACE, long_key));

    /* Test for a bad prefix, but with a short key */
    test_compare(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) ? MEMCACHED_INVALID_ARGUMENTS : MEMCACHED_SUCCESS,
                 memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_VERIFY_KEY, 1));

    test_compare(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) ? MEMCACHED_SUCCESS : MEMCACHED_BAD_KEY_PROVIDED,
                 memcached_callback_set(memc, MEMCACHED_CALLBACK_NAMESPACE, "dog cat"));
  }

  return TEST_SUCCESS;
}

static test_return_t set_namespace(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "mine";
  char *value;

  // Make sure we default to a null namespace
  value= (char*)memcached_callback_get(memc, MEMCACHED_CALLBACK_NAMESPACE, &rc);
  test_null(value);
  test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));

  /* Test a clean set */
  test_compare(MEMCACHED_SUCCESS,
               memcached_callback_set(memc, MEMCACHED_CALLBACK_NAMESPACE, (void *)key));

  value= (char*)memcached_callback_get(memc, MEMCACHED_CALLBACK_NAMESPACE, &rc);
  test_true(value);
  test_memcmp(value, key, 4);
  test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));

  return TEST_SUCCESS;
}

static test_return_t set_namespace_and_binary(memcached_st *memc)
{
  test_return_if(pre_binary(memc));
  test_return_if(set_namespace(memc));

  return TEST_SUCCESS;
}

#ifdef MEMCACHED_ENABLE_DEPRECATED
static test_return_t deprecated_set_memory_alloc(memcached_st *memc)
{
  void *test_ptr= NULL;
  void *cb_ptr= NULL;
  {
    memcached_malloc_fn malloc_cb=
      (memcached_malloc_fn)my_malloc;
    cb_ptr= *(void **)&malloc_cb;
    memcached_return_t rc;

    test_compare(MEMCACHED_SUCCESS,
                 memcached_callback_set(memc, MEMCACHED_CALLBACK_MALLOC_FUNCTION, cb_ptr));
    test_ptr= memcached_callback_get(memc, MEMCACHED_CALLBACK_MALLOC_FUNCTION, &rc);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(test_ptr == cb_ptr);
  }

  {
    memcached_realloc_fn realloc_cb=
      (memcached_realloc_fn)my_realloc;
    cb_ptr= *(void **)&realloc_cb;
    memcached_return_t rc;

    test_compare(MEMCACHED_SUCCESS,
                 memcached_callback_set(memc, MEMCACHED_CALLBACK_REALLOC_FUNCTION, cb_ptr));
    test_ptr= memcached_callback_get(memc, MEMCACHED_CALLBACK_REALLOC_FUNCTION, &rc);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(test_ptr == cb_ptr);
  }

  {
    memcached_free_fn free_cb=
      (memcached_free_fn)my_free;
    cb_ptr= *(void **)&free_cb;
    memcached_return_t rc;

    test_compare(MEMCACHED_SUCCESS,
                 memcached_callback_set(memc, MEMCACHED_CALLBACK_FREE_FUNCTION, cb_ptr));
    test_ptr= memcached_callback_get(memc, MEMCACHED_CALLBACK_FREE_FUNCTION, &rc);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(test_ptr == cb_ptr);
  }

  return TEST_SUCCESS;
}
#endif


static test_return_t set_memory_alloc(memcached_st *memc)
{
  test_compare(MEMCACHED_INVALID_ARGUMENTS,
               memcached_set_memory_allocators(memc, NULL, my_free,
                                               my_realloc, my_calloc, NULL));

  test_compare(MEMCACHED_SUCCESS,
               memcached_set_memory_allocators(memc, my_malloc, my_free,
                                               my_realloc, my_calloc, NULL));

  memcached_malloc_fn mem_malloc;
  memcached_free_fn mem_free;
  memcached_realloc_fn mem_realloc;
  memcached_calloc_fn mem_calloc;
  memcached_get_memory_allocators(memc, &mem_malloc, &mem_free,
                                  &mem_realloc, &mem_calloc);

  test_true(mem_malloc == my_malloc);
  test_true(mem_realloc == my_realloc);
  test_true(mem_calloc == my_calloc);
  test_true(mem_free == my_free);

  return TEST_SUCCESS;
}

static test_return_t enable_consistent_crc(memcached_st *memc)
{
  test_return_t rc;
  memcached_server_distribution_t value= MEMCACHED_DISTRIBUTION_CONSISTENT;
  memcached_hash_t hash;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION, value);
  if ((rc= pre_crc(memc)) != TEST_SUCCESS)
    return rc;

  value= (memcached_server_distribution_t)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION);
  test_true(value == MEMCACHED_DISTRIBUTION_CONSISTENT);

  hash= (memcached_hash_t)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH);

  if (hash != MEMCACHED_HASH_CRC)
    return TEST_SKIPPED;

  return TEST_SUCCESS;
}

static test_return_t enable_consistent_hsieh(memcached_st *memc)
{
  test_return_t rc;
  memcached_server_distribution_t value= MEMCACHED_DISTRIBUTION_CONSISTENT;
  memcached_hash_t hash;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION, value);
  if ((rc= pre_hsieh(memc)) != TEST_SUCCESS)
  {
    return rc;
  }

  value= (memcached_server_distribution_t)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION);
  test_true(value == MEMCACHED_DISTRIBUTION_CONSISTENT);

  hash= (memcached_hash_t)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH);

  if (hash != MEMCACHED_HASH_HSIEH)
    return TEST_SKIPPED;


  return TEST_SUCCESS;
}

static test_return_t enable_cas(memcached_st *memc)
{
  unsigned int set= 1;

  if (libmemcached_util_version_check(memc, 1, 2, 4))
  {
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, set);

    return TEST_SUCCESS;
  }

  return TEST_SKIPPED;
}

static test_return_t check_for_1_2_3(memcached_st *memc)
{
  memcached_version(memc);

  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  if ((instance->major_version >= 1 && (instance->minor_version == 2 && instance->micro_version >= 4))
      or instance->minor_version > 2)
  {
    return TEST_SUCCESS;
  }

  return TEST_SKIPPED;
}

static test_return_t pre_unix_socket(memcached_st *memc)
{
  struct stat buf;

  memcached_servers_reset(memc);
  const char *socket_file= default_socket();

  test_skip(0, stat(socket_file, &buf));

  test_compare(MEMCACHED_SUCCESS,
               memcached_server_add_unix_socket_with_weight(memc, socket_file, 0));

  return TEST_SUCCESS;
}

static test_return_t pre_nodelay(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 0);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, 0);

  return TEST_SUCCESS;
}

static test_return_t pre_settimer(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SND_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 1000);

  return TEST_SUCCESS;
}

static test_return_t MEMCACHED_BEHAVIOR_POLL_TIMEOUT_test(memcached_st *memc)
{
  const uint64_t timeout= 100; // Not using, just checking that it sets

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, timeout);

  test_compare(timeout, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT));

  return TEST_SUCCESS;
}

static test_return_t noreply_test(memcached_st *memc)
{
  test_compare(MEMCACHED_SUCCESS, 
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 1));
  test_compare(MEMCACHED_SUCCESS, 
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1));
  test_compare(MEMCACHED_SUCCESS, 
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1));
  test_compare(1LLU, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NOREPLY));
  test_compare(1LLU, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS));
  test_compare(1LLU, memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS));

  memcached_return_t ret;
  for (int count= 0; count < 5; ++count)
  {
    for (size_t x= 0; x < 100; ++x)
    {
      char key[10];
      int check_length= (size_t)snprintf(key, sizeof(key), "%lu", (unsigned long)x);
      test_false((size_t)check_length >= sizeof(key) || check_length < 0);

      size_t len= (size_t)check_length;

      switch (count)
      {
      case 0:
        ret= memcached_add(memc, key, len, key, len, 0, 0);
        break;
      case 1:
        ret= memcached_replace(memc, key, len, key, len, 0, 0);
        break;
      case 2:
        ret= memcached_set(memc, key, len, key, len, 0, 0);
        break;
      case 3:
        ret= memcached_append(memc, key, len, key, len, 0, 0);
        break;
      case 4:
        ret= memcached_prepend(memc, key, len, key, len, 0, 0);
        break;
      default:
        test_true(count);
        break;
      }
      test_true_got(ret == MEMCACHED_SUCCESS or ret == MEMCACHED_BUFFERED, memcached_strerror(NULL, ret));
    }

    /*
     ** NOTE: Don't ever do this in your code! this is not a supported use of the
     ** API and is _ONLY_ done this way to verify that the library works the
     ** way it is supposed to do!!!!
   */
    int no_msg=0;
    for (uint32_t x= 0; x < memcached_server_count(memc); ++x)
    {
      memcached_server_instance_st instance=
        memcached_server_instance_by_position(memc, x);
      no_msg+=(int)(instance->cursor_active);
    }

    test_true(no_msg == 0);
    test_compare(MEMCACHED_SUCCESS, memcached_flush_buffers(memc));

    /*
     ** Now validate that all items was set properly!
   */
    for (size_t x= 0; x < 100; ++x)
    {
      char key[10];

      int check_length= (size_t)snprintf(key, sizeof(key), "%lu", (unsigned long)x);

      test_false((size_t)check_length >= sizeof(key) || check_length < 0);

      size_t len= (size_t)check_length;
      size_t length;
      uint32_t flags;
      char* value=memcached_get(memc, key, strlen(key),
                                &length, &flags, &ret);
      test_true_got(ret == MEMCACHED_SUCCESS && value != NULL, memcached_strerror(NULL, ret));
      switch (count)
      {
      case 0: /* FALLTHROUGH */
      case 1: /* FALLTHROUGH */
      case 2:
        test_true(strncmp(value, key, len) == 0);
        test_true(len == length);
        break;
      case 3:
        test_true(length == len * 2);
        break;
      case 4:
        test_true(length == len * 3);
        break;
      default:
        test_true(count);
        break;
      }
      free(value);
    }
  }

  /* Try setting an illegal cas value (should not return an error to
   * the caller (because we don't expect a return message from the server)
 */
  const char* keys[]= {"0"};
  size_t lengths[]= {1};
  size_t length;
  uint32_t flags;
  memcached_result_st results_obj;
  memcached_result_st *results;
  test_compare(MEMCACHED_SUCCESS, 
               memcached_mget(memc, keys, lengths, 1));

  results= memcached_result_create(memc, &results_obj);
  test_true(results);
  results= memcached_fetch_result(memc, &results_obj, &ret);
  test_true(results);
  test_compare(MEMCACHED_SUCCESS, ret);
  uint64_t cas= memcached_result_cas(results);
  memcached_result_free(&results_obj);

  test_compare(MEMCACHED_SUCCESS, 
               memcached_cas(memc, keys[0], lengths[0], keys[0], lengths[0], 0, 0, cas));

  /*
   * The item will have a new cas value, so try to set it again with the old
   * value. This should fail!
 */
  test_compare(MEMCACHED_SUCCESS, 
               memcached_cas(memc, keys[0], lengths[0], keys[0], lengths[0], 0, 0, cas));
  test_true(memcached_flush_buffers(memc) == MEMCACHED_SUCCESS);
  char* value=memcached_get(memc, keys[0], lengths[0], &length, &flags, &ret);
  test_true(ret == MEMCACHED_SUCCESS && value != NULL);
  free(value);

  return TEST_SUCCESS;
}

static test_return_t analyzer_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_analysis_st *report;

  memcached_stat_st *memc_stat= memcached_stat(memc, NULL, &rc);
  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(memc_stat);

  report= memcached_analyze(memc, memc_stat, &rc);
  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(report);

  free(report);
  memcached_stat_free(NULL, memc_stat);

  return TEST_SUCCESS;
}

/* Count the objects */

static test_return_t dump_test(memcached_st *memc)
{
  /* No support for Binary protocol yet */
  test_skip(false, memc->flags.binary_protocol);

  test_compare(TEST_SUCCESS, set_test3(memc));

  // confirm_key_count() call dump
  size_t counter= confirm_key_count(memc);

  /* We may have more then 32 if our previous flush has not completed */
  test_true(counter >= 32);

  return TEST_SUCCESS;
}

static test_return_t util_version_test(memcached_st *memc)
{
  test_compare_hint(MEMCACHED_SUCCESS, memcached_version(memc), memcached_last_error_message(memc));
  test_true(libmemcached_util_version_check(memc, 0, 0, 0));

  bool if_successful= libmemcached_util_version_check(memc, 9, 9, 9);

  // We expect failure
  if (if_successful)
  {
    fprintf(stderr, "\n----------------------------------------------------------------------\n");
    fprintf(stderr, "\nDumping Server Information\n\n");
    memcached_server_fn callbacks[1];

    callbacks[0]= dump_server_information;
    memcached_server_cursor(memc, callbacks, (void *)stderr,  1);
    fprintf(stderr, "\n----------------------------------------------------------------------\n");
  }
  test_true(if_successful == false);

  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  memcached_version(memc);

  // We only use one binary when we test, so this should be just fine.
  if_successful= libmemcached_util_version_check(memc, instance->major_version, instance->minor_version, instance->micro_version);
  test_true(if_successful == true);

  if (instance->micro_version > 0)
  {
    if_successful= libmemcached_util_version_check(memc, instance->major_version, instance->minor_version, (uint8_t)(instance->micro_version -1));
  }
  else if (instance->minor_version > 0)
  {
    if_successful= libmemcached_util_version_check(memc, instance->major_version, (uint8_t)(instance->minor_version - 1), instance->micro_version);
  }
  else if (instance->major_version > 0)
  {
    if_successful= libmemcached_util_version_check(memc, (uint8_t)(instance->major_version -1), instance->minor_version, instance->micro_version);
  }

  test_true(if_successful == true);

  if (instance->micro_version > 0)
  {
    if_successful= libmemcached_util_version_check(memc, instance->major_version, instance->minor_version, (uint8_t)(instance->micro_version +1));
  }
  else if (instance->minor_version > 0)
  {
    if_successful= libmemcached_util_version_check(memc, instance->major_version, (uint8_t)(instance->minor_version +1), instance->micro_version);
  }
  else if (instance->major_version > 0)
  {
    if_successful= libmemcached_util_version_check(memc, (uint8_t)(instance->major_version +1), instance->minor_version, instance->micro_version);
  }

  test_true(if_successful == false);

  return TEST_SUCCESS;
}

static test_return_t getpid_connection_failure_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  // Test both the version that returns a code, and the one that does not.
  test_true(libmemcached_util_getpid(memcached_server_name(instance),
                                     memcached_server_port(instance) -1, NULL) == -1);

  test_true(libmemcached_util_getpid(memcached_server_name(instance),
                                     memcached_server_port(instance) -1, &rc) == -1);
  test_compare_got(MEMCACHED_CONNECTION_FAILURE, rc, memcached_strerror(memc, rc));

  return TEST_SUCCESS;
}


static test_return_t getpid_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  // Test both the version that returns a code, and the one that does not.
  test_true(libmemcached_util_getpid(memcached_server_name(instance),
                                     memcached_server_port(instance), NULL) > -1);

  test_true(libmemcached_util_getpid(memcached_server_name(instance),
                                     memcached_server_port(instance), &rc) > -1);
  test_compare(MEMCACHED_SUCCESS, rc);

  return TEST_SUCCESS;
}

static test_return_t ping_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  // Test both the version that returns a code, and the one that does not.
  test_true(libmemcached_util_ping(memcached_server_name(instance),
                                   memcached_server_port(instance), NULL));

  test_true(libmemcached_util_ping(memcached_server_name(instance),
                                   memcached_server_port(instance), &rc));

  test_compare(MEMCACHED_SUCCESS, rc);

  return TEST_SUCCESS;
}


#if 0
static test_return_t hash_sanity_test (memcached_st *memc)
{
  (void)memc;

  assert(MEMCACHED_HASH_DEFAULT == MEMCACHED_HASH_DEFAULT);
  assert(MEMCACHED_HASH_MD5 == MEMCACHED_HASH_MD5);
  assert(MEMCACHED_HASH_CRC == MEMCACHED_HASH_CRC);
  assert(MEMCACHED_HASH_FNV1_64 == MEMCACHED_HASH_FNV1_64);
  assert(MEMCACHED_HASH_FNV1A_64 == MEMCACHED_HASH_FNV1A_64);
  assert(MEMCACHED_HASH_FNV1_32 == MEMCACHED_HASH_FNV1_32);
  assert(MEMCACHED_HASH_FNV1A_32 == MEMCACHED_HASH_FNV1A_32);
#ifdef HAVE_HSIEH_HASH
  assert(MEMCACHED_HASH_HSIEH == MEMCACHED_HASH_HSIEH);
#endif
  assert(MEMCACHED_HASH_MURMUR == MEMCACHED_HASH_MURMUR);
  assert(MEMCACHED_HASH_JENKINS == MEMCACHED_HASH_JENKINS);
  assert(MEMCACHED_HASH_MAX == MEMCACHED_HASH_MAX);

  return TEST_SUCCESS;
}
#endif

static test_return_t hsieh_avaibility_test (memcached_st *memc)
{
  test_skip(true, libhashkit_has_algorithm(HASHKIT_HASH_HSIEH));

  test_compare(MEMCACHED_SUCCESS, 
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH,
                                      (uint64_t)MEMCACHED_HASH_HSIEH));

  return TEST_SUCCESS;
}

static test_return_t murmur_avaibility_test (memcached_st *memc)
{
  test_skip(true, libhashkit_has_algorithm(HASHKIT_HASH_MURMUR));

  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_MURMUR));

  return TEST_SUCCESS;
}

static test_return_t one_at_a_time_run (memcached_st *)
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(one_at_a_time_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_DEFAULT));
  }

  return TEST_SUCCESS;
}

static test_return_t md5_run (memcached_st *)
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(md5_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_MD5));
  }

  return TEST_SUCCESS;
}

static test_return_t crc_run (memcached_st *)
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(crc_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_CRC));
  }

  return TEST_SUCCESS;
}

static test_return_t fnv1_64_run (memcached_st *)
{
  test_skip(true, libhashkit_has_algorithm(HASHKIT_HASH_FNV1_64));

  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(fnv1_64_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_FNV1_64));
  }

  return TEST_SUCCESS;
}

static test_return_t fnv1a_64_run (memcached_st *)
{
  test_skip(true, libhashkit_has_algorithm(HASHKIT_HASH_FNV1A_64));

  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(fnv1a_64_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_FNV1A_64));
  }

  return TEST_SUCCESS;
}

static test_return_t fnv1_32_run (memcached_st *)
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(fnv1_32_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_FNV1_32));
  }

  return TEST_SUCCESS;
}

static test_return_t fnv1a_32_run (memcached_st *)
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(fnv1a_32_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_FNV1A_32));
  }

  return TEST_SUCCESS;
}

static test_return_t hsieh_run (memcached_st *)
{
  test_skip(true, libhashkit_has_algorithm(HASHKIT_HASH_HSIEH));

  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(hsieh_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_HSIEH));
  }

  return TEST_SUCCESS;
}

static test_return_t murmur_run (memcached_st *)
{
  test_skip(true, libhashkit_has_algorithm(HASHKIT_HASH_MURMUR));

#ifdef WORDS_BIGENDIAN
  (void)murmur_values;
  return TEST_SKIPPED;
#else
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(murmur_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_MURMUR));
  }

  return TEST_SUCCESS;
#endif
}

static test_return_t jenkins_run (memcached_st *)
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    test_compare(jenkins_values[x],
                 memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_JENKINS));
  }

  return TEST_SUCCESS;
}

static uint32_t hash_md5_test_function(const char *string, size_t string_length, void *)
{
  return libhashkit_md5(string, string_length);
}

static uint32_t hash_crc_test_function(const char *string, size_t string_length, void *)
{
  return libhashkit_crc32(string, string_length);
}

static test_return_t memcached_get_hashkit_test (memcached_st *)
{
  uint32_t x;
  const char **ptr;
  hashkit_st new_kit;

  memcached_st *memc= memcached(test_literal_param("--server=localhost:1 --server=localhost:2 --server=localhost:3 --server=localhost:4 --server=localhost5"));

  uint32_t md5_hosts[]= {4U, 1U, 0U, 1U, 4U, 2U, 0U, 3U, 0U, 0U, 3U, 1U, 0U, 0U, 1U, 3U, 0U, 0U, 0U, 3U, 1U, 0U, 4U, 4U, 3U};
  uint32_t crc_hosts[]= {2U, 4U, 1U, 0U, 2U, 4U, 4U, 4U, 1U, 2U, 3U, 4U, 3U, 4U, 1U, 3U, 3U, 2U, 0U, 0U, 0U, 1U, 2U, 4U, 0U};

  const hashkit_st *kit= memcached_get_hashkit(memc);

  hashkit_clone(&new_kit, kit);
  test_compare(HASHKIT_SUCCESS, hashkit_set_custom_function(&new_kit, hash_md5_test_function, NULL));

  memcached_set_hashkit(memc, &new_kit);

  /*
    Verify Setting the hash.
  */
  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= hashkit_digest(kit, *ptr, strlen(*ptr));
    test_compare_got(md5_values[x], hash_val, *ptr);
  }


  /*
    Now check memcached_st.
  */
  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash(memc, *ptr, strlen(*ptr));
    test_compare_got(md5_hosts[x], hash_val, *ptr);
  }

  test_compare(HASHKIT_SUCCESS, hashkit_set_custom_function(&new_kit, hash_crc_test_function, NULL));

  memcached_set_hashkit(memc, &new_kit);

  /*
    Verify Setting the hash.
  */
  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= hashkit_digest(kit, *ptr, strlen(*ptr));
    test_true(crc_values[x] == hash_val);
  }

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash(memc, *ptr, strlen(*ptr));
    test_compare(crc_hosts[x], hash_val);
  }

  memcached_free(memc);

  return TEST_SUCCESS;
}

/*
  Test case adapted from John Gorman <johngorman2@gmail.com>

  We are testing the error condition when we connect to a server via memcached_get()
  but find that the server is not available.
*/
static test_return_t memcached_get_MEMCACHED_ERRNO(memcached_st *)
{
  const char *key= "MemcachedLives";
  size_t len;
  uint32_t flags;
  memcached_return rc;

  // Create a handle.
  memcached_st *tl_memc_h= memcached(test_literal_param("--server=localhost:9898 --server=localhost:9899")); // This server should not exist

  // See if memcached is reachable.
  char *value= memcached_get(tl_memc_h, key, strlen(key), &len, &flags, &rc);

  test_false(value);
  test_zero(len);
  test_true(memcached_failed(rc));

  memcached_free(tl_memc_h);

  return TEST_SUCCESS;
}

/*
  We connect to a server which exists, but search for a key that does not exist.
*/
static test_return_t memcached_get_MEMCACHED_NOTFOUND(memcached_st *memc)
{
  const char *key= "MemcachedKeyNotEXIST";
  size_t len;
  uint32_t flags;
  memcached_return rc;

  // See if memcached is reachable.
  char *value= memcached_get(memc, key, strlen(key), &len, &flags, &rc);

  test_false(value);
  test_zero(len);
  test_compare(MEMCACHED_NOTFOUND, rc);

  return TEST_SUCCESS;
}

/*
  Test case adapted from John Gorman <johngorman2@gmail.com>

  We are testing the error condition when we connect to a server via memcached_get_by_key()
  but find that the server is not available.
*/
static test_return_t memcached_get_by_key_MEMCACHED_ERRNO(memcached_st *memc)
{
  (void)memc;
  memcached_st *tl_memc_h;
  memcached_server_st *servers;

  const char *key= "MemcachedLives";
  size_t len;
  uint32_t flags;
  memcached_return rc;
  char *value;

  // Create a handle.
  tl_memc_h= memcached_create(NULL);
  servers= memcached_servers_parse("localhost:9898,localhost:9899"); // This server should not exist
  memcached_server_push(tl_memc_h, servers);
  memcached_server_list_free(servers);

  // See if memcached is reachable.
  value= memcached_get_by_key(tl_memc_h, key, strlen(key), key, strlen(key), &len, &flags, &rc);

  test_false(value);
  test_zero(len);
  test_true(memcached_failed(rc));

  memcached_free(tl_memc_h);

  return TEST_SUCCESS;
}

/*
  We connect to a server which exists, but search for a key that does not exist.
*/
static test_return_t memcached_get_by_key_MEMCACHED_NOTFOUND(memcached_st *memc)
{
  const char *key= "MemcachedKeyNotEXIST";
  size_t len;
  uint32_t flags;
  memcached_return rc;
  char *value;

  // See if memcached is reachable.
  value= memcached_get_by_key(memc, key, strlen(key), key, strlen(key), &len, &flags, &rc);

  test_false(value);
  test_zero(len);
  test_compare(MEMCACHED_NOTFOUND, rc);

  return TEST_SUCCESS;
}

static test_return_t regression_bug_434484(memcached_st *memc)
{
  test_skip(TEST_SUCCESS, pre_binary(memc));

  const char *key= "regression_bug_434484";
  size_t keylen= strlen(key);

  memcached_return_t ret= memcached_append(memc, key, keylen, key, keylen, 0, 0);
  test_compare(MEMCACHED_NOTSTORED, ret);

  size_t size= 2048 * 1024;
  char *data= (char*)calloc(1, size);
  test_true(data);
  test_compare(MEMCACHED_E2BIG,
               memcached_set(memc, key, keylen, data, size, 0, 0));
  free(data);

  return TEST_SUCCESS;
}

static test_return_t regression_bug_434843(memcached_st *memc)
{
  test_skip(TEST_SUCCESS, pre_binary(memc));

  memcached_return_t rc;
  size_t counter= 0;
  memcached_execute_fn callbacks[]= { &callback_counter };

  /*
   * I only want to hit only _one_ server so I know the number of requests I'm
   * sending in the pipleine to the server. Let's try to do a multiget of
   * 1024 (that should satisfy most users don't you think?). Future versions
   * will include a mget_execute function call if you need a higher number.
 */
  uint32_t number_of_hosts= memcached_server_count(memc);
  memc->number_of_hosts= 1;
  const size_t max_keys= 1024;
  char **keys= (char**)calloc(max_keys, sizeof(char*));
  size_t *key_length= (size_t *)calloc(max_keys, sizeof(size_t));

  for (size_t x= 0; x < max_keys; ++x)
  {
    char k[251];

    key_length[x]= (size_t)snprintf(k, sizeof(k), "0200%lu", (unsigned long)x);
    keys[x]= strdup(k);
    test_true(keys[x]);
  }

  /*
   * Run two times.. the first time we should have 100% cache miss,
   * and the second time we should have 100% cache hits
 */
  for (size_t y= 0; y < 2; y++)
  {
    test_compare(MEMCACHED_SUCCESS,
                 memcached_mget(memc, (const char**)keys, key_length, max_keys));

    test_compare(y ?  MEMCACHED_SUCCESS : MEMCACHED_NOTFOUND, 
                 memcached_fetch_execute(memc, callbacks, (void *)&counter, 1));

    if (y == 0)
    {
      /* The first iteration should give me a 100% cache miss. verify that*/
      char blob[1024]= { 0 };

      test_false(counter);

      for (size_t x= 0; x < max_keys; ++x)
      {
        rc= memcached_add(memc, keys[x], key_length[x],
                          blob, sizeof(blob), 0, 0);
        test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
      }
    }
    else
    {
      /* Verify that we received all of the key/value pairs */
      test_compare(counter, max_keys);
    }
  }

  /* Release allocated resources */
  for (size_t x= 0; x < max_keys; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);

  memc->number_of_hosts= number_of_hosts;

  return TEST_SUCCESS;
}

static test_return_t regression_bug_434843_buffered(memcached_st *memc)
{
  memcached_return_t rc;
  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1);
  test_compare(MEMCACHED_SUCCESS, rc);

  return regression_bug_434843(memc);
}

static test_return_t regression_bug_421108(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_stat_st *memc_stat= memcached_stat(memc, NULL, &rc);
  test_compare(MEMCACHED_SUCCESS, rc);

  char *bytes_str= memcached_stat_get_value(memc, memc_stat, "bytes", &rc);
  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(bytes_str);
  char *bytes_read_str= memcached_stat_get_value(memc, memc_stat,
                                                 "bytes_read", &rc);
  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(bytes_read_str);

  char *bytes_written_str= memcached_stat_get_value(memc, memc_stat,
                                                    "bytes_written", &rc);
  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(bytes_written_str);

  unsigned long long bytes= strtoull(bytes_str, 0, 10);
  unsigned long long bytes_read= strtoull(bytes_read_str, 0, 10);
  unsigned long long bytes_written= strtoull(bytes_written_str, 0, 10);

  test_true(bytes != bytes_read);
  test_true(bytes != bytes_written);

  /* Release allocated resources */
  free(bytes_str);
  free(bytes_read_str);
  free(bytes_written_str);
  memcached_stat_free(NULL, memc_stat);

  return TEST_SUCCESS;
}

/*
 * The test case isn't obvious so I should probably document why
 * it works the way it does. Bug 442914 was caused by a bug
 * in the logic in memcached_purge (it did not handle the case
 * where the number of bytes sent was equal to the watermark).
 * In this test case, create messages so that we hit that case
 * and then disable noreply mode and issue a new command to
 * verify that it isn't stuck. If we change the format for the
 * delete command or the watermarks, we need to update this
 * test....
 */
static test_return_t regression_bug_442914(memcached_st *memc)
{
  test_skip(MEMCACHED_SUCCESS,  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 1));
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1);

  uint32_t number_of_hosts= memcached_server_count(memc);
  memc->number_of_hosts= 1;

  char k[250];
  size_t len;

  for (uint32_t x= 0; x < 250; ++x)
  {
    len= (size_t)snprintf(k, sizeof(k), "%0250u", x);
    memcached_return_t rc= memcached_delete(memc, k, len, 0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  (void)snprintf(k, sizeof(k), "%037u", 251U);
  len= strlen(k);

  memcached_return_t rc= memcached_delete(memc, k, len, 0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 0));
  test_compare(MEMCACHED_NOTFOUND, memcached_delete(memc, k, len, 0));

  memc->number_of_hosts= number_of_hosts;

  return TEST_SUCCESS;
}

static test_return_t regression_bug_447342(memcached_st *memc)
{
  memcached_server_instance_st instance_one;
  memcached_server_instance_st instance_two;

  if (memcached_server_count(memc) < 3 or pre_replication(memc) != TEST_SUCCESS)
    return TEST_SKIPPED;

  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS, 2));

  const unsigned int max_keys= 100;
  char **keys= (char**)calloc(max_keys, sizeof(char*));
  size_t *key_length= (size_t *)calloc(max_keys, sizeof(size_t));

  for (unsigned int x= 0; x < max_keys; ++x)
  {
    char k[251];

    key_length[x]= (size_t)snprintf(k, sizeof(k), "0200%lu", (unsigned long)x);
    keys[x]= strdup(k);
    test_true(keys[x]);
    test_compare(MEMCACHED_SUCCESS,
                 memcached_set(memc, k, key_length[x], k, key_length[x], 0, 0));
  }

  /*
   ** We are using the quiet commands to store the replicas, so we need
   ** to ensure that all of them are processed before we can continue.
   ** In the test we go directly from storing the object to trying to
   ** receive the object from all of the different servers, so we
   ** could end up in a race condition (the memcached server hasn't yet
   ** processed the quiet command from the replication set when it process
   ** the request from the other client (created by the clone)). As a
   ** workaround for that we call memcached_quit to send the quit command
   ** to the server and wait for the response ;-) If you use the test code
   ** as an example for your own code, please note that you shouldn't need
   ** to do this ;-)
 */
  memcached_quit(memc);

  /* Verify that all messages are stored, and we didn't stuff too much
   * into the servers
 */
  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, (const char* const *)keys, key_length, max_keys));

  unsigned int counter= 0;
  memcached_execute_fn callbacks[]= { &callback_counter };
  test_compare(MEMCACHED_SUCCESS, 
               memcached_fetch_execute(memc, callbacks, (void *)&counter, 1));

  /* Verify that we received all of the key/value pairs */
  test_compare(counter, max_keys);

  memcached_quit(memc);
  /*
   * Don't do the following in your code. I am abusing the internal details
   * within the library, and this is not a supported interface.
   * This is to verify correct behavior in the library. Fake that two servers
   * are dead..
 */
  instance_one= memcached_server_instance_by_position(memc, 0);
  instance_two= memcached_server_instance_by_position(memc, 2);
  in_port_t port0= instance_one->port;
  in_port_t port2= instance_two->port;

  ((memcached_server_write_instance_st)instance_one)->port= 0;
  ((memcached_server_write_instance_st)instance_two)->port= 0;

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, (const char* const *)keys, key_length, max_keys));

  counter= 0;
  test_compare(MEMCACHED_SUCCESS, 
               memcached_fetch_execute(memc, callbacks, (void *)&counter, 1));
  test_compare(counter, (unsigned int)max_keys);

  /* restore the memc handle */
  ((memcached_server_write_instance_st)instance_one)->port= port0;
  ((memcached_server_write_instance_st)instance_two)->port= port2;

  memcached_quit(memc);

  /* Remove half of the objects */
  for (size_t x= 0; x < max_keys; ++x)
  {
    if (x & 1)
    {
      test_compare(MEMCACHED_SUCCESS,
                   memcached_delete(memc, keys[x], key_length[x], 0));
    }
  }

  memcached_quit(memc);
  ((memcached_server_write_instance_st)instance_one)->port= 0;
  ((memcached_server_write_instance_st)instance_two)->port= 0;

  /* now retry the command, this time we should have cache misses */
  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(memc, (const char* const *)keys, key_length, max_keys));

  counter= 0;
  test_compare(MEMCACHED_SUCCESS, 
               memcached_fetch_execute(memc, callbacks, (void *)&counter, 1));
  test_compare(counter, (unsigned int)(max_keys >> 1));

  /* Release allocated resources */
  for (size_t x= 0; x < max_keys; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);

  /* restore the memc handle */
  ((memcached_server_write_instance_st)instance_one)->port= port0;
  ((memcached_server_write_instance_st)instance_two)->port= port2;

  return TEST_SUCCESS;
}

static test_return_t regression_bug_463297(memcached_st *memc)
{
  memcached_st *memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);
  test_true(memcached_version(memc_clone) == MEMCACHED_SUCCESS);

  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc_clone, 0);

  if (instance->major_version > 1 ||
      (instance->major_version == 1 &&
       instance->minor_version > 2))
  {
    /* Binary protocol doesn't support deferred delete */
    memcached_st *bin_clone= memcached_clone(NULL, memc);
    test_true(bin_clone);
    test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(bin_clone, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1));
    test_compare(MEMCACHED_INVALID_ARGUMENTS, memcached_delete(bin_clone, "foo", 3, 1));
    memcached_free(bin_clone);

    memcached_quit(memc_clone);

    /* If we know the server version, deferred delete should fail
     * with invalid arguments */
    test_compare(MEMCACHED_INVALID_ARGUMENTS, memcached_delete(memc_clone, "foo", 3, 1));

    /* If we don't know the server version, we should get a protocol error */
    memcached_return_t rc= memcached_delete(memc, "foo", 3, 1);

    /* but there is a bug in some of the memcached servers (1.4) that treats
     * the counter as noreply so it doesn't send the proper error message
   */
    test_true_got(rc == MEMCACHED_PROTOCOL_ERROR || rc == MEMCACHED_NOTFOUND || rc == MEMCACHED_CLIENT_ERROR || rc == MEMCACHED_INVALID_ARGUMENTS, memcached_strerror(NULL, rc));

    /* And buffered mode should be disabled and we should get protocol error */
    test_true(memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1) == MEMCACHED_SUCCESS);
    rc= memcached_delete(memc, "foo", 3, 1);
    test_true_got(rc == MEMCACHED_PROTOCOL_ERROR || rc == MEMCACHED_NOTFOUND || rc == MEMCACHED_CLIENT_ERROR || rc == MEMCACHED_INVALID_ARGUMENTS, memcached_strerror(NULL, rc));

    /* Same goes for noreply... */
    test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 1));
    rc= memcached_delete(memc, "foo", 3, 1);
    test_true_got(rc == MEMCACHED_PROTOCOL_ERROR || rc == MEMCACHED_NOTFOUND || rc == MEMCACHED_CLIENT_ERROR || rc == MEMCACHED_INVALID_ARGUMENTS, memcached_strerror(NULL, rc));

    /* but a normal request should go through (and be buffered) */
    test_compare(MEMCACHED_BUFFERED, (rc= memcached_delete(memc, "foo", 3, 0)));
    test_compare(MEMCACHED_SUCCESS, memcached_flush_buffers(memc));

    test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 0));
    /* unbuffered noreply should be success */
    test_compare(MEMCACHED_SUCCESS, memcached_delete(memc, "foo", 3, 0));
    /* unbuffered with reply should be not found... */
    test_compare(MEMCACHED_SUCCESS, memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 0));
    test_compare(MEMCACHED_NOTFOUND, memcached_delete(memc, "foo", 3, 0));
  }

  memcached_free(memc_clone);
  return TEST_SUCCESS;
}


/* Test memcached_server_get_last_disconnect
 * For a working server set, shall be NULL
 * For a set of non existing server, shall not be NULL
 */
static test_return_t test_get_last_disconnect(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_server_instance_st disconnected_server;

  /* With the working set of server */
  const char *key= "marmotte";
  const char *value= "milka";

  memcached_reset_last_disconnected_server(memc);
  test_false(memc->last_disconnected_server);
  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  disconnected_server = memcached_server_get_last_disconnect(memc);
  test_false(disconnected_server);

  /* With a non existing server */
  memcached_st *mine;
  memcached_server_st *servers;

  const char *server_list= "localhost:9";

  servers= memcached_servers_parse(server_list);
  test_true(servers);
  mine= memcached_create(NULL);
  rc= memcached_server_push(mine, servers);
  test_compare(MEMCACHED_SUCCESS, rc);
  memcached_server_list_free(servers);
  test_true(mine);

  rc= memcached_set(mine, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(memcached_failed(rc));

  disconnected_server= memcached_server_get_last_disconnect(mine);
  test_true_got(disconnected_server, memcached_strerror(mine, rc));
  test_compare(in_port_t(9), memcached_server_port(disconnected_server));
  test_false(strncmp(memcached_server_name(disconnected_server),"localhost",9));

  memcached_quit(mine);
  memcached_free(mine);

  return TEST_SUCCESS;
}

static test_return_t test_multiple_get_last_disconnect(memcached_st *)
{
  const char *server_string= "--server=localhost:8888 --server=localhost:8889 --server=localhost:8890 --server=localhost:8891 --server=localhost:8892";
  char buffer[BUFSIZ];

  test_compare(MEMCACHED_SUCCESS,
               libmemcached_check_configuration(server_string, strlen(server_string), buffer, sizeof(buffer)));

  memcached_st *memc= memcached(server_string, strlen(server_string));
  test_true(memc);

  // We will just use the error strings as our keys
  uint32_t counter= 100;
  while (--counter)
  {
    for (int x= int(MEMCACHED_SUCCESS); x < int(MEMCACHED_MAXIMUM_RETURN); ++x)
    {
      const char *msg=  memcached_strerror(memc, memcached_return_t(x));
      memcached_return_t ret= memcached_set(memc, msg, strlen(msg), NULL, 0, (time_t)0, (uint32_t)0);
      test_true_got((ret == MEMCACHED_CONNECTION_FAILURE or ret == MEMCACHED_SERVER_TEMPORARILY_DISABLED), memcached_last_error_message(memc));

      memcached_server_instance_st disconnected_server= memcached_server_get_last_disconnect(memc);
      test_true(disconnected_server);
      test_strcmp("localhost", memcached_server_name(disconnected_server));
      test_true(memcached_server_port(disconnected_server) >= 8888 and memcached_server_port(disconnected_server) <= 8892);

      if (random() % 2)
      {
        memcached_reset_last_disconnected_server(memc);
      }
    }
  }

  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t test_verbosity(memcached_st *memc)
{
  memcached_verbosity(memc, 3);

  return TEST_SUCCESS;
}


static memcached_return_t stat_printer(memcached_server_instance_st server,
                                       const char *key, size_t key_length,
                                       const char *value, size_t value_length,
                                       void *context)
{
  (void)server;
  (void)context;
  (void)key;
  (void)key_length;
  (void)value;
  (void)value_length;

  return MEMCACHED_SUCCESS;
}

static test_return_t memcached_stat_execute_test(memcached_st *memc)
{
  memcached_return_t rc= memcached_stat_execute(memc, NULL, stat_printer, NULL);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_stat_execute(memc, "slabs", stat_printer, NULL);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_stat_execute(memc, "items", stat_printer, NULL);
  test_compare(MEMCACHED_SUCCESS, rc);

  rc= memcached_stat_execute(memc, "sizes", stat_printer, NULL);
  test_compare(MEMCACHED_SUCCESS, rc);

  return TEST_SUCCESS;
}

/*
 * This test ensures that the failure counter isn't incremented during
 * normal termination of the memcached instance.
 */
static test_return_t wrong_failure_counter_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_server_instance_st instance;

  /* Set value to force connection to the server */
  const char *key= "marmotte";
  const char *value= "milka";

  /*
   * Please note that I'm abusing the internal structures in libmemcached
   * in a non-portable way and you shouldn't be doing this. I'm only
   * doing this in order to verify that the library works the way it should
 */
  uint32_t number_of_hosts= memcached_server_count(memc);
  memc->number_of_hosts= 1;

  /* Ensure that we are connected to the server by setting a value */
  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);


  instance= memcached_server_instance_by_position(memc, 0);
  /* The test is to see that the memcached_quit doesn't increase the
   * the server failure conter, so let's ensure that it is zero
   * before sending quit
 */
  ((memcached_server_write_instance_st)instance)->server_failure_counter= 0;

  memcached_quit(memc);

  /* Verify that it memcached_quit didn't increment the failure counter
   * Please note that this isn't bullet proof, because an error could
   * occur...
 */
  test_zero(instance->server_failure_counter);

  /* restore the instance */
  memc->number_of_hosts= number_of_hosts;

  return TEST_SUCCESS;
}

/*
 * This tests ensures expected disconnections (for some behavior changes
 * for instance) do not wrongly increase failure counter
 */
static test_return_t wrong_failure_counter_two_test(memcached_st *memc)
{
  memcached_return rc;

  memcached_st *memc_clone;
  memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);

  /* Set value to force connection to the server */
  const char *key= "marmotte";
  const char *value= "milka";
  char *string = NULL;
  size_t string_length;
  uint32_t flags;

  rc= memcached_set(memc_clone, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true_got(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED, memcached_strerror(NULL, rc));


  /* put failure limit to 1 */
  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, 1));

  /* Put a retry timeout to effectively activate failure_limit effect */
  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 1));

  /* change behavior that triggers memcached_quit()*/
  test_compare(MEMCACHED_SUCCESS,
               memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1));


  /* Check if we still are connected */
  string= memcached_get(memc_clone, key, strlen(key),
                        &string_length, &flags, &rc);

  test_compare_got(MEMCACHED_SUCCESS, rc, memcached_strerror(NULL, rc));
  test_true(string);
  free(string);
  memcached_free(memc_clone);

  return TEST_SUCCESS;
}




/*
 * Test that ensures mget_execute does not end into recursive calls that finally fails
 */
static test_return_t regression_bug_490486(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, 1);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 3600);

#ifdef __APPLE__
  return TEST_SKIPPED; // My MAC can't handle this test
#endif

  /*
   * I only want to hit _one_ server so I know the number of requests I'm
   * sending in the pipeline.
 */
  uint32_t number_of_hosts= memc->number_of_hosts;
  memc->number_of_hosts= 1;
  size_t max_keys= 20480;


  char **keys= (char **)calloc(max_keys, sizeof(char*));
  size_t *key_length= (size_t *)calloc(max_keys, sizeof(size_t));

  /* First add all of the items.. */
  char blob[1024]= { 0 };
  for (size_t x= 0; x < max_keys; ++x)
  {
    char k[251];
    key_length[x]= (size_t)snprintf(k, sizeof(k), "0200%lu", (unsigned long)x);
    keys[x]= strdup(k);
    test_true(keys[x]);
    memcached_return rc= memcached_set(memc, keys[x], key_length[x], blob, sizeof(blob), 0, 0);
    test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED); // MEMCACHED_TIMEOUT <-- hash been observed on OSX
  }

  {

    /* Try to get all of them with a large multiget */
    size_t counter= 0;
    memcached_execute_function callbacks[]= { &callback_counter };
    memcached_return_t rc= memcached_mget_execute(memc, (const char**)keys, key_length,
                                                  (size_t)max_keys, callbacks, &counter, 1);
    test_compare(MEMCACHED_SUCCESS, rc);

    char* the_value= NULL;
    char the_key[MEMCACHED_MAX_KEY];
    size_t the_key_length;
    size_t the_value_length;
    uint32_t the_flags;

    do {
      the_value= memcached_fetch(memc, the_key, &the_key_length, &the_value_length, &the_flags, &rc);

      if ((the_value!= NULL) && (rc == MEMCACHED_SUCCESS))
      {
        ++counter;
        free(the_value);
      }

    } while ( (the_value!= NULL) && (rc == MEMCACHED_SUCCESS));


    test_compare(MEMCACHED_END, rc);

    /* Verify that we got all of the items */
    test_compare(counter, max_keys);
  }

  /* Release all allocated resources */
  for (size_t x= 0; x < max_keys; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);

  memc->number_of_hosts= number_of_hosts;

  return TEST_SUCCESS;
}

static test_return_t regression_bug_583031(memcached_st *)
{
  memcached_st *memc= memcached_create(NULL);
  test_true(memc);
  test_compare(MEMCACHED_SUCCESS, memcached_server_add(memc, "10.2.3.4", 11211));

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SND_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, 3);

  memcached_return_t rc;
  size_t length;
  uint32_t flags;

  const char *value= memcached_get(memc, "dsf", 3, &length, &flags, &rc);
  test_false(value);
  test_zero(length);

  test_compare_got(MEMCACHED_TIMEOUT, rc, memcached_last_error_message(memc));

  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t regression_bug_581030(memcached_st *)
{
#ifndef DEBUG
  memcached_stat_st *local_stat= memcached_stat(NULL, NULL, NULL);
  test_false(local_stat);

  memcached_stat_free(NULL, NULL);
#endif

  return TEST_SUCCESS;
}

#define regression_bug_655423_COUNT 6000
static test_return_t regression_bug_655423(memcached_st *memc)
{
  memcached_st *clone= memcached_clone(NULL, memc);
  memc= NULL; // Just to make sure it is not used
  test_true(clone);
  char payload[100];

#ifdef __APPLE__
  return TEST_SKIPPED;
#endif

  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(clone, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1));
  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(clone, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1));
  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(clone, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1));
  test_skip(MEMCACHED_SUCCESS, memcached_behavior_set(clone, MEMCACHED_BEHAVIOR_IO_KEY_PREFETCH, 1));

  memset(payload, int('x'), sizeof(payload));

  for (uint32_t x= 0; x < regression_bug_655423_COUNT; x++)
  {
    char key[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
    snprintf(key, sizeof(key), "%u", x);

    test_compare(MEMCACHED_SUCCESS, memcached_set(clone, key, strlen(key), payload, sizeof(payload), 0, 0));
  }

  for (uint32_t x= 0; x < regression_bug_655423_COUNT; x++)
  {
    char key[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
    snprintf(key, sizeof(key), "%u", x);

    size_t value_length;
    memcached_return_t rc;
    char *value= memcached_get(clone, key, strlen(key), &value_length, NULL, &rc);

    if (rc == MEMCACHED_NOTFOUND)
    {
      test_false(value);
      test_zero(value_length);
      continue;
    }

    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(value);
    test_compare(100LLU, value_length);
    free(value);
  }

  char **keys= (char**)calloc(regression_bug_655423_COUNT, sizeof(char*));
  size_t *key_length= (size_t *)calloc(regression_bug_655423_COUNT, sizeof(size_t));
  for (uint32_t x= 0; x < regression_bug_655423_COUNT; x++)
  {
    char key[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
    snprintf(key, sizeof(key), "%u", x);

    keys[x]= strdup(key);
    test_true(keys[x]);
    key_length[x]= strlen(key);
    test_true(key_length[x]);
  }

  test_compare(MEMCACHED_SUCCESS,
               memcached_mget(clone, (const char* const *)keys, key_length, regression_bug_655423_COUNT));

  uint32_t count= 0;
  memcached_result_st *result= NULL;
  while ((result= memcached_fetch_result(clone, result, NULL)))
  {
    test_compare(size_t(100), memcached_result_length(result));
    count++;
  }

  test_true(count > 100); // If we don't get back atleast this, something is up

  /* Release all allocated resources */
  for (size_t x= 0; x < regression_bug_655423_COUNT; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);


  memcached_free(clone);

  return TEST_SUCCESS;
}

/*
 * Test that ensures that buffered set to not trigger problems during io_flush
 */
#define regression_bug_490520_COUNT 200480
static test_return_t regression_bug_490520(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK,1);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS,1);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT,1);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 3600);

  memc->number_of_hosts= 1;

  char **keys= (char **)calloc(regression_bug_490520_COUNT, sizeof(char*));
  size_t *key_length= (size_t *)calloc(regression_bug_490520_COUNT, sizeof(size_t));

  /* First add all of the items.. */
  char blob[3333] = {0};
  for (uint32_t x= 0; x < regression_bug_490520_COUNT; ++x)
  {
    char k[251];
    key_length[x]= (size_t)snprintf(k, sizeof(k), "0200%u", x);
    keys[x]= strdup(k);
    test_true(keys[x]);

    memcached_return rc= memcached_set(memc, keys[x], key_length[x], blob, sizeof(blob), 0, 0);
    test_true(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_BUFFERED);
  }

  for (uint32_t x= 0; x < regression_bug_490520_COUNT; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);

  return TEST_SUCCESS;
}


static test_return_t regression_bug_854604(memcached_st *)
{
  char buffer[1024];

  test_compare(MEMCACHED_INVALID_ARGUMENTS, libmemcached_check_configuration(0, 0, buffer, 0));

  test_compare(MEMCACHED_PARSE_ERROR, libmemcached_check_configuration(test_literal_param("syntax error"), buffer, 0));

  test_compare(MEMCACHED_PARSE_ERROR, libmemcached_check_configuration(test_literal_param("syntax error"), buffer, 1));
  test_compare(buffer[0], 0);

  test_compare(MEMCACHED_PARSE_ERROR, libmemcached_check_configuration(test_literal_param("syntax error"), buffer, 10));
  test_true(strlen(buffer));

  test_compare(MEMCACHED_PARSE_ERROR, libmemcached_check_configuration(test_literal_param("syntax error"), buffer, sizeof(buffer)));
  test_true(strlen(buffer));

  return TEST_SUCCESS;
}

static void memcached_die(memcached_st* mc, memcached_return error, const char* what, uint32_t it)
{
  fprintf(stderr, "Iteration #%u: ", it);

  if (error == MEMCACHED_ERRNO)
  {
    fprintf(stderr, "system error %d from %s: %s\n",
            errno, what, strerror(errno));
  }
  else
  {
    fprintf(stderr, "error %d from %s: %s\n", error, what,
            memcached_strerror(mc, error));
  }
}

#define TEST_CONSTANT_CREATION 200

static test_return_t regression_bug_(memcached_st *memc)
{
  const char *remote_server;
  (void)memc;

  if (! (remote_server= getenv("LIBMEMCACHED_REMOTE_SERVER")))
  {
    return TEST_SKIPPED;
  }

  for (uint32_t x= 0; x < TEST_CONSTANT_CREATION; x++)
  {
    memcached_st* mc= memcached_create(NULL);
    memcached_return rc;

    rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
    if (rc != MEMCACHED_SUCCESS)
    {
      memcached_die(mc, rc, "memcached_behavior_set", x);
    }

    rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_CACHE_LOOKUPS, 1);
    if (rc != MEMCACHED_SUCCESS)
    {
      memcached_die(mc, rc, "memcached_behavior_set", x);
    }

    rc= memcached_server_add(mc, remote_server, 0);
    if (rc != MEMCACHED_SUCCESS)
    {
      memcached_die(mc, rc, "memcached_server_add", x);
    }

    const char *set_key= "akey";
    const size_t set_key_len= strlen(set_key);
    const char *set_value= "a value";
    const size_t set_value_len= strlen(set_value);

    if (rc == MEMCACHED_SUCCESS)
    {
      if (x > 0)
      {
        size_t get_value_len;
        char *get_value;
        uint32_t get_value_flags;

        get_value= memcached_get(mc, set_key, set_key_len, &get_value_len,
                                 &get_value_flags, &rc);
        if (rc != MEMCACHED_SUCCESS)
        {
          memcached_die(mc, rc, "memcached_get", x);
        }
        else
        {

          if (x != 0 &&
              (get_value_len != set_value_len
               || 0!=strncmp(get_value, set_value, get_value_len)))
          {
            fprintf(stderr, "Values don't match?\n");
            rc= MEMCACHED_FAILURE;
          }
          free(get_value);
        }
      }

      rc= memcached_set(mc,
                        set_key, set_key_len,
                        set_value, set_value_len,
                        0, /* time */
                        0  /* flags */
                       );
      if (rc != MEMCACHED_SUCCESS)
      {
        memcached_die(mc, rc, "memcached_set", x);
      }
    }

    memcached_quit(mc);
    memcached_free(mc);

    if (rc != MEMCACHED_SUCCESS)
    {
      break;
    }
  }

  return TEST_SUCCESS;
}

/* Clean the server before beginning testing */
test_st tests[] ={
  {"util_version", true, (test_callback_fn*)util_version_test },
  {"flush", false, (test_callback_fn*)flush_test },
  {"init", false, (test_callback_fn*)init_test },
  {"allocation", false, (test_callback_fn*)allocation_test },
  {"server_list_null_test", false, (test_callback_fn*)server_list_null_test},
  {"server_unsort", false, (test_callback_fn*)server_unsort_test},
  {"server_sort", false, (test_callback_fn*)server_sort_test},
  {"server_sort2", false, (test_callback_fn*)server_sort2_test},
  {"memcached_server_remove", false, (test_callback_fn*)memcached_server_remove_test},
  {"clone_test", false, (test_callback_fn*)clone_test },
  {"connection_test", false, (test_callback_fn*)connection_test},
  {"callback_test", false, (test_callback_fn*)callback_test},
  {"userdata_test", false, (test_callback_fn*)userdata_test},
  {"set", false, (test_callback_fn*)set_test },
  {"set2", false, (test_callback_fn*)set_test2 },
  {"set3", false, (test_callback_fn*)set_test3 },
  {"dump", true, (test_callback_fn*)dump_test},
  {"add", true, (test_callback_fn*)add_test },
  {"memcached_fetch_result(MEMCACHED_NOTFOUND)", true, (test_callback_fn*)memcached_fetch_result_NOT_FOUND },
  {"replace", true, (test_callback_fn*)replace_test },
  {"delete", true, (test_callback_fn*)delete_test },
  {"get", true, (test_callback_fn*)get_test },
  {"get2", false, (test_callback_fn*)get_test2 },
  {"get3", false, (test_callback_fn*)get_test3 },
  {"get4", false, (test_callback_fn*)get_test4 },
  {"partial mget", false, (test_callback_fn*)get_test5 },
  {"stats_servername", false, (test_callback_fn*)stats_servername_test },
  {"increment", false, (test_callback_fn*)increment_test },
  {"increment_with_initial", true, (test_callback_fn*)increment_with_initial_test },
  {"decrement", false, (test_callback_fn*)decrement_test },
  {"decrement_with_initial", true, (test_callback_fn*)decrement_with_initial_test },
  {"increment_by_key", false, (test_callback_fn*)increment_by_key_test },
  {"increment_with_initial_by_key", true, (test_callback_fn*)increment_with_initial_by_key_test },
  {"decrement_by_key", false, (test_callback_fn*)decrement_by_key_test },
  {"decrement_with_initial_by_key", true, (test_callback_fn*)decrement_with_initial_by_key_test },
  {"binary_increment_with_prefix", 1, (test_callback_fn*)binary_increment_with_prefix_test },
  {"quit", false, (test_callback_fn*)quit_test },
  {"mget", true, (test_callback_fn*)mget_test },
  {"mget_result", true, (test_callback_fn*)mget_result_test },
  {"mget_result_alloc", true, (test_callback_fn*)mget_result_alloc_test },
  {"mget_result_function", true, (test_callback_fn*)mget_result_function },
  {"mget_execute", true, (test_callback_fn*)mget_execute },
  {"mget_end", false, (test_callback_fn*)mget_end },
  {"get_stats", false, (test_callback_fn*)get_stats },
  {"add_host_test", false, (test_callback_fn*)add_host_test },
  {"add_host_test_1", false, (test_callback_fn*)add_host_test1 },
  {"get_stats_keys", false, (test_callback_fn*)get_stats_keys },
  {"version_string_test", false, (test_callback_fn*)version_string_test},
  {"bad_key", true, (test_callback_fn*)bad_key_test },
  {"memcached_server_cursor", true, (test_callback_fn*)memcached_server_cursor_test },
  {"read_through", true, (test_callback_fn*)read_through },
  {"delete_through", true, (test_callback_fn*)test_MEMCACHED_CALLBACK_DELETE_TRIGGER },
  {"noreply", true, (test_callback_fn*)noreply_test},
  {"analyzer", true, (test_callback_fn*)analyzer_test},
  {"memcached_pool_st", true, (test_callback_fn*)connection_pool_test },
  {"memcached_pool_st #2", true, (test_callback_fn*)connection_pool2_test },
  {"memcached_pool_st #3", true, (test_callback_fn*)connection_pool3_test },
  {"memcached_pool_test", true, (test_callback_fn*)memcached_pool_test },
  {"test_get_last_disconnect", true, (test_callback_fn*)test_get_last_disconnect},
  {"verbosity", true, (test_callback_fn*)test_verbosity},
  {"memcached_stat_execute", true, (test_callback_fn*)memcached_stat_execute_test},
  {"memcached_exist(MEMCACHED_NOTFOUND)", true, (test_callback_fn*)memcached_exist_NOTFOUND },
  {"memcached_exist(MEMCACHED_SUCCESS)", true, (test_callback_fn*)memcached_exist_SUCCESS },
  {"memcached_exist_by_key(MEMCACHED_NOTFOUND)", true, (test_callback_fn*)memcached_exist_by_key_NOTFOUND },
  {"memcached_exist_by_key(MEMCACHED_SUCCESS)", true, (test_callback_fn*)memcached_exist_by_key_SUCCESS },
  {"memcached_touch", 0, (test_callback_fn*)test_memcached_touch},
  {"memcached_touch_with_prefix", 0, (test_callback_fn*)test_memcached_touch_by_key},
  {0, 0, 0}
};

test_st touch_tests[] ={
  {"memcached_touch", 0, (test_callback_fn*)test_memcached_touch},
  {"memcached_touch_with_prefix", 0, (test_callback_fn*)test_memcached_touch_by_key},
  {0, 0, 0}
};

test_st behavior_tests[] ={
  {"libmemcached_string_behavior()", false, (test_callback_fn*)libmemcached_string_behavior_test},
  {"libmemcached_string_distribution()", false, (test_callback_fn*)libmemcached_string_distribution_test},
  {"behavior_test", false, (test_callback_fn*)behavior_test},
  {"MEMCACHED_BEHAVIOR_CORK", false, (test_callback_fn*)MEMCACHED_BEHAVIOR_CORK_test},
  {"MEMCACHED_BEHAVIOR_TCP_KEEPALIVE", false, (test_callback_fn*)MEMCACHED_BEHAVIOR_TCP_KEEPALIVE_test},
  {"MEMCACHED_BEHAVIOR_TCP_KEEPIDLE", false, (test_callback_fn*)MEMCACHED_BEHAVIOR_TCP_KEEPIDLE_test},
  {"MEMCACHED_BEHAVIOR_POLL_TIMEOUT", false, (test_callback_fn*)MEMCACHED_BEHAVIOR_POLL_TIMEOUT_test},
  {"MEMCACHED_CALLBACK_DELETE_TRIGGER_and_MEMCACHED_BEHAVIOR_NOREPLY", false, (test_callback_fn*)test_MEMCACHED_CALLBACK_DELETE_TRIGGER_and_MEMCACHED_BEHAVIOR_NOREPLY},
  {0, 0, 0}
};

test_st libmemcachedutil_tests[] ={
  {"libmemcached_util_ping()", true, (test_callback_fn*)ping_test },
  {"libmemcached_util_getpid()", true, (test_callback_fn*)getpid_test },
  {"libmemcached_util_getpid(MEMCACHED_CONNECTION_FAILURE)", true, (test_callback_fn*)getpid_connection_failure_test },
  {0, 0, 0}
};

test_st basic_tests[] ={
  {"init", true, (test_callback_fn*)basic_init_test},
  {"clone", true, (test_callback_fn*)basic_clone_test},
  {"reset", true, (test_callback_fn*)basic_reset_stack_test},
  {"reset heap", true, (test_callback_fn*)basic_reset_heap_test},
  {"reset stack clone", true, (test_callback_fn*)basic_reset_stack_clone_test},
  {"reset heap clone", true, (test_callback_fn*)basic_reset_heap_clone_test},
  {"memcached_return_t", false, (test_callback_fn*)memcached_return_t_TEST },
  {0, 0, 0}
};

test_st regression_binary_vs_block[] ={
  {"block add", true, (test_callback_fn*)block_add_regression},
  {"binary add", true, (test_callback_fn*)binary_add_regression},
  {0, 0, 0}
};

test_st async_tests[] ={
  {"add", true, (test_callback_fn*)add_wrapper },
  {0, 0, 0}
};

test_st memcached_server_get_last_disconnect_tests[] ={
  {"memcached_server_get_last_disconnect()", false, (test_callback_fn*)test_multiple_get_last_disconnect},
  {0, 0, (test_callback_fn*)0}
};


test_st result_tests[] ={
  {"result static", false, (test_callback_fn*)result_static},
  {"result alloc", false, (test_callback_fn*)result_alloc},
  {0, 0, (test_callback_fn*)0}
};

test_st version_1_2_3[] ={
  {"append", false, (test_callback_fn*)append_test },
  {"prepend", false, (test_callback_fn*)prepend_test },
  {"cas", false, (test_callback_fn*)cas_test },
  {"cas2", false, (test_callback_fn*)cas2_test },
  {"append_binary", false, (test_callback_fn*)append_binary_test },
  {0, 0, (test_callback_fn*)0}
};

test_st haldenbrand_tests[] ={
  {"memcached_set", false, (test_callback_fn*)user_supplied_bug1 },
  {"memcached_get()", false, (test_callback_fn*)user_supplied_bug2 },
  {"memcached_mget()", false, (test_callback_fn*)user_supplied_bug3 },
  {0, 0, (test_callback_fn*)0}
};

test_st user_tests[] ={
  {"user_supplied_bug4", true, (test_callback_fn*)user_supplied_bug4 },
  {"user_supplied_bug5", true, (test_callback_fn*)user_supplied_bug5 },
  {"user_supplied_bug6", true, (test_callback_fn*)user_supplied_bug6 },
  {"user_supplied_bug7", true, (test_callback_fn*)user_supplied_bug7 },
  {"user_supplied_bug8", true, (test_callback_fn*)user_supplied_bug8 },
  {"user_supplied_bug9", true, (test_callback_fn*)user_supplied_bug9 },
  {"user_supplied_bug10", true, (test_callback_fn*)user_supplied_bug10 },
  {"user_supplied_bug11", true, (test_callback_fn*)user_supplied_bug11 },
  {"user_supplied_bug12", true, (test_callback_fn*)user_supplied_bug12 },
  {"user_supplied_bug13", true, (test_callback_fn*)user_supplied_bug13 },
  {"user_supplied_bug14", true, (test_callback_fn*)user_supplied_bug14 },
  {"user_supplied_bug15", true, (test_callback_fn*)user_supplied_bug15 },
  {"user_supplied_bug16", true, (test_callback_fn*)user_supplied_bug16 },
#if !defined(__sun) && !defined(__OpenBSD__)
  /*
   ** It seems to be something weird with the character sets..
   ** value_fetch is unable to parse the value line (iscntrl "fails"), so I
   ** guess I need to find out how this is supposed to work.. Perhaps I need
   ** to run the test in a specific locale (I tried zh_CN.UTF-8 without success,
   ** so just disable the code for now...).
 */
  {"user_supplied_bug17", true, (test_callback_fn*)user_supplied_bug17 },
#endif
  {"user_supplied_bug18", true, (test_callback_fn*)user_supplied_bug18 },
  {"user_supplied_bug19", true, (test_callback_fn*)user_supplied_bug19 },
  {"user_supplied_bug20", true, (test_callback_fn*)user_supplied_bug20 },
  {"user_supplied_bug21", true, (test_callback_fn*)user_supplied_bug21 },
  {"wrong_failure_counter_test", true, (test_callback_fn*)wrong_failure_counter_test},
  {"wrong_failure_counter_two_test", true, (test_callback_fn*)wrong_failure_counter_two_test},
  {0, 0, (test_callback_fn*)0}
};

test_st replication_tests[]= {
  {"set", true, (test_callback_fn*)replication_set_test },
  {"get", false, (test_callback_fn*)replication_get_test },
  {"mget", false, (test_callback_fn*)replication_mget_test },
  {"delete", true, (test_callback_fn*)replication_delete_test },
  {"rand_mget", false, (test_callback_fn*)replication_randomize_mget_test },
  {"fail", false, (test_callback_fn*)replication_randomize_mget_fail_test },
  {0, 0, (test_callback_fn*)0}
};

/*
 * The following test suite is used to verify that we don't introduce
 * regression bugs. If you want more information about the bug / test,
 * you should look in the bug report at
 *   http://bugs.launchpad.net/libmemcached
 */
test_st regression_tests[]= {
  {"lp:434484", true, (test_callback_fn*)regression_bug_434484 },
  {"lp:434843", true, (test_callback_fn*)regression_bug_434843 },
  {"lp:434843-buffered", true, (test_callback_fn*)regression_bug_434843_buffered },
  {"lp:421108", true, (test_callback_fn*)regression_bug_421108 },
  {"lp:442914", true, (test_callback_fn*)regression_bug_442914 },
  {"lp:447342", true, (test_callback_fn*)regression_bug_447342 },
  {"lp:463297", true, (test_callback_fn*)regression_bug_463297 },
  {"lp:490486", true, (test_callback_fn*)regression_bug_490486 },
  {"lp:583031", true, (test_callback_fn*)regression_bug_583031 },
  {"lp:?", true, (test_callback_fn*)regression_bug_ },
  {"lp:728286", true, (test_callback_fn*)regression_bug_728286 },
  {"lp:581030", true, (test_callback_fn*)regression_bug_581030 },
  {"lp:71231153 connect()", true, (test_callback_fn*)regression_bug_71231153_connect },
  {"lp:71231153 poll()", true, (test_callback_fn*)regression_bug_71231153_poll },
  {"lp:655423", true, (test_callback_fn*)regression_bug_655423 },
  {"lp:490520", true, (test_callback_fn*)regression_bug_490520 },
  {"lp:854604", true, (test_callback_fn*)regression_bug_854604 },
  {0, false, (test_callback_fn*)0}
};

test_st ketama_compatibility[]= {
  {"libmemcached", true, (test_callback_fn*)ketama_compatibility_libmemcached },
  {"spymemcached", true, (test_callback_fn*)ketama_compatibility_spymemcached },
  {0, 0, (test_callback_fn*)0}
};

test_st generate_tests[] ={
  {"generate_pairs", true, (test_callback_fn*)generate_pairs },
  {"generate_data", true, (test_callback_fn*)generate_data },
  {"get_read", false, (test_callback_fn*)get_read },
  {"delete_generate", false, (test_callback_fn*)delete_generate },
  {"generate_buffer_data", true, (test_callback_fn*)generate_buffer_data },
  {"delete_buffer", false, (test_callback_fn*)delete_buffer_generate},
  {"generate_data", true, (test_callback_fn*)generate_data },
  {"mget_read", false, (test_callback_fn*)mget_read },
  {"mget_read_result", false, (test_callback_fn*)mget_read_result },
  {"memcached_fetch_result() use internal result", false, (test_callback_fn*)mget_read_internal_result },
  {"memcached_fetch_result() partial read", false, (test_callback_fn*)mget_read_partial_result },
  {"mget_read_function", false, (test_callback_fn*)mget_read_function },
  {"cleanup", true, (test_callback_fn*)cleanup_pairs },
  {"generate_large_pairs", true, (test_callback_fn*)generate_large_pairs },
  {"generate_data", true, (test_callback_fn*)generate_data },
  {"generate_buffer_data", true, (test_callback_fn*)generate_buffer_data },
  {"cleanup", true, (test_callback_fn*)cleanup_pairs },
  {0, 0, (test_callback_fn*)0}
};

test_st consistent_tests[] ={
  {"generate_pairs", true, (test_callback_fn*)generate_pairs },
  {"generate_data", true, (test_callback_fn*)generate_data },
  {"get_read", 0, (test_callback_fn*)get_read_count },
  {"cleanup", true, (test_callback_fn*)cleanup_pairs },
  {0, 0, (test_callback_fn*)0}
};

test_st consistent_weighted_tests[] ={
  {"generate_pairs", true, (test_callback_fn*)generate_pairs },
  {"generate_data", true, (test_callback_fn*)generate_data_with_stats },
  {"get_read", false, (test_callback_fn*)get_read_count },
  {"cleanup", true, (test_callback_fn*)cleanup_pairs },
  {0, 0, (test_callback_fn*)0}
};

test_st hsieh_availability[] ={
  {"hsieh_avaibility_test", false, (test_callback_fn*)hsieh_avaibility_test},
  {0, 0, (test_callback_fn*)0}
};

test_st murmur_availability[] ={
  {"murmur_avaibility_test", false, (test_callback_fn*)murmur_avaibility_test},
  {0, 0, (test_callback_fn*)0}
};

#if 0
test_st hash_sanity[] ={
  {"hash sanity", 0, (test_callback_fn*)hash_sanity_test},
  {0, 0, (test_callback_fn*)0}
};
#endif

test_st ketama_auto_eject_hosts[] ={
  {"auto_eject_hosts", true, (test_callback_fn*)auto_eject_hosts },
  {"output_ketama_weighted_keys", true, (test_callback_fn*)output_ketama_weighted_keys },
  {0, 0, (test_callback_fn*)0}
};

test_st hash_tests[] ={
  {"one_at_a_time_run", false, (test_callback_fn*)one_at_a_time_run },
  {"md5", false, (test_callback_fn*)md5_run },
  {"crc", false, (test_callback_fn*)crc_run },
  {"fnv1_64", false, (test_callback_fn*)fnv1_64_run },
  {"fnv1a_64", false, (test_callback_fn*)fnv1a_64_run },
  {"fnv1_32", false, (test_callback_fn*)fnv1_32_run },
  {"fnv1a_32", false, (test_callback_fn*)fnv1a_32_run },
  {"hsieh", false, (test_callback_fn*)hsieh_run },
  {"murmur", false, (test_callback_fn*)murmur_run },
  {"jenkis", false, (test_callback_fn*)jenkins_run },
  {"memcached_get_hashkit", false, (test_callback_fn*)memcached_get_hashkit_test },
  {0, 0, (test_callback_fn*)0}
};

test_st error_conditions[] ={
  {"memcached_get(MEMCACHED_ERRNO)", false, (test_callback_fn*)memcached_get_MEMCACHED_ERRNO },
  {"memcached_get(MEMCACHED_NOTFOUND)", false, (test_callback_fn*)memcached_get_MEMCACHED_NOTFOUND },
  {"memcached_get_by_key(MEMCACHED_ERRNO)", false, (test_callback_fn*)memcached_get_by_key_MEMCACHED_ERRNO },
  {"memcached_get_by_key(MEMCACHED_NOTFOUND)", false, (test_callback_fn*)memcached_get_by_key_MEMCACHED_NOTFOUND },
  {"memcached_get_by_key(MEMCACHED_NOTFOUND)", false, (test_callback_fn*)memcached_get_by_key_MEMCACHED_NOTFOUND },
  {"memcached_increment(MEMCACHED_NO_SERVERS)", false, (test_callback_fn*)memcached_increment_MEMCACHED_NO_SERVERS },
  {0, 0, (test_callback_fn*)0}
};

test_st parser_tests[] ={
  {"behavior", false, (test_callback_fn*)behavior_parser_test },
  {"boolean_options", false, (test_callback_fn*)parser_boolean_options_test },
  {"configure_file", false, (test_callback_fn*)memcached_create_with_options_with_filename },
  {"distribtions", false, (test_callback_fn*)parser_distribution_test },
  {"hash", false, (test_callback_fn*)parser_hash_test },
  {"libmemcached_check_configuration", false, (test_callback_fn*)libmemcached_check_configuration_test },
  {"libmemcached_check_configuration_with_filename", false, (test_callback_fn*)libmemcached_check_configuration_with_filename_test },
  {"number_options", false, (test_callback_fn*)parser_number_options_test },
  {"randomly generated options", false, (test_callback_fn*)random_statement_build_test },
  {"namespace", false, (test_callback_fn*)parser_key_prefix_test },
  {"server", false, (test_callback_fn*)server_test },
  {"bad server strings", false, (test_callback_fn*)servers_bad_test },
  {"server with weights", false, (test_callback_fn*)server_with_weight_test },
  {"parsing servername, port, and weight", false, (test_callback_fn*)test_hostname_port_weight },
  {"--socket=", false, (test_callback_fn*)test_parse_socket },
  {"--namespace=", false, (test_callback_fn*)test_namespace_keyword },
  {0, 0, (test_callback_fn*)0}
};

test_st virtual_bucket_tests[] ={
  {"basic", false, (test_callback_fn*)virtual_back_map },
  {0, 0, (test_callback_fn*)0}
};

test_st memcached_server_add_tests[] ={
  {"memcached_server_add(\"\")", false, (test_callback_fn*)memcached_server_add_empty_test },
  {"memcached_server_add(NULL)", false, (test_callback_fn*)memcached_server_add_null_test },
  {0, 0, (test_callback_fn*)0}
};

test_st namespace_tests[] ={
  {"basic tests", true, (test_callback_fn*)selection_of_namespace_tests },
  {"increment", true, (test_callback_fn*)memcached_increment_namespace },
  {0, 0, (test_callback_fn*)0}
};

collection_st collection[] ={
#if 0
  {"hash_sanity", 0, 0, hash_sanity},
#endif
  {"libmemcachedutil", 0, 0, libmemcachedutil_tests},
  {"basic", 0, 0, basic_tests},
  {"hsieh_availability", 0, 0, hsieh_availability},
  {"murmur_availability", 0, 0, murmur_availability},
  {"memcached_server_add", 0, 0, memcached_server_add_tests},
  {"block", 0, 0, tests},
  {"binary", (test_callback_fn*)pre_binary, 0, tests},
  {"nonblock", (test_callback_fn*)pre_nonblock, 0, tests},
  {"nodelay", (test_callback_fn*)pre_nodelay, 0, tests},
  {"settimer", (test_callback_fn*)pre_settimer, 0, tests},
  {"md5", (test_callback_fn*)pre_md5, 0, tests},
  {"crc", (test_callback_fn*)pre_crc, 0, tests},
  {"hsieh", (test_callback_fn*)pre_hsieh, 0, tests},
  {"jenkins", (test_callback_fn*)pre_jenkins, 0, tests},
  {"fnv1_64", (test_callback_fn*)pre_hash_fnv1_64, 0, tests},
  {"fnv1a_64", (test_callback_fn*)pre_hash_fnv1a_64, 0, tests},
  {"fnv1_32", (test_callback_fn*)pre_hash_fnv1_32, 0, tests},
  {"fnv1a_32", (test_callback_fn*)pre_hash_fnv1a_32, 0, tests},
  {"ketama", (test_callback_fn*)pre_behavior_ketama, 0, tests},
  {"ketama_auto_eject_hosts", (test_callback_fn*)pre_behavior_ketama, 0, ketama_auto_eject_hosts},
  {"unix_socket", (test_callback_fn*)pre_unix_socket, 0, tests},
  {"unix_socket_nodelay", (test_callback_fn*)pre_nodelay, 0, tests},
  {"gets", (test_callback_fn*)enable_cas, 0, tests},
  {"consistent_crc", (test_callback_fn*)enable_consistent_crc, 0, tests},
  {"consistent_hsieh", (test_callback_fn*)enable_consistent_hsieh, 0, tests},
#ifdef MEMCACHED_ENABLE_DEPRECATED
  {"deprecated_memory_allocators", (test_callback_fn*)deprecated_set_memory_alloc, 0, tests},
#endif
  {"memory_allocators", (test_callback_fn*)set_memory_alloc, 0, tests},
  {"namespace", (test_callback_fn*)set_namespace, 0, tests},
  {"namespace(BINARY)", (test_callback_fn*)set_namespace_and_binary, 0, tests},
  {"specific namespace", 0, 0, namespace_tests},
  {"specific namespace(BINARY)", (test_callback_fn*)pre_binary, 0, namespace_tests},
  {"version_1_2_3", (test_callback_fn*)check_for_1_2_3, 0, version_1_2_3},
  {"result", 0, 0, result_tests},
  {"async", (test_callback_fn*)pre_nonblock, 0, async_tests},
  {"async(BINARY)", (test_callback_fn*)pre_nonblock_binary, 0, async_tests},
  {"Cal Haldenbrand's tests", 0, 0, haldenbrand_tests},
  {"user written tests", 0, 0, user_tests},
  {"generate", 0, 0, generate_tests},
  {"generate_hsieh", (test_callback_fn*)pre_hsieh, 0, generate_tests},
  {"generate_ketama", (test_callback_fn*)pre_behavior_ketama, 0, generate_tests},
  {"generate_hsieh_consistent", (test_callback_fn*)enable_consistent_hsieh, 0, generate_tests},
  {"generate_md5", (test_callback_fn*)pre_md5, 0, generate_tests},
  {"generate_murmur", (test_callback_fn*)pre_murmur, 0, generate_tests},
  {"generate_jenkins", (test_callback_fn*)pre_jenkins, 0, generate_tests},
  {"generate_nonblock", (test_callback_fn*)pre_nonblock, 0, generate_tests},
  // Too slow
  {"generate_corked", (test_callback_fn*)pre_cork, 0, generate_tests},
  {"generate_corked_and_nonblock", (test_callback_fn*)pre_cork_and_nonblock, 0, generate_tests},
  {"consistent_not", 0, 0, consistent_tests},
  {"consistent_ketama", (test_callback_fn*)pre_behavior_ketama, 0, consistent_tests},
  {"consistent_ketama_weighted", (test_callback_fn*)pre_behavior_ketama_weighted, 0, consistent_weighted_tests},
  {"ketama_compat", 0, 0, ketama_compatibility},
  {"test_hashes", 0, 0, hash_tests},
  {"replication", (test_callback_fn*)pre_replication, 0, replication_tests},
  {"replication_noblock", (test_callback_fn*)pre_replication_noblock, 0, replication_tests},
  {"regression", 0, 0, regression_tests},
  {"behaviors", 0, 0, behavior_tests},
  {"regression_binary_vs_block", (test_callback_fn*)key_setup, (test_callback_fn*)key_teardown, regression_binary_vs_block},
  {"error_conditions", 0, 0, error_conditions},
  {"parser", 0, 0, parser_tests},
  {"virtual buckets", 0, 0, virtual_bucket_tests},
  {"memcached_server_get_last_disconnect", 0, 0, memcached_server_get_last_disconnect_tests},
  {"touch", 0, 0, touch_tests},
  {0, 0, 0, 0}
};

#define TEST_PORT_BASE MEMCACHED_DEFAULT_PORT +10

#include "tests/libmemcached_world.h"

void get_world(Framework *world)
{
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

  world->set_socket();
}
