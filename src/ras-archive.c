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
#include "ras-utils.h"

#include <stdbool.h>
#include <string.h>

#include <zlib.h>

#define RAS_HEADER_LENGTH 44

struct _RasArchive
{
    GObject parent_instance;

    GMappedFile *mapped_file;

    struct
    {
        int32_t seed;
        uint32_t file_count;
        uint32_t directory_count;
        uint32_t file_table_size;
        uint32_t directory_table_size;
        uint32_t archiver_version;
        int32_t reserved[3];
        uint32_t format_version;
    } header;

    GList *file_table;
    GHashTable *directory_table;
};

G_DEFINE_TYPE (RasArchive, ras_archive, G_TYPE_OBJECT)

static void
finalize (GObject *object)
{
    RasArchive *self;

    self = RAS_ARCHIVE (object);

    g_list_free_full (self->file_table, g_object_unref);
    self->file_table = NULL;

    g_clear_pointer (&self->directory_table, g_hash_table_destroy);

    (void) memset (&self->header, 0, sizeof (self->header));

    g_clear_pointer (&self->mapped_file, g_mapped_file_unref);

    G_OBJECT_CLASS (ras_archive_parent_class)->finalize (object);
}

static void
ras_archive_class_init (RasArchiveClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = finalize;
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

uint32_t
ras_archive_get_file_count (RasArchive *self)
{
    g_return_val_if_fail (RAS_IS_ARCHIVE (self), 0);

    return self->header.file_count;
}

uint32_t
ras_archive_get_directory_count (RasArchive *archive)
{
    g_return_val_if_fail (RAS_IS_ARCHIVE (archive), 0);

    return archive->header.directory_count;
}

uint32_t
ras_archive_get_format_version (RasArchive *archive)
{
    g_return_val_if_fail (RAS_IS_ARCHIVE (archive), 0);

    return archive->header.format_version;
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
populate_header (RasArchive  *archive,
                 GError     **error)
{
    const char *cursor;

    cursor = g_mapped_file_get_contents (archive->mapped_file);

    if (g_mapped_file_get_length (archive->mapped_file) < RAS_HEADER_LENGTH)
    {
        g_set_error_literal (error,
                             RAS_ARCHIVE_ERROR,
                             RAS_ERROR_TRUNCATED,
                             "Truncated file");

        return false;
    }

    if (g_strcmp0 (cursor, "RAS") != 0)
    {
        g_set_error_literal (error,
                             RAS_ARCHIVE_ERROR,
                             RAS_ERROR_INVALID_MAGIC,
                             "Not a RAS archive");

        return false;
    }

    archive->header.seed = GINT32_FROM_LE (((int32_t *) cursor)[1]);

    ras_decrypt_with_seed (36, (uint8_t *) cursor + 8, archive->header.seed);

    archive->header.file_count = GUINT32_FROM_LE (((uint32_t *) cursor)[2]);
    archive->header.directory_count = GUINT32_FROM_LE (((uint32_t *) cursor)[3]);
    archive->header.file_table_size = GUINT32_FROM_LE (((uint32_t *) cursor)[4]);
    archive->header.directory_table_size = GUINT32_FROM_LE (((uint32_t *) cursor)[5]);
    archive->header.archiver_version = GUINT32_FROM_LE (((uint32_t *) cursor)[6]);
    archive->header.reserved[0] = GINT32_FROM_LE (((int32_t *) cursor)[7]);
    archive->header.reserved[1] = GINT32_FROM_LE (((int32_t *) cursor)[8]);
    archive->header.reserved[2] = GINT32_FROM_LE (((int32_t *) cursor)[9]);
    archive->header.format_version = GUINT32_FROM_LE (((uint32_t *) cursor)[10]);

    return true;
}

static bool
populate_file_table (RasArchive  *archive,
                     GError     **error)
{
    size_t file_length;
    const char *cursor;
    const char *file_table_end;
    const char *data_section;

    g_assert (RAS_IS_ARCHIVE (archive));

    file_length = g_mapped_file_get_length (archive->mapped_file);
    if (RAS_HEADER_LENGTH + archive->header.file_table_size > file_length)
    {
        g_set_error_literal (error,
                             RAS_ARCHIVE_ERROR,
                             RAS_ERROR_TRUNCATED,
                             "Truncated file");

        return false;
    }
    cursor = g_mapped_file_get_contents (archive->mapped_file) + RAS_HEADER_LENGTH;
    file_table_end = cursor + archive->header.file_table_size;
    data_section = file_table_end + archive->header.directory_table_size;

    ras_decrypt_with_seed (archive->header.file_table_size,
                           (unsigned char *) cursor,
                           archive->header.seed);

    while (cursor < file_table_end)
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

        name = cursor;
        name_length = strlen (name);

        cursor += name_length + 1;

        size = GUINT32_FROM_LE (((uint32_t *) cursor)[0]);
        entry_size = GUINT32_FROM_LE (((uint32_t *) cursor)[1]);
        reserved0 = GUINT32_FROM_LE (((uint32_t *) cursor)[2]);
        parent_directory_index = GUINT32_FROM_LE (((uint32_t *) cursor)[3]);
        reserved1 = GUINT32_FROM_LE (((uint32_t *) cursor)[4]);
        compression_method = GUINT32_FROM_LE (((uint32_t *) cursor)[5]);

        cursor += 24;

        for (size_t i = 0; G_N_ELEMENTS (creation_time) > i; i++)
        {
            creation_time[i] = GUINT16_FROM_LE (((uint16_t *) cursor)[i]);
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

        cursor += sizeof (creation_time);

        file = ras_file_new (name,
                             size,
                             entry_size,
                             reserved0,
                             parent_directory_index,
                             reserved1,
                             compression_method,
                             creation_date_time,
                             data_section);

        archive->file_table = g_list_prepend (archive->file_table, file);

        directory = ras_archive_get_directory_by_index (archive, parent_directory_index);

        ras_directory_add_file (directory, file);

        data_section += entry_size;
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
populate_directory_table (RasArchive  *archive,
                          GError     **error)
{
    const char *cursor;
    const char *directory_table_end;

    g_assert (RAS_IS_ARCHIVE (archive));

    cursor = g_mapped_file_get_contents (archive->mapped_file);
    cursor += RAS_HEADER_LENGTH + archive->header.file_table_size;
    directory_table_end = cursor + archive->header.directory_table_size;

    ras_decrypt_with_seed (archive->header.directory_table_size,
                           (uint8_t *) cursor,
                           archive->header.seed);

    while (cursor < directory_table_end)
    {
        const char *name;
        size_t name_length;
        uint16_t creation_time[8];
        g_autoptr (GDateTime) creation_date_time = NULL;

        name = cursor;
        name_length = strlen (name);

        cursor += name_length + 1;

        for (size_t i = 0; G_N_ELEMENTS (creation_time) > i; i++)
        {
            creation_time[i] = GUINT16_FROM_LE (((uint16_t *) cursor)[i]);
        }

        creation_date_time = g_date_time_new_utc (creation_time[0],
                                                  creation_time[1],
                                                  creation_time[3],
                                                  creation_time[4],
                                                  creation_time[5],
                                                  (double) creation_time[6] + ((double) creation_time[7] / (double) 1000));

        if (NULL == creation_date_time)
        {
            if (!(1 == name_length && '\\' == *name))
            {
                g_set_error_literal (error,
                                     RAS_ARCHIVE_ERROR, RAS_ERROR_MALFORMED,
                                     "Malformed file");

                return false;
            }
        }

        insert_directory (archive,
                          ras_directory_new (name, creation_date_time));

        cursor += sizeof (creation_time);
    }

    return true;
}

RasArchive *
ras_archive_new_for_path (const char  *path,
                          GError     **error)
{
    RasArchive *archive;

    g_return_val_if_fail (NULL != path, NULL);

    archive = g_object_new (RAS_TYPE_ARCHIVE, NULL);

    archive->mapped_file = g_mapped_file_new (path, true, error);
    if (NULL == archive->mapped_file)
    {
        return NULL;
    }

    /* Technically, the order is header -> file table -> directory table,
     * but we need directories to be present when populating the file table,
     * as they are added to their corresponding directory objects.
     */

    if (!populate_header (archive, error))
    {
        return NULL;
    }

    if (!populate_directory_table (archive, error))
    {
        return NULL;
    }

    if (!populate_file_table (archive, error))
    {
        return NULL;
    }

    return archive;
}
