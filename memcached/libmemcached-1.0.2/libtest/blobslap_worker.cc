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

#include <libtest/blobslap_worker.h>

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

#include <libgearman/gearman.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

namespace libtest {

class BlobslapWorker : public Server
{
private:
public:
  BlobslapWorker(in_port_t port_arg) :
    Server("localhost", port_arg)
  { 
    set_pid_file();
  }

  pid_t get_pid(bool error_is_ok)
  {
    if (pid_file().empty())
    {
      Error << "pid_file was empty";
      return -1;
    }

    Wait wait(pid_file(), 0);

    if (error_is_ok and not wait.successful())
    {
      Error << "Pidfile was not found:" << pid_file();
      return -1;
    }

    std::stringstream error_message;
    pid_t ret= get_pid_from_file(pid_file(), error_message);

    if (error_is_ok and is_pid_valid(ret) == false)
    {
      Error << error_message.str();
    }

    return ret;
  }

  bool ping()
  {
    if (pid_file().empty())
    {
      Error << "No pid file available";
      return false;
    }

    Wait wait(pid_file(), 0);
    if (not wait.successful())
    {
      Error << "Pidfile was not found:" << pid_file();
      return false;
    }

    std::stringstream error_message;
    pid_t local_pid= get_pid_from_file(pid_file(), error_message);
    if (is_pid_valid(local_pid) == false)
    {
      Error << error_message.str();
      return false;
    }

    // Use kill to determine is the process exist
    if (::kill(local_pid, 0) == 0)
    {
      return true;
    }

    return false;
  }

  const char *name()
  {
    return "blobslap_worker";
  };

  const char *executable()
  {
    return GEARMAND_BLOBSLAP_WORKER;
  }

  const char *pid_file_option()
  {
    return "--pid-file=";
  }

  const char *daemon_file_option()
  {
    return "--daemon";
  }

  const char *log_file_option()
  {
    return "--log-file=";
  }

  const char *port_option()
  {
    return "--port=";
  }

  bool is_libtool()
  {
    return true;
  }

  bool build(int argc, const char *argv[]);
};


#include <sstream>

bool BlobslapWorker::build(int argc, const char *argv[])
{
  std::stringstream arg_buffer;

  for (int x= 1 ; x < argc ; x++)
  {
    arg_buffer << " " << argv[x] << " ";
  }

  set_extra_args(arg_buffer.str());

  return true;
}

Server *build_blobslap_worker(in_port_t try_port)
{
  return new BlobslapWorker(try_port);
}

} // namespace libtest
