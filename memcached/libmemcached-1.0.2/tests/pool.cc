/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached Client and Server 
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  All rights reserved.
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

using namespace libtest;

#include <vector>
#include <iostream>
#include <string>
#include <cerrno>

#include <semaphore.h>

#include <libmemcached/memcached.h>
#include <libmemcached/util.h>
#include <tests/pool.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif


test_return_t memcached_pool_test(memcached_st *)
{
  memcached_return_t rc;
  const char *config_string= "--SERVER=host10.example.com --SERVER=host11.example.com --SERVER=host10.example.com --POOL-MIN=10 --POOL-MAX=32";

  char buffer[2048];
  rc= libmemcached_check_configuration(config_string, sizeof(config_string) -1, buffer, sizeof(buffer));

  test_true_got(rc != MEMCACHED_SUCCESS, buffer);

  memcached_pool_st* pool= memcached_pool(config_string, strlen(config_string));
  test_true_got(pool, strerror(errno));

  memcached_st *memc= memcached_pool_pop(pool, false, &rc);

  test_true(rc == MEMCACHED_SUCCESS);
  test_true(memc);

  /*
    Release the memc_ptr that was pulled from the pool
  */
  memcached_pool_push(pool, memc);

  /*
    Destroy the pool.
  */
  memcached_pool_destroy(pool);

  return TEST_SUCCESS;
}


#define POOL_SIZE 10
test_return_t connection_pool_test(memcached_st *memc)
{
  memcached_pool_st* pool= memcached_pool_create(memc, 5, POOL_SIZE);
  test_true(pool);
  memcached_st *mmc[POOL_SIZE];

  // Fill up our array that we will store the memc that are in the pool
  for (size_t x= 0; x < POOL_SIZE; ++x)
  {
    memcached_return_t rc;
    mmc[x]= memcached_pool_fetch(pool, NULL, &rc);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(mmc[x]);
  }

  // All memc should be gone
  {
    memcached_return_t rc;
    test_null(memcached_pool_fetch(pool, NULL, &rc));
    test_compare(MEMCACHED_NOTFOUND, rc);
  }

  // Release them..
  for (size_t x= 0; x < POOL_SIZE; ++x)
  {
    if (mmc[x])
    {
      test_compare(MEMCACHED_SUCCESS, memcached_pool_release(pool, mmc[x]));
    }
  }
  test_true(memcached_pool_destroy(pool) == memc);

  return TEST_SUCCESS;
}

test_return_t connection_pool2_test(memcached_st *memc)
{
  memcached_pool_st* pool= memcached_pool_create(memc, 5, POOL_SIZE);
  test_true(pool);
  memcached_st *mmc[POOL_SIZE];

  // Fill up our array that we will store the memc that are in the pool
  for (size_t x= 0; x < POOL_SIZE; ++x)
  {
    memcached_return_t rc;
    mmc[x]= memcached_pool_fetch(pool, NULL, &rc);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(mmc[x]);
  }

  // All memc should be gone
  {
    memcached_return_t rc;
    test_null(memcached_pool_fetch(pool, NULL, &rc));
    test_compare(MEMCACHED_NOTFOUND, rc);
  }

  // verify that I can do ops with all connections
  test_compare(MEMCACHED_SUCCESS,
               memcached_set(mmc[0],
                             test_literal_param("key"),
                             "0", 1, 0, 0));

  for (uint64_t x= 0; x < POOL_SIZE; ++x)
  {
    uint64_t number_value;
    test_compare(MEMCACHED_SUCCESS,
                 memcached_increment(mmc[x], 
                                     test_literal_param("key"),
                                     1, &number_value));
    test_compare(number_value, (x+1));
  }

  // Release them..
  for (size_t x= 0; x < POOL_SIZE; ++x)
  {
    test_compare(MEMCACHED_SUCCESS, memcached_pool_release(pool, mmc[x]));
  }


  /* verify that I can set behaviors on the pool when I don't have all
   * of the connections in the pool. It should however be enabled
   * when I push the item into the pool
 */
  mmc[0]= memcached_pool_fetch(pool, NULL, NULL);
  test_true(mmc[0]);

  test_compare(MEMCACHED_SUCCESS,
               memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK, 9999));

  {
    memcached_return_t rc;
    mmc[1]= memcached_pool_fetch(pool, NULL, &rc);
    test_true(mmc[1]);
    test_compare(MEMCACHED_SUCCESS, rc);
  }

  test_compare(UINT64_C(9999), memcached_behavior_get(mmc[1], MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK));
  test_compare(MEMCACHED_SUCCESS, memcached_pool_release(pool, mmc[1]));
  test_compare(MEMCACHED_SUCCESS, memcached_pool_release(pool, mmc[0]));

  {
    memcached_return_t rc;
    mmc[0]= memcached_pool_fetch(pool, NULL, &rc);
    test_true(mmc[0]);
    test_compare(MEMCACHED_SUCCESS, rc);
  }

  test_compare(UINT64_C(9999), memcached_behavior_get(mmc[0], MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK));
  test_compare(MEMCACHED_SUCCESS, memcached_pool_release(pool, mmc[0]));

  test_true(memcached_pool_destroy(pool) == memc);

  return TEST_SUCCESS;
}

struct test_pool_context_st {
  volatile memcached_return_t rc;
  memcached_pool_st* pool;
  memcached_st* mmc;
  sem_t _lock;

  test_pool_context_st(memcached_pool_st *pool_arg, memcached_st *memc_arg):
    rc(MEMCACHED_FAILURE),
    pool(pool_arg),
    mmc(memc_arg)
  {
    sem_init(&_lock, 0, 0);
  }

  void wait()
  {
    sem_wait(&_lock);
  }

  void release()
  {
    sem_post(&_lock);
  }

  ~test_pool_context_st()
  {
    sem_destroy(&_lock);
  }
};

static void* connection_release(void *arg)
{
  test_pool_context_st *resource= static_cast<test_pool_context_st *>(arg);
  assert(resource);
  if (resource == NULL)
  {
    abort();
  }

  // Release all of the memc we are holding 
  resource->rc= memcached_pool_release(resource->pool, resource->mmc);
  resource->release();

  pthread_exit(arg);
}

test_return_t connection_pool3_test(memcached_st *memc)
{
#ifdef __APPLE__
  return TEST_SKIPPED;
#endif

  memcached_pool_st* pool= memcached_pool_create(memc, 1, 1);
  test_true(pool);

  memcached_st *pool_memc;
  {
    memcached_return_t rc;
    pool_memc= memcached_pool_fetch(pool, NULL, &rc);
    test_compare(MEMCACHED_SUCCESS, rc);
    test_true(pool_memc);
  }

  /*
    @note This comment was written to describe what was believed to be the original authors intent.

    This portion of the test creates a thread that will wait until told to free a memcached_st
    that will be grabbed by the main thread.

    It is believed that this tests whether or not we are handling ownership correctly.
  */
  pthread_t tid;
  test_pool_context_st item(pool, pool_memc);

  test_zero(pthread_create(&tid, NULL, connection_release, &item));
  item.wait();

  memcached_return_t rc;
  memcached_st *pop_memc;
  // We do a hard loop, and try N times
  int counter= 5;
  do
  {
    struct timespec relative_time= { 0, 0 };
    pop_memc= memcached_pool_fetch(pool, &relative_time, &rc);

    if (memcached_success(rc))
    {
      break;
    }

    if (memcached_failed(rc))
    {
      test_null(pop_memc);
      test_true(rc != MEMCACHED_TIMEOUT); // As long as relative_time is zero, MEMCACHED_TIMEOUT is invalid
    }
  } while (--counter);

  if (memcached_failed(rc)) // Cleanup thread since we will exit once we test.
  {
    pthread_join(tid, NULL);
    test_compare(MEMCACHED_SUCCESS, rc);
  }

  {
    int pthread_ret= pthread_join(tid, NULL);
    test_true(pthread_ret == 0 or pthread_ret == ESRCH);
  }
  test_compare(MEMCACHED_SUCCESS, rc);
  test_true(pool_memc == pop_memc);

  test_true(memcached_pool_destroy(pool) == memc);

  return TEST_SUCCESS;
}
