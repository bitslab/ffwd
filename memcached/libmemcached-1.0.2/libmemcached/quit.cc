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

/*
  This closes all connections (forces flush of input as well).

  Maybe add a host specific, or key specific version?

  The reason we send "quit" is that in case we have buffered IO, this
  will force data to be completed.
*/

void memcached_quit_server(memcached_server_st *ptr, bool io_death)
{
  if (ptr->fd != INVALID_SOCKET)
  {
    if (io_death == false and memcached_is_udp(ptr->root) == false and ptr->options.is_shutting_down == false)
    {
      ptr->options.is_shutting_down= true;

      memcached_return_t rc;
      if (ptr->root->flags.binary_protocol)
      {
        protocol_binary_request_quit request= {}; // = {.bytes= {0}};
        request.message.header.request.magic = PROTOCOL_BINARY_REQ;
        request.message.header.request.opcode = PROTOCOL_BINARY_CMD_QUIT;
        request.message.header.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        rc= memcached_do(ptr, request.bytes, sizeof(request.bytes), true);
      }
      else
      {
        rc= memcached_do(ptr, memcached_literal_param("quit\r\n"), true);
      }

      WATCHPOINT_ASSERT(rc == MEMCACHED_SUCCESS or rc == MEMCACHED_FETCH_NOTFINISHED);

      /* read until socket is closed, or there is an error
       * closing the socket before all data is read
       * results in server throwing away all data which is
       * not read
       *
       * In .40 we began to only do this if we had been doing buffered
       * requests of had replication enabled.
       */
      if (ptr->root->flags.buffer_requests or ptr->root->number_of_replicas)
      {
        memcached_return_t rc_slurp;
        while (memcached_continue(rc_slurp= memcached_io_slurp(ptr))) {} ;
        WATCHPOINT_ASSERT(rc_slurp == MEMCACHED_CONNECTION_FAILURE);
      }

      /*
       * memcached_io_read may call memcached_quit_server with io_death if
       * it encounters problems, but we don't care about those occurences.
       * The intention of that loop is to drain the data sent from the
       * server to ensure that the server processed all of the data we
       * sent to the server.
       */
      ptr->server_failure_counter= 0;
    }
    memcached_io_close(ptr);
  }

  ptr->state= MEMCACHED_SERVER_STATE_NEW;
  ptr->cursor_active= 0;
  ptr->io_bytes_sent= 0;
  ptr->write_buffer_offset= size_t(ptr->root and memcached_is_udp(ptr->root) ? UDP_DATAGRAM_HEADER_LENGTH : 0);
  ptr->read_buffer_length= 0;
  ptr->read_ptr= ptr->read_buffer;
  ptr->options.is_shutting_down= false;
  memcached_server_response_reset(ptr);

  // We reset the version so that if we end up talking to a different server
  // we don't have stale server version information.
  ptr->major_version= ptr->minor_version= ptr->micro_version= UINT8_MAX;

  if (io_death)
  {
    memcached_mark_server_for_timeout(ptr);
  }
}

void send_quit(memcached_st *ptr)
{
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    memcached_quit_server(instance, false);
  }
}

void memcached_quit(memcached_st *ptr)
{
  if (memcached_failed(initialize_query(ptr)))
  {
    return;
  }

  send_quit(ptr);
}
