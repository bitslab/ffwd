/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Test memstat
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

using namespace libtest;

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

static std::string executable;

static test_return_t quiet_test(void *)
{
  const char *args[]= { "--quiet", 0 };

  test_true(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}


static test_return_t help_test(void *)
{
  const char *args[]= { "--help", "--quiet", 0 };

  test_true(exec_cmdline(executable, args));

  return TEST_SUCCESS;
}

static test_return_t ascii_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "-p %d", int(default_port()));
  const char *args[]= { "--quiet", buffer, " -a ", 0 };

  test_true(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t binary_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "-p %d", int(default_port()));
  const char *args[]= { "--quiet", buffer, " -b ", 0 };

  test_true(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

test_st memstat_tests[] ={
  {"--quiet", 0, quiet_test},
  {"--help", 0, help_test},
  {"-a, ascii", 0, ascii_test},
  {"-b, binary", 0, binary_test},
  {0, 0, 0}
};

collection_st collection[] ={
  {"memstat", 0, 0, memstat_tests },
  {0, 0, 0, 0}
};

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (HAVE_MEMCACHED_BINARY == 0)
  {
    error= TEST_FATAL;
    return NULL;
  }

  const char *argv[1]= { "memstat" };
  if (not server_startup(servers, "memcached", MEMCACHED_DEFAULT_PORT +10, 1, argv))
  {
    error= TEST_FAILURE;
  }

  return &servers;
}


void get_world(Framework *world)
{
  executable= "./clients/memstat";
  world->collections= collection;
  world->_create= world_create;
}

