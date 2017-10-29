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

#include <libmemcached/csl/common.h>
#include <libmemcached/csl/context.h>

void Context::abort(const char *error_arg, yytokentype last_token, const char *last_token_str)
{
  rc= MEMCACHED_PARSE_ERROR;
  (void)last_token;
  (void)last_token_str;

  if (error_arg)
  {
    memcached_set_parser_error(*memc, MEMCACHED_AT, "Error occured while parsing: %s", error_arg);
    return;
  }

  memcached_set_parser_error(*memc, MEMCACHED_AT, "Error occured while parsing: %s", memcached_strerror(NULL, MEMCACHED_PARSE_ERROR));
}

void Context::error(const char *error_arg, yytokentype last_token, const char *last_token_str)
{
  rc= MEMCACHED_PARSE_ERROR;
  if (not error_arg)
  {
    memcached_set_parser_error(*memc, MEMCACHED_AT, "Unknown error occured during parsing (%s)", last_token_str ? last_token_str : " ");
    return;
  }

  if (error_arg and strcmp(error_arg, "memory exhausted") == 0)
  {
    (void)memcached_set_error(*memc, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT, memcached_string_make_from_cstr(error_arg));
    return;
  }

  // We now test if it is something other then a syntax error, if it  we
  // return a generic message 
  if (error_arg and strcmp(error_arg, "syntax error") == 0)
  { }
  else if (error_arg)
  {
    memcached_set_parser_error(*memc, MEMCACHED_AT, "Error occured during parsing (%s)", error_arg);
    return;
  }

  if (last_token == UNKNOWN_OPTION and begin)
  {
    memcached_set_parser_error(*memc, MEMCACHED_AT, "Unknown option: %s", begin);
  }
  else if (last_token == UNKNOWN)
  {
    memcached_set_parser_error(*memc, MEMCACHED_AT, "Error occured durring parsing, an unknown token was found.");
  }
  else
  {
    memcached_set_parser_error(*memc, MEMCACHED_AT, "Error occured while parsing (%s)", last_token_str ? last_token_str : " ");
  }
}

const char *Context::set_hostname(const char *str, size_t size)
{
  size_t copy_length= size_t(NI_MAXHOST) > size ? size : size_t(NI_MAXHOST);
  memcpy(_hostname, str, copy_length);
  _hostname[copy_length]= 0;

  return _hostname;
}

