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

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

/**
  A structure describing the test case.
*/
struct test_st {
  const char *name;
  bool requires_flush;
  test_callback_fn *test_fn;
};

#define test_assert_errno(A) \
do \
{ \
  if ((A)) { \
    fprintf(stderr, "\n%s:%d: Assertion failed for %s: ", __FILE__, __LINE__, __func__);\
    perror(#A); \
    fprintf(stderr, "\n"); \
    libtest::create_core(); \
    assert((A)); \
  } \
} while (0)

#define test_assert(A, B) \
do \
{ \
  if ((A)) { \
    fprintf(stderr, "\n%s:%d: Assertion failed %s, with message %s, in %s", __FILE__, __LINE__, (B), #A, __func__ );\
    fprintf(stderr, "\n"); \
    libtest::create_core(); \
    assert((A)); \
  } \
} while (0)

#define test_truth(A) \
do \
{ \
  if (! (A)) { \
    fprintf(stderr, "\n%s:%d: Assertion \"%s\" failed, in %s\n", __FILE__, __LINE__, #A, __func__);\
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_true(A) \
do \
{ \
  if (! (A)) { \
    fprintf(stderr, "\n%s:%d: Assertion \"%s\" failed, in %s\n", __FILE__, __LINE__, #A, __func__);\
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_true_got(__expected, __hint) \
do \
{ \
  if (not libtest::_compare_truth_hint(__FILE__, __LINE__, __func__, ((__expected)), #__expected, ((__hint)))) \
  { \
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_skip(A,B) \
do \
{ \
  if ((A) != (B)) \
  { \
    return TEST_SKIPPED; \
  } \
} while (0)

#define test_fail(A) \
do \
{ \
  if (1) { \
    fprintf(stderr, "\n%s:%d: Failed with %s, in %s\n", __FILE__, __LINE__, #A, __func__);\
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)


#define test_false(A) \
do \
{ \
  if ((A)) { \
    fprintf(stderr, "\n%s:%d: Assertion failed %s, in %s\n", __FILE__, __LINE__, #A, __func__);\
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_false_with(A,B) \
do \
{ \
  if ((A)) { \
    fprintf(stderr, "\n%s:%d: Assertion failed %s with %s\n", __FILE__, __LINE__, #A, (B));\
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_compare(__expected, __actual) \
do \
{ \
  if (not libtest::_compare(__FILE__, __LINE__, __func__, ((__expected)), ((__actual)))) \
  { \
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_zero(__actual) \
do \
{ \
  if (not libtest::_compare_zero(__FILE__, __LINE__, __func__, ((__actual)))) \
  { \
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_null test_zero

#define test_compare_got(__expected, __actual, __hint) \
do \
{ \
  if (not libtest::_compare_hint(__FILE__, __LINE__, __func__, (__expected), (__actual), (__hint))) \
  { \
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_compare_hint test_compare_got

#define test_compare_warn(__expected, __actual) \
do \
{ \
  void(libtest::_compare(__FILE__, __LINE__, __func__, (__expected), (__actual))); \
} while (0)

#define test_compare_warn_hint(__expected, __actual, __hint) \
do \
{ \
  libtest::_compare_hint(__FILE__, __LINE__, __func__, (__expected), (__actual), (__hint)); \
} while (0)

#define test_warn(__truth) \
do \
{ \
  void(libtest::_truth(__FILE__, __LINE__, __func__, (__truth))); \
} while (0)

#define test_warn_hint(__truth, __hint) \
do \
{ \
  void(libtest::_compare_truth_hint(__FILE__, __LINE__, __func__, (__truth), #__truth, (__hint))); \
} while (0)


#define test_strcmp(A,B) \
do \
{ \
  if (strcmp((A), (B))) \
  { \
    fprintf(stderr, "\n%s:%d: Expected %s, got %s\n", __FILE__, __LINE__, (A), (B)); \
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_memcmp(A,B,C) \
do \
{ \
  if (memcmp((A), (B), (C))) \
  { \
    fprintf(stderr, "\n%s:%d: %.*s -> %.*s\n", __FILE__, __LINE__, (int)(C), (char *)(A), (int)(C), (char *)(B)); \
    libtest::create_core(); \
    return TEST_FAILURE; \
  } \
} while (0)

#define test_return_if(__test_return_t) \
do \
{ \
  if ((__test_return_t) != TEST_SUCCESS) \
  { \
    return __test_return_t; \
  } \
} while (0)

