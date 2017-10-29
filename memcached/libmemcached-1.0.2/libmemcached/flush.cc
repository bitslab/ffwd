/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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

static memcached_return_t memcached_flush_binary(memcached_st *ptr, 
                                                 time_t expiration);
static memcached_return_t memcached_flush_textual(memcached_st *ptr, 
                                                  time_t expiration);

memcached_return_t memcached_flush(memcached_st *ptr, time_t expiration)
{
  memcached_return_t rc;
  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  LIBMEMCACHED_MEMCACHED_FLUSH_START();
  if (ptr->flags.binary_protocol)
  {
    rc= memcached_flush_binary(ptr, expiration);
  }
  else
  {
    rc= memcached_flush_textual(ptr, expiration);
  }
  LIBMEMCACHED_MEMCACHED_FLUSH_END();

  return rc;
}

static memcached_return_t memcached_flush_textual(memcached_st *ptr, 
                                                  time_t expiration)
{
  bool reply= ptr->flags.no_reply ? false : true;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  int send_length;
  if (expiration)
  {
    send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, 
                          "flush_all %llu%s\r\n",
                          (unsigned long long)expiration, reply ? "" :  " noreply");
  }
  else
  {
    send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, 
                          "flush_all%s\r\n", reply ? "" : " noreply");
  }

  if (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE or send_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT, 
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }


  memcached_return_t rc= MEMCACHED_SUCCESS;
  for (unsigned int x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, x);

    memcached_return_t rrc= memcached_do(instance, buffer, (size_t)send_length, true);
    if (rrc == MEMCACHED_SUCCESS and reply == true)
    {
      char response_buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
      rrc= memcached_response(instance, response_buffer, sizeof(response_buffer), NULL);
    }

    if (memcached_failed(rrc))
    {
      // If an error has already been reported, then don't add to it
      if (instance->error_messages == NULL)
      {
        memcached_set_error(*instance, rrc, MEMCACHED_AT);
      }
      rc= MEMCACHED_SOME_ERRORS;
    }
  }

  return rc;
}

static memcached_return_t memcached_flush_binary(memcached_st *ptr, 
                                                 time_t expiration)
{
  protocol_binary_request_flush request= {};

  request.message.header.request.magic= (uint8_t)PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_FLUSH;
  request.message.header.request.extlen= 4;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl(request.message.header.request.extlen);
  request.message.body.expiration= htonl((uint32_t) expiration);

  memcached_return_t rc= MEMCACHED_SUCCESS;

  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, x);

    if (ptr->flags.no_reply)
    {
      request.message.header.request.opcode= PROTOCOL_BINARY_CMD_FLUSHQ;
    }
    else
    {
      request.message.header.request.opcode= PROTOCOL_BINARY_CMD_FLUSH;
    }

    memcached_return_t rrc;
    if ((rrc= memcached_do(instance, request.bytes, sizeof(request.bytes), true)))
    {
      memcached_set_error(*instance, rrc, MEMCACHED_AT);
      memcached_io_reset(instance);
      rc= MEMCACHED_SOME_ERRORS;
    } 
  }

  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, x);

    if (memcached_server_response_count(instance) > 0)
    {
      (void)memcached_response(instance, NULL, 0, NULL);
    }
  }

  return rc;
}
