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

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <iostream>

#include <algorithm> 
#include <functional> 
#include <locale>

// trim from end 
static inline std::string &rtrim(std::string &s)
{ 
  s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end()); 
  return s; 
}

namespace libtest {

void server_startup_st::push_server(Server *arg)
{
  servers.push_back(arg);

  char port_str[NI_MAXSERV];
  snprintf(port_str, sizeof(port_str), "%u", int(arg->port()));

  std::string server_config_string;
  if (arg->has_socket())
  {
    server_config_string+= "--socket=";
    server_config_string+= '"';
    server_config_string+= arg->socket();
    server_config_string+= '"';
    server_config_string+= " ";
  }
  else
  {
    server_config_string+= "--server=";
    server_config_string+= arg->hostname();
    server_config_string+= ":";
    server_config_string+= port_str;
    server_config_string+= " ";
  }

  server_list+= server_config_string;

}

Server* server_startup_st::pop_server()
{
  Server *tmp= servers.back();
  servers.pop_back();
  return tmp;
}

bool server_startup_st::shutdown(uint32_t number_of_host)
{
  assert(servers.size() > number_of_host);
  if (servers.size() > number_of_host)
  {
    Server* tmp= servers[number_of_host];

    if (tmp and tmp->has_pid() and not tmp->kill(tmp->pid()))
    { }
    else
    {
      return true;
    }
  }

  return false;
}

void server_startup_st::shutdown_and_remove()
{
  for (std::vector<Server *>::iterator iter= servers.begin(); iter != servers.end(); iter++)
  {
    delete *iter;
  }
  servers.clear();
}

void server_startup_st::shutdown()
{
  for (std::vector<Server *>::iterator iter= servers.begin(); iter != servers.end(); iter++)
  {
    if ((*iter)->has_pid() and not (*iter)->kill((*iter)->pid()))
    {
      Error << "Unable to kill:" <<  *(*iter);
    }
  }
}

void server_startup_st::restart()
{
  for (std::vector<Server *>::iterator iter= servers.begin(); iter != servers.end(); iter++)
  {
    (*iter)->start();
  }
}

server_startup_st::~server_startup_st()
{
  shutdown_and_remove();
}

bool server_startup_st::is_debug() const
{
  return bool(getenv("LIBTEST_MANUAL_GDB"));
}

bool server_startup_st::is_valgrind() const
{
  return bool(getenv("LIBTEST_MANUAL_VALGRIND"));
}

bool server_startup_st::is_helgrind() const
{
  return bool(getenv("LIBTEST_MANUAL_HELGRIND"));
}


bool server_startup(server_startup_st& construct, const std::string& server_type, in_port_t try_port, int argc, const char *argv[])
{
  Outn();
  (void)try_port;

  set_max_port(try_port);

  // Look to see if we are being provided ports to use
  {
    char variable_buffer[1024];
    snprintf(variable_buffer, sizeof(variable_buffer), "LIBTEST_PORT_%lu", (unsigned long)construct.count());

    char *var;
    if ((var= getenv(variable_buffer)))
    {
      in_port_t tmp= in_port_t(atoi(var));

      if (tmp > 0)
        try_port= tmp;
    }
  }

  libtest::Server *server= NULL;
  if (0)
  { }
  else if (server_type.compare("gearmand") == 0)
  {
    if (GEARMAND_BINARY)
    {
      if (HAVE_LIBGEARMAN)
      {
        server= build_gearmand("localhost", try_port);
      }
      else
      {
        Error << "Libgearman was not found";
      }
    } 
    else
    {
      Error << "No gearmand binary is available";
    }
  }
  else if (server_type.compare("blobslap_worker") == 0)
  {
    if (GEARMAND_BINARY and GEARMAND_BLOBSLAP_WORKER)
    {
      if (HAVE_LIBGEARMAN)
      {
        server= build_blobslap_worker(try_port);
      }
      else
      {
        Error << "Libgearman was not found";
      }
    }
    else
    {
      Error << "No gearmand binary is available";
    }
  }
  else if (server_type.compare("memcached-sasl") == 0)
  {
    if (MEMCACHED_SASL_BINARY)
    {
      if (HAVE_LIBMEMCACHED)
      {
        server= build_memcached_sasl("localhost", try_port, construct.username(), construct.password());
      }
      else
      {
        Error << "Libmemcached was not found";
      }
    }
    else
    {
      Error << "No memcached binary that was compiled with sasl is available";
    }
  }
  else if (server_type.compare("memcached") == 0)
  {
    if (MEMCACHED_BINARY)
    {
      if (HAVE_LIBMEMCACHED)
      {
        server= build_memcached("localhost", try_port);
      }
      else
      {
        Error << "Libmemcached was not found";
      }
    }
    else
    {
      Error << "No memcached binary is available";
    }
  }
  else
  {
    Error << "Failed to start " << server_type << ", no support was found to be compiled in for it.";
  }

  if (server == NULL)
  {
    Error << "Failure occured while creating server: " <<  server_type;
    return false;
  }

  /*
    We will now cycle the server we have created.
  */
  if (not server->cycle())
  {
    Error << "Could not start up server " << *server;
    delete server;
    return false;
  }

  server->build(argc, argv);

  if (construct.is_debug())
  {
    Out << "Pausing for startup, hit return when ready.";
    std::string gdb_command= server->base_command();
    std::string options;
    Out << "run " << server->args(options);
    getchar();
  }
  else if (not server->start())
  {
    Error << "Failed to start " << *server;
    delete server;
    return false;
  }
  else
  {
    Out << "STARTING SERVER(pid:" << server->pid() << "): " << server->running();
  }

  construct.push_server(server);

  if (default_port() == 0)
  {
    assert(server->has_port());
    set_default_port(server->port());
  }

  Outn();

  return true;
}

bool server_startup_st::start_socket_server(const std::string& server_type, const in_port_t try_port, int argc, const char *argv[])
{
  (void)try_port;
  Outn();

  Server *server= NULL;
  if (0)
  { }
  else if (server_type.compare("gearmand") == 0)
  {
    Error << "Socket files are not supported for gearmand yet";
  }
  else if (server_type.compare("memcached-sasl") == 0)
  {
    if (MEMCACHED_SASL_BINARY)
    {
      if (HAVE_LIBMEMCACHED)
      {
        server= build_memcached_sasl_socket("localhost", try_port, username(), password());
      }
      else
      {
        Error << "Libmemcached was not found";
      }
    }
    else
    {
      Error << "No memcached binary is available";
    }
  }
  else if (server_type.compare("memcached") == 0)
  {
    if (MEMCACHED_BINARY)
    {
      if (HAVE_LIBMEMCACHED)
      {
        server= build_memcached_socket("localhost", try_port);
      }
      else
      {
        Error << "Libmemcached was not found";
      }
    }
    else
    {
      Error << "No memcached binary is available";
    }
  }
  else
  {
    Error << "Failed to start " << server_type << ", no support was found to be compiled in for it.";
  }

  if (server == NULL)
  {
    Error << "Failure occured while creating server: " <<  server_type;
    return false;
  }

  /*
    We will now cycle the server we have created.
  */
  if (not server->cycle())
  {
    Error << "Could not start up server " << *server;
    delete server;
    return false;
  }

  server->build(argc, argv);

  if (is_debug())
  {
    Out << "Pausing for startup, hit return when ready.";
    std::string gdb_command= server->base_command();
    std::string options;
    Out << "run " << server->args(options);
    getchar();
  }
  else if (not server->start())
  {
    Error << "Failed to start " << *server;
    delete server;
    return false;
  }
  else
  {
    Out << "STARTING SERVER(pid:" << server->pid() << "): " << server->running();
  }

  push_server(server);

  set_default_socket(server->socket().c_str());

  Outn();

  return true;
}

std::string server_startup_st::option_string() const
{
  std::string temp= server_list;
  rtrim(temp);
  return temp;
}


} // namespace libtest
