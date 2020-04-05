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

#include "ras-directory.h"

#include "ras-file.h"

struct _RasDirectory
{
    GObject parent_instance;

    char *name;
    GDateTime *creation_date_time;

    GList *files;
};

G_DEFINE_TYPE (RasDirectory, ras_directory, G_TYPE_OBJECT)

static void
finalize (GObject *object)
{
    RasDirectory *directory;

    directory = RAS_DIRECTORY (object);

    g_clear_pointer (&directory->name, g_free);
    g_clear_pointer (&directory->creation_date_time, g_date_time_unref);

    G_OBJECT_CLASS (ras_directory_parent_class)->finalize (object);
}

static void
ras_directory_class_init (RasDirectoryClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = finalize;
}

static void
ras_directory_init (RasDirectory *self)
{
    self->files = NULL;
}

void
ras_directory_add_file (RasDirectory *self,
                        RasFile      *file)
{
    g_return_if_fail (RAS_IS_DIRECTORY (self));
    g_return_if_fail (RAS_IS_FILE (file));

    self->files = g_list_prepend (self->files, file);
}

GList *
ras_directory_get_files (RasDirectory *self)
{
    g_return_val_if_fail (RAS_IS_DIRECTORY (self), NULL);

    return g_list_copy (self->files);
}

char *
ras_directory_get_name (RasDirectory *self,
                        bool          replace_backslashes)
{
    g_return_val_if_fail (RAS_IS_DIRECTORY (self), NULL);

    if (replace_backslashes)
    {
        const char *name;
        g_autoptr (GRegex) regex = NULL;

        name = self->name;
        if (g_str_has_prefix (name, "\\"))
        {
            name++;
        }
        regex = g_regex_new ("\\\\", 0, 0, NULL);


        return g_regex_replace_literal (regex, name, -1, 0, "/", 0, NULL);
    }

    return g_strdup (self->name);
}

RasDirectory *
ras_directory_new (const char *name,
                   GDateTime  *creation_date_time)
{
    RasDirectory *directory;

    g_return_val_if_fail (NULL != name, NULL);

    directory = g_object_new (RAS_TYPE_DIRECTORY, NULL);

    directory->name = g_strdup (name);

    return directory;
}
