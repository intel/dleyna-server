Introduction:
-------------

TODO

Compilation
------------

TODO

Working with the source code repository
---------------------------------------

dleyna-server can be downloaded, compiled and installed as
follows:

   Clone repository
     # git clone https://github.com/phako/dleyna-server.git
     # cd dleyna-server

   Configure and build
```
     # meson setup build
     # ninja -C build
```

   Final installation
```
     # sudo ninja -C build install
 ```

Configure Options:
------------------

`-Dlog_level`

See logging.txt for more information about logging.

`-Dconnector`

Set the IPC mechanism to be used.

`-Dbuild_server`

This option is enabled by default. To disable it use `-Dbuild_server=false`.
When disbled, only the libdleyna-server library is built.

`-user_agent_prefix`

This option allows a prefix to be added to the SOUP session user agent.
For example, `-Duser_agent_prefix=MyPrefix` can be used to change a default user
agent string from "dLeyna/0.0.1 GUPnP/0.19.4 DLNADOC/1.50" to
"MyPrefix dLeyna/0.0.1 GUPnP/0.19.4 DLNADOC/1.50".

`-Ddbus_service_dir`

By default, the dbus service files are installed in `$(datadir)/dbus-1/services.`
This option allows to choose another installation directory.

`-Dman_pages`

Whether to build the man pages for the server
