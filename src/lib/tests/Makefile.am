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
#    This file is the GNU automake template for the chkconfig library
#    unit tests.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#
# Local headers to build against and distribute but not to install
# since they are not part of the package.
#
noinst_HEADERS                                   = \
    $(NULL)

#
# Other files we do want to distribute with the package.
#
EXTRA_DIST                                       = \
    $(NULL)

if CHKCONFIG_BUILD_TESTS
# C preprocessor option flags that will apply to all compiled objects in this
# makefile.

AM_CPPFLAGS                                      = \
    -I$(top_srcdir)/src/include                    \
    $(NLUNIT_TEST_CPPFLAGS)                        \
    $(NULL)

AM_LDFLAGS                                       = \
    $(NLUNIT_TEST_LDFLAGS)                         \
    $(NULL)

LIBS                                            += \
    $(NLUNIT_TEST_LIBS)                            \
    $(NULL)

COMMON_LDADD                                     = \
    $(top_builddir)/src/lib/libchkconfig.la        \
    $(NULL)

NLFOREIGN_FILE_DEPENDENCIES                      = \
    $(top_builddir)/src/lib/libchkconfig.la        \
    $(NULL)

# Test applications that should be run when the 'check' target is run.

check_PROGRAMS                                   = \
    test-libchkconfig                              \
    $(NULL)

# Test applications and scripts that should be built and run when the
# 'check' target is run.

TESTS                                            = \
    $(check_PROGRAMS)                              \
    $(NULL)

# The additional environment variables and their values that will be
# made available to all programs and scripts in TESTS.

TESTS_ENVIRONMENT                                = \
    $(NULL)

# Source, compiler, and linker options for test programs.

test_libchkconfig_SOURCES                        = test-libchkconfig.cpp
test_libchkconfig_LDADD                          = $(COMMON_LDADD)

#
# Foreign make dependencies
#

NLFOREIGN_SUBDIR_DEPENDENCIES                         = \
   $(NLUNIT_TEST_FOREIGN_SUBDIR_DEPENDENCY)             \
   $(NULL)

if CHKCONFIG_BUILD_COVERAGE
CLEANFILES                                       = $(wildcard *.gcda *.gcno)

if CHKCONFIG_BUILD_COVERAGE_REPORTS
# The bundle should positively be qualified with the absolute build
# path. Otherwise, VPATH will get auto-prefixed to it if there is
# already such a directory in the non-colocated source tree.

CHKCONFIG_COVERAGE_BUNDLE                        = ${abs_builddir}/${PACKAGE}${NL_COVERAGE_BUNDLE_SUFFIX}
CHKCONFIG_COVERAGE_INFO                          = ${CHKCONFIG_COVERAGE_BUNDLE}/${PACKAGE}${NL_COVERAGE_INFO_SUFFIX}

$(CHKCONFIG_COVERAGE_BUNDLE):
	$(call create-directory)

# Generate the coverage report, filtering out platform and system
# directories and this test directory.

INCLUDE_PATHS                                   := $(subst -I,,$(sort $(NLASSERT_CPPFLAGS) $(NLUNIT_TEST_CPPFLAGS)))
INCLUDE_PATTERNS                                := $(if $(INCLUDE_PATHS),$(addsuffix *,$(INCLUDE_PATHS)),)

$(CHKCONFIG_COVERAGE_INFO): check | $(CHKCONFIG_COVERAGE_BUNDLE)
	$(call generate-coverage-report,${top_builddir}/src/lib,${top_builddir}/src/lib/tests ${INCLUDE_PATTERNS})

coverage: $(CHKCONFIG_COVERAGE_INFO)

clean-local: clean-local-coverage

.PHONY: clean-local-coverage
clean-local-coverage:
	-$(AM_V_at)rm -rf $(CHKCONFIG_COVERAGE_BUNDLE)
endif # CHKCONFIG_BUILD_COVERAGE_REPORTS
endif # CHKCONFIG_BUILD_COVERAGE
endif # CHKCONFIG_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
