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
  Common include file for libtest
*/

#pragma once

#include <config.h>

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <string>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H 
#include <sys/resource.h> 
#endif
 
#ifdef HAVE_FNMATCH_H
#include <fnmatch.h>
#endif

#include <libtest/test.hpp>

#include <libtest/is_pid.hpp>

#include <libtest/gearmand.h>
#include <libtest/blobslap_worker.h>
#include <libtest/memcached.h>

#include <libtest/libtool.hpp>
#include <libtest/killpid.h>
#include <libtest/stats.h>
#include <libtest/signal.h>

