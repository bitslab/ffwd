/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Storage related functions, aka set, replace,..
 *
 */

#include <libmemcached/common.h>

enum memcached_storage_action_t {
  SET_OP,
  REPLACE_OP,
  ADD_OP,
  PREPEND_OP,
  APPEND_OP,
  CAS_OP
};

/* Inline this */
static inline const char *storage_op_string(memcached_storage_action_t verb)
{
  switch (verb)
  {
  case REPLACE_OP:
    return "replace ";

  case ADD_OP:
    return "add ";

  case PREPEND_OP:
    return "prepend ";

  case APPEND_OP:
    return "append ";

  case CAS_OP:
    return "cas ";

  case SET_OP:
    break;
  }

  return "set ";
}

static inline uint8_t get_com_code(memcached_storage_action_t verb, bool noreply)
{
  if (noreply)
  {
    switch (verb)
    {
    case SET_OP:
      return PROTOCOL_BINARY_CMD_SETQ;

    case ADD_OP:
      return PROTOCOL_BINARY_CMD_ADDQ;

    case CAS_OP: /* FALLTHROUGH */
    case REPLACE_OP:
      return PROTOCOL_BINARY_CMD_REPLACEQ;

    case APPEND_OP:
      return PROTOCOL_BINARY_CMD_APPENDQ;

    case PREPEND_OP:
      return PROTOCOL_BINARY_CMD_PREPENDQ;
    }
  }

  switch (verb)
  {
  case SET_OP:
    break;

  case ADD_OP:
    return PROTOCOL_BINARY_CMD_ADD;

  case CAS_OP: /* FALLTHROUGH */
  case REPLACE_OP:
    return PROTOCOL_BINARY_CMD_REPLACE;

  case APPEND_OP:
    return PROTOCOL_BINARY_CMD_APPEND;

  case PREPEND_OP:
    return PROTOCOL_BINARY_CMD_PREPEND;
  }

  return PROTOCOL_BINARY_CMD_SET;
}

static memcached_return_t memcached_send_binary(memcached_st *ptr,
                                                memcached_server_write_instance_st server,
                                                uint32_t server_key,
                                                const char *key,
                                                size_t key_length,
                                                const char *value,
                                                size_t value_length,
                                                time_t expiration,
                                                uint32_t flags,
                                                uint64_t cas,
                                                bool flush,
                                                memcached_storage_action_t verb)
{
  protocol_binary_request_set request= {};
  size_t send_length= sizeof(request.bytes);

  bool noreply= server->root->flags.no_reply;

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= get_com_code(verb, noreply);
  request.message.header.request.keylen= htons((uint16_t)(key_length + memcached_array_size(ptr->_namespace)));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  if (verb == APPEND_OP or verb == PREPEND_OP)
  {
    send_length -= 8; /* append & prepend does not contain extras! */
  }
  else
  {
    request.message.header.request.extlen= 8;
    request.message.body.flags= htonl(flags);
    request.message.body.expiration= htonl((uint32_t)expiration);
  }

  request.message.header.request.bodylen= htonl((uint32_t) (key_length + memcached_array_size(ptr->_namespace) + value_length +
                                                            request.message.header.request.extlen));

  if (cas)
  {
    request.message.header.request.cas= memcached_htonll(cas);
  }

  if (server->root->flags.use_udp and flush == false)
  {
    size_t cmd_size= send_length + key_length + value_length;

    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
    {
      return MEMCACHED_WRITE_FAILURE;
    }
    if (cmd_size + server->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(server, NULL, 0, true);
    }
  }

  struct libmemcached_io_vector_st vector[]=
  {
    { request.bytes, send_length },
    { memcached_array_string(ptr->_namespace),  memcached_array_size(ptr->_namespace) },
    { key, key_length },
    { value, value_length }
  };

  /* write the header */
  memcached_return_t rc;
  if ((rc= memcached_vdo(server, vector, 4, flush)) != MEMCACHED_SUCCESS)
  {
    memcached_io_reset(server);

    if (ptr->error_messages == NULL)
    {
      memcached_set_error(*server, rc, MEMCACHED_AT);
    }

    return MEMCACHED_WRITE_FAILURE;
  }

  if (verb == SET_OP && ptr->number_of_replicas > 0)
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_SETQ;
    WATCHPOINT_STRING("replicating");

    for (uint32_t x= 0; x < ptr->number_of_replicas; x++)
    {
      ++server_key;
      if (server_key == memcached_server_count(ptr))
      {
        server_key= 0;
      }

      memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

      if (memcached_vdo(instance, vector, 4, false) != MEMCACHED_SUCCESS)
      {
        memcached_io_reset(instance);
      }
      else
      {
        memcached_server_response_decrement(instance);
      }
    }
  }

  if (flush == false)
  {
    return MEMCACHED_BUFFERED;
  }

  if (noreply)
  {
    return MEMCACHED_SUCCESS;
  }

  return memcached_response(server, NULL, 0, NULL);
}

static memcached_return_t memcached_send_ascii(memcached_st *ptr,
                                               memcached_server_write_instance_st instance,
                                               const char *key,
                                               size_t key_length,
                                               const char *value,
                                               size_t value_length,
                                               time_t expiration,
                                               uint32_t flags,
                                               uint64_t cas,
                                               bool flush,
                                               memcached_storage_action_t verb)
{
  size_t write_length;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

  if (cas)
  {
    int check_length;
    check_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                           "%s %.*s%.*s %u %llu %lu %llu%s\r\n",
                           storage_op_string(verb),
                           memcached_print_array(ptr->_namespace),
                           (int)key_length, key, flags,
                           (unsigned long long)expiration, (unsigned long)value_length,
                           (unsigned long long)cas,
                           (ptr->flags.no_reply) ? " noreply" : "");
    if (check_length >= MEMCACHED_DEFAULT_COMMAND_SIZE or check_length < 0)
    {
      return memcached_set_error(*instance, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT, 
                                 memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
    }
    write_length= check_length;
  }
  else
  {
    char *buffer_ptr= buffer;
    const char *command= storage_op_string(verb);

    /* Copy in the command, no space needed, we handle that in the command function*/
    memcpy(buffer_ptr, command, strlen(command));

    /* Copy in the key prefix, switch to the buffer_ptr */
    buffer_ptr= (char *)memcpy((char *)(buffer_ptr + strlen(command)), (char *)memcached_array_string(ptr->_namespace), memcached_array_size(ptr->_namespace));

    /* Copy in the key, adjust point if a key prefix was used. */
    buffer_ptr= (char *)memcpy(buffer_ptr + memcached_array_size(ptr->_namespace),
                               key, key_length);
    buffer_ptr+= key_length;
    buffer_ptr[0]=  ' ';
    buffer_ptr++;

    write_length= (size_t)(buffer_ptr - buffer);
    int check_length= snprintf(buffer_ptr, MEMCACHED_DEFAULT_COMMAND_SIZE -(size_t)(buffer_ptr - buffer),
                               "%u %llu %lu%s\r\n",
                               flags,
                               (unsigned long long)expiration, (unsigned long)value_length,
                               ptr->flags.no_reply ? " noreply" : "");
    if ((size_t)check_length >= MEMCACHED_DEFAULT_COMMAND_SIZE -size_t(buffer_ptr - buffer) or check_length < 0)
    {
      return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT, 
                              memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
    }

    write_length+= (size_t)check_length;
    WATCHPOINT_ASSERT(write_length < MEMCACHED_DEFAULT_COMMAND_SIZE);
  }

  if (ptr->flags.use_udp and ptr->flags.buffer_requests)
  {
    size_t cmd_size= write_length + value_length +2;
    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
    {
      return memcached_set_error(*ptr, MEMCACHED_WRITE_FAILURE, MEMCACHED_AT);
    }

    if (cmd_size + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(instance, NULL, 0, true);
    }
  }

  if (write_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_WRITE_FAILURE, MEMCACHED_AT);
  }

  struct libmemcached_io_vector_st vector[]=
  {
    { buffer, write_length },
    { value, value_length },
    { memcached_literal_param("\r\n") }
  };

  if (memcached_is_udp(instance->root) and  (write_length +value_length +memcached_literal_param_size("\r\n") +UDP_DATAGRAM_HEADER_LENGTH > MAX_UDP_DATAGRAM_LENGTH))
  {
    return memcached_set_error(*instance, MEMCACHED_WRITE_FAILURE, MEMCACHED_AT, memcached_literal_param("UDP packet is too large"));
  }

  /* Send command header */
  memcached_return_t rc=  memcached_vdo(instance, vector, 3, flush);
  if (rc == MEMCACHED_SUCCESS)
  {
    if (ptr->flags.no_reply and flush)
    {
      rc= MEMCACHED_SUCCESS;
    }
    else if (flush == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else
    {
      rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

      if (rc == MEMCACHED_STORED)
      {
        rc= MEMCACHED_SUCCESS;
      }
    }
  }

  if (rc == MEMCACHED_WRITE_FAILURE)
  {
    memcached_io_reset(instance);
  }

  if (memcached_failed(rc) and ptr->error_messages == NULL)
  {
    memcached_set_error(*ptr, rc, MEMCACHED_AT);
  }

  return rc;
}

static inline memcached_return_t memcached_send(memcached_st *ptr,
                                                const char *group_key, size_t group_key_length,
                                                const char *key, size_t key_length,
                                                const char *value, size_t value_length,
                                                time_t expiration,
                                                uint32_t flags,
                                                uint64_t cas,
                                                memcached_storage_action_t verb)
{
  memcached_return_t rc;
  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  if (memcached_failed(rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }

  if (memcached_failed(memcached_key_test(*ptr, (const char **)&key, &key_length, 1)))
  {
    return MEMCACHED_BAD_KEY_PROVIDED;
  }

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  WATCHPOINT_SET(instance->io_wait_count.read= 0);
  WATCHPOINT_SET(instance->io_wait_count.write= 0);

  bool flush= (bool) ((instance->root->flags.buffer_requests && verb == SET_OP) ? 0 : 1);
  if (ptr->flags.binary_protocol)
  {
    rc= memcached_send_binary(ptr, instance, server_key,
                              key, key_length,
                              value, value_length, expiration,
                              flags, cas, flush, verb);
  }
  else
  {
    rc= memcached_send_ascii(ptr, instance,
                             key, key_length,
                             value, value_length, expiration,
                             flags, cas, flush, verb);
  }

  return rc;
}


memcached_return_t memcached_set(memcached_st *ptr, const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_SET_START();
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, SET_OP);
  LIBMEMCACHED_MEMCACHED_SET_END();
  return rc;
}

memcached_return_t memcached_add(memcached_st *ptr,
                                 const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_ADD_START();
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, ADD_OP);
  LIBMEMCACHED_MEMCACHED_ADD_END();
  return rc;
}

memcached_return_t memcached_replace(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     const char *value, size_t value_length,
                                     time_t expiration,
                                     uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_REPLACE_START();
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, REPLACE_OP);
  LIBMEMCACHED_MEMCACHED_REPLACE_END();
  return rc;
}

memcached_return_t memcached_prepend(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     const char *value, size_t value_length,
                                     time_t expiration,
                                     uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, PREPEND_OP);
  return rc;
}

memcached_return_t memcached_append(memcached_st *ptr,
                                    const char *key, size_t key_length,
                                    const char *value, size_t value_length,
                                    time_t expiration,
                                    uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, APPEND_OP);
  return rc;
}

memcached_return_t memcached_cas(memcached_st *ptr,
                                 const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags,
                                 uint64_t cas)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, cas, CAS_OP);
  return rc;
}

memcached_return_t memcached_set_by_key(memcached_st *ptr,
                                        const char *group_key,
                                        size_t group_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_SET_START();
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, SET_OP);
  LIBMEMCACHED_MEMCACHED_SET_END();
  return rc;
}

memcached_return_t memcached_add_by_key(memcached_st *ptr,
                                        const char *group_key, size_t group_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_ADD_START();
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, ADD_OP);
  LIBMEMCACHED_MEMCACHED_ADD_END();
  return rc;
}

memcached_return_t memcached_replace_by_key(memcached_st *ptr,
                                            const char *group_key, size_t group_key_length,
                                            const char *key, size_t key_length,
                                            const char *value, size_t value_length,
                                            time_t expiration,
                                            uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_REPLACE_START();
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, REPLACE_OP);
  LIBMEMCACHED_MEMCACHED_REPLACE_END();
  return rc;
}

memcached_return_t memcached_prepend_by_key(memcached_st *ptr,
                                            const char *group_key, size_t group_key_length,
                                            const char *key, size_t key_length,
                                            const char *value, size_t value_length,
                                            time_t expiration,
                                            uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, PREPEND_OP);
  return rc;
}

memcached_return_t memcached_append_by_key(memcached_st *ptr,
                                           const char *group_key, size_t group_key_length,
                                           const char *key, size_t key_length,
                                           const char *value, size_t value_length,
                                           time_t expiration,
                                           uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, APPEND_OP);
  return rc;
}

memcached_return_t memcached_cas_by_key(memcached_st *ptr,
                                        const char *group_key, size_t group_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags,
                                        uint64_t cas)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, cas, CAS_OP);
  return rc;
}

