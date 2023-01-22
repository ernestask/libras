#include <locale.h>
#include <stdlib.h>

#include <ras-archive.h>
#include <ras-directory.h>
#include <ras-file.h>

static bool
decompress_file (RasFile  *entry,
                 bool      force,
                 GFile    *location,
                 GError  **error)
{
    g_autofree char *file_name = NULL;
    g_autoptr (GFile) file = NULL;
    g_autoptr (GFileOutputStream) stream = NULL;

    file_name = ras_file_get_name (entry);
    file = g_file_get_child (location, file_name);
    if (force)
    {
        stream = g_file_replace (file, NULL, false,
                                 G_FILE_CREATE_REPLACE_DESTINATION, NULL,
                                 error);
    }
    else
    {
        stream = g_file_create (file, G_FILE_CREATE_NONE, NULL, error);
    }
    if (NULL == stream)
    {
        return false;
    }

    g_message ("Extracting %s…", file_name);

    return ras_file_extract (entry, G_OUTPUT_STREAM (stream), NULL, error);
}

int
main (int    argc,
      char **argv)
{
    g_autoptr (GOptionContext) option_context = NULL;
    gboolean decompress = false;
    gboolean force = false;
    const char *only = NULL;
    const char *output_dir = "";
    g_auto (GStrv) files = NULL;
    const GOptionEntry option_entries[] =
    {
        {
            "decompress", 0, G_OPTION_FLAG_NONE,
            G_OPTION_ARG_NONE, &decompress,
            "Decompress FILE", NULL,

        },
        {
            "force", 'f', G_OPTION_FLAG_NONE,
            G_OPTION_ARG_NONE, &force,
            "Overwrite existing files", NULL,
        },
        {
            "only", 0, G_OPTION_FLAG_NONE,
            G_OPTION_ARG_STRING, &only,
            "Only process ENTRY", "ENTRY",
        },
        {
            "output-dir", 'O', G_OPTION_FLAG_NONE,
            G_OPTION_ARG_STRING, &output_dir,
            "Output files to DIR", "DIR",
        },
        {
            G_OPTION_REMAINING, 0, G_OPTION_FLAG_NONE,
            G_OPTION_ARG_FILENAME_ARRAY, &files,
            NULL, NULL,
        },
        {
            NULL, 0, 0,
            0, NULL,
            NULL, NULL,
        }
    };
    g_autoptr (GMappedFile) file = NULL;
    g_autoptr (GBytes) bytes = NULL;
    g_autoptr (RasArchive) archive = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GList) directory_table = NULL;

    setlocale (LC_ALL, "");

    option_context = g_option_context_new ("[FILE]");

    g_option_context_add_main_entries (option_context, option_entries, NULL);

    if (!g_option_context_parse (option_context, &argc, &argv, &error))
    {
        g_printerr ("%s\n", error->message);

        return EXIT_FAILURE;
    }

    if (NULL == files)
    {
        g_printerr ("No file specified\n");

        return EXIT_FAILURE;
    }

    file = g_mapped_file_new (files[0], false, &error);
    if (NULL == file)
    {
        g_printerr ("Failed to open archive: %s\n", error->message);

        return EXIT_FAILURE;
    }
    bytes = g_mapped_file_get_bytes (file);
    archive = ras_archive_load (bytes, &error);
    if (NULL == archive)
    {
        g_printerr ("Failed to load archive: %s\n", error->message);

        return EXIT_FAILURE;
    }
    directory_table = ras_archive_get_directory_table (archive);

    if (!decompress)
    {
        uint32_t file_count;
        uint32_t directory_count;
        g_autoptr (GList) directories = NULL;

        file_count = ras_archive_get_file_count (archive);
        directory_count = ras_archive_get_directory_count (archive);
        directories = ras_archive_get_directory_table (archive);
        directories = g_list_reverse (directories);

        g_print ("%s:\n"
                 "\t%u files in %u directories\n\n",
                 files[0], file_count, directory_count);

        for (GList *directory = directories; NULL != directory; directory = g_list_next (directory))
        {
            g_autofree char *directory_name = NULL;
            g_autoptr (GList) files = NULL;

            directory_name = ras_directory_get_name (directory->data, false);
            files = ras_directory_get_files (directory->data);
            files = g_list_reverse (files);

            g_print ("\t%s:\n", directory_name);

            for (GList *file = files; NULL != file; file = g_list_next (file))
            {
                g_autofree char *file_name = NULL;

                file_name = ras_file_get_name (file->data);

                g_print ("\t\t%s\n", file_name);
            }
        }

        return EXIT_SUCCESS;
    }

    if (decompress)
    {
        for (GList *d = directory_table; NULL != d; d = d->next)
        {
            g_autoptr (GList) file_table = NULL;
            g_autofree char *directory_name = NULL;
            g_autofree char *location = NULL;
            g_autoptr (GFile) directory = NULL;

            file_table = ras_directory_get_files (RAS_DIRECTORY (d->data));
            directory_name = ras_directory_get_name (RAS_DIRECTORY (d->data), true);
            location = g_build_path (G_DIR_SEPARATOR_S, output_dir, directory_name, NULL);
            directory = g_file_new_for_path (location);

            if (!ras_directory_is_root (RAS_DIRECTORY (d->data)))
            {
                if (!g_file_make_directory_with_parents (directory, NULL, &error))
                {
                    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
                    {
                        g_clear_error (&error);
                    }
                    else
                    {
                        g_printerr ("Failed to create directory %s: %s\n",
                                    directory_name, error->message);
                    }
                }
            }

            for (GList *f = file_table; NULL != f; f = f->next)
            {
                g_autofree char *file_name = NULL;

                file_name = ras_file_get_name (RAS_FILE (f->data));

                if (g_strcmp0 (only, file_name) == 0)
                {
                    g_debug ("Skipping %s…", file_name);

                    continue;
                }

                if (!decompress_file (RAS_FILE (f->data), force, directory, &error))
                {
                    g_printerr ("Failed to extract %s: %s\n",
                                file_name, error->message);

                    return EXIT_FAILURE;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
