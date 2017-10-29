/* libMemcached Functions Test
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Description: This is the startup bits for any libmemcached test.
 *
 */

#pragma once

#include <cassert>

/* The structure we use for the test system */
struct libmemcached_test_container_st
{
  server_startup_st& construct;
  memcached_st *parent;
  memcached_st *memc;

  libmemcached_test_container_st(server_startup_st &construct_arg) :
    construct(construct_arg),
    parent(NULL),
    memc(NULL)
  { }
};

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (HAVE_MEMCACHED_BINARY == 0)
  {
    error= TEST_FATAL;
    return NULL;
  }

  if (servers.sasl() and (LIBMEMCACHED_WITH_SASL_SUPPORT == 0 or MEMCACHED_SASL_BINARY == 0))
  {
    error= TEST_SKIPPED;
    return NULL;
  }

  // Assume we are running under valgrind, and bail
  if (servers.sasl() and getenv("TESTS_ENVIRONMENT"))
  {
    error= TEST_SKIPPED;
    return NULL;
  }


  in_port_t max_port= TEST_PORT_BASE;
  for (uint32_t x= 0; x < servers.count(); x++)
  {
    in_port_t port;

    char variable_buffer[1024];
    snprintf(variable_buffer, sizeof(variable_buffer), "LIBMEMCACHED_PORT_%u", x);

    char *var;
    if ((var= getenv(variable_buffer)))
    {
      port= in_port_t(atoi(var));
    }
    else
    {
      port= in_port_t(TEST_PORT_BASE +x);
    }

    max_port= port;
    const char *argv[1]= { "memcached" };
    if (servers.sasl())
    {
      if (not server_startup(servers, "memcached-sasl", port, 1, argv))
      {
        error= TEST_FATAL;
        return NULL;
      }
    }
    else
    {
      if (not server_startup(servers, "memcached", port, 1, argv))
      {
        error= TEST_FATAL;
        return NULL;
      }
    }
  }

  if (servers.socket())
  {
    if (servers.sasl())
    {
      const char *argv[1]= { "memcached" };
      if (not servers.start_socket_server("memcached-sasl", max_port +1, 1, argv))
      {
        error= TEST_FATAL;
        return NULL;
      }
    }
    else
    {
      const char *argv[1]= { "memcached" };
      if (not servers.start_socket_server("memcached", max_port +1, 1, argv))
      {
        error= TEST_FATAL;
        return NULL;
      }
    }
  }


  libmemcached_test_container_st *global_container= new libmemcached_test_container_st(servers);
  if (global_container == NULL)
  {
    error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  error= TEST_SUCCESS;

  return global_container;
}

static test_return_t world_container_startup(libmemcached_test_container_st *container)
{
  char buffer[BUFSIZ];

  test_compare_got(MEMCACHED_SUCCESS,
                   libmemcached_check_configuration(container->construct.option_string().c_str(), container->construct.option_string().size(),
                                                    buffer, sizeof(buffer)),
                   container->construct.option_string().c_str());

  test_null(container->parent);
  container->parent= memcached(container->construct.option_string().c_str(), container->construct.option_string().size());
  test_true(container->parent);
  test_compare(MEMCACHED_SUCCESS, memcached_version(container->parent));

  if (container->construct.sasl())
  {
    if (memcached_failed(memcached_behavior_set(container->parent, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1)))
    {
      memcached_free(container->parent);
      return TEST_FAILURE;
    }

    if (memcached_failed(memcached_set_sasl_auth_data(container->parent, container->construct.username().c_str(), container->construct.password().c_str())))
    {
      memcached_free(container->parent);
      return TEST_FAILURE;
    }
  }

  for (uint32_t host= 0; host < memcached_server_count(container->parent); ++host)
  {
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(container->parent, host);

    if (instance->type == MEMCACHED_CONNECTION_TCP)
    {
      test_true_got(memcached_server_port(instance) >= TEST_PORT_BASE, memcached_server_port(instance));
    }
  }

  return TEST_SUCCESS;
}

static test_return_t world_container_shutdown(libmemcached_test_container_st *container)
{
  memcached_free(container->parent);
  container->parent= NULL;

  return TEST_SUCCESS;
}

static test_return_t world_test_startup(libmemcached_test_container_st *container)
{
  test_true(container);
  test_null(container->memc);
  test_true(container->parent);
  container->memc= memcached_clone(NULL, container->parent);
  test_true(container->memc);

  return TEST_SUCCESS;
}

test_return_t world_flush(libmemcached_test_container_st *container);
test_return_t world_flush(libmemcached_test_container_st *container)
{
  test_true(container->memc);
  memcached_flush(container->memc, 0);
  memcached_quit(container->memc);

  return TEST_SUCCESS;
}

static test_return_t world_pre_run(libmemcached_test_container_st *container)
{
  test_true(container->memc);
  for (uint32_t loop= 0; loop < memcached_server_list_count(container->memc->servers); loop++)
  {
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(container->memc, loop);

    test_compare(-1, instance->fd);
    test_compare(0U, instance->cursor_active);
  }

  return TEST_SUCCESS;
}


static test_return_t world_post_run(libmemcached_test_container_st *container)
{
  test_true(container->memc);

  return TEST_SUCCESS;
}

static test_return_t world_on_error(test_return_t , libmemcached_test_container_st *container)
{
  test_true(container->memc);
  memcached_free(container->memc);
  container->memc= NULL;

  return TEST_SUCCESS;
}

static bool world_destroy(void *object)
{
  libmemcached_test_container_st *container= (libmemcached_test_container_st *)object;
#if defined(LIBMEMCACHED_WITH_SASL_SUPPORT) && LIBMEMCACHED_WITH_SASL_SUPPORT
  if (LIBMEMCACHED_WITH_SASL_SUPPORT)
  {
    sasl_done();
  }
#endif

  delete container;

  return TEST_SUCCESS;
}

typedef test_return_t (*libmemcached_test_callback_fn)(memcached_st *);

static test_return_t _runner_default(libmemcached_test_callback_fn func, libmemcached_test_container_st *container)
{
  if (func)
  {
    test_true(container);
    test_true(container->memc);
    return func(container->memc);
  }

  return TEST_SUCCESS;
}

static test_return_t _pre_runner_default(libmemcached_test_callback_fn func, libmemcached_test_container_st *container)
{
  if (func)
  {
    return func(container->parent);
  }

  return TEST_SUCCESS;
}

static test_return_t _post_runner_default(libmemcached_test_callback_fn func, libmemcached_test_container_st *container)
{
  if (func)
  {
    return func(container->parent);
  }

  return TEST_SUCCESS;
}

class LibmemcachedRunner : public Runner {
public:
  test_return_t run(test_callback_fn* func, void *object)
  {
    return _runner_default(libmemcached_test_callback_fn(func), (libmemcached_test_container_st*)object);
  }

  test_return_t pre(test_callback_fn* func, void *object)
  {
    return _pre_runner_default(libmemcached_test_callback_fn(func), (libmemcached_test_container_st*)object);
  }

  test_return_t post(test_callback_fn* func, void *object)
  {
    return _post_runner_default(libmemcached_test_callback_fn(func), (libmemcached_test_container_st*)object);
  }
};

static LibmemcachedRunner defualt_libmemcached_runner;
