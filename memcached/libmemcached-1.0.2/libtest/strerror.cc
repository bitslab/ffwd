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

const char *test_strerror(test_return_t code)
{
  switch (code) {
  case TEST_SUCCESS:
    return "ok";

  case TEST_FAILURE:
    return "failed";

  case TEST_MEMORY_ALLOCATION_FAILURE:
    return "memory allocation";

  case TEST_SKIPPED:
    return "skipped";

  case TEST_FATAL:
    break;
  }

  return "failed";
}

} // namespace libtest


std::ostream& operator<<(std::ostream& output, const enum test_return_t &arg)
{
  output << libtest::test_strerror(arg);
  return output;
}

std::ostream& operator<<(std::ostream& output, const std::vector<char> &arg)
{
  output << "std::vector<char>:" << arg.size(); 
  return output;
}
