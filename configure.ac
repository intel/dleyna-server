AC_PREREQ([2.66])

AC_INIT([dleyna-server],
	[0.2.0],
	[https://github.com/01org/dleyna-server/issues/new],
	,
	[https://01.org/dleyna/])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([libdleyna/server/server.c])

AC_PREFIX_DEFAULT(/usr/local)

AM_INIT_AUTOMAKE([subdir-objects])

AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

DLEYNA_SERVER_COMPILER_FLAGS

# Checks for languages.
AC_LANG_C

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MKDIR_P

# Initialize libtool
# Disable generation of static libraries
LT_PREREQ([2.2.6])
LT_INIT([dlopen disable-static])
LT_LANG([C])

# Checks for libraries.
PKG_PROG_PKG_CONFIG(0.16)
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.28])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.28])
PKG_CHECK_MODULES([GSSDP], [gssdp-1.0 >= 0.13.2])
PKG_CHECK_MODULES([GUPNP], [gupnp-1.0 >= 0.20.3])
PKG_CHECK_MODULES([GUPNPAV], [gupnp-av-1.0 >= 0.11.5])
PKG_CHECK_MODULES([GUPNPDLNA], [gupnp-dlna-2.0 >= 0.9.4])
PKG_CHECK_MODULES([SOUP], [libsoup-2.4 >= 2.28.2])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h syslog.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT8_T
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strchr strrchr strstr])

# Define Log Level values
LOG_LEVEL_0=0x00
LOG_LEVEL_1=0x01
LOG_LEVEL_2=0x02
LOG_LEVEL_3=0x04
LOG_LEVEL_4=0x08
LOG_LEVEL_5=0x10
LOG_LEVEL_6=0x20
LOG_LEVEL_7=0x13
LOG_LEVEL_8=0x3F

AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL_DISABLED], [${LOG_LEVEL_0}], [Log level flag for disabled messages])
AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL_ERROR],    [${LOG_LEVEL_1}], [Log level flag for errors])
AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL_CRITICAL], [${LOG_LEVEL_2}], [Log level flag for critical messages])
AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL_WARNING],  [${LOG_LEVEL_3}], [Log level flag for warnings])
AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL_MESSAGE],  [${LOG_LEVEL_4}], [Log level flag for messages])
AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL_INFO],     [${LOG_LEVEL_5}], [Log level flag for informational messages])
AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL_DEBUG],    [${LOG_LEVEL_6}], [Log level flag for debug messages])
AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL_DEFAULT],  [${LOG_LEVEL_7}], [Log level flag to display default level messages])
AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL_ALL],      [${LOG_LEVEL_8}], [Log level flag for all messages])

AC_ARG_ENABLE(master-build,,
		[],
		[master_build=no])

AS_IF([test "x$master_build" = "xno"],
      [PKG_CHECK_MODULES([DLEYNA_CORE], [dleyna-core-1.0 >= 0.0.1])],
      [this_abs_top_srcdir=`cd "$srcdir" && pwd`;
       DLEYNA_CORE_CFLAGS="-I$this_abs_top_srcdir/../dleyna-core";
       DLEYNA_CORE_LIBS="-L$this_abs_top_srcdir/../dleyna-core/.libs -ldleyna-core-1.0"
      ])

AC_ARG_ENABLE(debug,
		AS_HELP_STRING(
			[--enable-debug],
			[enable compiling with debugging information]),
		[],
		[enable_debug=no])

AS_CASE("${enable_debug}",
	[yes], [CFLAGS="$CFLAGS -g";
		AC_DEFINE_UNQUOTED([DLEYNA_DEBUG_ENABLED],[1], [Compiling with debugging information enabled])
	       ],
	[no], [],
	[AC_MSG_ERROR([bad value ${enable_debug} for --enable-debug])])


AC_ARG_ENABLE(werror,
		AS_HELP_STRING(
			[--enable-werror],
			[warnings are treated as errors]),
		[],
		[enable_werror=no])

AS_CASE("${enable_werror}",
	[yes], [CFLAGS="$CFLAGS -Werror"],
	[no], [],
	[AC_MSG_ERROR([bad value ${enable_werror} for --enable-werror])])


AC_ARG_ENABLE(optimization,
		AS_HELP_STRING(
			[--disable-optimization],
			[disable code optimization through compiler]),
		[],
		[enable_optimization=yes])

AS_CASE("${enable_optimization}",
	[yes], [disable_optimization=no],
	[no], [CFLAGS="$CFLAGS -O0"; disable_optimization=yes],
	[AC_MSG_ERROR([bad value ${enable_optimization} for --enable-werror])])


AC_ARG_ENABLE(never-quit,
		AS_HELP_STRING(
			[--enable-never-quit],
			[Service doesn't quit when last client disconnects]),
		[],
		[enable_never_quit=no])

AS_CASE("${enable_never_quit}",
	[yes], [never_quit=true],
	[no], [never_quit=false],
	[AC_MSG_ERROR([bad value ${enable_never_quit} for --enable-never-quit])])


AC_ARG_WITH(connector-name,
		AS_HELP_STRING(
			[--with-connector-name],
			[IPC connector name]),
		[],
		[with_connector_name=dbus])


AC_ARG_WITH(log-level,
		AS_HELP_STRING(
			[--with-log-level],
			[enable logging information (0,1..6,7,8)\
			 0=disabled \
			 7=default (=1,2,5) \
			 8=all (=1,2,3,4,5,6) \
			 1,..,6=a comma separated list of log level\
			]),
		[],
		[with_log_level=7])

DLEYNA_LOG_LEVEL_CHECK([${with_log_level}])


AC_ARG_WITH(log-type,
		AS_HELP_STRING(
			[--with-log-type],
			[Select log output technology \
			 0=syslog  1=GLib \
			]),
		[],
		[with_log_type=0])

DLEYNA_LOG_TYPE_CHECK([${with_log_type}])


AC_ARG_WITH(ua-prefix,
		AS_HELP_STRING(
			[--with-ua-prefix],
			[Specify a user agent prefix]),
		[with_ua_prefix = "$withval"; AC_DEFINE_UNQUOTED([UA_PREFIX], "$with_ua_prefix", [User Agent prefix])],
		[])

AC_ARG_WITH(dbus_service_dir,
            AS_HELP_STRING([--with-dbus-service-dir=PATH],[choose directory for dbus service files, [default=PREFIX/share/dbus-1/services]]),
            with_dbus_service_dir="$withval", with_dbus_service_dir=$datadir/dbus-1/services)
DBUS_SERVICE_DIR=$with_dbus_service_dir
AC_SUBST(DBUS_SERVICE_DIR)


AC_ARG_ENABLE(lib-only,
		AS_HELP_STRING(
			[--enable-lib-only],
			[compile only the libdleyna-server library]),
		[],
		[enable_lib_only=no])

AM_CONDITIONAL([BUILD_SERVER], [test "x$enable_lib_only" = "xno"])

AC_DEFINE([DLEYNA_SERVER_OBJECT], "/com/intel/dLeynaServer", [Name of object exposed by dleyna-server])
AC_DEFINE([DLEYNA_SERVER_PATH], "/com/intel/dLeynaServer/server", [Path of server objects])

DLEYNA_SERVER_NAME=com.intel.dleyna-server
AC_SUBST(DLEYNA_SERVER_NAME)
AC_DEFINE([DLEYNA_SERVER_NAME], "com.intel.dleyna-server",
				 [d-Bus Name of dleyna-server])

DLEYNA_SERVER_INTERFACE_MANAGER=com.intel.dLeynaServer.Manager
AC_SUBST(DLEYNA_SERVER_INTERFACE_MANAGER)
AC_DEFINE([DLEYNA_SERVER_INTERFACE_MANAGER], "com.intel.dLeynaServer.Manager",
			       [d-Bus Name of dleyna-server main interface])

DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE=com.intel.dLeynaServer.MediaDevice
AC_SUBST(DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE)
AC_DEFINE([DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE], "com.intel.dLeynaServer.MediaDevice",
			       [d-Bus Name of dleyna-server device interface])

AC_SUBST([never_quit])
AC_SUBST([with_connector_name])
AC_SUBST([with_log_level])
AC_SUBST([with_log_type])

AC_CONFIG_FILES([Makefile					\
		 libdleyna/server/Makefile			\
		 libdleyna/server/dleyna-server-1.0.pc		\
		 libdleyna/server/dleyna-server-service.conf	\
		 server/dleyna-server-service-1.0.pc		\
		 server/Makefile				\
		 test/dbus/Makefile
		])

AC_OUTPUT

AS_ECHO(["-------------------------------------------------

${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Prefix    : '${prefix}'
Compiler  : '${CC}'
CFLAGS    : '${CFLAGS}'

-Package features:
	- enable-werror         : ${enable_werror}
	- enable-debug          : ${enable_debug}
	- enable-never-quit     : ${enable_never_quit}
	- with_connector-name   : ${with_connector_name}
	- disable-optimization  : ${disable_optimization}
	- with-log-level        : ${with_log_level}
	- with-log-type         : ${with_log_type}
	- with-ua-prefix        : ${with_ua_prefix}
	- enable-lib-only       : ${enable_lib_only}
	- with-dbus-service-dir : ${with_dbus_service_dir}

--------------------------------------------------"])
