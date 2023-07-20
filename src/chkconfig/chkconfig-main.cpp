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
 *      This file implements the chkconfig command line interface
 *      (CLI) utility for checking, getting, and listing chkconfig
 *      flag state(s).
 *
 */


#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chkconfig/chkconfig.h>

#include "chkconfig-assert.h"
#include "chkconfig-version.h"


// MARK: Preprocessor Definitions

#define CHKCONFIG_OPT_BASE                     0x1000

#define CHKCONFIG_OPT_USE_DEFAULT_DIRECTORY    'd'
#define CHKCONFIG_OPT_FORCE                    'f'
#define CHKCONFIG_OPT_HELP                     'h'
#define CHKCONFIG_OPT_QUIET                    'q'
#define CHKCONFIG_OPT_STATE                    's'
#define CHKCONFIG_OPT_VERSION                  'V'
#define CHKCONFIG_OPT_DEFAULT_DIRECTORY        (CHKCONFIG_OPT_BASE +  1)
#define CHKCONFIG_OPT_STATE_DIRECTORY          (CHKCONFIG_OPT_BASE +  2)

#define CHKCONFIG_SHORT_OPTIONS                "+dfhqsV"

#define CHKCONFIG_LIST_HEADER_FLAG_FORMAT      "%-19s"
#define CHKCONFIG_LIST_HEADER_COLUMN_SEPARATOR "  "
#define CHKCONFIG_LIST_HEADER_STATE_FORMAT     "%-5s"

#define CHKCONFIG_LIST_HEADER_FORMAT           \
    CHKCONFIG_LIST_HEADER_FLAG_FORMAT          \
    CHKCONFIG_LIST_HEADER_COLUMN_SEPARATOR     \
    CHKCONFIG_LIST_HEADER_STATE_FORMAT         \
    "\n"

#define CHKCONFIG_LIST_ROW_FLAG_FORMAT         "%-19s"
#define CHKCONFIG_LIST_ROW_COLUMN_SEPARATOR    "  "
#define CHKCONFIG_LIST_ROW_STATE_FORMAT        "%-5s"

#define CHKCONFIG_LIST_ROW_FORMAT              \
    CHKCONFIG_LIST_ROW_FLAG_FORMAT             \
    CHKCONFIG_LIST_ROW_COLUMN_SEPARATOR        \
    CHKCONFIG_LIST_ROW_STATE_FORMAT            \
    "\n"

namespace nuovations
{

namespace Detail
{

// MARK: Type Declarations

enum
{
    kChkconfigOptFlagNone                 = 0x00000000,

    kChkconfigOptFlagForce                = 0x00000001,
    kChkconfigOptFlagListAll              = 0x00000002,
    kChkconfigOptFlagQuiet                = 0x00000004,
    kChkconfigOptFlagState                = 0x00000008,
    kChkconfigOptFlagUseDefaultDirectory  = 0x00000010,
    kChkconfigOptFlagWantDefaultDirectory = 0x00000020,
    kChkconfigOptFlagWantStateDirectory   = 0x00000040
};

// MARK: Global Variables

// MARK: Private Global Variables

static const struct option sOptions[]          = {
    // General Options

    {
        "help",
        no_argument,
        nullptr,
        CHKCONFIG_OPT_HELP
    },

    {
        "quiet",
        no_argument,
        nullptr,
        CHKCONFIG_OPT_QUIET
    },

    {
        "version",
        no_argument,
        nullptr,
        CHKCONFIG_OPT_VERSION
    },

    // Directory Options

    {
        "default-directory",
        required_argument,
        nullptr,
        CHKCONFIG_OPT_DEFAULT_DIRECTORY
    },

    {
        "state-directory",
        required_argument,
        nullptr,
        CHKCONFIG_OPT_STATE_DIRECTORY
    },

    // Check / Get / List Options

    {
        "use-default-directory",
        no_argument,
        nullptr,
        CHKCONFIG_OPT_USE_DEFAULT_DIRECTORY
    },

    {
        "state",
        no_argument,
        nullptr,
        CHKCONFIG_OPT_STATE
    },

    // Set Options

    {
        "force",
        no_argument,
        nullptr,
        CHKCONFIG_OPT_FORCE
    },

    // Sentinel Terminator Option

    {
        nullptr,
        0,
        nullptr,
        0
    }
};

static const char * const  sShortUsageString =
"Usage: %1$s [ -hV ]\n"
"       %1$s [ <directory options> ] [ -dsq ]\n"
"       %1$s [ <directory options> ] [ -dq ] <flag>\n"
"       %1$s [ <directory options> ] [ -fq ] <flag> <on | off>\n";

static const char * const  sLongUsageString  =
"\n"
" General Options:\n"
"\n"
"  -h, --help                   Print this help, then exit.\n"
"  -q, --quiet                  Work silently, even if an error occurs.\n"
"  -V, --version                Enable verbose operation and log output.\n"
"\n"
" Directory Options:\n"
"\n"
"  --default-directory DIR      Use DIR directory as the read-only flag state\n"
"                               fallback default directory when a flag does not\n"
"                               exist in the state directory (default: \n"
"                               " CHKCONFIG_DEFAULTDIR_DEFAULT ").\n"
"  --state-directory DIR        Use DIR directory as the read-write flag state\n"
"                               directory (default: " CHKCONFIG_STATEDIR_DEFAULT ").\n"
"\n"
" Check / Get / List Options:\n"
"\n"
"  -d, --use-default-directory  Include the default directory as a fallback.\n"
"  -s, --state                  Print the state of every configuration flag,\n"
"                               sorting by state.\n"
"\n"
" Set Options:\n"
"\n"
"  -f, --force                  Forcibly create the specified flag state file\n"
"                               if it does not exist.\n"
"\n";

static const char *        sDefaultDirectory = CHKCONFIG_DEFAULTDIR_DEFAULT;
static const char *        sFlagString       = nullptr;
static bool                sState            = false;
static const char *        sStateDirectory   = CHKCONFIG_STATEDIR_DEFAULT;
static const char *        sStateString      = nullptr;
static uint32_t            sOptFlags         = kChkconfigOptFlagNone;

static void PrintUsage(
    const char *inProgram,
    const int &inStatus
)
{
    char *        lProgram = strdup(inProgram);
    const char *  lName    = basename(lProgram);


    // Regardless of the desired exit status, display a short usage
    // synopsis.

    fprintf(stdout, sShortUsageString, lName);

    // Depending on the desired exit status, display either a helpful
    // suggestion on obtaining more information or display a long
    // usage synopsis.

    if (inStatus != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s -h' for more information.\n", lName);

    if (inStatus != EXIT_FAILURE)
    {
        fprintf(stdout, sLongUsageString);
    }

    if (lProgram != nullptr)
    {
        free(lProgram);
    }

    exit(inStatus);
}

static void PrintVersion(const char *inProgram)
{
    char *        lProgram = strdup(inProgram);
    const char *  lName    = basename(lProgram);

    fprintf(stdout,
            "%s %s\n%s\n",
            lName,
            CHKCONFIG_VERSION_STRING,
            CHKCONFIG_COPYRIGHT_STRING);

    if (lProgram != nullptr)
    {
        free(lProgram);
    }

    exit(EXIT_SUCCESS);
}

static void PrintError(const char *inFormat, ...)
{
    va_list lArguments;

    va_start(lArguments, inFormat);

    if (!(sOptFlags & kChkconfigOptFlagQuiet))
    {
        vfprintf(stderr, inFormat, lArguments);
    }

    va_end(lArguments);
}

static void ProcessArguments(
    const char *inProgram,
    int &inArgumentCount,
    char * const inArgumentArray[],
	const struct option *inOptions,
	size_t &outConsumed
)
{
    static const char * const short_options = CHKCONFIG_SHORT_OPTIONS;
    const char * const        p = short_options;
    int                       c;
    unsigned int              errors = 0;
    chkconfig_status_t        status;

    // Start parsing invocation options

    while (!errors && (c = getopt_long(inArgumentCount, inArgumentArray, p, inOptions, nullptr)) != -1)
    {

        switch (c)
        {

        case CHKCONFIG_OPT_USE_DEFAULT_DIRECTORY:
            sOptFlags |= kChkconfigOptFlagUseDefaultDirectory;
            break;

        case CHKCONFIG_OPT_FORCE:
            sOptFlags |= kChkconfigOptFlagForce;
            break;

        case CHKCONFIG_OPT_HELP:
            PrintUsage(inProgram, EXIT_SUCCESS);
            break;

        case CHKCONFIG_OPT_QUIET:
            sOptFlags |= kChkconfigOptFlagQuiet;
            break;

        case CHKCONFIG_OPT_STATE:
            sOptFlags |= (kChkconfigOptFlagListAll | kChkconfigOptFlagState);
            break;

        case CHKCONFIG_OPT_VERSION:
            PrintVersion(inProgram);
            break;

        case CHKCONFIG_OPT_DEFAULT_DIRECTORY:
            sOptFlags |= kChkconfigOptFlagWantDefaultDirectory;
            sDefaultDirectory = optarg;
            break;

        case CHKCONFIG_OPT_STATE_DIRECTORY:
            sOptFlags |= kChkconfigOptFlagWantStateDirectory;
            sStateDirectory = optarg;
            break;

        default:
            fprintf(stderr, "Unknown chkconfig option '%c' (%d)!\n", optopt, optopt);
            errors++;
            break;

        }
    }

    // If we have accumulated any errors at this point, bail out since
    // any further handling of arguments is likely to fail due to bad
    // user input.

    if (errors)
    {
        goto exit;
    }

    // Update argument parameters to reflect those consumed by getopt.

    inArgumentCount -= optind;
    inArgumentArray += optind;

    outConsumed = static_cast<size_t>(optind);

    // Reset the optind value; otherwise, option processing in any
    // dispatched command will skip that many arguments before option
    // processing actually starts.

    optind = 0;

    // At this point, we may have positional parameters remaining
    // the count of which influences the mode of operation.

    switch (inArgumentCount)
    {

    case 0:
        if (sOptFlags & kChkconfigOptFlagForce)
        {
            PrintError("The '-f/--force' option is mutually exclusive with the check or list usage; please use one or the other.\n");

            errors++;
        }
        else
        {
            // If there are no positional parameters, then list usage
            // is implicit, so assert the flag.

            sOptFlags |= kChkconfigOptFlagListAll;
        }
        break;

    case 1:
    case 2:
        if (sOptFlags & kChkconfigOptFlagState)
        {
            PrintError("The '-s/--state' option is mutally exclusive with the check usage; please use one or the other.\n");

            errors++;
            break;
        }
        else
        {
            sFlagString = inArgumentArray[0];

            if (inArgumentCount == 2)
            {
                sStateString = inArgumentArray[1];
                status = chkconfig_state_string_get_state(sStateString, &sState);

                if (status < CHKCONFIG_STATUS_SUCCESS)
                {
                    PrintError("Unrecognized or unsupported state value: \"%s\"; please use 'off' or 'on'.\n", sStateString);

                    errors++;
                    break;
                }
            }

            inArgumentCount -= inArgumentCount;
            inArgumentArray += inArgumentCount;
            outConsumed     += static_cast<size_t>(inArgumentCount);
        }
        break;

    default:
        errors++;
        break;

    }

    // If there were any errors parsing the command line arguments,
    // remind the user of proper invocation semantics and return an
    // error to the parent process.

exit:
    if (errors)
    {
        PrintUsage(inProgram, EXIT_FAILURE);
    }

    return;
}

static int FlagSortFunction(const void *inLeft, const void *inRight)
{
    const chkconfig_flag_state_tuple_t *lLeftTuple  = static_cast<const chkconfig_flag_state_tuple_t *>(inLeft);
    const chkconfig_flag_state_tuple_t *lRightTuple = static_cast<const chkconfig_flag_state_tuple_t *>(inRight);
    int                                 lRetval;

    lRetval = strcmp(lLeftTuple->m_flag, lRightTuple->m_flag);

    return (lRetval);
}

static int StateSortFunction(const void *inLeft, const void *inRight)
{
    const chkconfig_flag_state_tuple_t *lLeftTuple  = static_cast<const chkconfig_flag_state_tuple_t *>(inLeft);
    const chkconfig_flag_state_tuple_t *lRightTuple = static_cast<const chkconfig_flag_state_tuple_t *>(inRight);
    int                                 lRetval;

    // Compare the states as the primary sort key such that on / true
    // sorts before off / false.

    lRetval = lRightTuple->m_state - lLeftTuple->m_state;

    // If the states are equal, then sort by flag as a secondary sort key.

    if (lRetval == 0)
    {
        lRetval = FlagSortFunction(inLeft, inRight);
    }

    return (lRetval);
}

static chkconfig_status_t SortAllFlags(chkconfig_flag_state_tuple_t *&inFlagStateTuples,
                                       const size_t &inFlagStateTuplesCount,
                                       const uint32_t &inOptFlags)
{
    int (* lSortFunction)(const void *, const void *);
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    if (inOptFlags & kChkconfigOptFlagState)
    {
        lSortFunction = StateSortFunction;
    }
    else
    {
        lSortFunction = FlagSortFunction;
    }

    qsort(&inFlagStateTuples[0],
          inFlagStateTuplesCount,
          sizeof(chkconfig_flag_state_tuple_t),
          lSortFunction);

    return (lRetval);
}

static void ListHeader(void)
{
    fprintf(stdout, CHKCONFIG_LIST_HEADER_FORMAT, "Flag", "State");
    fprintf(stdout, CHKCONFIG_LIST_HEADER_FORMAT, "====", "=====");
}

static chkconfig_status_t ListOne(const chkconfig_flag_state_tuple_t &inFlagStateTuple)
{
    const char *       lStateString;
    chkconfig_status_t lRetval  = CHKCONFIG_STATUS_SUCCESS;

    lRetval = chkconfig_state_get_state_string(inFlagStateTuple.m_state, &lStateString);
    nlREQUIRE_SUCCESS(lRetval, done);

    fprintf(stdout, CHKCONFIG_LIST_ROW_FORMAT, inFlagStateTuple.m_flag, lStateString);

 done:
    return (lRetval);
}

static chkconfig_status_t ListAllFlags(chkconfig_context_t &inContext)
{
    chkconfig_flag_state_tuple_t *       lFlagStateTuples = nullptr;
    size_t                               lFlagStateTuplesCount;
    const chkconfig_flag_state_tuple_t * lFirst;
    const chkconfig_flag_state_tuple_t * lLast;
    const chkconfig_flag_state_tuple_t * lCurrent;
    chkconfig_status_t                   lStatus;
    chkconfig_status_t                   lRetval  = CHKCONFIG_STATUS_SUCCESS;

    lRetval = chkconfig_state_get_all(&inContext,
                                      &lFlagStateTuples,
                                      &lFlagStateTuplesCount);
    nlREQUIRE_SUCCESS(lRetval, done);

    // Sort the flags according to the command line options
    // specified. By default, flags are shown sorted by flag name; if
    // the '-s' option is asserted, then sort them by state.

    lRetval = SortAllFlags(lFlagStateTuples, lFlagStateTuplesCount, sOptFlags);
    nlREQUIRE_SUCCESS(lRetval, done);

    ListHeader();

    lFirst   = &lFlagStateTuples[0];
    lLast    = lFirst + lFlagStateTuplesCount;
    lCurrent = lFirst;

    while (lCurrent != lLast)
    {
        lRetval = ListOne(*lCurrent);
        nlREQUIRE_SUCCESS(lRetval, done);

        lCurrent++;
    }

 done:
    if (lFlagStateTuples != nullptr)
    {
        lStatus = chkconfig_flag_state_tuple_destroy(lFlagStateTuples, lFlagStateTuplesCount);
        nlVERIFY_SUCCESS_ACTION(lStatus, lRetval = lStatus);
    }

    return (lRetval);
}

static chkconfig_status_t SetOrGetOneFlag(chkconfig_context_t &inContext,
                                          chkconfig_flag_t &inFlag,
                                          const char *inStateString,
                                          chkconfig_state_t &inState)
{
    chkconfig_status_t lRetval  = CHKCONFIG_STATUS_SUCCESS;

    if (inStateString != nullptr)
    {
        // If the user did not assert the force flag, then the
        // following will expectedly fail. Consequently, use the
        // EXPECT rather than REQUIRE assertion form.

        lRetval = chkconfig_state_set(&inContext,
                                      inFlag,
                                      inState);
        nlEXPECT_SUCCESS_ACTION(lRetval,
                                done,
                                PrintError("Failed to set flag \"%s\" to \"%s\": %s\n",
                                           inFlag,
                                           inStateString,
                                           strerror(-lRetval)));
    }
    else
    {
        lRetval = chkconfig_state_get(&inContext,
                                      inFlag,
                                      &inState);

        if (lRetval >= CHKCONFIG_STATUS_SUCCESS)
        {
            lRetval = (sState ? CHKCONFIG_STATUS_SUCCESS : -ENOENT);
        }
    }

 done:
    return (lRetval);
}

static chkconfig_status_t Init(chkconfig_context_pointer_t &inContextPointer,
                               chkconfig_options_pointer_t &inOptionsPointer,
                               const uint32_t &inOptFlags)
{
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    // Intialize the library.

    lRetval = chkconfig_init(&inContextPointer);
    nlREQUIRE_SUCCESS(lRetval, done);

    // Initialize the library options.

    lRetval = chkconfig_options_init(inContextPointer,
                                     &inOptionsPointer);
    nlREQUIRE_SUCCESS(lRetval, done);

    // Set the library options based on the user-specified options.

    lRetval = chkconfig_options_set(inContextPointer,
                                    inOptionsPointer,
                                    CHKCONFIG_OPTION_FORCE_STATE,
                                    ((inOptFlags & kChkconfigOptFlagForce) == kChkconfigOptFlagForce));
    nlREQUIRE_SUCCESS(lRetval, done);

    if (inOptFlags & kChkconfigOptFlagWantDefaultDirectory)
    {
        lRetval = chkconfig_options_set(inContextPointer,
                                        inOptionsPointer,
                                        CHKCONFIG_OPTION_DEFAULT_DIRECTORY,
                                        sDefaultDirectory);
        nlREQUIRE_SUCCESS(lRetval, done);
    }

    if (inOptFlags & kChkconfigOptFlagWantStateDirectory)
    {
        lRetval = chkconfig_options_set(inContextPointer,
                                        inOptionsPointer,
                                        CHKCONFIG_OPTION_STATE_DIRECTORY,
                                        sStateDirectory);
        nlREQUIRE_SUCCESS(lRetval, done);
    }

    if (inOptFlags & kChkconfigOptFlagUseDefaultDirectory)
    {
        lRetval = chkconfig_options_set(inContextPointer,
                                        inOptionsPointer,
                                        CHKCONFIG_OPTION_USE_DEFAULT_DIRECTORY,
                                        ((inOptFlags & kChkconfigOptFlagUseDefaultDirectory) == kChkconfigOptFlagUseDefaultDirectory));
        nlREQUIRE_SUCCESS(lRetval, done);
    }

 done:
    return (lRetval);
}

static chkconfig_status_t Destroy(chkconfig_context_pointer_t &inContextPointer,
                                  chkconfig_options_pointer_t &inOptionsPointer)
{
    chkconfig_status_t lStatus;
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    // Shutdown the library options.

    lStatus = chkconfig_options_destroy(inContextPointer,
                                        &inOptionsPointer);
    nlREQUIRE_SUCCESS_ACTION(lStatus, destroy, lRetval = lStatus);

 destroy:
    // Shutdown the library.

    lStatus = chkconfig_destroy(&inContextPointer);
    nlVERIFY_SUCCESS_ACTION(lStatus, lRetval = lStatus);

    return (lRetval);
}

static chkconfig_status_t Main(int &argc, char * const argv[])
{
    size_t                         n                = 0;
    chkconfig_context_pointer_t    lContextPointer  = nullptr;
    chkconfig_options_pointer_t    lOptionsPointer  = nullptr;
    chkconfig_status_t             lStatus;
    chkconfig_status_t             lRetval = CHKCONFIG_STATUS_SUCCESS;

    // Decode invocation parameters.

    ProcessArguments(argv[0], argc, argv, sOptions, n);

    // Intialize

    lRetval = Init(lContextPointer,
                   lOptionsPointer,
                   sOptFlags);
    nlREQUIRE_SUCCESS(lRetval, done);

    // Depending on the mode, do the requested work.

    if ((sOptFlags & kChkconfigOptFlagListAll) && (sFlagString == nullptr))
    {
        lRetval = ListAllFlags(*lContextPointer);
    }
    else if (sFlagString != nullptr)
    {
        lRetval = SetOrGetOneFlag(*lContextPointer, sFlagString, sStateString, sState);
    }

 done:
    // Shutdown

    lStatus = Destroy(lContextPointer,
                      lOptionsPointer);
    nlVERIFY_SUCCESS_ACTION(lStatus, lRetval = lStatus);

    return (lRetval);
}

}; // namespace Detail

}; // namespace nuovations

int main(int argc, char * const argv[])
{
    chkconfig_status_t status = CHKCONFIG_STATUS_SUCCESS;
    int                retval = EXIT_SUCCESS;

    status = nuovations::Detail::Main(argc, argv);

    retval = ((status >= CHKCONFIG_STATUS_SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE);

    return retval;
}
