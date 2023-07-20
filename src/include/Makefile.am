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
#    This file is the GNU automake template for the chkconfig public
#    header directory.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

noinst_HEADERS = \
    chkconfig-assert.h \
    $(NULL)

BUILT_SOURCES := \
    chkconfig \
    chkconfig-version.h \
    $(NULL)

CLEANFILES    := \
    chkconfig \
    chkconfig-version.h \
    $(NULL)

define check-file-chkconfig-version.h
$(top_srcdir)/build/scripts/gen-chkconfig-version "`cat $(1)`" -o "$(2)"
endef # check-file-chkconfig-version.h

$(top_builddir)/.local-version:
	$(NL_V_PROGRESS) "MAKE" "$(@)"
	+$(NL_V_AT)$(MAKE) $(MFLAGS) -C "$(@D)" "$(@F)"

chkconfig-version.h: $(top_builddir)/.local-version
	$(call check-file,$(@))

chkconfig: ../lib
	$(nl-create-link)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
