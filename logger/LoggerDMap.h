/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SL_LOGGER_DMAP_H
#define SL_LOGGER_DMAP_H

#include "Logger.h"

/*! @file
 * Provide runtime access to device specific functions based on defined string IDs
 */

//! Return device_callbacks structure for specified data source
device_callbacks dmap_getCallbacks(const char *source);

//! Get data source specific configuration handler
dc_parser dmap_getParser(const char *source);

#endif
