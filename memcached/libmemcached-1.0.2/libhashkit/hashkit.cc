/* HashKit
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <libhashkit/common.h>

static inline void _hashkit_init(hashkit_st *self)
{
  self->base_hash.function= hashkit_one_at_a_time;
  self->base_hash.context= NULL;

  self->distribution_hash.function= hashkit_one_at_a_time;
  self->distribution_hash.context= NULL;

  self->flags.is_base_same_distributed= true;
}

static inline hashkit_st *_hashkit_create(hashkit_st *self)
{
  if (not self)
  {
    self= (hashkit_st*)calloc(1, sizeof(hashkit_st));
    if (not self)
    {
      return NULL;
    }

    self->options.is_allocated= true;
  }
  else
  {
    self->options.is_allocated= false;
  }

  return self;
}

hashkit_st *hashkit_create(hashkit_st *self)
{
  self= _hashkit_create(self);
  if (not self)
    return self;

  _hashkit_init(self);

  return self;
}


void hashkit_free(hashkit_st *self)
{
  if (hashkit_is_allocated(self))
  {
    free(self);
  }
}

hashkit_st *hashkit_clone(hashkit_st *destination, const hashkit_st *source)
{
  if (source == NULL)
  {
    return hashkit_create(destination);
  }

  /* new_clone will be a pointer to destination */ 
  destination= _hashkit_create(destination);

  // Should only happen on allocation failure.
  if (destination == NULL)
  {
    return NULL;
  }

  destination->base_hash= source->base_hash;
  destination->distribution_hash= source->distribution_hash;
  destination->flags= source->flags;

  return destination;
}

bool hashkit_compare(const hashkit_st *first, const hashkit_st *second)
{
  if (not first or not second)
    return false;

  if (first->base_hash.function == second->base_hash.function and
      first->base_hash.context == second->base_hash.context and
      first->distribution_hash.function == second->distribution_hash.function and
      first->distribution_hash.context == second->distribution_hash.context and
      first->flags.is_base_same_distributed == second->flags.is_base_same_distributed)
  {
    return true;
  }

  return false;
}
