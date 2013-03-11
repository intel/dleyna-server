SUBDIRS = libdleyna/server

if BUILD_SERVER
SUBDIRS += server test/dbus
endif

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

MAINTAINERCLEANFILES =	Makefile.in		\
			aclocal.m4		\
			configure		\
			config.h.in		\
			config.h.in~		\
			build-aux/depcomp	\
			build-aux/compile	\
			build-aux/missing	\
			build-aux/install-sh

maintainer-clean-local:
	rm -rf build-aux
