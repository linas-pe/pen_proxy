#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Copyright (C) 2020 by linas<linas@justforfun.cn>

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

AC_PREREQ([2.69])

m4_include([pen.m4])

m4_define([v_major], [0]) # v_major = v_current - 1
m4_define([v_minor], [0])
m4_define([v_micro], [1])
PEN_INIT([pen_proxy], [source/main.c])

PEN_CHECK_HOST
PEN_DEFAULT_ARGS
PEN_INCLUDE_DIR
PEN_DEFAULT_FLAGS

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_MKDIR_P

# Checks for libraries.
PEN_CHECK_LIB_UTILS
PEN_CHECK_LIB_SOCKET

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_UINT16_T

# Checks for library functions.

# Export varible
PEN_EXPORT_VARS

AC_CONFIG_FILES([Makefile
                 source/Makefile])
AC_OUTPUT
