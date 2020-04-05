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

#include "ras-types.h"

#include <stdint.h>

#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (RasBuffer, ras_buffer, RAS, BUFFER, GObject)

const uint8_t *ras_buffer_get_data     (RasBuffer     *buffer,
                                        uint16_t      *length);
void           ras_buffer_push_literal (RasBuffer     *buffer,
                                        uint8_t        literal);
void           ras_buffer_push_pointer (RasBuffer     *buffer,
                                        uint16_t       pointer,
                                        uint8_t        length);

RasBuffer     *ras_buffer_new          (void);

G_END_DECLS
