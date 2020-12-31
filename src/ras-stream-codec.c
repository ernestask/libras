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

#include "ras-stream-codec.h"

#include <iso646.h>

struct _RasStreamCodec
{
    GObject parent_instance;

    int32_t initial_seed;
    int32_t seed;
};

static GConverterResult
ras_stream_codec_convert (GConverter       *converter,
                          const void       *inbuf,
                          gsize             inbuf_size,
                          void             *outbuf,
                          gsize             outbuf_size,
                          GConverterFlags   flags,
                          gsize            *bytes_read,
                          gsize            *bytes_written,
                          GError          **error)
{
    RasStreamCodec *self;
    const uint8_t *inbuf_c;
    uint8_t *outbuf_c;

    self = RAS_STREAM_CODEC (converter);
    inbuf_c = inbuf;
    outbuf_c = outbuf;

    if (outbuf_size < inbuf_size)
    {
        return G_IO_ERROR_NO_SPACE;
    }

    for (gsize i = 0; i < inbuf_size; i++)
    {
        self->seed = (self->seed * 0xAB) - ((self->seed / 0xB1) * 0x763D);

        outbuf_c[i] = (inbuf_c[i] << (i % 5)) | (inbuf_c[i] >> (8 - (i % 5)));
        outbuf_c[i] = (outbuf_c[i] ^ ((i + 3) * 6)) + (self->seed & 0xFF);
    }

    if (NULL not_eq bytes_read)
    {
        *bytes_read = inbuf_size;
    }
    if (NULL not_eq bytes_written)
    {
        *bytes_written = inbuf_size;
    }

    if (G_CONVERTER_INPUT_AT_END == flags)
    {
        return G_CONVERTER_FINISHED;
    }

    return G_CONVERTER_CONVERTED;
}

static void
ras_stream_codec_reset (GConverter *converter)
{
    RasStreamCodec *self;

    self = RAS_STREAM_CODEC (converter);

    self->seed = self->initial_seed;
}

static void
ras_stream_codec_iface_init (GConverterIface *iface)
{
    iface->convert = ras_stream_codec_convert;
    iface->reset = ras_stream_codec_reset;
}

G_DEFINE_TYPE_WITH_CODE (RasStreamCodec, ras_stream_codec,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_CONVERTER,
                                                ras_stream_codec_iface_init))

static void
ras_stream_codec_class_init (RasStreamCodecClass *klass)
{
}

static void
ras_stream_codec_init (RasStreamCodec *self)
{
}

RasStreamCodec *
ras_stream_codec_new (int32_t seed)
{
    RasStreamCodec *codec;

    codec = g_object_new (RAS_TYPE_STREAM_CODEC, NULL);

    if (0 == seed)
    {
        seed = 1;
    }

    codec->initial_seed = seed;
    codec->seed = seed;

    return codec;
}
