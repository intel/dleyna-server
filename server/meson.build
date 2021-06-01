server_service = executable(
    'dleyna-server-service',
    files('daemon.c'),
    dependencies : [
        glib,
        gio,
        dleyna_core,
        server_dep,
    ],
    install: true,
    install_dir: join_paths(get_option('prefix'), get_option('libexecdir')),
    install_rpath: join_paths(get_option('prefix'), get_option('libdir'), 'dleyna-server')
)

server_name = get_option('server_name')

dbus_conf = configuration_data()
dbus_conf.set('DLEYNA_SERVER_NAME', server_name)
dbus_conf.set('libexecdir', join_paths(get_option('prefix'), get_option('libexecdir')))

configure_file(
    input: 'com.intel.dleyna-server.service.in',
    output: server_name + '.service',
    configuration: dbus_conf,
    install: true,
    install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1/services')
)
