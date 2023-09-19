# ============
# Example code
# ============

project(
  'example_program',
  'c',
)
cc = meson.get_compiler('c')

if meson.get_compiler('c').has_multi_arguments('-Wall', '-Wextra', '-pedantic', '-Werror')
  add_project_arguments('-Wall', language : ['c'])
  add_project_arguments('-Wextra', language : ['c'])
  add_project_arguments('-pedantic', language : ['c'])
endif

project_source_files = [
  'example.c'
]

libyang_dep = cc.find_library('yang', 
  dirs: ['/usr/local/lib']
)

libcdada_dep = cc.find_library('cdada', 
  dirs: ['/usr/local/lib']
)

libcurl_dep = cc.find_library('curl', 
  dirs: ['/usr/local/lib']
)

libnetconf_dep = cc.find_library('libnetconf2', 
  dirs: ['/usr/local/lib']
)

project_dependencies = [
  dependency('LibXml2'),
  dependency('jansson'),
  dependency('yangpush'),
  libcurl_dep,
  libyang_dep,
  libcdada_dep,
  libnetconf_dep,
]

project_target = executable(
  meson.project_name(),
  project_source_files,
  files('example.c'),
  dependencies: [project_dependencies],
  install: false
)