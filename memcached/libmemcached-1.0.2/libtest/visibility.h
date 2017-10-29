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

#if defined(BUILDING_LIBTEST)
# if defined(HAVE_VISIBILITY) && HAVE_VISIBILITY
#  define LIBTEST_API __attribute__ ((visibility("default")))
#  define LIBTEST_LOCAL  __attribute__ ((visibility("default")))
# elif defined (__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#  define LIBTEST_API __global
#  define LIBTEST_LOCAL __global
# elif defined(_MSC_VER)
#  define LIBTEST_API extern __declspec(dllexport) 
#  define LIBTEST_LOCAL extern __declspec(dllexport)
# else
#  define LIBTEST_API
#  define LIBTEST_LOCAL
# endif
#else
# if defined(BUILDING_LIBTEST)
#  if defined(HAVE_VISIBILITY) && HAVE_VISIBILITY
#   define LIBTEST_API __attribute__ ((visibility("default")))
#   define LIBTEST_LOCAL  __attribute__ ((visibility("hidden")))
#  elif defined (__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#   define LIBTEST_API __global
#   define LIBTEST_LOCAL __hidden
#  elif defined(_MSC_VER)
#   define LIBTEST_API extern __declspec(dllexport) 
#   define LIBTEST_LOCAL
#  else
#   define LIBTEST_API
#   define LIBTEST_LOCAL
#  endif /* defined(HAVE_VISIBILITY) */
# else  /* defined(BUILDING_LIBTEST) */
#  if defined(_MSC_VER)
#   define LIBTEST_API extern __declspec(dllimport) 
#   define LIBTEST_LOCAL
#  else
#   define LIBTEST_API
#   define LIBTEST_LOCAL
#  endif /* defined(_MSC_VER) */
# endif /* defined(BUILDING_LIBTEST) */
#endif /* defined(BUILDING_LIBTESTINTERNAL) */
