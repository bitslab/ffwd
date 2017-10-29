/* HashKit
 * Copyright (C) 2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <libhashkit/common.h>

const char *hashkit_strerror(hashkit_st *ptr, hashkit_return_t rc)
{
  (void)ptr;
  switch (rc)
  {
  case HASHKIT_SUCCESS: return "SUCCESS";
  case HASHKIT_FAILURE: return "FAILURE";
  case HASHKIT_MEMORY_ALLOCATION_FAILURE: return "MEMORY ALLOCATION FAILURE";
  case HASHKIT_INVALID_ARGUMENT: return "INVALID ARGUMENT";
  case HASHKIT_INVALID_HASH: return "INVALID hashkit_hash_algorithm_t";
  case HASHKIT_MAXIMUM_RETURN:
  default:
    return "INVALID hashkit_return_t";
  }
}
