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

#include "ras-buffer.h"
#include "ras-file.h"

#include <string.h>

#define CMPHEADER "RA->"

struct _RasFile
{
    GObject parent_instance;

    char *name;
    uint32_t size;
    uint32_t entry_size;
    uint32_t _;
    uint32_t parent_directory_index;
    uint32_t __;
    uint32_t compression_method;
    GDateTime *creation_date_time;

    const uint8_t *data;
};

G_DEFINE_TYPE (RasFile, ras_file, G_TYPE_OBJECT)

static void
finalize (GObject *object)
{
    RasFile *file;

    file = RAS_FILE (object);

    g_clear_pointer (&file->name, g_free);
    g_clear_pointer (&file->creation_date_time, g_date_time_unref);

    G_OBJECT_CLASS (ras_file_parent_class)->finalize (object);
}

static void
ras_file_class_init (RasFileClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = finalize;
}

static void
ras_file_init (RasFile *self)
{
    (void) self;
}

RasCompressionMethod
ras_file_get_compression_method (RasFile *self)
{
    g_return_val_if_fail (RAS_IS_FILE (self), RAS_FILE_COMPRESSION_METHOD_INVALID);

    return self->compression_method;
}

char *
ras_file_get_name (RasFile *self)
{
    g_return_val_if_fail (RAS_IS_FILE (self), NULL);

    return g_strdup (self->name);
}

static void
ras_file_write_out_decompression_buffer (RasBuffer *buffer,
                                         gpointer   user_data)
{
    GOutputStream *stream;
    uint16_t length;
    const uint8_t *data;
    g_autoptr (GError) error = NULL;

    stream = G_OUTPUT_STREAM (user_data);
    data = ras_buffer_get_data (buffer, &length);

    if (!g_output_stream_write_all (stream, data, length, NULL, NULL, &error))
    {
        g_message ("Error: %s", error->message);
    }
}

static bool
decompress (RasFile        *self,
            GOutputStream  *stream,
            GCancellable   *cancellable,
            GError        **error)
{
    g_autoptr (RasBuffer) buffer = NULL;

    g_return_val_if_fail (memcmp (self->data, CMPHEADER, strlen (CMPHEADER)) == 0, false);

    buffer = ras_buffer_new ();

    g_signal_connect (buffer, "filled",
                      G_CALLBACK (ras_file_write_out_decompression_buffer),
                      stream);

    for (const uint8_t *s = self->data + 12; self->data + self->entry_size > s; )
    {
        bool raw[8];
        uint8_t byte;

        byte = *(s++);

        for (int i = 0; i < 8; i++)
        {
            if (byte & (0x1 << i))
            {
                raw[i] = true;
            }
            else
            {
                raw[i] = false;
            }
        }

        for (int i = 0; i < 8 && self->data + self->entry_size > s; i++)
        {
            if (raw[i])
            {
                ras_buffer_push_literal (buffer, *s);

                s += 1;
            }
            else
            {
                uint16_t pointer;
                uint8_t length;

                pointer = (*(s + 1) & 0xF0) << 4  | (*s & 0xFF);
                length = (*(s + 1) & 0xF) + 3;

                ras_buffer_push_pointer (buffer, pointer, length);

                s += 2;
            }
        }
    }

    ras_file_write_out_decompression_buffer (buffer, stream);

    return true;
}

bool
ras_file_extract (RasFile        *self,
                  GOutputStream  *stream,
                  GCancellable   *cancellable,
                  GError        **error)
{
    g_return_val_if_fail (RAS_IS_FILE (self), false);
    g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), false);

    if (RAS_FILE_COMPRESSION_METHOD_STORE == self->compression_method)
    {
        return g_output_stream_write_all (stream, self->data, self->entry_size,
                                          NULL, cancellable, error);
    }
    else if (RAS_FILE_COMPRESSION_METHOD_COMPRESS == self->compression_method)
    {
        return decompress (self, stream, cancellable, error);
    }

    return true;
}

RasFile *
ras_file_new (const char           *name,
              uint32_t              size,
              uint32_t              entry_size,
              uint32_t              _,
              uint32_t              parent_directory_index,
              uint32_t              __,
              RasCompressionMethod  compression_method,
              GDateTime            *creation_date_time,
              const uint8_t        *data)
{
    RasFile *file;

    g_return_val_if_fail (NULL != name, NULL);
    g_return_val_if_fail (NULL != creation_date_time, NULL);

    file = g_object_new (RAS_TYPE_FILE, NULL);

    file->name = g_strdup (name);
    file->size = size;
    file->entry_size = entry_size;
    file->_ = _;
    file->parent_directory_index = parent_directory_index;
    file->__ = __;
    file->compression_method = compression_method;
    file->creation_date_time = g_date_time_ref (creation_date_time);
    file->data = data;

    return file;
}
