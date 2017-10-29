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


/*
  Structures for generic tests.
*/

#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <arpa/inet.h>

#include <libtest/visibility.h>
#include <libtest/version.h>

#include <libtest/error.h>
#include <libtest/strerror.h>
#include <libtest/stream.h>
#include <libtest/comparison.hpp>
#include <libtest/server.h>
#include <libtest/server_container.h>
#include <libtest/wait.h>
#include <libtest/callbacks.h>
#include <libtest/test.h>
#include <libtest/core.h>
#include <libtest/runner.h>
#include <libtest/port.h>
#include <libtest/is_local.hpp>
#include <libtest/socket.hpp>
#include <libtest/stats.h>
#include <libtest/collection.h>
#include <libtest/framework.h>
#include <libtest/get.h>
#include <libtest/stream.h>
#include <libtest/cmdline.h>
#include <libtest/string.hpp>
#include <libtest/binaries.h>
