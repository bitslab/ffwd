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

char *memcached_fetch(memcached_st *ptr, char *key, size_t *key_length, 
                      size_t *value_length, 
                      uint32_t *flags,
                      memcached_return_t *error)
{
  memcached_result_st *result_buffer= &ptr->result;
  memcached_return_t unused;
  if (not error)
    error= &unused;


  unlikely (ptr->flags.use_udp)
  {
    if (value_length)
      *value_length= 0;

    if (key_length)
      *key_length= 0;

    if (flags)
      *flags= 0;

    if (key)
      *key= 0;

    *error= MEMCACHED_NOT_SUPPORTED;
    return NULL;
  }

  result_buffer= memcached_fetch_result(ptr, result_buffer, error);
  if (result_buffer == NULL or memcached_failed(*error))
  {
    WATCHPOINT_ASSERT(result_buffer == NULL);
    if (value_length)
      *value_length= 0;

    if (key_length)
      *key_length= 0;

    if (flags)
      *flags= 0;

    if (key)
      *key= 0;

    return NULL;
  }

  if (value_length)
  {
    *value_length= memcached_string_length(&result_buffer->value);
  }

  if (key)
  {
    if (result_buffer->key_length > MEMCACHED_MAX_KEY)
    {
      *error= MEMCACHED_KEY_TOO_BIG;
      if (value_length)
        *value_length= 0;

    if (key_length)
      *key_length= 0;

    if (flags)
      *flags= 0;

    if (key)
      *key= 0;

      return NULL;
    }
    strncpy(key, result_buffer->item_key, result_buffer->key_length); // For the binary protocol we will cut off the key :(
    if (key_length)
      *key_length= result_buffer->key_length;
  }

  if (flags)
    *flags= result_buffer->item_flags;

  return memcached_string_take_value(&result_buffer->value);
}

memcached_result_st *memcached_fetch_result(memcached_st *ptr,
                                            memcached_result_st *result,
                                            memcached_return_t *error)
{
  memcached_return_t unused;
  if (not error)
    error= &unused;

  if (not ptr)
  {
    *error= MEMCACHED_INVALID_ARGUMENTS;
    return NULL;
  }

  if (ptr->flags.use_udp)
  {
    *error= MEMCACHED_NOT_SUPPORTED;
    return NULL;
  }

  if (not result)
  {
    // If we have already initialized (ie it is in use) our internal, we
    // create one.
    if (memcached_is_initialized(&ptr->result))
    {
      if (not (result= memcached_result_create(ptr, NULL)))
      {
        *error= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
        return NULL;
      }
    }
    else
    {
      result= memcached_result_create(ptr, &ptr->result);
    }
  }

  *error= MEMCACHED_MAXIMUM_RETURN; // We use this to see if we ever go into the loop
  memcached_server_st *server;
  while ((server= memcached_io_get_readable_server(ptr)))
  {
    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
    *error= memcached_response(server, buffer, sizeof(buffer), result);

    if (*error == MEMCACHED_IN_PROGRESS)
    {
      continue;
    }
    else if (*error == MEMCACHED_SUCCESS)
    {
      result->count++;
      return result;
    }
    else if (*error == MEMCACHED_END)
    {
      memcached_server_response_reset(server);
    }
    else if (*error != MEMCACHED_NOTFOUND)
    {
      break;
    }
  }

  if (*error == MEMCACHED_NOTFOUND and result->count)
  {
    *error= MEMCACHED_END;
  }
  else if (*error == MEMCACHED_MAXIMUM_RETURN and result->count)
  {
    *error= MEMCACHED_END;
  }
  else if (*error == MEMCACHED_MAXIMUM_RETURN) // while() loop was never entered
  {
    *error= MEMCACHED_NOTFOUND;
  }
  else if (*error == MEMCACHED_SUCCESS)
  {
    *error= MEMCACHED_END;
  }
  else if (result->count == 0)
  {
    *error= MEMCACHED_NOTFOUND;
  }

  /* We have completed reading data */
  if (memcached_is_allocated(result))
  {
    memcached_result_free(result);
  }
  else
  {
    result->count= 0;
    memcached_string_reset(&result->value);
  }

  return NULL;
}

memcached_return_t memcached_fetch_execute(memcached_st *ptr, 
                                           memcached_execute_fn *callback,
                                           void *context,
                                           uint32_t number_of_callbacks)
{
  memcached_result_st *result= &ptr->result;
  memcached_return_t rc;
  bool some_errors= false;

  while ((result= memcached_fetch_result(ptr, result, &rc)))
  {
    if (memcached_failed(rc) and rc == MEMCACHED_NOTFOUND)
    {
      continue;
    }
    else if (memcached_failed(rc))
    {
      memcached_set_error(*ptr, rc, MEMCACHED_AT);
      some_errors= true;
      continue;
    }

    for (uint32_t x= 0; x < number_of_callbacks; x++)
    {
      memcached_return_t ret= (*callback[x])(ptr, result, context);
      if (memcached_failed(ret))
      {
        some_errors= true;
        memcached_set_error(*ptr, ret, MEMCACHED_AT);
        break;
      }
    }
  }

  if (some_errors)
  {
    return MEMCACHED_SOME_ERRORS;
  }

  // If we were able to run all keys without issue we return
  // MEMCACHED_SUCCESS
  if (memcached_success(rc))
  {
    return MEMCACHED_SUCCESS;
  }

  return rc;
}
