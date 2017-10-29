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

class server_startup_st
{
private:
  std::string server_list;
  bool _socket;
  bool _sasl;
  uint32_t _count;
  std::string _username;
  std::string _password;

public:

  uint8_t udp;
  std::vector<Server *> servers;

  server_startup_st() :
    _socket(false),
    _sasl(false),
    _count(5),
    udp(0)
  { }

  bool start_socket_server(const std::string& server_type, const in_port_t try_port, int argc, const char *argv[]);

  uint32_t count() const
  {
    return _count;
  }

  void set_count(uint32_t arg)
  {
    _count= arg;
  }

  void restart();

  std::string option_string() const;

  const std::string& password() const
  {
    return _password;
  }

  const std::string& username() const
  {
    return _username;
  }


  bool is_debug() const;
  bool is_helgrind() const;
  bool is_valgrind() const;

  bool socket()
  {
    return _socket;
  }

  bool sasl()
  {
    return _sasl;
  }

  void set_socket()
  {
    _socket= true;
  }

  void set_sasl(const std::string& username_arg, const std::string& password_arg)
  {
    _sasl= true;
    _username= username_arg;
    _password= password_arg;
  }


  void shutdown_and_remove();
  void shutdown();
  bool shutdown(uint32_t number_of_host);

  void push_server(Server *);
  Server *pop_server();

  ~server_startup_st();
};

bool server_startup(server_startup_st&, const std::string&, in_port_t try_port, int argc, const char *argv[]);

} // namespace libtest
