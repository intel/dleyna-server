AM_CFLAGS =	$(GLIB_CFLAGS)				\
		$(GIO_CFLAGS)				\
		$(DLEYNA_CORE_CFLAGS)			\
		-I$(top_builddir)/libdleyna/server	\
		-include config.h

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
		
libexec_PROGRAMS = dleyna-server-service

dleyna_server_service_SOURCES =	daemon.c

dleyna_server_service_LDADD =	$(GLIB_LIBS)				    			\
				$(GIO_LIBS)				    			\
				$(DLEYNA_CORE_LIBS)			    			\
				$(top_builddir)/libdleyna/server/libdleyna-server-1.0.la
			
dbusservicedir = $(DBUS_SERVICE_DIR)
dbusservice_in_files = com.intel.dleyna-server.service.in
dbusservice_DATA = com.intel.dleyna-server.service

# Replace the 'libexecdir' marker with its fully expanded value
%.service: %.service.in Makefile
	$(AM_V_GEN) $(SED) -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dleyna-server-service-1.0.pc

EXTRA_DIST = $(dbusservice_in_files)

CLEANFILES = $(dbusservice_DATA)
DISTCLEANFILES = $(dbusservice_DATA)
