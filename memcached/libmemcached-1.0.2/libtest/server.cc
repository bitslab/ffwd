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

#include <libtest/server.h>
#include <libtest/stream.h>
#include <libtest/killpid.h>

extern "C" {
  static bool exited_successfully(int status)
  {
    if (WEXITSTATUS(status) == 0)
    {
      return true;
    }

    return true;
  }
}


namespace libtest {

std::ostream& operator<<(std::ostream& output, const Server &arg)
{
  if (arg.is_socket())
  {
    output << arg.hostname();
  }
  else
  {
    output << arg.hostname() << ":" << arg.port();
  }

  if (arg.has_pid())
  {
    output << " Pid:" <<  arg.pid();
  }

  if (arg.has_socket())
  {
    output << " Socket:" <<  arg.socket();
  }

  if (not arg.running().empty())
  {
    output << " Exec:" <<  arg.running();
  }


  return output;  // for multiple << operators
}

void Server::nap(void)
{
#ifdef WIN32
  sleep(1);
#else
  struct timespec global_sleep_value= { 0, 50000 };
  nanosleep(&global_sleep_value, NULL);
#endif
}

Server::Server(const std::string& host_arg, const in_port_t port_arg, bool is_socket_arg) :
  _is_socket(is_socket_arg),
  _pid(-1),
  _port(port_arg),
  _hostname(host_arg)
{
}

Server::~Server()
{
  if (has_pid() and not kill(_pid))
  {
    Error << "Unable to kill:" << *this;
  }
}

// If the server exists, kill it
bool Server::cycle()
{
  uint32_t limit= 3;

  // Try to ping, and kill the server #limit number of times
  pid_t current_pid;
  while (--limit and is_pid_valid(current_pid= get_pid()))
  {
    if (kill(current_pid))
    {
      Log << "Killed existing server," << *this << " with pid:" << current_pid;
      nap();
      continue;
    }
  }

  // For whatever reason we could not kill it, and we reached limit
  if (limit == 0)
  {
    Error << "Reached limit, could not kill server pid:" << current_pid;
    return false;
  }

  return true;
}

// Grab a one off command
bool Server::command(std::string& command_arg)
{
  rebuild_base_command();

  command_arg+= _base_command;

  if (args(command_arg))
  {
    return true;
  }

  return false;
}

bool Server::wait_for_pidfile() const
{
  Wait wait(pid_file(), 4);

  return wait.successful();
}

bool Server::start()
{
  // If we find that we already have a pid then kill it.
  if (has_pid() and not kill(_pid))
  {
    Error << "Could not kill() existing server during start() pid:" << _pid;
    return false;
  }
  assert(not has_pid());

  _running.clear();
  if (not command(_running))
  {
    Error << "Could not build command()";
    return false;
  }

  if (is_valgrind() or is_helgrind())
  {
    _running+= " &";
  }

  int ret= system(_running.c_str());
  if (not exited_successfully(ret))
  {
    Error << "system() failed:" << strerror(errno);
    _running.clear();
    return false;
  }

  if (is_helgrind() or is_valgrind())
  {
    sleep(4);
  }

  if (pid_file_option() and not pid_file().empty())
  {
    Wait wait(pid_file(), 8);

    if (not wait.successful())
    {
      Error << "Unable to open pidfile for: " << _running;
    }
  }

  int count= is_helgrind() or is_valgrind() ? 20 : 5;
  while (not ping() and --count)
  {
    nap();
  }

  if (count == 0)
  {
    // If we happen to have a pid file, lets try to kill it
    if (pid_file_option() and not pid_file().empty())
    {
      kill_file(pid_file());
    }
    Error << "Failed to ping() server started with:" << _running;
    _running.clear();
    return false;
  }

  // A failing get_pid() at this point is considered an error
  _pid= get_pid(true);

  return has_pid();
}

void Server::reset_pid()
{
  _running.clear();
  _pid_file.clear();
  _pid= -1;
}

pid_t Server::pid()
{
  return _pid;
}

bool Server::set_socket_file()
{
  char file_buffer[FILENAME_MAX];
  file_buffer[0]= 0;

  if (broken_pid_file())
  {
    snprintf(file_buffer, sizeof(file_buffer), "/tmp/%s.socketXXXXXX", name());
  }
  else
  {
    snprintf(file_buffer, sizeof(file_buffer), "var/run/%s.socketXXXXXX", name());
  }

  int fd;
  if ((fd= mkstemp(file_buffer)) == -1)
  {
    perror(file_buffer);
    return false;
  }
  close(fd);
  unlink(file_buffer);

  _socket= file_buffer;

  return true;
}

bool Server::set_pid_file()
{
  char file_buffer[FILENAME_MAX];
  file_buffer[0]= 0;

  if (broken_pid_file())
  {
    snprintf(file_buffer, sizeof(file_buffer), "/tmp/%s.pidXXXXXX", name());
  }
  else
  {
    snprintf(file_buffer, sizeof(file_buffer), "var/run/%s.pidXXXXXX", name());
  }

  int fd;
  if ((fd= mkstemp(file_buffer)) == -1)
  {
    perror(file_buffer);
    return false;
  }
  close(fd);
  unlink(file_buffer);

  _pid_file= file_buffer;

  return true;
}

bool Server::set_log_file()
{
  char file_buffer[FILENAME_MAX];
  file_buffer[0]= 0;

  snprintf(file_buffer, sizeof(file_buffer), "var/log/%s.logXXXXXX", name());
  int fd;
  if ((fd= mkstemp(file_buffer)) == -1)
  {
    perror(file_buffer);
    return false;
  }
  close(fd);

  _log_file= file_buffer;

  return true;
}

void Server::rebuild_base_command()
{
  _base_command.clear();
  if (is_libtool())
  {
    _base_command+= libtool();
  }

  if (is_debug() and getenv("GDB_COMMAND"))
  {
    _base_command+= getenv("GDB_COMMAND");
    _base_command+= " ";
  }
  else if (is_valgrind() and getenv("VALGRIND_COMMAND"))
  {
    _base_command+= getenv("VALGRIND_COMMAND");
    _base_command+= " ";
  }
  else if (is_helgrind() and getenv("HELGRIND_COMMAND"))
  {
    _base_command+= getenv("HELGRIND_COMMAND");
    _base_command+= " ";
  }

  _base_command+= executable();
}

void Server::set_extra_args(const std::string &arg)
{
  _extra_args= arg;
}

bool Server::args(std::string& options)
{
  std::stringstream arg_buffer;

  // Set a log file if it was requested (and we can)
  if (getenv("LIBTEST_LOG") and log_file_option())
  {
    if (not set_log_file())
    {
      return false;
    }

    arg_buffer << " " << log_file_option() << _log_file;
  }

  // Update pid_file
  if (pid_file_option())
  {
    if (_pid_file.empty() and not set_pid_file())
    {
      return false;
    }

    arg_buffer << " " << pid_file_option() << pid_file(); 
  }

  assert(daemon_file_option());
  if (daemon_file_option() and not is_valgrind() and not is_helgrind())
  {
    arg_buffer << " " << daemon_file_option();
  }

  if (_is_socket and socket_file_option())
  {
    if (not set_socket_file())
    {
      return false;
    }

    arg_buffer << " " << socket_file_option() << "\"" <<  _socket << "\"";
  }

  assert(port_option());
  if (port_option() and _port > 0)
  {
    arg_buffer << " " << port_option() << _port;
  }

  options+= arg_buffer.str();

  if (not _extra_args.empty())
  {
    options+= _extra_args;
  }

  return true;
}

bool Server::is_debug() const
{
  return bool(getenv("LIBTEST_MANUAL_GDB"));
}

bool Server::is_valgrind() const
{
  return bool(getenv("LIBTEST_MANUAL_VALGRIND"));
}

bool Server::is_helgrind() const
{
  return bool(getenv("LIBTEST_MANUAL_HELGRIND"));
}

bool Server::kill(pid_t pid_arg)
{
  if (check_pid(pid_arg) and kill_pid(pid_arg)) // If we kill it, reset
  {
    if (broken_pid_file() and not pid_file().empty())
    {
      unlink(pid_file().c_str());
    }

    if (broken_socket_cleanup() and has_socket() and not socket().empty())
    {
      unlink(socket().c_str());
    }

    reset_pid();

    return true;
  }

  return false;
}

} // namespace libtest
