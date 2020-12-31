/* Copyright (C) 2017 Ernestas Kulik <ernestas DOT kulik AT gmail DOT com>
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

#include <stdint.h>

#include <gio/gio.h>
#include <glib-object.h>

#define RAS_TYPE_ARCHIVE (ras_archive_get_type ())
#define RAS_ARCHIVE_ERROR (ras_archive_error_quark ())

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (RasArchive, ras_archive, RAS, ARCHIVE, GObject)

typedef enum
{
    RAS_ERROR_EMPTY,
    RAS_ERROR_INVALID_CHECKSUM,
    RAS_ERROR_INVALID_MAGIC,
    RAS_ERROR_MALFORMED,
    RAS_ERROR_TRUNCATED,
    RAS_ERROR_UNSUPPORTED_VERSION,
} RasErrorEnum;

RasDirectory *ras_archive_get_directory_by_index (RasArchive   *archive,
                                                  unsigned int  index);

size_t        ras_archive_get_file_count         (RasArchive *archive);
size_t        ras_archive_get_directory_count    (RasArchive *archive);

GList        *ras_archive_get_directory_table    (RasArchive *archive);
GList        *ras_archive_get_file_table         (RasArchive *archive);

RasArchive   *ras_archive_load                   (GBytes     *bytes,
                                                  GError     **error);

G_END_DECLS
