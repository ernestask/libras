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

#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (RasDirectory, ras_directory, RAS, DIRECTORY, GObject)

void          ras_directory_add_file  (RasDirectory *directory,
                                       RasFile      *file);
GList        *ras_directory_get_files (RasDirectory *directory);
char         *ras_directory_get_name  (RasDirectory *directory,
                                       bool          replace_backslashes);

bool          ras_directory_is_root   (RasDirectory *directory);

RasDirectory *ras_directory_new       (const char *name,
                                       GDateTime  *creation_date_time);

G_END_DECLS
