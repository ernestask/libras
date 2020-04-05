/* Copyright (C) 2019 Ernestas Kulik <ernestas DOT kulik AT gmail DOT com>
 *
 * This file is part of libras.
 *
 * libras is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libras is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libras.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

#define RAS_TYPE_BUFFER (ras_buffer_get_type ())
#define RAS_TYPE_DIRECTORY (ras_directory_get_type ())
#define RAS_TYPE_FILE (ras_file_get_type ())

typedef struct _RasBuffer RasBuffer;
typedef struct _RasDirectory RasDirectory;
typedef struct _RasFile RasFile;

G_END_DECLS
