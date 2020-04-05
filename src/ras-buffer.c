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

#include "ras-buffer.h"

struct _RasBuffer
{
    GObject parent_instance;

    size_t capacity;
    GByteArray *data;
};

G_DEFINE_TYPE (RasBuffer, ras_buffer, G_TYPE_OBJECT)

enum
{
    FILLED,
    N_SIGNALS,
};

static uint32_t signals[N_SIGNALS];

static void
ras_buffer_init (RasBuffer *self)
{
    self->capacity = 0x1000;
    self->data = g_byte_array_sized_new (self->capacity);
}

static void
ras_buffer_finalize (GObject *object)
{
    RasBuffer *self;

    self = RAS_BUFFER (object);

    g_clear_pointer (&self->data, g_byte_array_unref);

    G_OBJECT_CLASS (ras_buffer_parent_class)->finalize (object);
}

static void
ras_buffer_class_init (RasBufferClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = ras_buffer_finalize;

    signals[FILLED] = g_signal_new ("filled",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    0,
                                    NULL, NULL,
                                    NULL,
                                    G_TYPE_NONE,
                                    0,
                                    NULL);
}

const uint8_t *
ras_buffer_get_data (RasBuffer *self,
                     uint16_t  *length)
{
    g_return_val_if_fail (RAS_IS_BUFFER (self), NULL);

    if (NULL != length)
    {
        *length = self->data->len;
    }

    return self->data->data;
}

void
ras_buffer_push_literal (RasBuffer *self,
                         uint8_t    literal)
{
    g_return_if_fail (RAS_IS_BUFFER (self));
    g_return_if_fail (self->data->len < self->capacity);

    g_byte_array_append (self->data, &literal, 1);

    if (self->data->len == self->capacity)
    {
        g_signal_emit (self, signals[FILLED], 0);

        g_byte_array_remove_range (self->data, 0, self->capacity);
    }
}

void
ras_buffer_push_pointer (RasBuffer *self,
                         uint16_t   pointer,
                         uint8_t    length)
{
    g_return_if_fail (RAS_IS_BUFFER (self));

    for (uint8_t i = 0; i < length; i++)
    {
        ras_buffer_push_literal (self, self->data->data[(pointer + 0x12 + i) % self->capacity]);
    }
}

RasBuffer *
ras_buffer_new (void)
{
    return g_object_new (RAS_TYPE_BUFFER, NULL);
}
