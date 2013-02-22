AM_CFLAGS =	$(GLIB_CFLAGS)		\
		$(GIO_CFLAGS)		\
		$(DLEYNA_CORE_CFLAGS)	\
		-I$(top_builddir)/lib	\
		-include config.h

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
		
libexec_PROGRAMS = dleyna-server-service

dleyna_server_service_SOURCES =	daemon.c

dleyna_server_service_LDADD =	$(GLIB_LIBS)				    \
				$(GIO_LIBS)				    \
				$(DLEYNA_CORE_LIBS)			    \
				$(top_builddir)/lib/libdleyna-server-1.0.la

dms_info_sources = ../test/dbus/dms-info.c

noinst_PROGRAMS = dms-info
dms_info_SOURCES = $(dms_info_sources)

dms_info_CFLAGS =	$(GLIB_CFLAGS)	\
			$(GIO_CFLAGS)

dms_info_LDADD =	$(GLIB_LIBS)	\
			$(GIO_LIBS)
			
dbussessiondir = @DBUS_SESSION_DIR@
dbussession_DATA = com.intel.dleyna-server.service

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dleyna-server-service-1.0.pc

CLEANFILES = $(dbussession_DATA) $(pkgconfig_DATA)
DISTCLEANFILES = $(dbussession_DATA) $(pkgconfig_DATA)
