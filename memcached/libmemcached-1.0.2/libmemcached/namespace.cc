/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
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

#include <libmemcached/common.h>

memcached_return_t memcached_set_namespace(memcached_st *self, const char *key, size_t key_length)
{
  WATCHPOINT_ASSERT(self);

  if (key and key_length == 0)
  { 
    WATCHPOINT_ASSERT(key_length);
    return memcached_set_error(*self, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT, memcached_literal_param("Invalid namespace, namespace string had value but length was 0"));
  }
  else if (key_length and key == NULL)
  {
    WATCHPOINT_ASSERT(key);
    return memcached_set_error(*self, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT, memcached_literal_param("Invalid namespace, namespace string length was > 1 but namespace string was null "));
  }
  else if (key and key_length)
  {
    bool orig= self->flags.verify_key;
    self->flags.verify_key= true;
    if (memcached_failed(memcached_key_test(*self, (const char **)&key, &key_length, 1)))
    {
      self->flags.verify_key= orig;
      return memcached_set_error(*self, MEMCACHED_BAD_KEY_PROVIDED, MEMCACHED_AT);
    }
    self->flags.verify_key= orig;

    if ((key_length > MEMCACHED_PREFIX_KEY_MAX_SIZE -1))
    {
      return memcached_set_error(*self, MEMCACHED_KEY_TOO_BIG, MEMCACHED_AT);
    }

    memcached_array_free(self->_namespace);
    self->_namespace= memcached_strcpy(self, key, key_length);

    if (not self->_namespace)
    {
      return memcached_set_error(*self, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT);
    }
  }
  else
  {
    memcached_array_free(self->_namespace);
    self->_namespace= NULL;
  }

  return MEMCACHED_SUCCESS;
}

const char * memcached_get_namespace(memcached_st *self)
{
  if (not self->_namespace)
    return NULL;

  return memcached_array_string(self->_namespace);
}
