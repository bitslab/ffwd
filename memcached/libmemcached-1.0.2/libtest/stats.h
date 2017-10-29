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

struct Stats {
  int32_t collection_success;
  int32_t collection_skipped;
  int32_t collection_failed;
  int32_t collection_total;

  uint32_t success;
  uint32_t skipped;
  uint32_t failed;
  uint32_t total;

  Stats() :
    collection_success(0),
    collection_skipped(0),
    collection_failed(0),
    collection_total(0),
    success(0),
    skipped(0),
    failed(0),
    total(0)
  { }
};

