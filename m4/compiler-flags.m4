dnl
dnl dLeyna
dnl
dnl Copyright (C) 2012-2015 Intel Corporation. All rights reserved.
dnl
dnl This program is free software; you can redistribute it and/or modify it
dnl under the terms and conditions of the GNU Lesser General Public License,
dnl version 2.1, as published by the Free Software Foundation.
dnl
dnl This program is distributed in the hope it will be useful, but WITHOUT
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
dnl FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
dnl for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public License
dnl along with this program; if not, write to the Free Software Foundation, Inc.,
dnl 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
dnl
dnl Ludovic Ferrandis <ludovic.ferrandis@intel.com>
dnl Regis Merlino <regis.merlino@intel.com>
dnl

AC_DEFUN_ONCE([DLEYNA_SERVER_COMPILER_FLAGS], [
	if test x"${CFLAGS}" = x""; then
		CFLAGS="-Wall"
		AS_VAR_APPEND([CFLAGS], [" -O2"])
		AS_VAR_APPEND([CFLAGS], [" -D_FORTIFY_SOURCE=2"])
	fi

	if test x"$USE_MAINTAINER_MODE" = x"yes"; then
		AS_VAR_APPEND([CFLAGS], [" -Wextra"])
		AS_VAR_APPEND([CFLAGS], [" -Wno-unused-parameter"])
		AS_VAR_APPEND([CFLAGS], [" -Wno-missing-field-initializers"])
		AS_VAR_APPEND([CFLAGS], [" -Wdeclaration-after-statement"])
		AS_VAR_APPEND([CFLAGS], [" -Wmissing-declarations"])
		AS_VAR_APPEND([CFLAGS], [" -Wredundant-decls"])
		AS_VAR_APPEND([CFLAGS], [" -Wcast-align"])

		AS_VAR_APPEND([CFLAGS], [" -Wstrict-prototypes"])
		AS_VAR_APPEND([CFLAGS], [" -Wmissing-prototypes"])
		AS_VAR_APPEND([CFLAGS], [" -Wnested-externs"])
		AS_VAR_APPEND([CFLAGS], [" -Wshadow"])
		AS_VAR_APPEND([CFLAGS], [" -Wformat=2"])
		AS_VAR_APPEND([CFLAGS], [" -Winit-self"])

		AS_VAR_APPEND([CFLAGS], [" -std=gnu99"])
		AS_VAR_APPEND([CFLAGS], [" -pedantic"])
		AS_VAR_APPEND([CFLAGS], [" -Wno-overlength-strings"])

		AS_VAR_APPEND([CFLAGS], [" -DG_DISABLE_DEPRECATED"])
		AS_VAR_APPEND([CFLAGS], [" -DGLIB_DISABLE_DEPRECATION_WARNINGS"])
	fi

	AS_VAR_APPEND([CFLAGS], [" -Wno-format-extra-args"])
	AS_VAR_APPEND([CFLAGS], [" -Wl,--no-undefined"])
])
