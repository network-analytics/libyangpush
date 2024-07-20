# ============
# Example code
# ============

project(
  'example_program',
  'c',
  default_options : ['warning_level=3']
)
cc = meson.get_compiler('c')
if meson.get_compiler('c').has_link_argument('-fsanitize=address')
    add_project_arguments('-fsanitize=address,undefined', language : ['c'])
    add_project_link_arguments('-fsanitize=address,undefined', language : ['c'])
endif
project_source_files = [
  'example.c'
]

# libyang_dep = cc.find_library('yang', 
#   dirs: ['/usr/lib']
# )

libnetconf2_dep = cc.find_library('netconf2', 
  dirs: ['/usr/lib']
)


libcdada_dep = cc.find_library('cdada', 
  dirs: ['/usr/local/lib']
)

libcurl_dep = cc.find_library('curl', 
  dirs: ['/usr/local/lib']
)

libyangpush_dep = cc.find_library('yangpush', 
  dirs: ['/usr/local/lib64']
)

project_dependencies = [
  dependency('LibXml2'),
  dependency('jansson'),
  # dependency('yangpush'),
  dependency('libyang'),
  # dependency('libnetconf2'),
  libcurl_dep,
  # libyang_dep,
  libcdada_dep,
  libnetconf2_dep,
  libyangpush_dep
]

project_target = executable(
  meson.project_name(),
  project_source_files,
  files('example.c'),
  dependencies: [project_dependencies],
  install: false
)