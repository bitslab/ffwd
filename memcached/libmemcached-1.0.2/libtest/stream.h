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

#include <iostream>
#include <cassert>
#include <sstream>
#include <ctime>
#include <ostream>

namespace libtest {
namespace stream {

namespace detail {

template<class Ch, class Tr, class A>
  class cerr {
  private:

  public:
    typedef std::basic_ostringstream<Ch, Tr, A> stream_buffer;

  public:
    void operator()(const stream_buffer &s)
    {
      std::cerr << s.str() << std::endl;
    }
  };

template<class Ch, class Tr, class A>
  class make_cerr {
  private:

  public:
    typedef std::basic_ostringstream<Ch, Tr, A> stream_buffer;

  public:
    void operator()(const stream_buffer &s)
    {
      std::cerr << std::endl << s.str() << std::endl;
    }
  };

template<class Ch, class Tr, class A>
  class cout {
  private:

  public:
    typedef std::basic_ostringstream<Ch, Tr, A> stream_buffer;

  public:
    void operator()(const stream_buffer &s)
    {
      std::cout << s.str() << std::endl;
    }
  };

template<class Ch, class Tr, class A>
  class clog {
  private:

  public:
    typedef std::basic_ostringstream<Ch, Tr, A> stream_buffer;

  public:
    void operator()(const stream_buffer &s)
    {
      std::cerr<< s.str() << std::endl;
    }
  };

template<template <class Ch, class Tr, class A> class OutputPolicy, class Ch = char, class Tr = std::char_traits<Ch>, class A = std::allocator<Ch> >
  class log {
  private:
    typedef OutputPolicy<Ch, Tr, A> output_policy;
    const char *_filename;
    int _line_number;
    const char *_func;

  public:
    log() :
      _filename(NULL),
      _line_number(0)
    { }

    void set_filename(const char *filename, int line_number, const char *func)
    {
      _filename= filename;
      _line_number= line_number;
      _func= func;
    }

    ~log()
    {
      output_policy()(arg);
    }

  public:
    template<class T>
      log &operator<<(const T &x)
      {
        if (_filename)
        {
          arg << _filename << ":" << _line_number << ": in " << _func << "() ";
          _filename= NULL;
        }
        arg << x;
        return *this;
      }

  private:
    typename output_policy::stream_buffer arg;
  };
}

class make_cerr : public detail::log<detail::make_cerr> {
public:
  make_cerr(const char *filename, int line_number, const char *func)
  {
    set_filename(filename, line_number, func);
  }
};

class cerr : public detail::log<detail::cerr> {
public:
  cerr(const char *filename, int line_number, const char *func)
  {
    set_filename(filename, line_number, func);
  }
};

class clog : public detail::log<detail::clog> {
public:
  clog(const char *, int, const char*)
  { }
};

class cout : public detail::log<detail::cout> {
public:
  cout(const char *, int, const char *)
  { }
};


} // namespace stream

#define Error stream::cerr(__FILE__, __LINE__, __func__)

#define Out stream::cout(NULL, __LINE__, __func__)

#define Outn() stream::cout(NULL, __LINE__, __func__) << " "

#define Log stream::clog(NULL, __LINE__, __func__)

#define Logn() stream::clog(NULL, __LINE__, __func__) << " "

} // namespace libtest
