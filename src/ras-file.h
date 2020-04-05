/* Copyright (C) 2018 Ernestas Kulik <ernestas DOT kulik AT gmail DOT com>
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

#include "ras-types.h"

#include <stdbool.h>
#include <stdint.h>

#include <gio/gio.h>

#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (RasFile, ras_file, RAS, FILE, GObject)

typedef enum
{
    RAS_FILE_COMPRESSION_METHOD_INVALID = -1,
    RAS_FILE_COMPRESSION_METHOD_COMPRESS = 1,
    RAS_FILE_COMPRESSION_METHOD_STORE = 3
} RasCompressionMethod;

RasCompressionMethod  ras_file_get_compression_method (RasFile               *file);
char                 *ras_file_get_name               (RasFile               *file);

bool                  ras_file_extract                (RasFile               *file,
                                                       GOutputStream         *stream,
                                                       GCancellable          *cancellable,
                                                       GError               **error);

RasFile              *ras_file_new                    (const char            *name,
                                                       uint32_t               size,
                                                       uint32_t               entry_size,
                                                       uint32_t               _,
                                                       uint32_t               parent_directory_index,
                                                       uint32_t               __,
                                                       RasCompressionMethod   compression_method,
                                                       GDateTime             *creation_date_time,
                                                       const char            *data);
