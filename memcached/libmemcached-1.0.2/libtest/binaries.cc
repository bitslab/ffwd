/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libtest
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <libtest/common.h>

namespace libtest {

bool has_gearmand_binary()
{
#if defined(HAVE_GEARMAND_BINARY) && HAVE_GEARMAND_BINARY
  if (access(GEARMAND_BINARY,R_OK|X_OK) == 0)
  {
    return true;
  }
#endif

  return false;
}

bool has_memcached_binary()
{
#if defined(HAVE_MEMCACHED_BINARY) && HAVE_MEMCACHED_BINARY
  if (access(MEMCACHED_BINARY,R_OK|X_OK) == 0)
  {
    return true;
  }
#endif

  return false;
}

bool has_memcached_sasl_binary()
{
#if defined(HAVE_MEMCACHED_SASL_BINARY) && HAVE_MEMCACHED_SASL_BINARY
  if (access(MEMCACHED_SASL_BINARY, R_OK|X_OK) == 0)
  {
    return true;
  }
#endif

  return false;
}

}
