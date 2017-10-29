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


#pragma once

enum test_return_t {
  TEST_SUCCESS,
  TEST_FAILURE,
  TEST_MEMORY_ALLOCATION_FAILURE,
  TEST_SKIPPED,
  TEST_FATAL // Collection should not be continued
};


static inline bool test_success(test_return_t rc)
{
  return (rc == TEST_SUCCESS);
}

static inline bool test_failed(test_return_t rc)
{
  return (rc != TEST_SUCCESS);
}
