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

#include <unistd.h>
#include <string>

namespace libtest {

class Wait 
{
public:

  Wait(const std::string &filename, uint32_t timeout= 6) :
    _successful(false)
  {
    uint32_t waited;
    uint32_t this_wait;
    uint32_t retry;

    if (filename.empty())
    {
      _successful= false;
      return;
    }

    for (waited= 0, retry= 1; ; retry++, waited+= this_wait)
    {
      if (access(filename.c_str(), R_OK) == 0)
      {
        _successful= true;
        break;
      }
      else if (waited >= timeout)
      {
        break;
      }

      this_wait= retry * retry / 3 + 1;
      sleep(this_wait);
#ifdef WIN32
      sleep(this_wait);
#else
      struct timespec global_sleep_value= { this_wait, 0 };
      nanosleep(&global_sleep_value, NULL);
#endif
    }
  }

  bool successful() const
  {
    return _successful;
  }

private:
  bool _successful;
};

} // namespace libtest
