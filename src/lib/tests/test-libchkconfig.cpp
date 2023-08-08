/*
 *    Copyright (c) 2023 Nuovation System Designs, LLC
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file implements unit tests for the chkconfig library interfaces.
 *
 */


#include <algorithm>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/stat.h>
#if defined(__APPLE__)
#include <sys/syslimits.h>
#endif

#include <nlunit-test.h>

#include <chkconfig/chkconfig.h>

#include "chkconfig-assert.h"


static int TestSuiteInitialize(void *inContext __attribute__((unused)))
{
    int lRetval  = 0;

    return (lRetval);
}

static int TestSuiteFinalize(void *inContext __attribute__((unused)))
{
    int lRetval  = 0;
    return (lRetval);
}

/**
 *  Test Suite. It lists all the test functions.
 *
 */
static const nlTest sTests[] = {

    NL_TEST_SENTINEL()
};

int main(void)
{
    nlTestSuite theSuite = {
        "libchkconfig",
        &sTests[0],
        TestSuiteInitialize,
        TestSuiteFinalize,
        nullptr,
        nullptr,
        0,
        0,
        0,
        0,
        0
    };

    // Generate human-readable output.
    nlTestSetOutputStyle(OUTPUT_DEF);

    // Run test suite against one context.
    nlTestRunner(&theSuite, nullptr);

    return (nlTestRunnerStats(&theSuite));
}
