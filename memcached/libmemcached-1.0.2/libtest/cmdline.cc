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

using namespace libtest;

#include <cstdlib>
#include <string>
#include <sstream>

namespace libtest {

bool exec_cmdline(const std::string& executable, const char *args[])
{
  std::stringstream arg_buffer;

  arg_buffer << libtool();

  if (getenv("PWD"))
  {
    arg_buffer << getenv("PWD");
    arg_buffer << "/";
  }

  arg_buffer << executable;
  for (const char **ptr= args; *ptr; ++ptr)
  {
    arg_buffer << " " << *ptr;
  }

#if 0
    arg_buffer << " > /dev/null 2>&1";
#endif

  if (system(arg_buffer.str().c_str()) == -1)
  {
    return false;
  }

  return true;
}

const char *gearmand_binary() 
{
  return GEARMAND_BINARY;
}

} // namespace exec_cmdline
