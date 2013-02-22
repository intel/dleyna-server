AM_CFLAGS =	$(GLIB_CFLAGS)		\
		$(GIO_CFLAGS)		\
		$(DLEYNA_CORE_CFLAGS)	\
		-I$(top_builddir)/lib	\
		-include config.h

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
		
libexec_PROGRAMS = dleyna-server

dleyna_server_SOURCES =	daemon.c

dleyna_server_LDADD =	$(GLIB_LIBS)					\
			$(GIO_LIBS)					\
			$(DLEYNA_CORE_LIBS)				\
			$(top_builddir)/lib/libdleyna-server-1.0.la