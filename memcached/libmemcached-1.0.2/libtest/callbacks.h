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

#ifdef	__cplusplus
extern "C" {
#endif

typedef void* (test_callback_create_fn)(libtest::server_startup_st&, test_return_t&);
typedef bool test_callback_destroy_fn(void *);
typedef enum test_return_t (test_callback_fn)(void *);
typedef enum test_return_t (test_callback_runner_fn)(test_callback_fn*, void *);
typedef enum test_return_t (test_callback_error_fn)(const test_return_t, void *);

#ifdef	__cplusplus
}
#endif

