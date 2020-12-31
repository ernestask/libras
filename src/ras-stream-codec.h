/* Copyright (C) 2020 Ernestas Kulik <ernestas AT baltic DOT engineering>
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

#include "ras-types.h"

#include <gio/gio.h>
#include <stdint.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (RasStreamCodec, ras_stream_codec, RAS, STREAM_CODEC, GObject)

RasStreamCodec *ras_stream_codec_new (int32_t seed);

G_END_DECLS
