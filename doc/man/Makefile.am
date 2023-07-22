#
#    Copyright 2023 Nuovation System Designs, LLC. All Rights Reserved.
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
#    This file is the GNU automake template for the chkconfig in-package,
#    manual reference page documentation.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

if CHKCONFIG_BUILD_MAN
man8_MANS                    = \
   chkconfig.8                 \
   $(NULL)
else
man8_MANS                    = $(NULL)
endif # CHKCONFIG_BUILD_MAN

man8_TXTS                    = $(patsubst %.8,%.adoc,$(man8_MANS))

man8_XMLS                    = $(patsubst %.8,%.xml,$(man8_MANS))

EXTRA_DIST                   = \
    asciidoc.conf              \
    chkconfig.adoc             \
    manpage-base-url.xsl.in    \
    manpage-bold-literal.xsl   \
    manpage-normal.xsl         \
    $(NULL)

#
# Override autotool's default notion of the package version variables.
# This ensures that when we create a doc distribution that the
# version is always the current version, not the version when the
# package was bootstrapped.
#
PACKAGE_VERSION              = $(shell cat $(top_builddir)/.local-version)
VERSION                      = $(PACKAGE_VERSION)

MANPAGE_BASE_URL             = file://$(htmldir)/

CLEANFILES                   = \
  manpage-base-url.xsl         \
  $(NULL)

CLEANFILES                  += \
  $(man8_MANS)                 \
  $(man8_XMLS)                 \
  $(NULL)

#
# Local verbosity macros
#
CHKCONFIG_V_PROGRESS_ASCIIDOC  = $(NL_V_PROGRESS) "ASCIIDOC" "$(@)"
CHKCONFIG_V_PROGRESS_SED       = $(NL_V_PROGRESS) "SED"      "$(@)"
CHKCONFIG_V_PROGRESS_XMLTO     = $(NL_V_PROGRESS) "XMLTO"    "$(@)"

#
# Pattern rule to convert an AsciiDoc-formatted text file to a
# Docbook XML file.
#
%.xml: %.adoc $(srcdir)/asciidoc.conf $(srcdir)/Makefile.am
	$(CHKCONFIG_V_PROGRESS_ASCIIDOC)
	$(NL_V_AT)$(ASCIIDOC) \
	    --conf-file $(srcdir)/asciidoc.conf \
	    --attribute manversion=$(PACKAGE_VERSION) \
	    --attribute manmanual='chkconfig Manual' \
	    --attribute mansource='chkconfig' \
	    --backend docbook \
	    --doctype manpage \
	    -o $(@) $(<)

#
# Pattern rule to convert a Docbook XML file to a roff manual page.
#
%.8: %.xml manpage-base-url.xsl $(srcdir)/manpage-bold-literal.xsl $(srcdir)/manpage-normal.xsl
	$(CHKCONFIG_V_PROGRESS_XMLTO)
	$(NL_V_AT)$(XMLTO) \
	    -m $(srcdir)/manpage-normal.xsl \
	    -m $(srcdir)/manpage-bold-literal.xsl \
	    -m manpage-base-url.xsl \
	    man $(<)

manpage-base-url.xsl: manpage-base-url.xsl.in
	$(CHKCONFIG_V_PROGRESS_SED)
	$(NL_V_AT)$(SED) \
	    "s|@@MANPAGE_BASE_URL@@|$(MANPAGE_BASE_URL)|" \
	    $(<) > $(@)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
