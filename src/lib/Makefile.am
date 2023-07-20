#
#    Copyright (c) 2023 Nuovation System Designs, LLC. All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

##
#  @file
#    This file is the GNU automake template for the chkconfig library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

lib_LTLIBRARIES                                                  = \
    libchkconfig.la                                                \
    $(NULL)

# Ensure that libraries are built before trying to run any recursive
# targets from SUBDIRS that might depend on them.

AM_RECURSIVE_TARGETS: $(LIBRARIES)

SUBDIRS                                                          = \
    tests                                                          \
    $(NULL)

noinst_HEADERS                                                   = \
    $(NULL)

# The 'install' target directory transform. Headers in
# libchkconfig_la_include_HEADERS will get installed in
# libchkconfig_la_includedir.

libchkconfig_la_includedir                                       = \
    ${includedir}/chkconfig

# Public library headers to distribute and install.

libchkconfig_la_include_HEADERS                                  = \
    chkconfig.h                                                    \
    $(NULL)

libchkconfig_la_LDFLAGS                                          = \
    -version_info $(LIBCHKCONFIG_VERSION_INFO)                     \
    $(NULL)

libchkconfig_la_CPPFLAGS                                         = \
    -DCHKCONFIG_DEFAULTDIR_DEFAULT="\"$(chkconfig_defaultdir)\""   \
    -DCHKCONFIG_STATEDIR_DEFAULT="\"$(chkconfig_statedir)\""       \
    -I$(top_srcdir)/src/include                                    \
    $(NULL)

libchkconfig_la_SOURCES                                          = \
    chkconfig.cpp                                                  \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
