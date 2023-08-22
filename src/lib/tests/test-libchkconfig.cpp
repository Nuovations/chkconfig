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

/**
 *  @brief
 *    Return the number of elements in a C-style array.
 *
 *  @tparam  T  The type of the elements in the array.
 *  @tparam  N  The number of elements in the array.
 *
 *  @param[in]  aArray  A reference to the array.
 *
 *  @returns
 *    The number of elements in the C-style array.
 *
 */
template <typename T, size_t N>
inline constexpr size_t
ElementsOf(__attribute__((unused)) const T (&aArray)[N])
{
    return (N);
}

static int TestSuiteCreateDirectory(const char *inProgram, const char *inDescription, const size_t &inNameSize, char *outName)
{
    int    lStatus;
    char * lResult;
    int    lRetval = 0;

    lStatus = snprintf(outName,
                       inNameSize,
                       "%s-%s-XXXXXX",
                       inProgram,
                       inDescription);
    nlREQUIRE_ACTION(lStatus > 0,
                     done,
                     lRetval = -EOVERFLOW);
    nlREQUIRE_ACTION(static_cast<size_t>(lStatus) < inNameSize,
                     done,
                     lRetval = -EOVERFLOW);

    lResult = mkdtemp(outName);
    nlREQUIRE_ACTION(lResult != nullptr, done, lRetval = -errno);

 done:
    return (lRetval);
}

static int TestSuiteDestroyDirectory(const char *inPath)
{
    int lStatus;
    int lRetval = 0;

    lStatus = access(inPath, F_OK);
    nlREQUIRE_SUCCESS_ACTION(lStatus, done, lRetval = -errno);

    lStatus = rmdir(inPath);
    nlREQUIRE_SUCCESS_ACTION(lStatus, done, lRetval = -errno);

 done:
    return (-lRetval);
}

static chkconfig_status_t FlagPathCopy(const char *inDirectory,
                                       const chkconfig_flag_t &inFlag,
                                       const size_t &inPathSize,
                                       char *outPath)
{
    int                lStatus;
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inDirectory != nullptr, done, lRetval = -EINVAL);
    nlREQUIRE_ACTION(inFlag      != nullptr, done, lRetval = -EINVAL);
    nlREQUIRE_ACTION(inFlag[0]   != '\0',    done, lRetval = -EINVAL);
    nlREQUIRE_ACTION(inPathSize > 0,         done, lRetval = -EINVAL);
    nlREQUIRE_ACTION(outPath     != nullptr, done, lRetval = -EINVAL);

    lStatus = snprintf(&outPath[0],
                       inPathSize,
                       "%s/%s",
                       inDirectory,
                       inFlag);
    nlREQUIRE_ACTION(lStatus > 0,
                     done,
                     lRetval = -EOVERFLOW);
    nlREQUIRE_ACTION(static_cast<size_t>(lStatus) < inPathSize,
                     done,
                     lRetval = -EOVERFLOW);

 done:
    return (lRetval);
}

static chkconfig_status_t CreateBackingStoreFlag(const char *inDirectory,
                                                 const chkconfig_flag_t &inFlag,
                                                 const chkconfig_state_t &inState)
{
    constexpr int      lFlags = (O_WRONLY | O_TRUNC | O_CREAT);
    char               lFlagPath[PATH_MAX];
    int                lDescriptor;
    const char *       lStateString;
    int                lStatus;
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    lRetval = FlagPathCopy(inDirectory,
                           inFlag,
                           PATH_MAX,
                           &lFlagPath[0]);
    nlREQUIRE_SUCCESS(lRetval, done);

    lDescriptor = open(lFlagPath, lFlags, DEFFILEMODE);
    nlEXPECT_ACTION(lDescriptor != -1, done, lRetval = -errno);

    lRetval = chkconfig_state_get_state_string(inState, &lStateString);
    nlREQUIRE_SUCCESS(lRetval, done);

    lStatus = dprintf(lDescriptor, "%s\n", lStateString);
    nlREQUIRE_ACTION(lStatus > 0,
                     done,
                     lRetval = -EOVERFLOW);

 done:
    if (lDescriptor != -1)
    {
        lStatus = close(lDescriptor);
        nlVERIFY_ACTION(lStatus == 0, lRetval = -errno);
    }

    return (lRetval);
}

static chkconfig_status_t CreateBackingStoreFlag(const TestContext &inTestContext,
                                                 const chkconfig_flag_state_tuple_t &inFlagStateTuple)
{
    const char *       lDirectory = nullptr;
    chkconfig_status_t lRetval    = CHKCONFIG_STATUS_SUCCESS;

    switch (inFlagStateTuple.m_origin)
    {

    case CHKCONFIG_ORIGIN_DEFAULT:
        lDirectory = inTestContext.mDefaultDirectory;
        break;

    case CHKCONFIG_ORIGIN_STATE:
        lDirectory = inTestContext.mStateDirectory;
        break;

    default:
        lRetval = -EINVAL;
        break;

    }

    if (lDirectory != nullptr)
    {
        lRetval = CreateBackingStoreFlag(lDirectory,
                                         inFlagStateTuple.m_flag,
                                         inFlagStateTuple.m_state);
        nlREQUIRE_SUCCESS(lRetval, done);
    }

 done:
    return (lRetval);
}

static chkconfig_status_t DestroyBackingStoreFlag(const char *inDirectory,
                                                  const chkconfig_flag_t &inFlag)
{
    char               lFlagPath[PATH_MAX];
    int                lStatus;
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    lRetval = FlagPathCopy(inDirectory,
                           inFlag,
                           PATH_MAX,
                           &lFlagPath[0]);
    nlREQUIRE_SUCCESS(lRetval, done);

    lStatus = unlink(lFlagPath);
    nlREQUIRE_SUCCESS_ACTION(lStatus,
                             done,
                             lRetval = -errno;
                             fprintf(stderr, "Failed to remove \"%s\": %d: %s\n",
                                     lFlagPath,
                                     lRetval,
                                     strerror(-lRetval)));

 done:
    return (lRetval);
}

static chkconfig_status_t DestroyBackingStoreFlag(const TestContext &inTestContext,
                                                  const chkconfig_flag_state_tuple_t &inFlagStateTuple)
{
    const char *       lDirectory = nullptr;
    chkconfig_status_t lRetval    = CHKCONFIG_STATUS_SUCCESS;

    switch (inFlagStateTuple.m_origin)
    {

    case CHKCONFIG_ORIGIN_DEFAULT:
        lDirectory = inTestContext.mDefaultDirectory;
        break;

    case CHKCONFIG_ORIGIN_STATE:
        lDirectory = inTestContext.mStateDirectory;
        break;

    default:
        lRetval = -EINVAL;
        break;

    }

    if (lDirectory != nullptr)
    {
        lRetval = DestroyBackingStoreFlag(lDirectory,
                                          inFlagStateTuple.m_flag);
        nlREQUIRE_SUCCESS(lRetval, done);
    }

 done:
    return (lRetval);
}

static chkconfig_status_t CreateBackingStoreFlags(const TestContext &inTestContext,
                                                  const chkconfig_flag_state_tuple_t * inFlagStateTupleFirst,
                                                  const chkconfig_flag_state_tuple_t * inFlagStateTupleLast)
{
    const chkconfig_flag_state_tuple_t * lFlagStateTupleCurrent = inFlagStateTupleFirst;
    chkconfig_status_t                   lRetval                = CHKCONFIG_STATUS_SUCCESS;

    while (lFlagStateTupleCurrent != inFlagStateTupleLast)
    {
        lRetval = CreateBackingStoreFlag(inTestContext, *lFlagStateTupleCurrent);
        nlREQUIRE_SUCCESS(lRetval, done);

        lFlagStateTupleCurrent++;
    }

 done:
    return (lRetval);
}

static chkconfig_status_t DestroyBackingStoreFlags(const TestContext &inTestContext,
                                                   const chkconfig_flag_state_tuple_t * inFlagStateTupleFirst,
                                                   const chkconfig_flag_state_tuple_t * inFlagStateTupleLast)
{
    const chkconfig_flag_state_tuple_t * lFlagStateTupleCurrent = inFlagStateTupleFirst;
    chkconfig_status_t                   lRetval                = CHKCONFIG_STATUS_SUCCESS;

    while (lFlagStateTupleCurrent != inFlagStateTupleLast)
    {
        lRetval = DestroyBackingStoreFlag(inTestContext, *lFlagStateTupleCurrent);
        nlREQUIRE_SUCCESS(lRetval, done);

        lFlagStateTupleCurrent++;
    }

 done:
    return (lRetval);
}

static int TestSuiteInitialize(void *inContext)
{
    static const char * const kProgram = "test-libchkconfig";
    TestContext *             lContext = static_cast<TestContext *>(inContext);
    int                       lStatus;
    int                       lRetval  = 0;

    lStatus = TestSuiteCreateDirectory(kProgram,
                                       "default",
                                       PATH_MAX,
                                       &lContext->mDefaultDirectory[0]);
    nlREQUIRE_SUCCESS_ACTION(lStatus, done, lRetval = FAILURE);

    lStatus = TestSuiteCreateDirectory(kProgram,
                                       "state",
                                       PATH_MAX,
                                       &lContext->mStateDirectory[0]);
    nlREQUIRE_SUCCESS_ACTION(lStatus, done, lRetval = FAILURE);

 done:
    return (lRetval);
}

static int TestSuiteFinalize(void *inContext)
{
    TestContext * lContext = static_cast<TestContext *>(inContext);
    int           lStatus;
    int           lRetval  = 0;

    lStatus = TestSuiteDestroyDirectory(&lContext->mDefaultDirectory[0]);
    nlREQUIRE_SUCCESS_ACTION(lStatus, done, lRetval = FAILURE);

    lStatus = TestSuiteDestroyDirectory(&lContext->mStateDirectory[0]);
    nlREQUIRE_SUCCESS_ACTION(lStatus, done, lRetval = FAILURE);

 done:
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

/*
 * Utility (Tuples Lifetime)
 */
static void TestUtilityTuplesLifetime(nlTestSuite *inSuite, void *inContext __attribute__((unused)))
{
    chkconfig_flag_state_tuple_t * lFlagStateTuples;
    size_t                         lFlagStateTuplesCount;
    chkconfig_status_t             lStatus;

    // 1.0. Negative Tests

    // 1.0.0. Ensure that passing a null tuples argument to
    //        chkconfig_flag_state_tuples_init returns -EINVAL.

    lFlagStateTuplesCount = 1;

    lStatus = chkconfig_flag_state_tuples_init(nullptr, lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.1.0. Ensure that passing a zero count argument to
    //        chkconfig_flag_state_tuples_init returns -EINVAL.

    lFlagStateTuplesCount = 0;

    lStatus = chkconfig_flag_state_tuples_init(&lFlagStateTuples, lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.2.0. Ensure that passing a null tuples argument to
    //        chkconfig_flag_state_tuples_destroy returns -EINVAL.

    lFlagStateTuplesCount = 1;

    lStatus = chkconfig_flag_state_tuples_destroy(nullptr, lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.3.0. Ensure that passing a zero count argument to
    //        chkconfig_flag_state_tuples_destroy returns -EINVAL.

    lFlagStateTuples      = new chkconfig_flag_state_tuple_t;
    NL_TEST_ASSERT(inSuite, lFlagStateTuples != nullptr);

    lFlagStateTuplesCount = 0;

    lStatus = chkconfig_flag_state_tuples_destroy(lFlagStateTuples, lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    delete lFlagStateTuples;

    // 2.0. Positive Tests

    // 2.0.0. Ensure that passing a valid pointer and size to
    //        chkconfig_flag_state_tuples_init succeeds and yields a non-null
    //        result.

    lFlagStateTuples      = nullptr;
    lFlagStateTuplesCount = 7;

    lStatus = chkconfig_flag_state_tuples_init(&lFlagStateTuples, lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lFlagStateTuples != nullptr);

    // 2.0.0. Ensure that passing a valid pointer and size to
    //        chkconfig_flag_state_tuples_destroy succeeds and yields a
    //        non-null result.

    lStatus = chkconfig_flag_state_tuples_destroy(lFlagStateTuples, lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
}

/*
 * Utility (Tuples Compare)
 */
static void TestUtilityTuplesCompare(nlTestSuite *inSuite, void *inContext __attribute__((unused)))
{
    static constexpr chkconfig_flag_state_tuple_t kTupleFirst  = { "a", true,  CHKCONFIG_ORIGIN_UNKNOWN };
    static constexpr chkconfig_flag_state_tuple_t kTupleSecond = { "a", false, CHKCONFIG_ORIGIN_UNKNOWN };
    static constexpr chkconfig_flag_state_tuple_t kTupleThird  = { "b", true,  CHKCONFIG_ORIGIN_UNKNOWN };
    int                                           lComparison;

    // 1.0. Compare by flag.

    // 1.1. Ensure the first example compares by flag equal to the
    //      second example.

    lComparison = chkconfig_flag_state_tuple_flag_compare_function(&kTupleFirst,
                                                                   &kTupleSecond);
    NL_TEST_ASSERT(inSuite, lComparison == 0);

    // 1.2. Ensure the first example compares by flag less than the
    //      third example.

    lComparison = chkconfig_flag_state_tuple_flag_compare_function(&kTupleFirst,
                                                                   &kTupleThird);
    NL_TEST_ASSERT(inSuite, lComparison < 0);

    // 1.3. Ensure the third example compares by flag greater than the
    //      first example.

    lComparison = chkconfig_flag_state_tuple_flag_compare_function(&kTupleThird,
                                                                   &kTupleFirst);
    NL_TEST_ASSERT(inSuite, lComparison > 0);

    // 1.4. Ensure the third example compares by flag equal to itself.

    lComparison = chkconfig_flag_state_tuple_flag_compare_function(&kTupleThird,
                                                                   &kTupleThird);
    NL_TEST_ASSERT(inSuite, lComparison == 0);

    // 2.0. Compare by state.

    // 2.1. Ensure the first example compares by state equal to
    //      itself.

    lComparison = chkconfig_flag_state_tuple_state_compare_function(&kTupleFirst,
                                                                    &kTupleFirst);
    NL_TEST_ASSERT(inSuite, lComparison == 0);

    // 2.2. Ensure the first example compares by state less than the
    //      second example.

    lComparison = chkconfig_flag_state_tuple_state_compare_function(&kTupleFirst,
                                                                    &kTupleSecond);
    NL_TEST_ASSERT(inSuite, lComparison < 0);

    // 2.3. Ensure the first example compares by state less than the
    //      third example.

    lComparison = chkconfig_flag_state_tuple_state_compare_function(&kTupleFirst,
                                                                    &kTupleThird);
    NL_TEST_ASSERT(inSuite, lComparison < 0);

    // 2.3. Ensure the second example compares by state greater than the
    //      third example.

    lComparison = chkconfig_flag_state_tuple_state_compare_function(&kTupleSecond,
                                                                    &kTupleThird);
    NL_TEST_ASSERT(inSuite, lComparison > 0);
}

/*
 * Context Lifetime Management
 */
static void TestContextLifetimeManagement(nlTestSuite *inSuite, void *inContext __attribute__((unused)))
{
    chkconfig_status_t          lStatus;
    chkconfig_context_pointer_t lContextPointer;

    // 1.0. Negative Tests

    // 1.0.0. Ensure that passing a null argument to chkconfig_init
    //        returns -EINVAL.

    lStatus = chkconfig_init(nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.1.0. Ensure that passing a null argument to chkconfig_destroy
    //        returns -EINVAL.

    lStatus = chkconfig_destroy(nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.1.1. Ensure that passing a null value to chkconfig_destroy
    //        returns -EINVAL.

    lContextPointer = nullptr;

    lStatus = chkconfig_destroy(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // MARK: 2.0. Positive Tests

    // 2.0.0. Ensure that passing a valid pointer to chkconfig_init
    //        succeeds and yields a non-null result.

    lStatus = chkconfig_init(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lContextPointer != nullptr);

    // 2.1.0. Ensure that passing a valid pointer to chkconfig_destroy
    //        succeeds and yields a non-null result.

    lStatus = chkconfig_destroy(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    // Test Finalization

    lStatus = chkconfig_init(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lContextPointer != nullptr);
}

/*
 * Options Lifetime Management
 */
static void TestOptionsLifetimeManagement(nlTestSuite *inSuite, void *inContext __attribute__((unused)))
{
    chkconfig_status_t          lStatus;
    chkconfig_context_pointer_t lContextPointer;
    chkconfig_options_pointer_t lOptionsPointer;

    // Test Initialization

    lStatus = chkconfig_init(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lContextPointer != nullptr);

    // 1.0. Negative Tests

    // 1.0.0. Ensure that passing a null context pointer argument to
    //        chkconfig_options_init returns -EINVAL.

    lStatus = chkconfig_options_init(nullptr, &lOptionsPointer);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.1. Ensure that passing a null options pointer argument to
    //        chkconfig_options_init returns -EINVAL.

    lStatus = chkconfig_options_init(lContextPointer, nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.1.0. Ensure that passing a null context pointer argument to
    //        chkconfig_options_destroy returns -EINVAL.

    lStatus = chkconfig_options_destroy(nullptr, &lOptionsPointer);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.1.1. Ensure that passing a null options pointer argument to
    //        chkconfig_options_destroy returns -EINVAL.

    lStatus = chkconfig_options_destroy(lContextPointer, nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.1.1. Ensure that passing a null options value argument to
    //        chkconfig_options_destroy returns -EINVAL.

    lOptionsPointer = nullptr;

    lStatus = chkconfig_options_destroy(lContextPointer, &lOptionsPointer);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 2.0. Positive Tests

    // 2.0.0. Ensure that passing a valid context and options pointer
    //        to chkconfig_options_init succeeds and yields a non-null
    //        result.

    lStatus = chkconfig_options_init(lContextPointer, &lOptionsPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lOptionsPointer != nullptr);

    // 2.1.0. Ensure that passing a valid context and options pointer
    //        to chkconfig_options_destroy succeeds and yields a non-null
    //        result.

    lStatus = chkconfig_options_destroy(lContextPointer, &lOptionsPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    // Test Finalization

    lStatus = chkconfig_destroy(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
}

/*
 * Options Mutation
 */
static void TestOptionsMutation(nlTestSuite *inSuite, void *inContext)
{
    TestContext *               lTestContext = static_cast<TestContext *>(inContext);
    chkconfig_status_t          lStatus;
    chkconfig_context_pointer_t lContextPointer;
    chkconfig_options_pointer_t lOptionsPointer;
    chkconfig_option_t          lOption;

    // Test Initialization

    lStatus = chkconfig_init(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lContextPointer != nullptr);

    lStatus = chkconfig_options_init(lContextPointer, &lOptionsPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lOptionsPointer != nullptr);

    // 1.0. Negative Tests

    lOption = CHKCONFIG_OPTION_FORCE_STATE;

    // 1.0.0. Ensure that passing a null context pointer argument to
    //        chkconfig_options_set returns -EINVAL.

    lStatus = chkconfig_options_set(nullptr,
                                    lOptionsPointer,
                                    lOption,
                                    true);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.1. Ensure that passing a null options pointer argument to
    //        chkconfig_options_set returns -EINVAL.

    lStatus = chkconfig_options_set(lContextPointer,
                                    nullptr,
                                    lOption,
                                    true);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.2. Ensure that passing an invalid option to
    //        chkconfig_options_set returns -EINVAL.

    lOption = _CHKCONFIG_OPTION_ENCODE(_CHKCONFIG_OPTION_TYPE_BOOLEAN, 0),

    lStatus = chkconfig_options_set(lContextPointer,
                                    lOptionsPointer,
                                    lOption,
                                    true);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 2.0. Positive Tests

    // 2.0.0. Ensure that CHKCONFIG_OPTION_STATE_DIRECTORY can be
    //        successfully set.

    lOption = CHKCONFIG_OPTION_STATE_DIRECTORY;

    lStatus = chkconfig_options_set(lContextPointer,
                                    lOptionsPointer,
                                    lOption,
                                    &lTestContext->mStateDirectory[0]);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    // 2.0.1. Ensure that CHKCONFIG_OPTION_FORCE_STATE can be
    //        successfully.

    lOption = CHKCONFIG_OPTION_FORCE_STATE;

    // 2.0.1.0. Ensure that CHKCONFIG_OPTION_FORCE_STATE can be
    //          successfully set to true.

    lStatus = chkconfig_options_set(lContextPointer,
                                    lOptionsPointer,
                                    lOption,
                                    true);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    // 2.0.1.1. Ensure that CHKCONFIG_OPTION_FORCE_STATE can be
    //          successfully set to false.

    lStatus = chkconfig_options_set(lContextPointer,
                                    lOptionsPointer,
                                    lOption,
                                    false);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    // 2.0.2. Ensure that CHKCONFIG_OPTION_DEFAULT_DIRECTORY can be
    //        successfully set.

    lOption = CHKCONFIG_OPTION_DEFAULT_DIRECTORY;

    lStatus = chkconfig_options_set(lContextPointer,
                                    lOptionsPointer,
                                    lOption,
                                    &lTestContext->mDefaultDirectory[0]);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    // 2.0.3. Ensure that CHKCONFIG_OPTION_USE_DEFAULT_DIRECTORY can be
    //        successfully.

    lOption = CHKCONFIG_OPTION_USE_DEFAULT_DIRECTORY;

    // 2.0.3.0. Ensure that CHKCONFIG_OPTION_USE_DEFAULT_DIRECTORY can
    //          be successfully set to true.

    lStatus = chkconfig_options_set(lContextPointer,
                                    lOptionsPointer,
                                    lOption,
                                    true);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    // 2.0.3.1. Ensure that CHKCONFIG_OPTION_USE_DEFAULT_DIRECTORY can
    //          be successfully set to false.

    lStatus = chkconfig_options_set(lContextPointer,
                                    lOptionsPointer,
                                    lOption,
                                    false);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    // Test Finalization

    lStatus = chkconfig_options_destroy(lContextPointer, &lOptionsPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    lStatus = chkconfig_destroy(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
}

/*
 * Flag Observation w/o Backing Store Flags
 */
static void TestFlagObservationWithoutBackingStoreFlags(nlTestSuite *inSuite,
                                                        chkconfig_context_pointer_t &inContextPointer)
{
    static const char * const      kFlagFirst   = "test-a";
    chkconfig_status_t             lStatus;
    chkconfig_state_t              lState;
    chkconfig_origin_t             lOrigin;
    chkconfig_flag_state_tuple_t * lFlagStateTuples;
    size_t                         lFlagStateTuplesCount;

    // 2.0.0.0. Ensure that one or more arbitrary flags with no backing
    //          store return deasserted ('false') state with
    //          CHKCONFIG_ORIGIN_NONE.

    lStatus = chkconfig_state_get(inContextPointer,
                                  kFlagFirst,
                                  &lState);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lState == false);

    lStatus = chkconfig_state_get_with_origin(inContextPointer,
                                              kFlagFirst,
                                              &lState,
                                              &lOrigin);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lState == false);
    NL_TEST_ASSERT(inSuite, lOrigin == CHKCONFIG_ORIGIN_NONE);

    // 2.0.0.1. Ensure that chkconfig_state_get_count and
    //          chkconfig_state_copy_all return zero and an empty set with no
    //          backing store flags.

    lStatus = chkconfig_state_get_count(inContextPointer,
                                        &lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lFlagStateTuplesCount == 0);

    lStatus = chkconfig_state_copy_all(inContextPointer,
                                       &lFlagStateTuples,
                                       &lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lFlagStateTuplesCount == 0);
}

/*
 * Flag Observation w/ Backing Store Flags
 */
static void TestFlagObservationWithBackingStoreFlags(nlTestSuite *inSuite,
                                                     chkconfig_context_pointer_t &inContextPointer,
                                                     const chkconfig_flag_state_tuple_t * inExpectedFlagStateTupleFirst,
                                                     const chkconfig_flag_state_tuple_t * inExpectedFlagStateTupleLast)
{
    const chkconfig_flag_state_tuple_t * lExpectedFlagStateTupleCurrent = inExpectedFlagStateTupleFirst;
    const size_t                         lExpectedFlagStateTuplesCount = std::distance(inExpectedFlagStateTupleFirst, inExpectedFlagStateTupleLast);
    chkconfig_flag_state_tuple_t *       lActualFlagStateTuples = nullptr;
    size_t                               lActualFlagStateTuplesCount;
    const chkconfig_flag_state_tuple_t * lActualFlagStateTupleCurrent;
    const chkconfig_flag_state_tuple_t * lActualFlagStateTupleLast;
    chkconfig_state_t                    lState;
    chkconfig_origin_t                   lOrigin;
    chkconfig_status_t                   lStatus;

    // 2.0.0.0. For each flag/state tuple, ensure that the flag
    //          returns the expected state and origin.

    while (lExpectedFlagStateTupleCurrent != inExpectedFlagStateTupleLast)
    {
        lStatus = chkconfig_state_get(inContextPointer,
                                      lExpectedFlagStateTupleCurrent->m_flag,
                                      &lState);
        NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
        NL_TEST_ASSERT(inSuite, lState == lExpectedFlagStateTupleCurrent->m_state);

        lStatus = chkconfig_state_get_with_origin(inContextPointer,
                                                  lExpectedFlagStateTupleCurrent->m_flag,
                                                  &lState,
                                                  &lOrigin);
        NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
        NL_TEST_ASSERT(inSuite, lState == lExpectedFlagStateTupleCurrent->m_state);
        NL_TEST_ASSERT(inSuite, lOrigin == lExpectedFlagStateTupleCurrent->m_origin);

        lExpectedFlagStateTupleCurrent++;
    }

    // 2.0.0.1. Ensure that chkconfig_state_get_count and
    //          chkconfig_state_copy_all return the expected count and
    //          set of backing store flags.

    lStatus = chkconfig_state_get_count(inContextPointer,
                                        &lActualFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lActualFlagStateTuplesCount == lExpectedFlagStateTuplesCount);

    lStatus = chkconfig_state_copy_all(inContextPointer,
                                       &lActualFlagStateTuples,
                                       &lActualFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lActualFlagStateTuplesCount == lExpectedFlagStateTuplesCount);

    // There is no sorting implied for chkconfig_state_copy_all. So,
    // first sort the returned tuples by flag as the primary sort key.

    qsort(&lActualFlagStateTuples[0],
          lActualFlagStateTuplesCount,
          sizeof(chkconfig_flag_state_tuple_t),
          chkconfig_flag_state_tuple_flag_compare_function);

    // With the tuples sorted by flag, we can now accurately
    // comparethe actual results with the expected results.

    lExpectedFlagStateTupleCurrent = inExpectedFlagStateTupleFirst;
    lActualFlagStateTupleCurrent   = lActualFlagStateTuples;
    lActualFlagStateTupleLast      = lActualFlagStateTupleCurrent + lActualFlagStateTuplesCount;

    while ((lExpectedFlagStateTupleCurrent != inExpectedFlagStateTupleLast) &&
           (lActualFlagStateTupleCurrent   != lActualFlagStateTupleLast))
    {
        NL_TEST_ASSERT(inSuite, strcmp(lExpectedFlagStateTupleCurrent->m_flag,
                                       lActualFlagStateTupleCurrent->m_flag) == 0);
        NL_TEST_ASSERT(inSuite, (lExpectedFlagStateTupleCurrent->m_state ==
                                 lActualFlagStateTupleCurrent->m_state));
        NL_TEST_ASSERT(inSuite, (lExpectedFlagStateTupleCurrent->m_origin ==
                                 lActualFlagStateTupleCurrent->m_origin));

        lExpectedFlagStateTupleCurrent++;
        lActualFlagStateTupleCurrent++;
    }

    if (lActualFlagStateTuples != nullptr)
    {
        chkconfig_flag_state_tuples_destroy(lActualFlagStateTuples,
                                            lActualFlagStateTuplesCount);
    }
}

static void TestFlagObservationWithBackingStoreFlags(nlTestSuite *inSuite,
                                                     TestContext &inTestContext,
                                                     chkconfig_context_pointer_t &inContextPointer,
                                                     const chkconfig_flag_state_tuple_t * inInputFlagStateTupleFirst,
                                                     const chkconfig_flag_state_tuple_t * inInputFlagStateTupleLast,
                                                     const chkconfig_flag_state_tuple_t * inExpectedFlagStateTupleFirst,
                                                     const chkconfig_flag_state_tuple_t * inExpectedFlagStateTupleLast)
{
    chkconfig_status_t lStatus;

    lStatus = CreateBackingStoreFlags(inTestContext,
                                      inInputFlagStateTupleFirst,
                                      inInputFlagStateTupleLast);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    TestFlagObservationWithBackingStoreFlags(inSuite,
                                             inContextPointer,
                                             inExpectedFlagStateTupleFirst,
                                             inExpectedFlagStateTupleLast);

    lStatus = DestroyBackingStoreFlags(inTestContext,
                                       inInputFlagStateTupleFirst,
                                       inInputFlagStateTupleLast);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
}

static void TestNegativeFlagObservation(nlTestSuite *inSuite,
                                        chkconfig_context_pointer_t &inContextPointer)
{
    static const char * const      kFlagFirst = "test-a";
    chkconfig_status_t             lStatus;
    chkconfig_state_t              lState;
    chkconfig_origin_t             lOrigin;
    chkconfig_flag_state_tuple_t * lFlagStateTuples = nullptr;
    size_t                         lFlagStateTuplesCount;

    // 1.0.0. Ensure that passing a null context pointer argument to
    //        any observer returns -EINVAL.

    // 1.0.0.0. Ensure that passing a null context pointer argument to
    //          chkconfig_state_get returns -EINVAL.

    lStatus = chkconfig_state_get(nullptr,
                                  kFlagFirst,
                                  &lState);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.0.1. Ensure that passing a null context pointer argument to
    //          chkconfig_state_get_with_origin returns -EINVAL.

    lStatus = chkconfig_state_get_with_origin(nullptr,
                                              kFlagFirst,
                                              &lState,
                                              &lOrigin);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.0.2. Ensure that passing a null context pointer argument to
    //          chkconfig_state_get_multiple returns -EINVAL.

    lFlagStateTuples      = new chkconfig_flag_state_tuple_t;
    NL_TEST_ASSERT(inSuite, lFlagStateTuples != nullptr);

    lFlagStateTuplesCount = 1;

    lStatus = chkconfig_state_get_multiple(nullptr,
                                           lFlagStateTuples,
                                           lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    delete lFlagStateTuples;

    // 1.0.0.3. Ensure that passing a null context pointer argument to
    //          chkconfig_state_get_count returns -EINVAL.

    lStatus = chkconfig_state_get_count(nullptr,
                                        &lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.0.4. Ensure that passing a null context pointer argument to
    //          chkconfig_state_copy_all returns -EINVAL.

    lStatus = chkconfig_state_copy_all(nullptr,
                                       &lFlagStateTuples,
                                       &lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.1. Ensure that passing a null flag argument to
    //        chkconfig_state_get returns -EINVAL.

    lStatus = chkconfig_state_get(inContextPointer,
                                  nullptr,
                                  &lState);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.2. Ensure that passing a null state argument to
    //        chkconfig_state_get returns -EINVAL.

    lStatus = chkconfig_state_get(inContextPointer,
                                  kFlagFirst,
                                  nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.3. Ensure that passing a null flag argument to
    //        chkconfig_state_get_with_origin returns -EINVAL.

    lStatus = chkconfig_state_get_with_origin(inContextPointer,
                                              nullptr,
                                              &lState,
                                              &lOrigin);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.4. Ensure that passing a null state argument to
    //        chkconfig_state_get_with_origin returns -EINVAL.

    lStatus = chkconfig_state_get_with_origin(inContextPointer,
                                              kFlagFirst,
                                              nullptr,
                                              &lOrigin);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.5. Ensure that passing a null origin argument to
    //        chkconfig_state_get_with_origin returns -EINVAL.

    lStatus = chkconfig_state_get_with_origin(inContextPointer,
                                              kFlagFirst,
                                              &lState,
                                              nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.6. Ensure that passing a null tuples argument to
    //        chkconfig_state_get_multiple returns -EINVAL.

    lFlagStateTuplesCount = 1;

    lStatus = chkconfig_state_get_multiple(inContextPointer,
                                           nullptr,
                                           lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.7. Enusre that passing a null count argument to
    //        chkconfig_state_get_count returns -EINVAL.

    lStatus = chkconfig_state_get_count(inContextPointer,
                                        nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.8. Ensure that passing a null tuples argument to
    //        chkconfig_state_copy_all returns -EINVAL.

    lStatus = chkconfig_state_copy_all(inContextPointer,
                                       nullptr,
                                       &lFlagStateTuplesCount);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);

    // 1.0.9. Ensure that passing a null tuple count argument to
    //        chkconfig_state_copy_all returns -EINVAL.

    lStatus = chkconfig_state_copy_all(inContextPointer,
                                       &lFlagStateTuples,
                                       nullptr);
    NL_TEST_ASSERT(inSuite, lStatus == -EINVAL);
}

/*
 * Flag Observation w/o Defaults
 */
static void TestFlagObservationWithoutDefaults(nlTestSuite *inSuite, void *inContext)
{
    static const char * const                     kFlagFirst   = "test-a";
    static const char * const                     kFlagSecond  = "test-b";
    static const chkconfig_flag_state_tuple_t     kFlagStateTuples[] =
    {
        { kFlagFirst,  true,  CHKCONFIG_ORIGIN_STATE },
        { kFlagSecond, false, CHKCONFIG_ORIGIN_STATE }
    };
    TestContext *                                 lTestContext = static_cast<TestContext *>(inContext);
    chkconfig_status_t                            lStatus;
    chkconfig_context_pointer_t                   lContextPointer;
    chkconfig_options_pointer_t                   lOptionsPointer;
    chkconfig_option_t                            lOption;
    const chkconfig_flag_state_tuple_t *          lFlagStateTupleFirst = nullptr;
    const chkconfig_flag_state_tuple_t *          lFlagStateTupleLast  = nullptr;

    // Test Initialization

    lStatus = chkconfig_init(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lContextPointer != nullptr);

    lStatus = chkconfig_options_init(lContextPointer, &lOptionsPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
    NL_TEST_ASSERT(inSuite, lOptionsPointer != nullptr);

    lOption = CHKCONFIG_OPTION_STATE_DIRECTORY;

    lStatus = chkconfig_options_set(lContextPointer,
                                    lOptionsPointer,
                                    lOption,
                                    &lTestContext->mStateDirectory[0]);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    lOption = CHKCONFIG_OPTION_USE_DEFAULT_DIRECTORY;

    lStatus = chkconfig_options_set(lContextPointer,
                                    lOptionsPointer,
                                    lOption,
                                    false);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    // 1.0. Negative Tests

    TestNegativeFlagObservation(inSuite, lContextPointer);

    // 2.0. Positive Tests

    // 2.0.0. With zero (0) backing store flags.

    TestFlagObservationWithoutBackingStoreFlags(inSuite, lContextPointer);

    // 2.0.1. With two (2) state backing store flags.

    lFlagStateTupleFirst = &kFlagStateTuples[0];
    lFlagStateTupleLast  = lFlagStateTupleFirst + ElementsOf(kFlagStateTuples);

    TestFlagObservationWithBackingStoreFlags(inSuite,
                                             *lTestContext,
                                             lContextPointer,
                                             lFlagStateTupleFirst,
                                             lFlagStateTupleLast,
                                             lFlagStateTupleFirst,
                                             lFlagStateTupleLast);

    // Test Finalization

    lStatus = chkconfig_options_destroy(lContextPointer, &lOptionsPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);

    lStatus = chkconfig_destroy(&lContextPointer);
    NL_TEST_ASSERT(inSuite, lStatus == CHKCONFIG_STATUS_SUCCESS);
}

/**
 *  Test Suite. It lists all the test functions.
 *
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Utility (State)",               TestUtilityState),
    NL_TEST_DEF("Utility (Origin)",              TestUtilityOrigin),
    NL_TEST_DEF("Utility (Tuples Lifetime)",     TestUtilityTuplesLifetime),
    NL_TEST_DEF("Utility (Tuples Compare)",      TestUtilityTuplesCompare),
    NL_TEST_DEF("Context Lifetime Management",   TestContextLifetimeManagement),
    NL_TEST_DEF("Options Lifetime Management",   TestOptionsLifetimeManagement),
    NL_TEST_DEF("Options Mutation",              TestOptionsMutation),
    NL_TEST_DEF("Flag Observation w/o Defaults", TestFlagObservationWithoutDefaults),

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
