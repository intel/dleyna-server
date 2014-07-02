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
dnl


AC_DEFUN([_DLEYNA_LOG_LEVEL_CHECK_VALUE],
[
	AS_CASE($1,
		[[[1-6]]], [AS_IF([test "x${log_unique}" = xyes],
				[
					AC_MSG_ERROR(["Log levels 0, 7 and 8 cannot be combined with other values"], 1)
				])
				let log_level_count++
			],

		[0|7|8], [AS_IF([test ${log_level_count} -ne 0],
				[
					AC_MSG_ERROR(["Log level $1 cannot be combined with other values"], 1)
				])
				log_unique=yes
			],
		[AC_MSG_ERROR(["$1 is not a valid value"], 1)]
	)
]
)

AC_DEFUN([DLEYNA_LOG_LEVEL_CHECK],
[
	AC_MSG_CHECKING([for --with-log-level=$1])

	old_IFS=${IFS}
	IFS=","

	log_ok=yes
	log_unique=no
	log_level_count=0
	LOG_LEVEL=0

	for log_level in $1
	do
		IFS=${old_IFS}
		_DLEYNA_LOG_LEVEL_CHECK_VALUE([$log_level])
		IFS=","
		log_name=LOG_LEVEL_${log_level}
		eval log_value=\$${log_name}
		let "LOG_LEVEL |= ${log_value}"
	done

	IFS=${old_IFS}

	AC_DEFINE_UNQUOTED([DLEYNA_LOG_LEVEL], [${LOG_LEVEL}], [Log level flag for debug messages])

	AC_MSG_RESULT([ok])
]
)

AC_DEFUN([DLEYNA_LOG_TYPE_CHECK],
[
	AC_MSG_CHECKING([for --with-log-type=$1])

	AS_CASE($1,
		[0|1], [],

		[AC_MSG_ERROR(["$1 is not a valid value"], 1)]
	)

	AC_MSG_RESULT([ok])
]
)
