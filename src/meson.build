libras_headers = files(
  'ras-archive.h',
  'ras-buffer.h',
  'ras-directory.h',
  'ras-file.h',
  'ras-types.h',
)

libras_sources = files(
  'ras-archive.c',
  'ras-buffer.c',
  'ras-directory.c',
  'ras-file.c',
  'ras-utils.c',
  'ras-utils.h',
)

libras_dependencies = [
  gio,
  glib,
  zlib
]

libras = library(
  'ras', [
    libras_headers,
    libras_sources,
  ],
  dependencies: libras_dependencies
)

libras_dep = declare_dependency(
  include_directories: include_directories ('.'),
  link_with: libras,
  dependencies: libras_dependencies
)
