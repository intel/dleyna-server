libdleyna_serverincdir = $(includedir)/dleyna-1.0/libdleyna/server

DLEYNA_SERVER_VERSION = 1:3:0

AM_CFLAGS =	$(GLIB_CFLAGS)				\
		$(GIO_CFLAGS)				\
		$(DLEYNA_CORE_CFLAGS)			\
		$(GSSDP_CFLAGS)				\
		$(GUPNP_CFLAGS)				\
		$(GUPNPAV_CFLAGS)			\
		$(GUPNPDLNA_CFLAGS)			\
		$(SOUP_CFLAGS)				\
		-include config.h

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

pkglib_LTLIBRARIES = libdleyna-server-1.0.la

libdleyna_serverinc_HEADERS = 	control-point-server.h

libdleyna_server_1_0_la_LDFLAGS =	-version-info $(DLEYNA_SERVER_VERSION) \
					-no-undefined

libdleyna_server_1_0_la_SOURCES =	$(libdleyna_serverinc_HEADERS)	\
					server.c		\
					async.c				\
					device.c	 		\
					manager.c	 		\
					path.c		 		\
					props.c		 		\
					search.c	 		\
					sort.c		 		\
					task.c		 		\
					upnp.c

libdleyna_server_1_0_la_LIBADD =	$(GLIB_LIBS)		\
					$(GIO_LIBS)		\
					$(DLEYNA_CORE_LIBS)	\
					$(GSSDP_LIBS)		\
					$(GUPNP_LIBS)		\
					$(GUPNPAV_LIBS) 	\
					$(GUPNPDLNA_LIBS) 	\
					$(SOUP_LIBS)

MAINTAINERCLEANFILES =	Makefile.in		\
			aclocal.m4		\
			configure		\
			config.h.in		\
			config.h.in~		\
			build-aux/depcomp	\
			build-aux/compile	\
			build-aux/missing	\
			build-aux/install-sh

sysconf_DATA = dleyna-server-service.conf

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dleyna-server-1.0.pc

EXTRA_DIST = 	$(sysconf_DATA)			\
		async.h				\
		client.h			\
		device.h			\
		interface.h			\
		manager.h			\
		path.h				\
		props.h				\
		search.h			\
		server.h			\
		sort.h				\
		task.h				\
		upnp.h

CLEANFILES = $(pkgconfig_DATA) dleyna-server-service.conf
DISTCLEANFILES = $(pkgconfig_DATA) dleyna-server-service.conf

maintainer-clean-local:
	rm -rf build-aux
