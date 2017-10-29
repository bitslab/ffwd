/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libmcachedd client library.
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

#include <cstring>
#include <cstdlib>

#ifdef __GNUC__
#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#include <cxxabi.h>
#endif // HAVE_BACKTRACE
#endif // __GNUC__


void custom_backtrace(void)
{
#ifdef __GNUC__
#ifdef HAVE_BACKTRACE
  void *array[50];

  size_t size= backtrace(array, 50);
  char **strings= backtrace_symbols(array, size);

  fprintf(stderr, "Number of stack frames obtained: %lu\n", (unsigned long)size);

  for (size_t x= 1; x < size; x++) 
  {
    size_t sz= 200;
    char *function= (char *)malloc(sz);
    char *begin= 0;
    char *end= 0;

    for (char *j = strings[x]; *j; ++j)
    {
      if (*j == '(') {
        begin = j;
      }
      else if (*j == '+') {
        end = j;
      }
    }
    if (begin && end)
    {
      begin++;
      *end= '\0';

      int status;
      char *ret = abi::__cxa_demangle(begin, function, &sz, &status);
      if (ret) 
      {
        function= ret;
      }
      else
      {
        strncpy(function, begin, sz);
        strncat(function, "()", sz);
        function[sz-1] = '\0';
      }
      fprintf(stderr, "%s\n", function);
    }
    else
    {
      fprintf(stderr, "%s\n", strings[x]);
    }
    free(function);
  }


  free (strings);
#endif // HAVE_BACKTRACE
#endif // __GNUC__
}
