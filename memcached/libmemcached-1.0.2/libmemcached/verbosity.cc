/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2010 Brian Aker All rights reserved.
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

#include <libmemcached/common.h>

struct context_st
{
  size_t length;
  const char *buffer;
};

static memcached_return_t _set_verbosity(const memcached_st *,
                                         const memcached_server_st *server,
                                         void *context)
{
  struct context_st *execute= (struct context_st *)context;

  memcached_st local_memc;
  memcached_st *memc_ptr= memcached_create(&local_memc);

  memcached_return_t rc= memcached_server_add(memc_ptr, memcached_server_name(server), memcached_server_port(server));

  if (rc == MEMCACHED_SUCCESS)
  {
    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

    memcached_server_write_instance_st instance= memcached_server_instance_fetch(memc_ptr, 0);


    rc= memcached_do(instance, execute->buffer, execute->length, true);

    if (rc == MEMCACHED_SUCCESS)
    {
      rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
    }
  }

  memcached_free(memc_ptr);

  return rc;
}

memcached_return_t memcached_verbosity(memcached_st *ptr, uint32_t verbosity)
{
  int send_length;
  memcached_server_fn callbacks[1];

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

  send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                 "verbosity %u\r\n", verbosity);
  if (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE || send_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT, 
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  struct context_st context = { (size_t)send_length, buffer };

  callbacks[0]= _set_verbosity;

  return memcached_server_cursor(ptr, callbacks, &context, 1);
}
