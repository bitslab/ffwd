/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2006-2009 Brian Aker All rights reserved.
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

static memcached_return_t text_incr_decr(memcached_st *ptr,
                                         const char *verb,
                                         const char *group_key, size_t group_key_length,
                                         const char *key, size_t key_length,
                                         uint64_t offset,
                                         uint64_t *value)
{
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  uint32_t server_key;
  memcached_server_write_instance_st instance;
  bool no_reply= ptr->flags.no_reply;

  if (memcached_failed(memcached_key_test(*ptr, (const char **)&key, &key_length, 1)))
  {
    return memcached_set_error(*ptr, MEMCACHED_BAD_KEY_PROVIDED, MEMCACHED_AT);
  }

  server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  instance= memcached_server_instance_fetch(ptr, server_key);

  int send_length;
  send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                        "%s %.*s%.*s %" PRIu64 "%s\r\n", verb,
                        memcached_print_array(ptr->_namespace),
                        (int)key_length, key,
                        offset, no_reply ? " noreply" : "");
  if (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE || send_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT, 
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  memcached_return_t rc= memcached_do(instance, buffer, (size_t)send_length, true);
  if (no_reply or memcached_failed(rc))
    return rc;

  rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

  if (rc != MEMCACHED_SUCCESS)
  {
    return memcached_set_error(*instance, rc, MEMCACHED_AT);
  }

  /*
    So why recheck responce? Because the protocol is brain dead :)
    The number returned might end up equaling one of the string
    values. Less chance of a mistake with strncmp() so we will
    use it. We still called memcached_response() though since it
    worked its magic for non-blocking IO.
  */
  if (not strncmp(buffer, memcached_literal_param("ERROR\r\n")))
  {
    *value= 0;
    rc= MEMCACHED_PROTOCOL_ERROR;
  }
  else if (not strncmp(buffer, memcached_literal_param("CLIENT_ERROR\r\n")))
  {
    *value= 0;
    rc= MEMCACHED_PROTOCOL_ERROR;
  }
  else if (not strncmp(buffer, memcached_literal_param("NOT_FOUND\r\n")))
  {
    *value= 0;
    rc= MEMCACHED_NOTFOUND;
  }
  else
  {
    *value= strtoull(buffer, (char **)NULL, 10);
    rc= MEMCACHED_SUCCESS;
  }

  return memcached_set_error(*instance, rc, MEMCACHED_AT);
}

static memcached_return_t binary_incr_decr(memcached_st *ptr, uint8_t cmd,
                                           const char *group_key, size_t group_key_length,
                                           const char *key, size_t key_length,
                                           uint64_t offset, uint64_t initial,
                                           uint32_t expiration,
                                           uint64_t *value)
{
  bool no_reply= ptr->flags.no_reply;

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  if (no_reply)
  {
    if(cmd == PROTOCOL_BINARY_CMD_DECREMENT)
      cmd= PROTOCOL_BINARY_CMD_DECREMENTQ;

    if(cmd == PROTOCOL_BINARY_CMD_INCREMENT)
      cmd= PROTOCOL_BINARY_CMD_INCREMENTQ;
  }
  protocol_binary_request_incr request= {}; // = {.bytes= {0}};

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= cmd;
  request.message.header.request.keylen= htons((uint16_t)(key_length + memcached_array_size(ptr->_namespace)));
  request.message.header.request.extlen= 20;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl((uint32_t)(key_length + memcached_array_size(ptr->_namespace) +request.message.header.request.extlen));
  request.message.body.delta= memcached_htonll(offset);
  request.message.body.initial= memcached_htonll(initial);
  request.message.body.expiration= htonl((uint32_t) expiration);

  struct libmemcached_io_vector_st vector[]=
  {
    { request.bytes, sizeof(request.bytes) },
    { memcached_array_string(ptr->_namespace), memcached_array_size(ptr->_namespace) },
    { key, key_length }
  };

  memcached_return_t rc;
  if (memcached_failed(rc= memcached_vdo(instance, vector, 3, true)))
  {
    memcached_io_reset(instance);
    return (rc == MEMCACHED_SUCCESS) ? MEMCACHED_WRITE_FAILURE : rc;
  }

  if (no_reply)
  {
    return MEMCACHED_SUCCESS;
  }

  return memcached_response(instance, (char*)value, sizeof(*value), NULL);
}

memcached_return_t memcached_increment(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       uint32_t offset,
                                       uint64_t *value)
{
  return memcached_increment_by_key(ptr, key, key_length, key, key_length, offset, value);
}

memcached_return_t memcached_decrement(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       uint32_t offset,
                                       uint64_t *value)
{
  return memcached_decrement_by_key(ptr, key, key_length, key, key_length, offset, value);
}

memcached_return_t memcached_increment_by_key(memcached_st *ptr,
                                              const char *group_key, size_t group_key_length,
                                              const char *key, size_t key_length,
                                              uint64_t offset,
                                              uint64_t *value)
{
  memcached_return_t rc;
  uint64_t local_value;
  if (value == NULL)
  {
    value= &local_value;
  }

  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  if (memcached_failed(rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }

  LIBMEMCACHED_MEMCACHED_INCREMENT_START();
  if (ptr->flags.binary_protocol)
  {
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_INCREMENT,
                         group_key, group_key_length, key, key_length,
                         (uint64_t)offset, 0, MEMCACHED_EXPIRATION_NOT_ADD,
                         value);
  }
  else
  {
     rc= text_incr_decr(ptr, "incr", group_key, group_key_length, key, key_length, offset, value);
  }

  LIBMEMCACHED_MEMCACHED_INCREMENT_END();

  return rc;
}

memcached_return_t memcached_decrement_by_key(memcached_st *ptr,
                                              const char *group_key, size_t group_key_length,
                                              const char *key, size_t key_length,
                                              uint64_t offset,
                                              uint64_t *value)
{
  uint64_t local_value;
  if (value == NULL)
  {
    value= &local_value;
  }

  memcached_return_t rc;
  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  if (memcached_failed(rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }


  LIBMEMCACHED_MEMCACHED_DECREMENT_START();
  if (ptr->flags.binary_protocol)
  {
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_DECREMENT,
                         group_key, group_key_length, key, key_length,
                         (uint64_t)offset, 0, MEMCACHED_EXPIRATION_NOT_ADD,
                         value);
  }
  else
  {
    rc= text_incr_decr(ptr, "decr", group_key, group_key_length, key, key_length, offset, value);
  }

  LIBMEMCACHED_MEMCACHED_DECREMENT_END();

  return rc;
}

memcached_return_t memcached_increment_with_initial(memcached_st *ptr,
                                                    const char *key,
                                                    size_t key_length,
                                                    uint64_t offset,
                                                    uint64_t initial,
                                                    time_t expiration,
                                                    uint64_t *value)
{
  uint64_t local_value;
  if (value == NULL)
  {
    value= &local_value;
  }

  return memcached_increment_with_initial_by_key(ptr, key, key_length,
                                                 key, key_length,
                                                 offset, initial, expiration, value);
}

memcached_return_t memcached_increment_with_initial_by_key(memcached_st *ptr,
                                                         const char *group_key,
                                                         size_t group_key_length,
                                                         const char *key,
                                                         size_t key_length,
                                                         uint64_t offset,
                                                         uint64_t initial,
                                                         time_t expiration,
                                                         uint64_t *value)
{
  uint64_t local_value;
  if (value == NULL)
  {
    value= &local_value;
  }

  memcached_return_t rc;
  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  if (memcached_failed(rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }

  LIBMEMCACHED_MEMCACHED_INCREMENT_WITH_INITIAL_START();
  if (ptr->flags.binary_protocol)
  {
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_INCREMENT,
                         group_key, group_key_length, key, key_length,
                         offset, initial, (uint32_t)expiration,
                         value);
  }
  else
  {
    rc= MEMCACHED_PROTOCOL_ERROR;
  }

  LIBMEMCACHED_MEMCACHED_INCREMENT_WITH_INITIAL_END();

  return rc;
}

memcached_return_t memcached_decrement_with_initial(memcached_st *ptr,
                                                    const char *key,
                                                    size_t key_length,
                                                    uint64_t offset,
                                                    uint64_t initial,
                                                    time_t expiration,
                                                    uint64_t *value)
{
  uint64_t local_value;
  if (value == NULL)
  {
    value= &local_value;
  }

  return memcached_decrement_with_initial_by_key(ptr, key, key_length,
                                                 key, key_length,
                                                 offset, initial, expiration, value);
}

memcached_return_t memcached_decrement_with_initial_by_key(memcached_st *ptr,
                                                           const char *group_key,
                                                           size_t group_key_length,
                                                           const char *key,
                                                           size_t key_length,
                                                           uint64_t offset,
                                                           uint64_t initial,
                                                           time_t expiration,
                                                           uint64_t *value)
{
  uint64_t local_value;
  if (value == NULL)
  {
    value= &local_value;
  }

  memcached_return_t rc;
  if (memcached_failed(rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }

  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }


  LIBMEMCACHED_MEMCACHED_INCREMENT_WITH_INITIAL_START();
  if (ptr->flags.binary_protocol)
  {
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_DECREMENT,
                         group_key, group_key_length, key, key_length,
                         offset, initial, (uint32_t)expiration,
                         value);
  }
  else
  {
    rc= MEMCACHED_PROTOCOL_ERROR;
  }

  LIBMEMCACHED_MEMCACHED_INCREMENT_WITH_INITIAL_END();

  return rc;
}

