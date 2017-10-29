/*
  We use this to dump all keys.

  At this point we only support a callback method. This could be optimized by first
  calling items and finding active slabs. For the moment though we just loop through
  all slabs on servers and "grab" the keys.
*/

#include <libmemcached/common.h>

static memcached_return_t ascii_dump(memcached_st *ptr, memcached_dump_fn *callback, void *context, uint32_t number_of_callbacks)
{
  memcached_return_t rc= MEMCACHED_SUCCESS;

  for (uint32_t server_key= 0; server_key < memcached_server_count(ptr); server_key++)
  {
    memcached_server_write_instance_st instance;
    instance= memcached_server_instance_fetch(ptr, server_key);

    /* 256 I BELIEVE is the upper limit of slabs */
    for (uint32_t x= 0; x < 256; x++)
    {
      char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
      int send_length;
      send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                            "stats cachedump %u 0 0\r\n", x);

      if (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE || send_length < 0)
      {
        return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT, 
                                   memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
      }

      rc= memcached_do(instance, buffer, (size_t)send_length, true);

      if (rc != MEMCACHED_SUCCESS)
      {
        goto error;
      }

      while (1)
      {
        uint32_t callback_counter;
        rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

        if (rc == MEMCACHED_ITEM)
        {
          char *string_ptr, *end_ptr;

          string_ptr= buffer;
          string_ptr+= 5; /* Move past ITEM */

          for (end_ptr= string_ptr; isgraph(*end_ptr); end_ptr++) {} ;

          char *key= string_ptr;
          key[(size_t)(end_ptr-string_ptr)]= 0;

          for (callback_counter= 0; callback_counter < number_of_callbacks; callback_counter++)
          {
            rc= (*callback[callback_counter])(ptr, key, (size_t)(end_ptr-string_ptr), context);
            if (rc != MEMCACHED_SUCCESS)
            {
              break;
            }
          }
        }
        else if (rc == MEMCACHED_END)
        {
          break;
        }
        else if (rc == MEMCACHED_SERVER_ERROR or rc == MEMCACHED_CLIENT_ERROR)
        {
          /* If we try to request stats cachedump for a slab class that is too big
           * the server will return an incorrect error message:
           * "MEMCACHED_SERVER_ERROR failed to allocate memory"
           * This isn't really a fatal error, so let's just skip it. I want to
           * fix the return value from the memcached server to a CLIENT_ERROR,
           * so let's add support for that as well right now.
           */
          rc= MEMCACHED_END;
          break;
        }
        else
        {
          goto error;
        }
      }
    }
  }

error:
  if (rc == MEMCACHED_END)
  {
    return MEMCACHED_SUCCESS;
  }
  else
  {
    return rc;
  }
}

memcached_return_t memcached_dump(memcached_st *ptr, memcached_dump_fn *callback, void *context, uint32_t number_of_callbacks)
{
  memcached_return_t rc;
  if ((rc= initialize_query(ptr)) != MEMCACHED_SUCCESS)
  {
    return rc;
  }

  /* 
    No support for Binary protocol yet
    @todo Fix this so that we just flush, switch to ascii, and then go back to binary.
  */
  if (ptr->flags.binary_protocol)
  {
    return MEMCACHED_FAILURE;
  }

  return ascii_dump(ptr, callback, context, number_of_callbacks);
}
