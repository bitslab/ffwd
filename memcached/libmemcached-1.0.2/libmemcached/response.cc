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
#include <libmemcached/string.hpp>

static memcached_return_t textual_read_one_response(memcached_server_write_instance_st ptr,
                                                    char *buffer, size_t buffer_length,
                                                    memcached_result_st *result);
static memcached_return_t binary_read_one_response(memcached_server_write_instance_st ptr,
                                                   char *buffer, size_t buffer_length,
                                                   memcached_result_st *result);

memcached_return_t memcached_read_one_response(memcached_server_write_instance_st ptr,
                                               char *buffer, size_t buffer_length,
                                               memcached_result_st *result)
{
  memcached_server_response_decrement(ptr);

  if (result == NULL)
  {
    memcached_st *root= (memcached_st *)ptr->root;
    result = &root->result;
  }

  memcached_return_t rc;
  if (ptr->root->flags.binary_protocol)
  {
    rc= binary_read_one_response(ptr, buffer, buffer_length, result);
  }
  else
  {
    rc= textual_read_one_response(ptr, buffer, buffer_length, result);
  }

  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE or
           rc == MEMCACHED_PROTOCOL_ERROR or
           rc == MEMCACHED_CLIENT_ERROR or
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE)
  {
    memcached_io_reset(ptr);
  }

  return rc;
}

memcached_return_t memcached_response(memcached_server_write_instance_st ptr,
                                      char *buffer, size_t buffer_length,
                                      memcached_result_st *result)
{
  /* We may have old commands in the buffer not set, first purge */
  if ((ptr->root->flags.no_block) && (memcached_is_processing_input(ptr->root) == false))
  {
    (void)memcached_io_write(ptr, NULL, 0, true);
  }

  /*
   * The previous implementation purged all pending requests and just
   * returned the last one. Purge all pending messages to ensure backwards
   * compatibility.
 */
  if (ptr->root->flags.binary_protocol == false)
  {
    while (memcached_server_response_count(ptr) > 1)
    {
      memcached_return_t rc= memcached_read_one_response(ptr, buffer, buffer_length, result);

      unlikely (rc != MEMCACHED_END &&
                rc != MEMCACHED_STORED &&
                rc != MEMCACHED_SUCCESS &&
                rc != MEMCACHED_STAT &&
                rc != MEMCACHED_DELETED &&
                rc != MEMCACHED_NOTFOUND &&
                rc != MEMCACHED_NOTSTORED &&
                rc != MEMCACHED_DATA_EXISTS)
        return rc;
    }
  }

  return memcached_read_one_response(ptr, buffer, buffer_length, result);
}

static memcached_return_t textual_value_fetch(memcached_server_write_instance_st ptr,
                                              char *buffer,
                                              memcached_result_st *result)
{
  char *string_ptr;
  char *end_ptr;
  char *next_ptr;
  size_t value_length;
  size_t to_read;
  ssize_t read_length= 0;

  if (ptr->root->flags.use_udp)
  {
    return memcached_set_error(*ptr, MEMCACHED_NOT_SUPPORTED, MEMCACHED_AT);
  }

  WATCHPOINT_ASSERT(ptr->root);
  end_ptr= buffer + MEMCACHED_DEFAULT_COMMAND_SIZE;

  memcached_result_reset(result);

  string_ptr= buffer;
  string_ptr+= 6; /* "VALUE " */


  /* We load the key */
  {
    char *key;
    size_t prefix_length;

    key= result->item_key;
    result->key_length= 0;

    for (prefix_length= memcached_array_size(ptr->root->_namespace); !(iscntrl(*string_ptr) || isspace(*string_ptr)) ; string_ptr++)
    {
      if (prefix_length == 0)
      {
        *key= *string_ptr;
        key++;
        result->key_length++;
      }
      else
        prefix_length--;
    }
    result->item_key[result->key_length]= 0;
  }

  if (end_ptr == string_ptr)
    goto read_error;

  /* Flags fetch move past space */
  string_ptr++;
  if (end_ptr == string_ptr)
    goto read_error;

  for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++) {};
  result->item_flags= (uint32_t) strtoul(next_ptr, &string_ptr, 10);

  if (end_ptr == string_ptr)
    goto read_error;

  /* Length fetch move past space*/
  string_ptr++;
  if (end_ptr == string_ptr)
    goto read_error;

  for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++) {};
  value_length= (size_t)strtoull(next_ptr, &string_ptr, 10);

  if (end_ptr == string_ptr)
    goto read_error;

  /* Skip spaces */
  if (*string_ptr == '\r')
  {
    /* Skip past the \r\n */
    string_ptr+= 2;
  }
  else
  {
    string_ptr++;
    for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++) {};
    result->item_cas= strtoull(next_ptr, &string_ptr, 10);
  }

  if (end_ptr < string_ptr)
    goto read_error;

  /* We add two bytes so that we can walk the \r\n */
  if (memcached_failed(memcached_string_check(&result->value, value_length +2)))
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT);
  }

  {
    char *value_ptr= memcached_string_value_mutable(&result->value);
    /*
      We read the \r\n into the string since not doing so is more
      cycles then the waster of memory to do so.

      We are null terminating through, which will most likely make
      some people lazy about using the return length.
    */
    to_read= (value_length) + 2;
    memcached_return_t rrc= memcached_io_read(ptr, value_ptr, to_read, &read_length);
    if (memcached_failed(rrc) and rrc == MEMCACHED_IN_PROGRESS)
    {
      memcached_quit_server(ptr, true);
      return memcached_set_error(*ptr, MEMCACHED_IN_PROGRESS, MEMCACHED_AT);
    }
    else if (memcached_failed(rrc))
    {
      return rrc;
    }
  }

  if (read_length != (ssize_t)(value_length + 2))
  {
    goto read_error;
  }

  /* This next bit blows the API, but this is internal....*/
  {
    char *char_ptr;
    char_ptr= memcached_string_value_mutable(&result->value);;
    char_ptr[value_length]= 0;
    char_ptr[value_length +1]= 0;
    memcached_string_set_length(&result->value, value_length);
  }

  return MEMCACHED_SUCCESS;

read_error:
  memcached_io_reset(ptr);

  return MEMCACHED_PARTIAL_READ;
}

static memcached_return_t textual_read_one_response(memcached_server_write_instance_st ptr,
                                                    char *buffer, size_t buffer_length,
                                                    memcached_result_st *result)
{
  size_t total_read;
  memcached_return_t rc= memcached_io_readline(ptr, buffer, buffer_length, total_read);

  if (memcached_failed(rc))
  {
    return rc;
  }

  switch(buffer[0])
  {
  case 'V': /* VALUE || VERSION */
    if (buffer[1] == 'A') /* VALUE */
    {
      /* We add back in one because we will need to search for END */
      memcached_server_response_increment(ptr);
      return textual_value_fetch(ptr, buffer, result);
    }
    else if (buffer[1] == 'E') /* VERSION */
    {
      return MEMCACHED_SUCCESS;
    }
    else
    {
      WATCHPOINT_STRING(buffer);
      return MEMCACHED_UNKNOWN_READ_FAILURE;
    }
  case 'O': /* OK */
    return MEMCACHED_SUCCESS;

  case 'S': /* STORED STATS SERVER_ERROR */
    {
      if (buffer[2] == 'A') /* STORED STATS */
      {
        memcached_server_response_increment(ptr);
        return MEMCACHED_STAT;
      }
      else if (buffer[1] == 'E') /* SERVER_ERROR */
      {
        if (total_read == memcached_literal_param_size("SERVER_ERROR"))
        {
          return MEMCACHED_SERVER_ERROR;
        }

        if (total_read > memcached_literal_param_size("SERVER_ERROR object too large for cache") and
            (memcmp(buffer, memcached_literal_param("SERVER_ERROR object too large for cache")) == 0))
        {
          return MEMCACHED_E2BIG;
        }

        // Move past the basic error message and whitespace
        char *startptr= buffer + memcached_literal_param_size("SERVER_ERROR");
        if (startptr[0] == ' ')
        {
          startptr++;
        }

        char *endptr= startptr;
        while (*endptr != '\r' && *endptr != '\n') endptr++;

        return memcached_set_error(*ptr, MEMCACHED_SERVER_ERROR, MEMCACHED_AT, startptr, size_t(endptr - startptr));
      }
      else if (buffer[1] == 'T')
      {
        return MEMCACHED_STORED;
      }
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'D': /* DELETED */
    return MEMCACHED_DELETED;

  case 'N': /* NOT_FOUND */
    {
      if (buffer[4] == 'F')
      {
        return MEMCACHED_NOTFOUND;
      }
      else if (buffer[4] == 'S')
      {
        return MEMCACHED_NOTSTORED;
      }
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'E': /* PROTOCOL ERROR or END */
    {
      if (buffer[1] == 'N')
      {
        return MEMCACHED_END;
      }
      else if (buffer[1] == 'R')
      {
        return MEMCACHED_PROTOCOL_ERROR;
      }
      else if (buffer[1] == 'X')
      {
        return MEMCACHED_DATA_EXISTS;
      }
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }

    }
  case 'T': /* TOUCHED */
    {
      if (buffer[1] == 'O' and buffer[2] == 'U' 
          and buffer[3] == 'C' and buffer[4] == 'H' 
          and buffer[5] == 'E' and buffer[6] == 'D')
      {
        return MEMCACHED_SUCCESS;
      }
    }
    return MEMCACHED_UNKNOWN_READ_FAILURE;

  case 'I': /* CLIENT ERROR */
    /* We add back in one because we will need to search for END */
    memcached_server_response_increment(ptr);
    return MEMCACHED_ITEM;

  case 'C': /* CLIENT ERROR */
    return MEMCACHED_CLIENT_ERROR;

  default:
    {
      unsigned long long auto_return_value;

      if (sscanf(buffer, "%llu", &auto_return_value) == 1)
        return MEMCACHED_SUCCESS;

      WATCHPOINT_STRING(buffer);
      return MEMCACHED_UNKNOWN_READ_FAILURE;
    }
  }

  /* NOTREACHED */
}

static memcached_return_t binary_read_one_response(memcached_server_write_instance_st ptr,
                                                   char *buffer, size_t buffer_length,
                                                   memcached_result_st *result)
{
  memcached_return_t rc;
  protocol_binary_response_header header;

  if ((rc= memcached_safe_read(ptr, &header.bytes, sizeof(header.bytes))) != MEMCACHED_SUCCESS)
  {
    WATCHPOINT_ERROR(rc);
    return rc;
  }

  if (header.response.magic != PROTOCOL_BINARY_RES)
  {
    return MEMCACHED_PROTOCOL_ERROR;
  }

  /*
   ** Convert the header to host local endian!
 */
  header.response.keylen= ntohs(header.response.keylen);
  header.response.status= ntohs(header.response.status);
  header.response.bodylen= ntohl(header.response.bodylen);
  header.response.cas= memcached_ntohll(header.response.cas);
  uint32_t bodylen= header.response.bodylen;

  if (header.response.status == PROTOCOL_BINARY_RESPONSE_SUCCESS or
      header.response.status == PROTOCOL_BINARY_RESPONSE_AUTH_CONTINUE)
  {
    switch (header.response.opcode)
    {
    case PROTOCOL_BINARY_CMD_GETKQ:
      /*
       * We didn't increment the response counter for the GETKQ packet
       * (only the final NOOP), so we need to increment the counter again.
       */
      memcached_server_response_increment(ptr);
      /* FALLTHROUGH */
    case PROTOCOL_BINARY_CMD_GETK:
      {
        uint16_t keylen= header.response.keylen;
        memcached_result_reset(result);
        result->item_cas= header.response.cas;

        if ((rc= memcached_safe_read(ptr, &result->item_flags, sizeof (result->item_flags))) != MEMCACHED_SUCCESS)
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        result->item_flags= ntohl(result->item_flags);
        bodylen -= header.response.extlen;

        result->key_length= keylen;
        if (memcached_failed(rc= memcached_safe_read(ptr, result->item_key, keylen)))
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        // Only bother with doing this if key_length > 0
        if (result->key_length)
        {
          if (memcached_array_size(ptr->root->_namespace) and memcached_array_size(ptr->root->_namespace) >= result->key_length)
          {
            return memcached_set_error(*ptr, MEMCACHED_UNKNOWN_READ_FAILURE, MEMCACHED_AT);
          }

          if (memcached_array_size(ptr->root->_namespace))
          {
            result->key_length-= memcached_array_size(ptr->root->_namespace);
            memmove(result->item_key, result->item_key +memcached_array_size(ptr->root->_namespace), result->key_length);
          }
        }

        bodylen -= keylen;
        if (memcached_failed(memcached_string_check(&result->value, bodylen)))
        {
          return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
        }

        char *vptr= memcached_string_value_mutable(&result->value);
        if (memcached_failed(rc= memcached_safe_read(ptr, vptr, bodylen)))
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        memcached_string_set_length(&result->value, bodylen);
      }
      break;

    case PROTOCOL_BINARY_CMD_INCREMENT:
    case PROTOCOL_BINARY_CMD_DECREMENT:
      {
        if (bodylen != sizeof(uint64_t) || buffer_length != sizeof(uint64_t))
        {
          return MEMCACHED_PROTOCOL_ERROR;
        }

        WATCHPOINT_ASSERT(bodylen == buffer_length);
        uint64_t val;
        if ((rc= memcached_safe_read(ptr, &val, sizeof(val))) != MEMCACHED_SUCCESS)
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        val= memcached_ntohll(val);
        memcpy(buffer, &val, sizeof(val));
      }
      break;

    case PROTOCOL_BINARY_CMD_SASL_LIST_MECHS:
    case PROTOCOL_BINARY_CMD_VERSION:
      {
        memset(buffer, 0, buffer_length);
        if (bodylen >= buffer_length)
        {
          /* not enough space in buffer.. should not happen... */
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }
        else if ((rc= memcached_safe_read(ptr, buffer, bodylen)) != MEMCACHED_SUCCESS)
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }
      }
      break;
    case PROTOCOL_BINARY_CMD_FLUSH:
    case PROTOCOL_BINARY_CMD_QUIT:
    case PROTOCOL_BINARY_CMD_SET:
    case PROTOCOL_BINARY_CMD_ADD:
    case PROTOCOL_BINARY_CMD_REPLACE:
    case PROTOCOL_BINARY_CMD_APPEND:
    case PROTOCOL_BINARY_CMD_PREPEND:
    case PROTOCOL_BINARY_CMD_DELETE:
    case PROTOCOL_BINARY_CMD_TOUCH:
      {
        WATCHPOINT_ASSERT(bodylen == 0);
        return MEMCACHED_SUCCESS;
      }

    case PROTOCOL_BINARY_CMD_NOOP:
      {
        WATCHPOINT_ASSERT(bodylen == 0);
        return MEMCACHED_END;
      }

    case PROTOCOL_BINARY_CMD_STAT:
      {
        if (bodylen == 0)
        {
          return MEMCACHED_END;
        }
        else if (bodylen + 1 > buffer_length)
        {
          /* not enough space in buffer.. should not happen... */
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }
        else
        {
          size_t keylen= header.response.keylen;
          memset(buffer, 0, buffer_length);
          if ((rc= memcached_safe_read(ptr, buffer, keylen)) != MEMCACHED_SUCCESS ||
              (rc= memcached_safe_read(ptr, buffer + keylen + 1, bodylen - keylen)) != MEMCACHED_SUCCESS)
          {
            WATCHPOINT_ERROR(rc);
            return MEMCACHED_UNKNOWN_READ_FAILURE;
          }
        }
      }
      break;

    case PROTOCOL_BINARY_CMD_SASL_AUTH:
    case PROTOCOL_BINARY_CMD_SASL_STEP:
      {
        memcached_result_reset(result);
        result->item_cas= header.response.cas;

        if (memcached_string_check(&result->value,
                                   bodylen) != MEMCACHED_SUCCESS)
          return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

        char *vptr= memcached_string_value_mutable(&result->value);
        if ((rc= memcached_safe_read(ptr, vptr, bodylen)) != MEMCACHED_SUCCESS)
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        memcached_string_set_length(&result->value, bodylen);
      }
      break;
    default:
      {
        /* Command not implemented yet! */
        WATCHPOINT_ASSERT(0);
        return MEMCACHED_PROTOCOL_ERROR;
      }
    }
  }
  else if (header.response.bodylen)
  {
    /* What should I do with the error message??? just discard it for now */
    char hole[SMALL_STRING_LEN];
    while (bodylen > 0)
    {
      size_t nr= (bodylen > SMALL_STRING_LEN) ? SMALL_STRING_LEN : bodylen;
      if ((rc= memcached_safe_read(ptr, hole, nr)) != MEMCACHED_SUCCESS)
      {
        WATCHPOINT_ERROR(rc);
        return memcached_set_error(*ptr, MEMCACHED_UNKNOWN_READ_FAILURE, MEMCACHED_AT);
      }
      bodylen-= (uint32_t) nr;
    }

    /* This might be an error from one of the quiet commands.. if
     * so, just throw it away and get the next one. What about creating
     * a callback to the user with the error information?
   */
    switch (header.response.opcode)
    {
    case PROTOCOL_BINARY_CMD_SETQ:
    case PROTOCOL_BINARY_CMD_ADDQ:
    case PROTOCOL_BINARY_CMD_REPLACEQ:
    case PROTOCOL_BINARY_CMD_APPENDQ:
    case PROTOCOL_BINARY_CMD_PREPENDQ:
      return binary_read_one_response(ptr, buffer, buffer_length, result);

    default:
      break;
    }
  }

  rc= MEMCACHED_SUCCESS;
  if (header.response.status != 0)
  {
    switch (header.response.status)
    {
    case PROTOCOL_BINARY_RESPONSE_KEY_ENOENT:
      rc= MEMCACHED_NOTFOUND;
      break;

    case PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS:
      rc= MEMCACHED_DATA_EXISTS;
      break;

    case PROTOCOL_BINARY_RESPONSE_NOT_STORED:
      rc= MEMCACHED_NOTSTORED;
      break;

    case PROTOCOL_BINARY_RESPONSE_E2BIG:
      rc= MEMCACHED_E2BIG;
      break;

    case PROTOCOL_BINARY_RESPONSE_ENOMEM:
      rc= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
      break;

    case PROTOCOL_BINARY_RESPONSE_AUTH_CONTINUE:
      rc= MEMCACHED_AUTH_CONTINUE;
      break;

    case PROTOCOL_BINARY_RESPONSE_AUTH_ERROR:
      rc= MEMCACHED_AUTH_FAILURE;
      break;

    case PROTOCOL_BINARY_RESPONSE_EINVAL:
    case PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND:
    default:
      /* @todo fix the error mappings */
      rc= MEMCACHED_PROTOCOL_ERROR;
      break;
    }
  }

  return rc;
}
