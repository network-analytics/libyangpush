project(
    'yangpush',
    'c',
    version : '0.1',
    default_options: [
      'c_std=c99'
    ]
)
cc = meson.get_compiler('c')

project_description = 'libyangpush'

if meson.get_compiler('c').has_multi_arguments('-Wall', '-Wextra', '-pedantic', '-Werror')
    add_project_arguments('-Wall', language : ['c'])
    add_project_arguments('-Wextra', language : ['c'])
    add_project_arguments('-pedantic', language : ['c'])
endif

project_headers = [
  'src/libyangpush.h'
]

project_source_files = [
  'src/libyangpush.c',
  'src/tool.c'
]

project_test_files = [
  'test/test_find_dependency.c',
  'test/test_yangpush.c',
  'test/test_tool.c'
]

public_headers = include_directories('src')

libyang_dep = cc.find_library('libyang', 
  dirs: ['/usr/local/lib']
)

libcdada_dep = cc.find_library('libcdada', 
  dirs: ['/usr/local/lib']
)

project_dependencies = [
    dependency('LibXml2'),
    libyang_dep,
    libcdada_dep,
]

build_args = []

build_args += [
  '-DPROJECT_NAME=' + meson.project_name(),
  '-DPROJECT_VERSION=' + meson.project_version(),
]

project_target = shared_library(
  meson.project_name(),
  project_source_files,
  dependencies: project_dependencies,
  install : true,
  c_args : build_args,
)

project_dep = declare_dependency(
  include_directories: public_headers,
  link_with: project_target
)

# make this library usable from sysrem's package manager

install_headers(project_headers, subdir : meson.project_name())

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : meson.project_name(),
  filebase : meson.project_name(),
  description : project_description,
  subdirs : meson.project_name(),
  libraries : project_target,
)

# ==========
# Unit Tests
# ==========

test_dep = declare_dependency(
    dependencies: dependency('cmocka'),
)

test('test_tool',
  executable(
    'test_tool',
    files('test/test_tool.c'),
    dependencies: [project_dep, test_dep],
    install: false
  )
)

test('test_yangpush',
  executable(
    'test_yangpush',
    files('test/test_yangpush.c'),
    dependencies: [project_dependencies, project_dep, test_dep],
    install: false
  )
)

test('test_find_dependency',
  executable(
    'test_find_dependency',
    files('test/test_find_dependency.c'),
    dependencies: [project_dependencies, project_dep, test_dep],
    install: false
  )
)