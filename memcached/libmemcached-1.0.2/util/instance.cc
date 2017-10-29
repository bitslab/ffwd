/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  DataDifferential Utility Library
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

#include "util/instance.hpp"

#include <cstdio>
#include <sstream>
#include <iostream>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


namespace datadifferential {
namespace util {

Instance::Instance(const std::string& hostname_arg, const std::string& service_arg) :
  _host(hostname_arg),
  _service(service_arg),
  _sockfd(INVALID_SOCKET),
  state(NOT_WRITING),
  _addrinfo(0),
  _addrinfo_next(0),
  _finish_fn(NULL),
  _operations()
  {
  }

Instance::Instance(const std::string& hostname_arg, const in_port_t port_arg) :
  _host(hostname_arg),
  _sockfd(INVALID_SOCKET),
  state(NOT_WRITING),
  _addrinfo(0),
  _addrinfo_next(0),
  _finish_fn(NULL),
  _operations()
  {
    char tmp[BUFSIZ];
    snprintf(tmp, sizeof(tmp), "%u", static_cast<unsigned int>(port_arg));
    _service= tmp;
  }

Instance::~Instance()
{
  close_socket();
  free_addrinfo();
  for (Operation::vector::iterator iter= _operations.begin(); iter != _operations.end(); ++iter)
  {
    delete *iter;
  }
  _operations.clear();

  delete _finish_fn;
}

bool Instance::run()
{
  while (not _operations.empty())
  {
    Operation::vector::value_type operation= _operations.back();

    switch (state)
    {
    case NOT_WRITING:
      {
        free_addrinfo();

        struct addrinfo ai;
        memset(&ai, 0, sizeof(struct addrinfo));
        ai.ai_socktype= SOCK_STREAM;
        ai.ai_protocol= IPPROTO_TCP;

        int ret= getaddrinfo(_host.c_str(), _service.c_str(), &ai, &_addrinfo);
        if (ret)
        {
          std::stringstream message;
          message << "Failed to connect on " << _host.c_str() << ":" << _service.c_str() << " with "  << gai_strerror(ret);
          _last_error= message.str();
          return false;
        }
      }
      _addrinfo_next= _addrinfo;
      state= CONNECT;
      break;

    case NEXT_CONNECT_ADDRINFO:
      if (_addrinfo_next->ai_next == NULL)
      {
        std::stringstream message;
        message << "Error connecting to " << _host.c_str() << "." << std::endl;
        _last_error= message.str();
        return false;
      }
      _addrinfo_next= _addrinfo_next->ai_next;


    case CONNECT:
      close_socket();

      _sockfd= socket(_addrinfo_next->ai_family,
                      _addrinfo_next->ai_socktype,
                      _addrinfo_next->ai_protocol);
      if (_sockfd == INVALID_SOCKET)
      {
        perror("socket");
        continue;
      }

      if (connect(_sockfd, _addrinfo_next->ai_addr, _addrinfo_next->ai_addrlen) < 0)
      {
        switch(errno)
        {
        case EAGAIN:
        case EINTR:
          state= CONNECT;
          break;

        case EINPROGRESS:
          state= CONNECTING;
          break;

        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:
        default:
          state= NEXT_CONNECT_ADDRINFO;
          break;
        }
      }
      else
      {
        state= CONNECTING;
      }
      break;

    case CONNECTING:
      // Add logic for poll() for nonblocking.
      state= CONNECTED;
      break;

    case CONNECTED:
    case WRITING:
      {
        size_t packet_length= operation->size();
        const char *packet= operation->ptr();

        while(packet_length)
        {
          ssize_t write_size= send(_sockfd, packet, packet_length, 0);

          if (write_size < 0)
          {
            switch(errno)
            {
            default:
              std::cerr << "Failed during send(" << strerror(errno) << ")" << std::endl;
              break;
            }
          }

          packet_length-= static_cast<size_t>(write_size);
          packet+= static_cast<size_t>(write_size);
        }
      }
      state= READING;
      break;

    case READING:
      if (operation->has_response())
      {
        size_t total_read;
        ssize_t read_length;

        do
        {
          char buffer[BUFSIZ];
          read_length= recv(_sockfd, buffer, sizeof(buffer), 0);

          if (read_length < 0)
          {
            switch(errno)
            {
            default:
              std::cerr << "Error occured while reading data from " << _host.c_str() << std::endl;
              return false;
            }
          }

          operation->push(buffer, static_cast<size_t>(read_length));
          total_read+= static_cast<size_t>(read_length);
        } while (more_to_read());
      } // end has_response

      state= FINISHED;
      break;

    case FINISHED:
      std::string response;
      bool success= operation->response(response);
      if (_finish_fn)
      {
        if (not _finish_fn->call(success, response))
        {
          // Error was sent from _finish_fn 
          return false;
        }
      }

      if (operation->reconnect())
      {
      }
      _operations.pop_back();
      delete operation;

      state= CONNECTED;
      break;
    } // end switch
  }

  return true;
} // end run()

bool Instance::more_to_read() const
{
  struct pollfd fds;
  fds.fd= _sockfd;
  fds.events = POLLIN;

  if (poll(&fds, 1, 5) < 1) // Default timeout is 5
  {
    return false;
  }

  return true;
}

void Instance::close_socket()
{
  if (_sockfd == INVALID_SOCKET)
    return;

  /* in case of death shutdown to avoid blocking at close() */
  if (shutdown(_sockfd, SHUT_RDWR) == SOCKET_ERROR && get_socket_errno() != ENOTCONN)
  {
    perror("shutdown");
  }
  else if (closesocket(_sockfd) == SOCKET_ERROR)
  {
    perror("close");
  }

  _sockfd= INVALID_SOCKET;
}

void Instance::free_addrinfo()
{
  if (not _addrinfo)
    return;

  freeaddrinfo(_addrinfo);
  _addrinfo= NULL;
  _addrinfo_next= NULL;
}

} /* namespace util */
} /* namespace datadifferential */
