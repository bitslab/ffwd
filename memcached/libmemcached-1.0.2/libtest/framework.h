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



#pragma once

/**
  Framework is the structure which is passed to the test implementation to be filled.
  This must be implemented in order for the test framework to load the tests. We call
  get_world() in order to fill this structure.
*/

class Framework {
public:
  collection_st *collections;

  /* These methods are called outside of any collection call. */
  test_callback_create_fn *_create;
  test_callback_destroy_fn *_destroy;

  /* This is called a the beginning of any collection run. */
  test_callback_fn *collection_startup;

  /* This is called a the end of any collection run. */
  test_callback_fn *collection_shutdown;

  void set_collection_shutdown(test_callback_error_fn *arg)
  {
    _on_error= arg;
  }

public:
  void* create(test_return_t& arg);

  test_return_t startup(void*);

  test_return_t shutdown(void* arg)
  {
    if (collection_shutdown)
    {
      return collection_shutdown(arg);
    }

    return TEST_SUCCESS;
  }

  /**
    These are run before/after the test. If implemented. Their execution is not controlled
    by the test.
  */
  class Item {
  public:
    /* This is called a the beginning of any run. */
    test_callback_fn *_startup;

    test_return_t startup(void*);

    /* 
      This called on a test if the test requires a flush call (the bool is
      from test_st) 
    */
    test_callback_fn *_flush;

  private:
    /*
       Run before and after the runnner is executed.
    */
    test_callback_fn *pre_run;
    test_callback_fn *post_run;

  public:

    Item() :
      _startup(NULL),
      _flush(NULL),
      pre_run(NULL),
      post_run(NULL)
    { }

    void set_startup(test_callback_fn *arg)
    {
      _startup= arg;
    }

    void set_collection(test_callback_fn *arg)
    {
      _flush= arg;
    }

    void set_flush(test_callback_fn *arg)
    {
      _flush= arg;
    }

    void set_pre(test_callback_fn *arg)
    {
      pre_run= arg;
    }

    void set_post(test_callback_fn *arg)
    {
      pre_run= arg;
    }

    test_return_t pre(void *arg);
    test_return_t flush(void* arg, test_st* run);
    test_return_t post(void *arg);

  } item;

  /**
    If an error occurs during the test, this is called.
  */
  test_callback_error_fn *_on_error;

  void set_on_error(test_callback_error_fn *arg)
  {
    _on_error= arg;
  }

  test_return_t on_error(const enum test_return_t, void *);

  void set_socket()
  {
    _servers.set_socket();
  }

  void set_sasl(const std::string& username_arg, const std::string& password_arg)
  {
    _servers.set_sasl(username_arg, password_arg);
  }

  libtest::server_startup_st& servers()
  {
    return _servers;
  }
  
  /**
    Runner represents the callers for the tests. If not implemented we will use
    a set of default implementations.
  */
  libtest::Runner *_runner;

  void set_runner(libtest::Runner *arg)
  {
    _runner= arg;
  }

  libtest::Runner *runner();


  Framework();

  virtual ~Framework();

  Framework(const Framework&);

private:
  Framework& operator=(const Framework&);
  libtest::server_startup_st _servers;
  bool _socket;
  void *_creators_ptr;
};
