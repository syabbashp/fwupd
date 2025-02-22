if get_option('plugin_tpm')
cargs = ['-DG_LOG_DOMAIN="FuPluginTpmEventlog"']

shared_module('fu_plugin_tpm_eventlog',
  fu_hash,
  sources : [
    'fu-plugin-tpm-eventlog.c',
    'fu-tpm-eventlog-common.c',
    'fu-tpm-eventlog-device.c',
    'fu-tpm-eventlog-parser.c',
  ],
  include_directories : [
    root_incdir,
    fwupd_incdir,
    fwupdplugin_incdir,
  ],
  install : true,
  install_dir: plugin_dir,
  link_with : [
    fwupdplugin,
    fwupd,
  ],
  c_args : cargs,
  dependencies : [
    plugin_deps,
    tpm2tss,
  ],
)

if get_option('tests')
  env = environment()
  env.set('G_TEST_SRCDIR', meson.current_source_dir())
  env.set('G_TEST_BUILDDIR', meson.current_build_dir())
  e = executable(
    'tpm-eventlog-self-test',
    fu_hash,
    sources : [
      'fu-self-test.c',
      'fu-tpm-eventlog-common.c',
      'fu-tpm-eventlog-device.c',
      'fu-tpm-eventlog-parser.c',
    ],
    include_directories : [
      root_incdir,
      fwupd_incdir,
      fwupdplugin_incdir,
    ],
    link_with : [
      fwupdplugin,
      fwupd,
    ],
    dependencies : [
      plugin_deps,
      tpm2tss,
    ],
    c_args : cargs
  )
  test('tpm-eventlog-self-test', e, env : env)  # added to installed-tests
endif

fwupdtpmevlog = executable(
  'fwupdtpmevlog',
  fu_hash,
  sources : [
    'fu-tpm-eventlog.c',
    'fu-tpm-eventlog-common.c',
    'fu-tpm-eventlog-parser.c',
  ],
  include_directories : [
    root_incdir,
    fwupd_incdir,
    fwupdplugin_incdir,
  ],
  dependencies : [
      plugin_deps,
      tpm2tss,
  ],
  link_with : [
    fwupd,
    fwupdplugin,
  ],
  install : true,
  install_dir : bindir
)

if get_option('man')
  configure_file(
    input : 'fwupdtpmevlog.1',
    output : 'fwupdtpmevlog.1',
    configuration : conf,
    install: true,
    install_dir: join_paths(mandir, 'man1'),
  )
endif

endif
