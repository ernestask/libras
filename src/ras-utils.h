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

#ifndef RAS_UTILS_H_INCLUDED
#define RAS_UTILS_H_INCLUDED

#include <glib.h>

#include <stdint.h>

G_BEGIN_DECLS

/**
 * ras_decrypt_with_seed:
 * @size: size of the buffer to decrypt
 * @buffer: the buffer, holding the data to decrypt
 * @seed: encryption seed
 */
void ras_decrypt_with_seed (size_t        size,
                            unsigned char buffer[static size],
                            int32_t       seed);

G_END_DECLS

#endif
