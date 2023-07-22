#
#    Copyright 2023 Nuovation System Designs, LLC. All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

##
#  @file
#    This file defines a GNU autoconf M4-style macro that adds an
#    --disable-man configuration option to the package and controls
#    whether the package will be built with or without manual
#    reference pages.
#

#
# CHKCONFIG_ENABLE_MAN(default)
#
#   default     - Whether the option should be automatic (auto), enabled
#                 (yes), disabled (no) by default.
#
# Adds an --disable-man configuration option to the package with a
# default value of 'default' (should be 'auto', 'no' or 'yes') and
# controls whether the package will be built with or without manual
# reference pages.
#
# The value 'chkconfig_cv_build_man' will be set to the result. In addition:
#
#   ASCIIDOC        - Will be set to the path of the AsciiDoc executable.
#   XMLTO           - Will be set to the path of the xmlto executable.
#
#------------------------------------------------------------------------------
AC_DEFUN([CHKCONFIG_ENABLE_MAN],
[
    # Check whether or not the 'default' value is sane.

    m4_case([$1],
        [auto],[],
        [yes],[],
        [no],[],
        [m4_fatal([$0: invalid default value '$1'; must be 'auto', 'yes' or 'no'])])

    AC_ARG_VAR(ASCIIDOC, [AsciiDoc executable])
    AC_ARG_VAR(XMLTO,    ['xmlto' executable])

    AC_PATH_PROG(ASCIIDOC, asciidoc)
    AC_PATH_PROG(XMLTO, xmlto)

    AC_CACHE_CHECK([whether to build manual reference pages],
        chkconfig_cv_build_man,
        [
	    AC_ARG_ENABLE(man,
		[AS_HELP_STRING([--disable-man],[Enable building manual reference pages (requires AsciiDoc and xmlto) @<:@default=$1@:>@.])],
		[
		    case "${enableval}" in 

		    auto|no|yes)
			chkconfig_cv_build_man=${enableval}
			;;

		    *)
			AC_MSG_ERROR([Invalid value ${enableval} for --disable-man])
			;;

		    esac
		],
		[chkconfig_cv_build_man=$1])

	    if test "x${ASCIIDOC}" != "x"; then
		chkconfig_cv_have_asciidoc=yes
	    else
		chkconfig_cv_have_asciidoc=no
	    fi

	    if test "${chkconfig_cv_build_man}" = "auto"; then
		if test "${chkconfig_cv_have_asciidoc}" = "no"; then
		    chkconfig_cv_build_man=no
		else
		    chkconfig_cv_build_man=yes
		fi
	    fi

	    if test "${chkconfig_cv_build_man}" = "yes"; then
		if test "${chkconfig_cv_have_asciidoc}" = "no"; then
		    AC_MSG_ERROR([Building manual reference pages was explicitly requested but AsciiDoc cannot be found])
		fi
	    fi
    ])
])
