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

memcached_return_t memcached_key_test(memcached_st &memc,
                                      const char * const *keys,
                                      const size_t *key_length,
                                      size_t number_of_keys)
{
  if (keys == NULL or key_length == NULL)
  {
    return memcached_set_error(memc, MEMCACHED_BAD_KEY_PROVIDED, MEMCACHED_AT);
  }

  if (not memc.flags.verify_key)
  {
    for (uint32_t x= 0; x < number_of_keys; x++)
    {
      memcached_return_t rc= memcached_validate_key_length(*(key_length +x), false);
      if (memcached_failed(rc))
      {
        return rc;
      }
    }

    return MEMCACHED_SUCCESS;
  }

  if (memc.flags.binary_protocol)
  {
    for (uint32_t x= 0; x < number_of_keys; x++)
    {
      memcached_return_t rc= memcached_validate_key_length(*(key_length +x), false);
      if (memcached_failed(rc))
      {
        return rc;
      }
    }

    return MEMCACHED_SUCCESS;
  }

  for (uint32_t x= 0; x < number_of_keys; x++)
  {
    memcached_return_t rc= memcached_validate_key_length(*(key_length + x), false);
    if (memcached_failed(rc))
    {
      return rc;
    }
 
    for (size_t y= 0; y < *(key_length + x); y++)
    {
      if ((isgraph(keys[x][y])) == 0)
      {
        return MEMCACHED_BAD_KEY_PROVIDED;
      }
    }
  }

  return MEMCACHED_SUCCESS;
}

