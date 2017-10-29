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

#include <libmemcached/memcached.h>
#include <libmemcached/util.h>

using namespace libtest;

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libtest/server.h>
#include <libtest/wait.h>

#include <libtest/memcached.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

using namespace libtest;

class Memcached : public libtest::Server
{
  std::string _username;
  std::string _password;

public:
  Memcached(const std::string& host_arg, const in_port_t port_arg, const bool is_socket_arg, const std::string& username_arg, const std::string& password_arg) :
    libtest::Server(host_arg, port_arg, is_socket_arg),
    _username(username_arg),
    _password(password_arg)
  { }

  Memcached(const std::string& host_arg, const in_port_t port_arg, const bool is_socket_arg) :
    libtest::Server(host_arg, port_arg, is_socket_arg)
  {
    set_pid_file();
  }

  virtual const char *sasl() const
  {
    return NULL;
  }

  const std::string& password() const
  {
    return _password;
  }

  const std::string& username() const
  {
    return _username;
  }

  pid_t get_pid(bool error_is_ok)
  {
    // Memcached is slow to start, so we need to do this
    if (not pid_file().empty())
    {
      if (error_is_ok and not wait_for_pidfile())
      {
        Error << "Pidfile was not found:" << pid_file();
        return -1;
      }
    }

    pid_t local_pid;
    memcached_return_t rc= MEMCACHED_SUCCESS;
    if (has_socket())
    {
      local_pid= libmemcached_util_getpid(socket().c_str(), 0, &rc);
    }
    else
    {
      local_pid= libmemcached_util_getpid(hostname().c_str(), port(), &rc);
    }

    if (error_is_ok and ((memcached_failed(rc) or not is_pid_valid(local_pid))))
    {
      Error << "libmemcached_util_getpid(" << memcached_strerror(NULL, rc) << ") pid: " << local_pid << " for:" << *this;
    }

    return local_pid;
  }

  bool ping()
  {
    // Memcached is slow to start, so we need to do this
    if (not pid_file().empty())
    {
      if (not wait_for_pidfile())
      {
        Error << "Pidfile was not found:" << pid_file();
        return -1;
      }
    }

    memcached_return_t rc;
    bool ret;

    if (has_socket())
    {
        ret= libmemcached_util_ping(socket().c_str(), 0, &rc);
    }
    else
    {
      ret= libmemcached_util_ping(hostname().c_str(), port(), &rc);
    }

    if (memcached_failed(rc) or not ret)
    {
      Error << "libmemcached_util_ping(" << hostname() << ", " << port() << ") error: " << memcached_strerror(NULL, rc);
    }

    return ret;
  }

  const char *name()
  {
    return "memcached";
  };

  const char *executable()
  {
    return MEMCACHED_BINARY;
  }

  const char *pid_file_option()
  {
    return "-P ";
  }

  const char *socket_file_option() const
  {
    return "-s ";
  }

  const char *daemon_file_option()
  {
    return "-d";
  }

  const char *log_file_option()
  {
    return NULL;
  }

  const char *port_option()
  {
    return "-p ";
  }

  bool is_libtool()
  {
    return false;
  }

  bool broken_socket_cleanup()
  {
    return true;
  }

  // Memcached's pidfile is broken
  bool broken_pid_file()
  {
    return true;
  }

  bool build(int argc, const char *argv[]);
};

class MemcachedSaSL : public Memcached
{
public:
  MemcachedSaSL(const std::string& host_arg, const in_port_t port_arg, const bool is_socket_arg, const std::string& username_arg, const std::string &password_arg) :
    Memcached(host_arg, port_arg, is_socket_arg, username_arg, password_arg)
  { }

  const char *name()
  {
    return "memcached-sasl";
  };

  const char *sasl() const
  {
    return " -S -B binary ";
  }

  const char *executable()
  {
    return MEMCACHED_SASL_BINARY;
  }

  pid_t get_pid(bool error_is_ok)
  {
    // Memcached is slow to start, so we need to do this
    if (not pid_file().empty())
    {
      if (error_is_ok and not wait_for_pidfile())
      {
        Error << "Pidfile was not found:" << pid_file();
        return -1;
      }
    }

    pid_t local_pid;
    memcached_return_t rc;
    if (has_socket())
    {
      local_pid= libmemcached_util_getpid2(socket().c_str(), 0, username().c_str(), password().c_str(), &rc);
    }
    else
    {
      local_pid= libmemcached_util_getpid2(hostname().c_str(), port(), username().c_str(), password().c_str(), &rc);
    }

    if (error_is_ok and ((memcached_failed(rc) or not is_pid_valid(local_pid))))
    {
      Error << "libmemcached_util_getpid2(" << memcached_strerror(NULL, rc) << ") username: " << username() << " password: " << password() << " pid: " << local_pid << " for:" << *this;
    }

    return local_pid;
  }

  bool ping()
  {
    // Memcached is slow to start, so we need to do this
    if (not pid_file().empty())
    {
      if (not wait_for_pidfile())
      {
        Error << "Pidfile was not found:" << pid_file();
        return -1;
      }
    }

    memcached_return_t rc;
    bool ret;

    if (has_socket())
    {
      ret= libmemcached_util_ping2(socket().c_str(), 0, username().c_str(), password().c_str(), &rc);
    }
    else
    {
      ret= libmemcached_util_ping2(hostname().c_str(), port(), username().c_str(), password().c_str(), &rc);
    }

    if (memcached_failed(rc) or not ret)
    {
      Error << "libmemcached_util_ping2(" << hostname() << ", " << port() << ", " << username() << ", " << password() << ") error: " << memcached_strerror(NULL, rc);
    }

    return ret;
  }

};


#include <sstream>

bool Memcached::build(int argc, const char *argv[])
{
  std::stringstream arg_buffer;

  if (getuid() == 0 or geteuid() == 0)
  {
    arg_buffer << " -u root ";
  }

  arg_buffer << " -l 127.0.0.1 ";
  arg_buffer << " -m 128 ";
  arg_buffer << " -M ";

  if (sasl())
  {
    arg_buffer << sasl();
  }

  for (int x= 1 ; x < argc ; x++)
  {
    arg_buffer << " " << argv[x] << " ";
  }

  set_extra_args(arg_buffer.str());

  return true;
}

namespace libtest {

libtest::Server *build_memcached(const std::string& hostname, const in_port_t try_port)
{
  return new Memcached(hostname, try_port, false);
}

libtest::Server *build_memcached_socket(const std::string& socket_file, const in_port_t try_port)
{
  return new Memcached(socket_file, try_port, true);
}


libtest::Server *build_memcached_sasl(const std::string& hostname, const in_port_t try_port, const std::string& username, const std::string &password)
{
  if (username.empty())
  {
    return new MemcachedSaSL(hostname, try_port, false,  "memcached", "memcached");
  }

  return new MemcachedSaSL(hostname, try_port, false,  username, password);
}

libtest::Server *build_memcached_sasl_socket(const std::string& socket_file, const in_port_t try_port, const std::string& username, const std::string &password)
{
  if (username.empty())
  {
    return new MemcachedSaSL(socket_file, try_port, true, "memcached", "memcached");
  }

  return new MemcachedSaSL(socket_file, try_port, true, username, password);
}

}

