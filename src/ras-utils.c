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

#include "ras-utils.h"

void
ras_decrypt_with_seed (size_t  size,
                       uint8_t buffer[static size],
                       int32_t seed)
{
    if (seed == 0)
    {
        seed = 1;
    }

    for (gsize i = 0; i < size; i++)
    {
        seed = (seed * 171) - ((seed / 177) * 30269);

        buffer[i] = (buffer[i] << (i % 5)) | (buffer[i] >> (8 - (i % 5)));
        buffer[i] = (buffer[i] ^ ((i + 3) * 6)) + (seed & 0xFF);
    }
}
