install_headers(
    files(
        'control-point-server.h'
    ),
    subdir: 'dleyna-1.0/libdleyna/server'
)

libdleyna_server = library(
    'dleyna-server-1.0',
    files(
        'async.c',
        'device.c',
        'manager.c',
        'path.c',
        'props.c',
        'search.c',
        'server.c',
        'sort.c',
        'task.c',
        'upnp.c',
        'xml-util.c'
    ),
    version: '1.0.3',
    dependencies : [
        glib,
        gio,
        dleyna_core,
        gupnp,
        gupnp_av,
        gupnp_dlna,
        soup,
        math,
        config_h
    ],
    install: true,
    install_dir: join_paths(get_option('prefix'), get_option('libdir'), 'dleyna-server')
)

# FIXME: Change the .in file to directly take the values we put in config.h
config_conf = configuration_data()
config_conf.set('with_connector_name', get_option('connector'))
config_conf.set('with_log_type', log_types[get_option('log_type')])
config_conf.set('with_log_level', ','.join(get_option('log_level')))
config_conf.set('never_quit', get_option('never_quit').to_string().to_lower())

configure_file(
    input: 'dleyna-server-service.conf.in',
    output: 'dleyna-server-service.conf',
    configuration: config_conf,
    install: true,
    install_dir: get_option('sysconfdir')
)

server_dep = declare_dependency (
    link_with: libdleyna_server,
    include_directories: include_directories('../..')
)

pkg.generate(
    libdleyna_server,
    name: 'dleyna-server-service-1.0',
    description: 'UPnP & DLNA server library',
    version: meson.project_version(),
    requires: ['gupnp-1.2', 'glib-2.0', 'gio-2.0', 'dleyna-core-1.0'],
    install_dir: join_paths(get_option('prefix'), get_option('libdir'), 'pkgconfig')
)

meson.override_dependency('dleyna-server-1.0', server_dep)
