#
#    Copyright (c) 2023 Nuovation System Designs, LLC. All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an "AS
#    IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
#    express or implied.  See the License for the specific language
#    governing permissions and limitations under the License.
#

##
#  @file
#    This file is the GNU automake template for the chkconfig command
#    line interface (CLI) binary.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

sbin_PROGRAMS                                                    = \
    chkconfig                                                      \
    $(NULL)

SUBDIRS                                                          = \
    tests                                                          \
    $(NULL)

chkconfig_LDADD                                                  = \
    $(top_builddir)/src/lib/libchkconfig.la                        \
    $(NULL)

chkconfig_CPPFLAGS                                               = \
    -DCHKCONFIG_DEFAULTDIR_DEFAULT="\"$(chkconfig_defaultdir)\""   \
    -DCHKCONFIG_STATEDIR_DEFAULT="\"$(chkconfig_statedir)\""       \
    -I$(top_srcdir)/src/include                                    \
    $(NULL)

chkconfig_SOURCES                                                = \
    chkconfig-main.cpp                                             \
    $(NULL)

NLFOREIGN_SUBDIR_DEPENDENCIES                                    = \
   ${top_builddir}/src/lib                                         \
   $(NULL)

$(sbin_PROGRAMS): $(NLFOREIGN_SUBDIR_DEPENDENCIES)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
