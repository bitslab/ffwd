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

#include <cassert>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace libtest {

struct Server {
private:
  bool _is_socket;
  std::string _socket;
  std::string _sasl;
  std::string _pid_file;
  std::string _log_file;
  std::string _base_command; // executable command which include libtool, valgrind, gdb, etc
  std::string _running; // Current string being used for system()
  pid_t _pid;

protected:
  in_port_t _port;
  std::string _hostname;
  std::string _extra_args;

public:
  Server(const std::string& hostname, const in_port_t port_arg, const bool is_socket_arg= false);

  virtual ~Server();

  virtual const char *name()= 0;
  virtual const char *executable()= 0;
  virtual const char *port_option()= 0;
  virtual const char *pid_file_option()= 0;
  virtual const char *daemon_file_option()= 0;
  virtual const char *log_file_option()= 0;
  virtual bool is_libtool()= 0;

  virtual bool broken_socket_cleanup()
  {
    return false;
  }

  virtual const char *socket_file_option() const
  {
    return NULL;
  }

  virtual bool broken_pid_file()
  {
    return false;
  }

  const std::string& pid_file() const
  {
    return _pid_file;
  }

  const std::string& base_command() const
  {
    return _base_command;
  }

  const std::string& log_file() const
  {
    return _log_file;
  }

  const std::string& hostname() const
  {
    return _hostname;
  }

  const std::string& socket() const
  {
    return _socket;
  }

  bool has_socket() const
  {
    return _is_socket;
  }

  bool cycle();

  virtual bool ping()= 0;

  virtual pid_t get_pid(bool error_is_ok= false)= 0;

  virtual bool build(int argc, const char *argv[])= 0;

  in_port_t port() const
  {
    return _port;
  }

  bool has_port() const
  {
    return (_port != 0);
  }

  // Reset a server if another process has killed the server
  void reset()
  {
    _pid= -1;
    _pid_file.clear();
    _log_file.clear();
  }

  void set_extra_args(const std::string &arg);

  bool args(std::string& options);

  pid_t pid();

  pid_t pid() const
  {
    return _pid;
  }

  bool has_pid() const
  {
    return (_pid > 1);
  }

  bool wait_for_pidfile() const;

  bool check_pid(pid_t pid_arg) const
  {
    return (pid_arg > 1);
  }

  bool is_socket() const
  {
    return _hostname[0] == '/';
  }

  const std::string running() const
  {
    return _running;
  }

  std::string log_and_pid();

  bool kill(pid_t pid_arg);
  bool start();
  bool command(std::string& command_arg);

protected:
  void nap();
  bool set_pid_file();

private:
  bool is_helgrind() const;
  bool is_valgrind() const;
  bool is_debug() const;
  bool set_log_file();
  bool set_socket_file();
  void rebuild_base_command();
  void reset_pid();
};

std::ostream& operator<<(std::ostream& output, const libtest::Server &arg);

} // namespace libtest


