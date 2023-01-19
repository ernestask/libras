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

#include "ras-archive.h"
#include "ras-directory.h"
#include "ras-file.h"
#include "ras-stream-codec.h"
#include "ras-utils.h"

#include <iso646.h>
#include <stdbool.h>
#include <string.h>
#include <zlib.h>

enum
{
    RAS_HEADER_OFFSET_MAGIC = 0x0,
    RAS_HEADER_OFFSET_ENCRYPTION_SEED = 0x4,
    RAS_HEADER_OFFSET_FILE_COUNT = 0x8,
    RAS_HEADER_OFFSET_DIRECTORY_COUNT = 0xC,
    RAS_HEADER_OFFSET_FILE_TABLE_SIZE = 0x10,
    RAS_HEADER_OFFSET_DIRECTORY_TABLE_SIZE = 0x14,
    RAS_HEADER_OFFSET_ARCHIVE_VERSION = 0x18,
    RAS_HEADER_OFFSET_HEADER_CHECKSUM = 0x1C,
    RAS_HEADER_OFFSET_FILE_TABLE_CHECKSUM = 0x20,
    RAS_HEADER_OFFSET_DIRECTORY_TABLE_CHECKSUM = 0x24,
    RAS_HEADER_OFFSET_FORMAT_VERSION = 0x28,
};

#define RAS_FORMAT_VERSION 3
#define RAS_HEADER_LENGTH 0x2C
#define RAS_MAGIC "RAS"
#define RAS_MAGIC_LENGTH (strlen (RAS_MAGIC) + 1)

struct _RasArchive
{
    GObject parent_instance;

    GBytes *bytes;

    GList *file_table;
    GHashTable *directory_table;
};

G_DEFINE_TYPE (RasArchive, ras_archive, G_TYPE_OBJECT)

static void
ras_archive_finalize (GObject *object)
{
    RasArchive *self;

    self = RAS_ARCHIVE (object);

    g_clear_list (&self->file_table, g_object_unref);
    g_clear_pointer (&self->directory_table, g_hash_table_destroy);

    g_clear_pointer (&self->bytes, g_bytes_unref);

    G_OBJECT_CLASS (ras_archive_parent_class)->finalize (object);
}

static void
ras_archive_class_init (RasArchiveClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = ras_archive_finalize;
}

static void
ras_archive_init (RasArchive *self)
{
    self->file_table = NULL;
    self->directory_table = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL, g_object_unref);
}

GQuark
ras_archive_error_quark (void)
{
    return g_quark_from_static_string ("ras-archive-error-quark");
}

RasDirectory *
ras_archive_get_directory_by_index (RasArchive   *self,
                                    unsigned int  index)
{
    const void *key;
    void *value;

    g_return_val_if_fail (RAS_IS_ARCHIVE (self), NULL);

    key = GUINT_TO_POINTER (index);
    value = g_hash_table_lookup (self->directory_table, key);

    return RAS_DIRECTORY (value);
}

size_t
ras_archive_get_file_count (RasArchive *self)
{
    g_return_val_if_fail (RAS_IS_ARCHIVE (self), 0);

    return g_list_length (self->file_table);
}

size_t
ras_archive_get_directory_count (RasArchive *self)
{
    g_return_val_if_fail (RAS_IS_ARCHIVE (self), 0);

    return g_hash_table_size (self->directory_table);
}

GList *
ras_archive_get_directory_table (RasArchive *self)
{
    g_return_val_if_fail (RAS_IS_ARCHIVE (self), NULL);

    return g_hash_table_get_values (self->directory_table);
}

GList *
ras_archive_get_file_table (RasArchive *archive)
{
    g_return_val_if_fail (RAS_IS_ARCHIVE (archive), NULL);

    return g_list_copy (g_list_reverse (archive->file_table));
}

static bool
populate_file_table (RasArchive     *archive,
                     const uint8_t  *data,
                     size_t          file_count,
                     const uint8_t  *file_data,
                     GError        **error)
{
    g_assert (RAS_IS_ARCHIVE (archive));
    g_assert (data not_eq NULL);

    for (size_t i = 0; i < file_count; i++)
    {
        const char *name;
        size_t name_length;
        uint32_t size;
        uint32_t entry_size;
        uint32_t reserved0;
        uint32_t parent_directory_index;
        uint32_t reserved1;
        uint32_t compression_method;
        uint16_t creation_time[8];
        g_autoptr (GDateTime) creation_date_time = NULL;
        RasFile *file;
        RasDirectory *directory;

        name = (const char *) data;
        name_length = strlen (name);

        data += name_length + 1;

        size = GUINT32_FROM_LE (((uint32_t *) data)[0]);
        entry_size = GUINT32_FROM_LE (((uint32_t *) data)[1]);
        reserved0 = GUINT32_FROM_LE (((uint32_t *) data)[2]);
        parent_directory_index = GUINT32_FROM_LE (((uint32_t *) data)[3]);
        reserved1 = GUINT32_FROM_LE (((uint32_t *) data)[4]);
        compression_method = GUINT32_FROM_LE (((uint32_t *) data)[5]);

        data += 24;

        for (size_t i = 0; G_N_ELEMENTS (creation_time) > i; i++)
        {
            creation_time[i] = GUINT16_FROM_LE (((uint16_t *) data)[i]);
        }

        creation_date_time = g_date_time_new_utc (creation_time[0],
                                                  creation_time[1],
                                                  creation_time[3],
                                                  creation_time[4],
                                                  creation_time[5],
                                                  (double) creation_time[6] + ((double) creation_time[7] / (double) 1000));
        if (NULL == creation_date_time)
        {
            g_set_error_literal (error, RAS_ARCHIVE_ERROR, RAS_ERROR_MALFORMED,
                                 "Malformed file");

            return false;
        }

        data += sizeof (creation_time);

        file = ras_file_new (name,
                             size,
                             entry_size,
                             reserved0,
                             parent_directory_index,
                             reserved1,
                             compression_method,
                             creation_date_time,
                             file_data);

        archive->file_table = g_list_prepend (archive->file_table, file);

        directory = ras_archive_get_directory_by_index (archive, parent_directory_index);

        ras_directory_add_file (directory, file);

        file_data += entry_size;
    }

    return true;
}

static void
insert_directory (RasArchive   *archive,
                  RasDirectory *directory)
{
    uint32_t table_size;
    void *key;

    g_assert (RAS_IS_ARCHIVE (archive));
    g_assert (RAS_IS_DIRECTORY (directory));

    table_size = g_hash_table_size (archive->directory_table);
    key = GUINT_TO_POINTER (table_size);

    g_assert (g_hash_table_insert (archive->directory_table, key, directory));
}

static bool
populate_directory_table (RasArchive     *archive,
                          const uint8_t  *data,
                          size_t          directory_count,
                          GError        **error)
{
    g_assert (RAS_IS_ARCHIVE (archive));

    for (size_t i = 0; i < directory_count; i++)
    {
        const char *name;
        size_t name_length;
        uint16_t creation_time[8];
        g_autoptr (GDateTime) creation_date_time = NULL;

        name = (const char *) data;
        name_length = strlen (name);

        data += name_length + 1;

        for (size_t i = 0; G_N_ELEMENTS (creation_time) > i; i++)
        {
            creation_time[i] = GUINT16_FROM_LE (((uint16_t *) data)[i]);
        }

        creation_date_time = g_date_time_new_utc (creation_time[0],
                                                  creation_time[1],
                                                  creation_time[3],
                                                  creation_time[4],
                                                  creation_time[5],
                                                  (double) creation_time[6] + ((double) creation_time[7] / (double) 1000));

        if (NULL == creation_date_time)
        {
            if (strcmp (name, "\\") not_eq 0)
            {
                g_set_error_literal (error,
                                     RAS_ARCHIVE_ERROR, RAS_ERROR_MALFORMED,
                                     "Malformed file");

                return false;
            }
        }

        g_debug ("Inserting directory to table: %s", name);

        insert_directory (archive,
                          ras_directory_new (name, creation_date_time));

        data += sizeof (creation_time);
    }

    return true;
}

RasArchive *
ras_archive_load (GBytes  *bytes,
                  GError **error)
{
    size_t size;
    const uint8_t *data;
    uint8_t header[RAS_HEADER_LENGTH];
    int32_t encryption_seed;
    g_autoptr (RasStreamCodec) codec = NULL;
    size_t bytes_read;
    size_t bytes_written;
    g_autoptr (RasArchive) archive = NULL;
    size_t file_count;
    size_t directory_count;
    size_t file_table_size;
    size_t directory_table_size;

    g_return_val_if_fail (bytes not_eq NULL, NULL);

    data = g_bytes_get_data (bytes, &size);

    if (size < RAS_HEADER_LENGTH)
    {
        g_set_error_literal (error,
                             RAS_ARCHIVE_ERROR,
                             RAS_ERROR_TRUNCATED,
                             "Truncated file");

        return NULL;
    }
    if (memcmp (data, RAS_MAGIC, RAS_MAGIC_LENGTH) not_eq 0)
    {
        g_set_error_literal (error,
                             RAS_ARCHIVE_ERROR,
                             RAS_ERROR_INVALID_MAGIC,
                             "Not a RAS archive");

        return NULL;
    }

    (void) memcpy (header, data, RAS_HEADER_LENGTH);

    encryption_seed = GINT32_FROM_LE (*((int32_t *) (data + RAS_HEADER_OFFSET_ENCRYPTION_SEED)));
    codec = ras_stream_codec_new (encryption_seed);

    g_converter_convert (G_CONVERTER (codec),
                         header + 8, 36,
                         (void *) header + 8, 36,
                         G_CONVERTER_INPUT_AT_END,
                         &bytes_read, &bytes_written,
                         NULL);

    if (GUINT32_FROM_LE (*(uint32_t *) (header + RAS_HEADER_OFFSET_FORMAT_VERSION)) not_eq RAS_FORMAT_VERSION)
    {
        g_set_error_literal (error,
                             RAS_ARCHIVE_ERROR,
                             RAS_ERROR_UNSUPPORTED_VERSION,
                             "Unsupported format version");

        return NULL;
    }

    {
        uint32_t checksum;
        uint32_t crc;

        checksum = GUINT32_FROM_LE (*((uint32_t *) (header + RAS_HEADER_OFFSET_HEADER_CHECKSUM)));

        *((uint32_t *) (header + RAS_HEADER_OFFSET_HEADER_CHECKSUM)) = 0;

        crc = crc32_z (0, Z_NULL, 0);
        crc = crc32_z (crc, header, RAS_HEADER_LENGTH);
        if (crc not_eq checksum)
        {
            g_set_error_literal (error,
                                 RAS_ARCHIVE_ERROR,
                                 RAS_ERROR_UNSUPPORTED_VERSION,
                                 "Invalid header checksum");

            return NULL;
        }
    }

    archive = g_object_new (RAS_TYPE_ARCHIVE, NULL);

    archive->bytes = g_bytes_ref (bytes);

    file_count = GUINT32_FROM_LE (*((uint32_t *) (header + RAS_HEADER_OFFSET_FILE_COUNT)));
    directory_count = GUINT32_FROM_LE (*((uint32_t *) (header + RAS_HEADER_OFFSET_DIRECTORY_COUNT)));
    file_table_size = GUINT32_FROM_LE (*((uint32_t *) (header + RAS_HEADER_OFFSET_FILE_TABLE_SIZE)));
    directory_table_size = GUINT32_FROM_LE (*((uint32_t *) (header + RAS_HEADER_OFFSET_DIRECTORY_TABLE_SIZE)));

    {
        g_autofree uint8_t *directory_table = NULL;
        uint32_t checksum;
        uint32_t crc;

        directory_table = malloc (directory_table_size);
        checksum = GUINT32_FROM_LE (*((uint32_t *) (header + RAS_HEADER_OFFSET_DIRECTORY_TABLE_CHECKSUM)));

        g_converter_reset (G_CONVERTER (codec));
        g_converter_convert (G_CONVERTER (codec),
                             data + RAS_HEADER_LENGTH + file_table_size, directory_table_size,
                             directory_table, directory_table_size,
                             G_CONVERTER_INPUT_AT_END,
                             &bytes_read, &bytes_written,
                             NULL);

        crc = crc32_z (0, Z_NULL, 0);
        crc = crc32_z (crc, directory_table, directory_table_size);
        if (crc not_eq checksum)
        {
            g_set_error_literal (error,
                                 RAS_ARCHIVE_ERROR,
                                 RAS_ERROR_UNSUPPORTED_VERSION,
                                 "Invalid directory table checksum");

            return NULL;
        }

        if (!populate_directory_table (archive, directory_table, directory_count, error))
        {
            return NULL;
        }
    }

    {
        g_autofree uint8_t *file_table = NULL;
        uint32_t checksum;
        uint32_t crc;
        const uint8_t *file_data;

        file_table = malloc (file_table_size);
        checksum = GUINT32_FROM_LE (*((uint32_t *) (header + RAS_HEADER_OFFSET_FILE_TABLE_CHECKSUM)));

        g_converter_reset (G_CONVERTER (codec));
        g_converter_convert (G_CONVERTER (codec),
                             data + RAS_HEADER_LENGTH, file_table_size,
                             file_table, file_table_size,
                             G_CONVERTER_INPUT_AT_END,
                             &bytes_read, &bytes_written,
                             NULL);

        crc = crc32_z (0, Z_NULL, 0);
        crc = crc32_z (crc, file_table, file_table_size);
        if (crc not_eq checksum)
        {
            g_set_error_literal (error,
                                 RAS_ARCHIVE_ERROR,
                                 RAS_ERROR_UNSUPPORTED_VERSION,
                                 "Invalid file table checksum");

            return NULL;
        }

        file_data = data + RAS_HEADER_LENGTH + file_table_size + directory_table_size;

        if (!populate_file_table (archive, file_table, file_count, file_data, error))
        {
            return NULL;
        }
    }

    return g_steal_pointer (&archive);
}
