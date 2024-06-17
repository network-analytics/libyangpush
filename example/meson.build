# ============
# Example code
# ============

project(
  'example_program',
  'c',
  default_options : ['warning_level=3']
)
cc = meson.get_compiler('c')

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

libnetconf_dep = cc.find_library('netconf2', 
  dirs: ['/usr/local/lib']
)

libyangpush_dep = cc.find_library('yangpush', 
  dirs: ['/usr/local/lib']
)

project_dependencies = [
  dependency('LibXml2'),
  dependency('jansson'),
  libcurl_dep,
  libyang_dep,
  libcdada_dep,
  libnetconf_dep,
  libyangpush_dep
]

project_target = executable(
  meson.project_name(),
  project_source_files,
  files('example.c'),
  dependencies: [project_dependencies],
  install: false
)