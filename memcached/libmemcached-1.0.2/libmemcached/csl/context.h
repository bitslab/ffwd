/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Configure Scripting Language
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

#pragma once

#include <libmemcached/csl/common.h>
#include <libmemcached/csl/parser.h>

class Context
{
public:
  Context(const char *option_string, size_t option_string_length, memcached_st *memc_arg,
          memcached_return_t &rc_arg) :
    previous_token(END),
    scanner(NULL),
    begin(NULL),
    pos(0),
    memc(NULL),
    rc(rc_arg),
    _is_server(false),
    _end(false)
  {
    _hostname[0]= 0;
    buf= option_string;
    length= option_string_length;
    memc= memc_arg;
    init_scanner();
    rc= MEMCACHED_SUCCESS;
  }

  bool end()
  {
    return _end;
  }

  void start();

  void set_end()
  {
    rc= MEMCACHED_SUCCESS;
    _end= true;
  }

  void set_server()
  {
    _is_server= true;
  }

  void unset_server()
  {
    _is_server= false;
  }

  bool is_server()
  {
    return _is_server;
  }

  const char *set_hostname(const char *str, size_t size);

  const char *hostname()
  {
    return _hostname;
  }

  void abort(const char *, yytokentype, const char *);
  void error(const char *, yytokentype, const char* );

  ~Context()
  {
    destroy_scanner();
  }

  yytokentype previous_token;
  void *scanner;
  const char *buf;
  const char *begin;
  size_t pos;
  size_t length;
  memcached_st *memc;
  memcached_return_t &rc;

protected:
  void init_scanner();   
  void destroy_scanner();

private:
  bool _is_server;
  bool _end;
  char _hostname[NI_MAXHOST];
}; 
