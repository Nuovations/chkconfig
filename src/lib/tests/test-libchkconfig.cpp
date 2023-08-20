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


// MARK: Type Declarations

struct TestContext
{
    char mDefaultDirectory[PATH_MAX];
    char mStateDirectory[PATH_MAX];
};

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

/*
 * Utility (Origin)
 */
static void TestUtilityOrigin(nlTestSuite *inSuite, void *inContext __attribute__((unused)))
{
    static constexpr int kBadOrigin = -42;
    chkconfig_origin_t   lOrigin;
    const char *         lOriginString;
    chkconfig_status_t   lStatus;
    size_t               lLength;

    // 1.0. Negative Tests

    // 1.0.0. Ensure that passing a null string argument to
    //        chkconfig_origin_get_origin_string returns -EINVAL.

    lOrigin = CHKCONFIG_ORIGIN_UNKNOWN;

    lStatus = chkconfig_origin_get_origin_string(lOrigin, nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.1.0. Ensure that passing an invalid origin value to
    //        chkconfig_origin_get_origin_string returns -EINVAL.

    lOrigin = static_cast<chkconfig_origin_t>(kBadOrigin);

    lStatus = chkconfig_origin_get_origin_string(lOrigin, &lOriginString);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 2.0. Positive Tests

    // 2.0.0. Ensure that passing CHKCONFIG_ORIGIN_UNKNOWN to
    //        chkconfig_origin_get_origin_string succeeds and yields a
    //        non-null, non-zero length null-terminated C string
    //        pointer.

    lOrigin = CHKCONFIG_ORIGIN_UNKNOWN;

    lStatus = chkconfig_origin_get_origin_string(lOrigin, &lOriginString);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lOriginString != nullptr);

    lLength = strlen(lOriginString);
    NL_TEST_ASSERT(inSuite, lLength > 0);

    // 2.1.0. Ensure that passing CHKCONFIG_ORIGIN_NONE to
    //        chkconfig_origin_get_origin_string succeeds and yields a
    //        non-null, non-zero length null-terminated C string
    //        pointer.

    lOrigin = CHKCONFIG_ORIGIN_NONE;

    lStatus = chkconfig_origin_get_origin_string(lOrigin, &lOriginString);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lOriginString != nullptr);

    lLength = strlen(lOriginString);
    NL_TEST_ASSERT(inSuite, lLength > 0);

    // 2.2.0. Ensure that passing CHKCONFIG_ORIGIN_DEFAULT to
    //        chkconfig_origin_get_origin_string succeeds and yields a
    //        non-null, non-zero length null-terminated C string
    //        pointer.

    lOrigin = CHKCONFIG_ORIGIN_DEFAULT;

    lStatus = chkconfig_origin_get_origin_string(lOrigin, &lOriginString);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lOriginString != nullptr);

    lLength = strlen(lOriginString);
    NL_TEST_ASSERT(inSuite, lLength > 0);

    // 2.3.0. Ensure that passing CHKCONFIG_ORIGIN_STATE to
    //        chkconfig_origin_get_origin_string succeeds and yields a
    //        non-null, non-zero length null-terminated C string
    //        pointer.

    lOrigin = CHKCONFIG_ORIGIN_STATE;

    lStatus = chkconfig_origin_get_origin_string(lOrigin, &lOriginString);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lOriginString != nullptr);

    lLength = strlen(lOriginString);
    NL_TEST_ASSERT(inSuite, lLength > 0);
}

/*
 * Utility (State)
 */
static void TestUtilityState(nlTestSuite *inSuite, void *inContext __attribute__((unused)))
{
    static const char * const kOffString = "off";
    static const char * const kOnString= "on";
    chkconfig_state_t         lState;
    const char *              lStateString;
    chkconfig_status_t        lStatus;
    int                       lComparison;

    // 1.0. Negative Tests

    // 1.0.0. Ensure that passing a null string argument to
    //        chkconfig_state_string_get_state returns -EINVAL.

    lStatus = chkconfig_state_string_get_state(nullptr, &lState);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.1.0. Ensure that passing a null state argument to
    //        chkconfig_state_string_get_state returns -EINVAL.

    lStateString = kOffString;

    lStatus = chkconfig_state_string_get_state(lStateString, nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.2.0. Ensure that passing a null string argument to
    //        chkconfig_state_get_state_string returns -EINVAL.

    lState = true;

    lStatus = chkconfig_state_get_state_string(lState, nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.3.0. Ensure that passing an invalid state string to
    //        chkconfig_state_string_get_state returns -EINVAL.

    lStateString = "invalid";

    lStatus = chkconfig_state_string_get_state(lStateString, &lState);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 2.0. Positive Tests

    // 2.0.0. Ensure that passing "on" to
    //        chkconfig_state_string_get_state returns success and
    //        yields an asserted ('true') state.

    lStateString = kOnString;

    lStatus = chkconfig_state_string_get_state(lStateString, &lState);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lState == true);

    // 2.1.0. Ensure that passing "off" to
    //        chkconfig_state_string_get_state returns success and
    //        yields a deasserted ('false') state.

    lStateString = kOffString;

    lStatus = chkconfig_state_string_get_state(lStateString, &lState);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lState == false);

    // 2.2.0. Ensure that passing an asserted ('true') state to
    //        chkconfig_state_get_state_string returns success and
    //        yields the "on" string.

    lState = true;

    lStatus = chkconfig_state_get_state_string(lState, &lStateString);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lStateString != nullptr);

    lComparison = strcmp(lStateString, kOnString);
    NL_TEST_ASSERT(inSuite, lComparison == 0);

    // 2.3.0. Ensure that passing a deasserted ('false') state to
    //        chkconfig_state_get_state_string returns success and
    //        yields the "off" string.

    lState = false;

    lStatus = chkconfig_state_get_state_string(lState, &lStateString);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lStateString != nullptr);

    lComparison = strcmp(lStateString, kOffString);
    NL_TEST_ASSERT(inSuite, lComparison == 0);
}

/**
 *  Test Suite. It lists all the test functions.
 *
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Utility (State)",               TestUtilityState),
    NL_TEST_DEF("Utility (Origin)",              TestUtilityOrigin),

    NL_TEST_SENTINEL()
};

int main(void)
{
    TestContext theContext;
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
    nlTestRunner(&theSuite, &theContext);

    return (nlTestRunnerStats(&theSuite));
}
