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
 *      This file implements interfaces for the chkconfig
 *      configuruation management library.
 *
 */


#include "chkconfig.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "chkconfig-assert.h"


using namespace std;


// MARK: Type Declarations

/**
 *  @brief
 *    A client-opaque type for chkconfig library context.
 *
 *  Most chkconfig library interfaces take a pointer to this context.
 *
 *  @private
 *
 */
struct _chkconfig_context
{
    const chkconfig_options_t * m_options; //!< A pointer to the current
                                           //!< immutable library runtime
                                           //!< options.
};

/**
 *  @brief
 *    A client-opaque type for chkconfig library runtime options.
 *
 *  @private
 *
 */
struct _chkconfig_options
{
    const char * m_state_dir;       //!< A pointer to an immutable null-
                                    //!< terminated C string containing
                                    //!< the read/write flag state backing
                                    //!< file directory.
    bool         m_force_state;     //!< When asserted, create backing
                                    //!< state files that do not already
                                    //!< exist.
    bool         m_use_default_dir; //!< When asserted, use the read-only
                                    //!< flag state fallback default
                                    //!< directory when a flag does not
                                    //!< exist in the state directory.
    const char * m_default_dir;     //!< A pointer to an immutable null-
                                    //!< terminated C string containing
                                    //!< read-only flag state fallback
                                    //!< 'default' backing file directory
                                    //!< to use when a flag does not exist
                                    //!< in the 'state' directory.
};

// MARK: C++

namespace nuovations
{

namespace Detail
{

// MARK: Global Variables

static const chkconfig_options_t sChkconfigOptionsDefault =
{
    .m_state_dir        = CHKCONFIG_STATEDIR_DEFAULT,
    .m_force_state      = false,
    .m_use_default_dir  = false,
    .m_default_dir      = CHKCONFIG_DEFAULTDIR_DEFAULT
};
static const char * const        sOffString               = "off";
static const char * const        sOnString                = "on";


// MARK: Utility

static chkconfig_status_t chkconfigStateStringGetState(const char *inStateString,
                                                       chkconfig_state_t &outState)
{
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inStateString != nullptr, done, lRetval = -EINVAL);

    if (strncasecmp(inStateString, sOnString, strlen(sOnString)) == 0)
    {
        outState = true;
    }
    else if (strncasecmp(inStateString, sOffString, strlen(sOffString)) == 0)
    {
        outState = false;
    }
    else
    {
        lRetval = -EINVAL;
    }

 done:
    return (lRetval);
}

static chkconfig_status_t chkconfigStateGetStateString(const chkconfig_state_t &inState,
                                                       const char *&outStateString)
{
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    outStateString = ((inState) ? sOnString : sOffString);

    return (lRetval);
}

static chkconfig_status_t chkconfigFlagStateTupleInit(chkconfig_flag_state_tuple_t *&inFlagStateTuples,
                                                      const size_t &inFlagStateTupleCount)
{
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inFlagStateTupleCount > 0, done, lRetval = -EINVAL);

    inFlagStateTuples = reinterpret_cast<chkconfig_flag_state_tuple_t *>
        (malloc(inFlagStateTupleCount * sizeof (chkconfig_flag_state_tuple_t)));
    nlREQUIRE_ACTION(inFlagStateTuples != nullptr, done, lRetval = -ENOMEM);

 done:
    return (lRetval);
}

static chkconfig_status_t chkconfigFlagStateTupleDestroy(chkconfig_flag_state_tuple_t *&inFlagStateTuples,
                                                         const size_t &inFlagStateTupleCount)
{
    chkconfig_flag_state_tuple_t * const lFirst   = &inFlagStateTuples[0];
    chkconfig_flag_state_tuple_t * const lLast    = (lFirst + inFlagStateTupleCount);
    chkconfig_flag_state_tuple_t *       lCurrent = lFirst;
    chkconfig_status_t                   lRetval  = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inFlagStateTuples != nullptr, done, lRetval = -EINVAL);

    while (lCurrent != lLast)
    {
        if (lCurrent->m_flag != nullptr)
        {
            free(const_cast<char *>(lCurrent->m_flag));
            lCurrent->m_flag = nullptr;
        }

        lCurrent++;
    }

    free(inFlagStateTuples);
    inFlagStateTuples = nullptr;

 done:
    return (lRetval);
}

// MARK: Lifetime Management

static chkconfig_status_t chkconfigInit(chkconfig_context_pointer_t &outContextPointer)
{
    chkconfig_context_pointer_t lContextPointer;
    chkconfig_status_t          lRetval = CHKCONFIG_STATUS_SUCCESS;

    lContextPointer = new chkconfig_context_t;
    nlREQUIRE_ACTION(lContextPointer != nullptr, done, lRetval = -ENOMEM);

    lContextPointer->m_options = &sChkconfigOptionsDefault;

    outContextPointer = lContextPointer;

 done:
    return (lRetval);
}

static chkconfig_status_t chkconfigOptionsInit(chkconfig_context_t &inContext,
                                               chkconfig_options_pointer_t &outOptionsPointer)
{
    chkconfig_options_pointer_t lOptionsPointer;
    chkconfig_status_t          lRetval = CHKCONFIG_STATUS_SUCCESS;

    lOptionsPointer = new chkconfig_options_t;
    nlREQUIRE_ACTION(lOptionsPointer != nullptr, done, lRetval = -ENOMEM);

    // Start by initializing the newly-allocations options to the defaults.

    lOptionsPointer->m_state_dir       = strdup(sChkconfigOptionsDefault.m_state_dir);
    nlREQUIRE_ACTION(lOptionsPointer->m_state_dir != nullptr, done, lRetval = -ENOMEM);

    lOptionsPointer->m_force_state     = sChkconfigOptionsDefault.m_force_state;
    lOptionsPointer->m_use_default_dir = sChkconfigOptionsDefault.m_use_default_dir;
    lOptionsPointer->m_default_dir     = strdup(sChkconfigOptionsDefault.m_default_dir);
    nlREQUIRE_ACTION(lOptionsPointer->m_default_dir != nullptr, done, lRetval = -ENOMEM);


    inContext.m_options = lOptionsPointer;

    outOptionsPointer = lOptionsPointer;

 done:
    return (lRetval);
}

static chkconfig_status_t chkconfigOptionsDestroy(chkconfig_context_t &inContext,
                                                  chkconfig_options_pointer_t &inOptionsPointer)
{
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inOptionsPointer != nullptr, done, lRetval = -EINVAL);

    // If the current context has options that match those being
    // destroyed, then reset the current context to the default
    // options.

    if (inContext.m_options == inOptionsPointer)
    {
        inContext.m_options = &sChkconfigOptionsDefault;
    }

    // Destroy any leaf data.

    if (inOptionsPointer->m_state_dir != nullptr)
    {
        free(const_cast<char *>(inOptionsPointer->m_state_dir));
        inOptionsPointer->m_state_dir = nullptr;
    }

    if (inOptionsPointer->m_default_dir != nullptr)
    {
        free(const_cast<char *>(inOptionsPointer->m_default_dir));
        inOptionsPointer->m_default_dir = nullptr;
    }

    // Destroy the options data itself.

    delete inOptionsPointer;

    inOptionsPointer = nullptr;

 done:
    return (lRetval);
}

static chkconfig_status_t chkconfigDestroy(chkconfig_context_pointer_t &inContextPointer)
{
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inContextPointer != nullptr, done, lRetval = -EINVAL);

    delete inContextPointer;

    inContextPointer = nullptr;

 done:
    return (lRetval);
}

// MARK: Option Management

static chkconfig_status_t chkconfigOptionsSet(chkconfig_context_t &inContext,
                                              chkconfig_options_t &inOptions,
                                              const chkconfig_option_t &inOption,
                                              va_list inArguments)
{
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    (void)inContext;

    switch (inOption)
    {

    case CHKCONFIG_OPTION_STATE_DIRECTORY:
        if (inOptions.m_state_dir != nullptr)
        {
            free(const_cast<char *>(inOptions.m_state_dir));
        }

        inOptions.m_state_dir = strdup(va_arg(inArguments, const char *));
        nlREQUIRE_ACTION(inOptions.m_state_dir != nullptr, done, lRetval = -ENOMEM);
        break;

    case CHKCONFIG_OPTION_FORCE_STATE:
        inOptions.m_force_state = va_arg(inArguments, int);
        break;

    case CHKCONFIG_OPTION_DEFAULT_DIRECTORY:
        if (inOptions.m_default_dir != nullptr)
        {
            free(const_cast<char *>(inOptions.m_default_dir));
        }

        inOptions.m_default_dir = strdup(va_arg(inArguments, const char *));
        nlREQUIRE_ACTION(inOptions.m_default_dir != nullptr, done, lRetval = -ENOMEM);
        break;

    case CHKCONFIG_OPTION_USE_DEFAULT_DIRECTORY:
        inOptions.m_use_default_dir = va_arg(inArguments, int);
        break;

    default:
        lRetval = -EINVAL;
        break;

    }

 done:
    return (lRetval);
}

// MARK: Observers

static chkconfig_status_t chkconfigStateGet(const char *inFlagPath,
                                            chkconfig_state_t &outState,
                                            const bool &inUseDefaultDirectory)
{
    int                lStatus;
    int                lDescriptor = -1;
    struct stat        lMetadata;
    void *             lData   = MAP_FAILED;
    chkconfig_state_t  lState  = false;
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inFlagPath != nullptr, done, lRetval = -EINVAL);

    // The path may very well not exist, so use the EXPECT rather than
    // REQUIRE assertion form.

    lDescriptor = open(inFlagPath, O_RDONLY);
    nlEXPECT_ACTION(lDescriptor != -1,
                    done,
                    switch (errno)
                    {

                    case ENOENT:
                        outState = false;

                        // If called with inUseDefaultDirectory
                        // asserted, then the caller does NOT want the
                        // return value "hidden" to success such that
                        // they can fallback to the default directory
                        // on failure.

                        if (inUseDefaultDirectory)
                        {
                            lRetval = -errno;
                        }
                        break;

                    default:
                        lRetval = -errno;
                        break;

                    });

    // At this point, the file exists and is open. Determine its size
    // for memory-mapping.

    lStatus = fstat(lDescriptor, &lMetadata);
    nlREQUIRE_ACTION(lStatus == 0, done, lRetval = -errno);

    // If the size of the file is non-zero, memory-map the file and
    // attempt to determine its state value.
    //
    // Otherwise, there is no data to map and we must assume the
    // default state of off or false.

    if (lMetadata.st_size > 0)
    {
        lData = mmap(nullptr, lMetadata.st_size, PROT_READ, MAP_PRIVATE, lDescriptor, 0);
        nlREQUIRE_ACTION(lData != MAP_FAILED, done, lRetval = -errno);

        lRetval = chkconfigStateStringGetState(reinterpret_cast<const char *>(lData), lState);
        nlREQUIRE_SUCCESS_ACTION(lRetval, done, outState = false);
    }
    else
    {
        lState = false;
    }

    outState = lState;

 done:
    if (lData != MAP_FAILED)
    {
        lStatus = munmap(lData, lMetadata.st_size);
        nlREQUIRE_ACTION(lStatus == 0, close, lRetval = -errno);
    }

 close:
    if (lDescriptor != -1)
    {
        lStatus = close(lDescriptor);
        nlVERIFY_ACTION(lStatus == 0, lRetval = -errno);
    }

    return (lRetval);
}

static chkconfig_status_t chkconfigFlagPathCopy(const char *inDirectory,
                                                const chkconfig_flag_t &inFlag,
                                                const size_t &inPathSize,
                                                char *&outPath)
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

static chkconfig_status_t chkconfigDefaultPathCopy(const chkconfig_context_t &inContext,
                                                  const chkconfig_flag_t &inFlag,
                                                  const size_t &inPathSize,
                                                  char *outPath)
{
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    lRetval = chkconfigFlagPathCopy(inContext.m_options->m_default_dir,
                                    inFlag,
                                    inPathSize,
                                    outPath);

    return (lRetval);
}

static chkconfig_status_t chkconfigStatePathCopy(const chkconfig_context_t &inContext,
                                                 const chkconfig_flag_t &inFlag,
                                                 const size_t &inPathSize,
                                                 char *outPath)
{
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    lRetval = chkconfigFlagPathCopy(inContext.m_options->m_state_dir,
                                    inFlag,
                                    inPathSize,
                                    outPath);

    return (lRetval);
}

static chkconfig_status_t chkconfigStateGet(chkconfig_context_t &inContext,
                                            const chkconfig_flag_t &inFlag,
                                            chkconfig_state_t &outState)
{
    const bool         lUseDefaultDirectory = (inContext.m_options->m_use_default_dir &&
                                               (inContext.m_options->m_default_dir != nullptr));
    char               lFlagPath[PATH_MAX];
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    // First, form the state directory path for the flag and attempt
    // to get the state there.

    lRetval = chkconfigStatePathCopy(inContext,
                                     inFlag,
                                     PATH_MAX,
                                     &lFlagPath[0]);
    nlREQUIRE_SUCCESS(lRetval, done);

    lRetval = chkconfigStateGet(lFlagPath,
                                outState,
                                lUseDefaultDirectory);

    // If that failed and the caller wants to use the default
    // directory, make a second attempt with the default directory
    // path.

    if ((lRetval < CHKCONFIG_STATUS_SUCCESS) && lUseDefaultDirectory)
    {
        lRetval = chkconfigDefaultPathCopy(inContext,
                                           inFlag,
                                           PATH_MAX,
                                           &lFlagPath[0]);

        nlREQUIRE_SUCCESS(lRetval, done);

        lRetval = chkconfigStateGet(lFlagPath, outState, !lUseDefaultDirectory);
    }

 done:
    return (lRetval);
}

static chkconfig_status_t chkconfigStateGetMultiple(chkconfig_context_t &inContext,
                                                    chkconfig_flag_state_tuple_t *inFlagStateTuples,
                                                    const size_t &inCount)
{
    chkconfig_flag_state_tuple_t * const lFirst   = &inFlagStateTuples[0];
    chkconfig_flag_state_tuple_t * const lLast    = lFirst + inCount;
    chkconfig_flag_state_tuple_t *       lCurrent = lFirst;
    chkconfig_status_t                   lRetval  = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inFlagStateTuples != nullptr, done, lRetval = -EINVAL);

    while (lCurrent != lLast)
    {
        lRetval = chkconfigStateGet(inContext,
                                    lCurrent->m_flag,
                                    lCurrent->m_state);
        nlREQUIRE_SUCCESS(lRetval, done);

        lCurrent++;
    }

 done:
    return (lRetval);
}

static chkconfig_status_t chkconfigStateGetCount(chkconfig_context_t &inContext,
                                                 size_t &outCount)
{
    DIR *              lDirectory = nullptr;
    struct dirent *    lDirent;
    int                lStatus;
    struct stat        lMetadata;
    size_t             lCount = 0;
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    lDirectory = opendir(inContext.m_options->m_state_dir);
    nlREQUIRE_ACTION(lDirectory != nullptr, done, lRetval = -errno);

    while ((lDirent = readdir(lDirectory)) != nullptr)
    {
        char lFlagPath[PATH_MAX];

        lRetval = chkconfigStatePathCopy(inContext,
                                         lDirent->d_name,
                                         PATH_MAX,
                                         &lFlagPath[0]);
        nlREQUIRE_SUCCESS(lRetval, done);

        lStatus = stat(lFlagPath, &lMetadata);
        nlREQUIRE_ACTION(lStatus == 0, done, lRetval = -errno);

        if (S_ISREG(lMetadata.st_mode))
        {
            lCount++;
        }
    }

    outCount = lCount;

 done:
    if (lDirectory != nullptr)
    {
        lStatus = closedir(lDirectory);
        nlVERIFY_ACTION(lStatus == 0, lRetval = -errno);
    }

    return (lRetval);
}

static chkconfig_status_t chkconfigStateGetAll(chkconfig_context_t &inContext,
                                               chkconfig_flag_state_tuple_t *&outFlagStateTuples,
                                               size_t &outCount)
{
    size_t                         lCount           = 0;
    chkconfig_flag_state_tuple_t * lFlagStateTuples = nullptr;
    DIR *                          lDirectory       = nullptr;
    struct dirent *                lDirent;
    size_t                         lIndex = 0;
    struct stat                    lMetadata;
    int                            lStatus;
    chkconfig_status_t             lRetval = CHKCONFIG_STATUS_SUCCESS;

    lRetval = chkconfigStateGetCount(inContext, lCount);
    nlREQUIRE_SUCCESS(lRetval, done);

    lRetval = chkconfigFlagStateTupleInit(lFlagStateTuples, lCount);
    nlREQUIRE_SUCCESS(lRetval, done);

    lDirectory = opendir(inContext.m_options->m_state_dir);
    nlREQUIRE_ACTION(lDirectory != nullptr, done, lRetval = -errno);

    while (((lDirent = readdir(lDirectory)) != nullptr) && (lIndex < lCount))
    {
        char lFlagPath[PATH_MAX];

        lRetval = chkconfigStatePathCopy(inContext,
                                         lDirent->d_name,
                                         PATH_MAX,
                                         &lFlagPath[0]);
        nlREQUIRE_SUCCESS(lRetval, done);

        lStatus = stat(lFlagPath, &lMetadata);
        nlREQUIRE_ACTION(lStatus == 0, done, lRetval = -errno);

        if (S_ISREG(lMetadata.st_mode))
        {
            constexpr bool lUseDefaultDirectory = true;

            lRetval = chkconfigStateGet(lFlagPath, lFlagStateTuples[lIndex].m_state, !lUseDefaultDirectory);
            nlREQUIRE_SUCCESS(lRetval, done);

            lFlagStateTuples[lIndex].m_flag = strdup(lDirent->d_name);
            nlREQUIRE_ACTION(lFlagStateTuples[lIndex].m_flag != nullptr, done, lRetval = -ENOMEM);
            lIndex++;
        }
    }

    outFlagStateTuples = lFlagStateTuples;
    outCount           = lCount;

 done:
    if (lDirectory != nullptr)
    {
        lStatus = closedir(lDirectory);
        nlVERIFY_ACTION(lStatus == 0, lRetval = -errno);
    }

    return (lRetval);
}

// MARK: Mutators

static chkconfig_status_t chkconfigStateSet(chkconfig_context_t &inContext,
                                            const chkconfig_flag_t &inFlag,
                                            const chkconfig_state_t &inState)
{
    char               lFlagPath[PATH_MAX];
    int                lStatus;
    int                lFlags;
    int                lDescriptor = -1;
    const char *       lStateString;
    chkconfig_status_t lRetval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inFlag    != nullptr, done, lRetval = -EINVAL);
    nlREQUIRE_ACTION(inFlag[0] != '\0',    done, lRetval = -EINVAL);

    lRetval = chkconfigStatePathCopy(inContext,
                                     inFlag,
                                     PATH_MAX,
                                     &lFlagPath[0]);
    nlREQUIRE_SUCCESS(lRetval, done);

    lFlags = (O_WRONLY | O_TRUNC);

    if (inContext.m_options->m_force_state)
    {
        lFlags |= (O_CREAT);
    }

    // If 'm_force_state' was not asserted and, consequently, the
    // O_CREAT flag not used, then the following open call will
    // expectdly fail. Therefore, use the EXPECT rather than REQUIRE
    // assertion form.

    lDescriptor = open(lFlagPath, lFlags, DEFFILEMODE);
    nlEXPECT_ACTION(lDescriptor != -1, done, lRetval = -errno);

    lRetval = chkconfigStateGetStateString(inState, lStateString);
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

static chkconfig_status_t chkconfigStateSetMultiple(chkconfig_context_t &inContext,
                                                    const chkconfig_flag_state_tuple_t *inFlagStateTuples,
                                                    const size_t &inCount)
{

    const chkconfig_flag_state_tuple_t * const lFirst   = &inFlagStateTuples[0];
    const chkconfig_flag_state_tuple_t * const lLast    = lFirst + inCount;
    const chkconfig_flag_state_tuple_t *       lCurrent = lFirst;
    chkconfig_status_t                         lRetval  = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(inFlagStateTuples != nullptr, done, lRetval = -EINVAL);

    while (lCurrent != lLast)
    {
        lRetval = chkconfigStateSet(inContext,
                                    lCurrent->m_flag,
                                    lCurrent->m_state);
        nlREQUIRE_SUCCESS(lRetval, done);

        lCurrent++;
    }

 done:
    return (lRetval);
}

}; // namespace Detail

}; // namespace nuovations

// MARK: C

// MARK: Lifetime Management

using namespace nuovations;

/**
 *  @brief
 *    Initialize a chkconfig library context.
 *
 *  This attempts to initialize and return a chkconfig library context
 *  that will be used with nearly all library interfaces.
 *
 *  The context will be intialized with the default options the
 *  library was instantiated with when it was built. Callers that do
 *  not want to use the default options may call
 *  #chkconfig_options_init and then #chkconfig_options_set to
 *  customize runtime library behavior.
 *
 *  @param[out]  context_pointer  TBD
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer is null.
 *  @retval  -ENOMEM                   If resources could not be allocated.
 *
 *  @sa chkconfig_options_init
 *  @sa chkconfig_options_set
 *  @sa chkconfig_destroy
 *
 *  @ingroup lifetime
 *
 */
chkconfig_status_t chkconfig_init(chkconfig_context_pointer_t *context_pointer)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigInit(*context_pointer);

 done:
    return (retval);
}

/**
 *  @brief
 *    Initialize a chkconfig runtime library options context.
 *
 *  This attempts to initialize and return a chkconfig runtime library
 *  options context.
 *
 *  The context will be intialized with the default options the
 *  library was instantiated with when it was built. Callers that do
 *  not want to use the default options may call
 *  #chkconfig_options_set to customize runtime library behavior.
 *
 *  @param[in,out]  context_pointer  TBD
 *  @param[out]     options_pointer  TBD
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer or @a
 *                                     options_pointer is null.
 *
 *  @sa chkconfig_options_set
 *  @sa chkconfig_options_destroy
 *
 *  @ingroup options_lifetime
 *
 */
chkconfig_status_t chkconfig_options_init(chkconfig_context_pointer_t context_pointer,
                                          chkconfig_options_pointer_t *options_pointer)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);
    nlREQUIRE_ACTION(options_pointer != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigOptionsInit(*context_pointer,
                                          *options_pointer);

 done:
    return (retval);
}

/**
 *  @brief
 *    Deinitialize a chkconfig runtime library options context.
 *
 *  This attempts to deinitialize and release all resources associated
 *  with the specified chkconfig runtime library options context.
 *
 *  @param[in,out]  context_pointer  TBD
 *  @param[out]     options_pointer  TBD
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer or @a
 *                                     options_pointer is null.
 *
 *  @sa chkconfig_options_init
 *
 *  @ingroup options_lifetime
 *
 */
chkconfig_status_t chkconfig_options_destroy(chkconfig_context_pointer_t context_pointer,
                                             chkconfig_options_pointer_t *options_pointer)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);
    nlREQUIRE_ACTION(options_pointer != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigOptionsDestroy(*context_pointer,
                                             *options_pointer);

 done:
    return (retval);
}

/**
 *  @brief
 *    Deinitialize a chkconfig library context.
 *
 *  This attempts to deinitialize and deallocate all resources
 *  associated with the specified chkconfig library context.
 *
 *  @warning
 *    The caller is responsible for calling #chkconfig_options_destroy
 *    to deinitialize any non-default options associated with the
 *    context before calling this interface. Failure to do so will
 *    result in a loss of such resources as this interface does not
 *    take on this responsibility.
 *
 *  @param[out]  context_pointer  TBD
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer is null.
 *
 *  @sa chkconfig_init
 *
 *  @ingroup lifetime
 *
 */
chkconfig_status_t chkconfig_destroy(chkconfig_context_pointer_t *context_pointer)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigDestroy(*context_pointer);

 done:
    return (retval);
}

// MARK: Option Management

/**
 *  @brief
 *    Set a chkconfig library runtime option.
 *
 *  This attempts to set the specified chkconfig library runtime
 *  key/value option pair, modifying the runtime behavior of the
 *  library.
 *
 *  @param[in]      context_pointer  A pointer to the chkconfig
 *                                   library context whose runtime
 *                                   behavior is to change with the
 *                                   specified option.
 *  @param[in,out]  options_pointer  A pointer to the chkconfig
 *                                   library runtime option context
 *                                   which will be updated with the
 *                                   specified runtime option value if
 *                                   successful.
 *  @param[in]      option           The key of the option key/value
 *                                   pair indicating the option to set.
 *  @param[in]      ...              A variadic argument list, where
 *                                   each argument corresponds to the
 *                                   value or the key/value pair
 *                                   associated with its peer option
 *                                   key in @a option.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer or @a
 *                                     options_pointer is null or if
 *                                     @a option is not a recognized
 *                                     option key.
 *
 *  @ingroup options
 *
 */
chkconfig_status_t chkconfig_options_set(chkconfig_context_pointer_t context_pointer,
                                         chkconfig_options_pointer_t options_pointer,
                                         chkconfig_option_t option,
                                         ...)
{
    va_list            arguments;
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);
    nlREQUIRE_ACTION(options_pointer != nullptr, done, retval = -EINVAL);

    va_start(arguments, option);

    retval = Detail::chkconfigOptionsSet(*context_pointer,
                                         *options_pointer,
                                         option,
                                         arguments);

    va_end(arguments);

 done:
    return (retval);
}

// MARK: Observers

/**
 *  @brief
 *    Get the state value associated with a flag.
 *
 *  This attempts to get the state value associated with the specified
 *  flag.
 *
 *  @param[in]   context_pointer  A pointer to the chkconfig library
 *                                context for which to get the state
 *                                value for the specified flag.
 *  @param[in]   flag             The flag for which to get the associated
 *                                state value.
 *  @param[out]  state            A pointer to storage by which to return
 *                                the state value if successful.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer or @a
 *                                     state is null.
 *  @retval  -ENOENT                   If the backing file associated
 *                                     with @a flag does not exist.
 *
 *  @sa chkconfig_state_get_count
 *  @sa chkconfig_state_get_multiple
 *  @sa chkconfig_state_get_all
 *
 *  @ingroup observers
 *
 */
chkconfig_status_t chkconfig_state_get(chkconfig_context_pointer_t context_pointer,
                                       chkconfig_flag_t flag,
                                       chkconfig_state_t *state)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);
    nlREQUIRE_ACTION(state           != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigStateGet(*context_pointer,
                                       flag,
                                       *state);

 done:
    return (retval);
}

/**
 *  @brief
 *    Get the state values associated with one or more flags.
 *
 *  This attempts to get the state values associated with the specified
 *  flags.
 *
 *  @param[in]      context_pointer    A pointer to the chkconfig
 *                                     library context for which to
 *                                     get the state values for the
 *                                     specified flags.
 *  @param[in,out]  flag_state_tuples  A pointer to the flag/state
 *                                     tuples array for which to get
 *                                     the state values corresponding
 *                                     to each flag.
 *  @param[in]      count              The number of array elements in
 *                                     @a flag_state_tuples.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer or @a
 *                                     flag_state_tuples is null.
 *  @retval  -ENOENT                   If the backing file associated
 *                                     with a flag in @a
 *                                     flag_state_tuples does not
 *                                     exist.
 *
 *  @sa chkconfig_state_get_count
 *  @sa chkconfig_state_get
 *  @sa chkconfig_state_get_all
 *  @sa chkconfig_flag_state_tuple_init
 *
 *  @ingroup observers
 *
 */
chkconfig_status_t chkconfig_state_get_multiple(chkconfig_context_pointer_t context_pointer,
                                                chkconfig_flag_state_tuple_t *flag_state_tuples,
                                                size_t count)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigStateGetMultiple(*context_pointer,
                                               flag_state_tuples,
                                               count);

 done:
    return (retval);
}

/**
 *  @brief
 *    Get the count of all flags covered by a backing store file.
 *
 *  This attempts to get a count of all flags covered by a backing
 *  store file.
 *
 *  @param[in]   context_pointer  A pointer to the chkconfig
 *                                library context for which to
 *                                get the count of all flags
 *                                covered by a backing store
 *                                file.
 *  @param[out]  count            A pointer to storage by which
 *                                to return the count if successful.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer or @a
 *                                     count is null.
 *
 *  @sa chkconfig_state_get_count
 *  @sa chkconfig_state_get
 *  @sa chkconfig_state_get_all
 *  @sa chkconfig_flag_state_tuple_init
 *
 *  @ingroup observers
 *
 */
chkconfig_status_t chkconfig_state_get_count(chkconfig_context_pointer_t context_pointer,
                                             size_t *count)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);
    nlREQUIRE_ACTION(count           != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigStateGetCount(*context_pointer,
                                            *count);

 done:
    return (retval);
}

/**
 *  @brief
 *    Get the state values associated with all flags covered by a
 *    backing store file.
 *
 *  This attempts to get the state values associated with all flags
 *  covered by a backing store file.
 *
 *  @note
 *    The caller is responsible for deallocating resources on success
 *    associated with @a flag_state_tuples by calling
 *    #chkconfig_flag_state_tuple_destroy.
 *
 *  @param[in]      context_pointer    A pointer to the chkconfig
 *                                     library context for which to
 *                                     get the state values for all
 *                                     flags covered by a backing
 *                                     store file.
 *  @param[in,out]  flag_state_tuples  A pointer to storage for a
 *                                     pointer to a flag/state tuples
 *                                     array which will be populated
 *                                     with the flags and state for
 *                                     all flags covered by a backing
 *                                     store file.
 *  @param[out]  count                 A pointer to storage by which
 *                                     to return the count of the
 *                                     number of elements in @a
 *                                     flag_state_tuples if
 *                                     successful.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer, @a
 *                                     flag_state_tuples, or @a count
 *                                     is null.
 *  @retval  -ENOMEM                   Resources could not be allocated
 *                                     for the @a flag_state_tuples
 *                                     array.
 *
 *  @sa chkconfig_state_get_count
 *  @sa chkconfig_state_get
 *  @sa chkconfig_state_get_all
 *  @sa chkconfig_flag_state_tuple_init
 *
 *  @ingroup observers
 *
 */
chkconfig_status_t chkconfig_state_get_all(chkconfig_context_pointer_t context_pointer,
                                           chkconfig_flag_state_tuple_t **flag_state_tuples,
                                           size_t *count)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer   != nullptr, done, retval = -EINVAL);
    nlREQUIRE_ACTION(flag_state_tuples != nullptr, done, retval = -EINVAL);
    nlREQUIRE_ACTION(count             != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigStateGetAll(*context_pointer,
                                          *flag_state_tuples,
                                          *count);

 done:
    return (retval);
}

// MARK: Mutators

/**
 *  @brief
 *    Set the state value associated with a flag.
 *
 *  This attempts to set the state value associated with the specified
 *  flag.
 *
 *  @param[in]  context_pointer  A pointer to the chkconfig library
 *                               context for which to set the state
 *                               value for the specified flag.
 *  @param[in]  flag             The flag for which to set the associated
 *                               state value.
 *  @param[in]  state            The state value to set.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer or @a flag
 *                                     is null or if @a flag is the
 *                                     null character ('\0').
 *  @retval  -ENOENT                   If the backing file associated
 *                                     with @a flag does not exist and
 *                                     the #CHKCONFIG_OPTION_FORCE_STATE
 *                                     runtime library option has not
 *                                     been asserted.
 *
 *  @sa chkconfig_options_set
 *  @sa chkconfig_state_set_multiple
 *
 *  @ingroup mutators
 *
 */
chkconfig_status_t chkconfig_state_set(chkconfig_context_pointer_t context_pointer,
                                       chkconfig_flag_t flag,
                                       chkconfig_state_t state)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigStateSet(*context_pointer,
                                       flag,
                                       state);

 done:
    return (retval);
}

/**
 *  @brief
 *    Set the state values associated with one or more flags.
 *
 *  This attempts to set the state values associated with the specified
 *  flags.
 *
 *  @param[in]  context_pointer    A pointer to the chkconfig
 *                                 library context for which to set
 *                                 the state values for the specified
 *                                 flags.

 *  @param[in]  flag_state_tuples  A pointer to the flag/state
 *                                 tuples array for which to set the
 *                                 state values corresponding to each
 *                                 flag.
 *  @param[in]  count              The number of array elements in
 *                                 @a flag_state_tuples.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a context_pointer or @a
 *                                     flag_state_tuples is null.
 *  @retval  -ENOENT                   If the backing file associated
 *                                     with a flag in @a
 *                                     flag_state_tuples does not
 *                                     exist.
 *
 *  @sa chkconfig_options_set
 *  @sa chkconfig_state_set
 *  @sa chkconfig_flag_state_tuple_init
 *
 *  @ingroup mutators
 *
 */
chkconfig_status_t chkconfig_state_set_multiple(chkconfig_context_pointer_t context_pointer,
                                                const chkconfig_flag_state_tuple_t *flag_state_tuples,
                                                size_t count)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(context_pointer != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigStateSetMultiple(*context_pointer,
                                               flag_state_tuples,
                                               count);

 done:
    return (retval);
}

// MARK: Utility

/**
 *  @brief
 *    Attempt to convert a state string into a state value.
 *
 *  This routine attempts to convert the specified null-terminated C
 *  state string into a state Boolean value.
 *
 *  @param[in]   state_string  A pointer to an immutable null-terminated
 *                             C string containing the state for which
 *                             to get the corresponding Boolean value.
 *  @param[out]  state         A pointer to storage by which to return
 *                             the state value if successful.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a state is null.
 *
 *  @sa chkconfig_state_get_state_string
 *
 *  @ingroup utility
 *
 */
chkconfig_status_t chkconfig_state_string_get_state(const char *state_string,
                                                    chkconfig_state_t *state)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(state != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigStateStringGetState(state_string,
                                                  *state);

 done:
    return (retval);
}

/**
 *  @brief
 *    Attempt to convert a state value into a state string.
 *
 *  This routine attempts to convert the specified state Boolean value
 *  into a null-terminated C state string.

 *
 *  @param[in]   state         The state value for which to get the
 *                             corresponding null-terminated C string.
 *  @param[out]  state_string  A pointer to storage for a pointer to an
 *                             immutable null-terminated C string by
 *                             which the state string if successful.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a state is null.
 *
 *  @sa chkconfig_state_string_get_state
 *
 *  @ingroup utility
 *
 */
chkconfig_status_t chkconfig_state_get_state_string(chkconfig_state_t state,
                                                    const char **state_string)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(state_string != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigStateGetStateString(state,
                                                  *state_string);

 done:
    return (retval);
}

/**
 *  @brief
 *    Allocate and initialize an array of flag/state tuples.
 *
 *  This attempts to allocate and initialize an array of the specified
 *  number of flag/state tuples.
 *
 *  @param[out]  flag_state_tuples  A pointer to storage for a pointer
 *                                  to the array of flag/state tuples
 *                                  to be allocated and initialized if
 *                                  successful.
 *  @param[in]   count              The number of flag/state tuples to
 *                                  be allocated and initialized.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a flag_state_tuples is null.
 *  @retval  -ENOMEM                   Resources could not be allocated
 *                                     for the requested array.
 *
 *  @sa chkconfig_flag_state_tuple_destroy
 *
 *  @ingroup utility
 *
 */
chkconfig_status_t chkconfig_flag_state_tuple_init(chkconfig_flag_state_tuple_t **flag_state_tuples,
                                                   size_t count)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(flag_state_tuples != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigFlagStateTupleInit(*flag_state_tuples,
                                                 count);

 done:
    return (retval);
}

/**
 *  @brief
 *    Deallocate an array of flag/state tuples.
 *
 *  This attempts to deallocate an array of the specified number of
 *  flag/state tuples.
 *
 *  @param[in,out]  flag_state_tuples  A pointer to the array of flag/
 *                                     state tuples to be allocated
 *                                     and initialized if successful.
 *  @param[in]      count              The number of flag/state tuples to
 *                                     be deallocated.
 *
 *  @retval  CHKCONFIG_STATUS_SUCCESS  If successful.
 *  @retval  -EINVAL                   If @a flag_state_tuples is null.
 *
 *  @sa chkconfig_flag_state_tuple_init
 *
 *  @ingroup utility
 *
 */
chkconfig_status_t chkconfig_flag_state_tuple_destroy(chkconfig_flag_state_tuple_t *flag_state_tuples,
                                                      size_t count)
{
    chkconfig_status_t retval = CHKCONFIG_STATUS_SUCCESS;

    nlREQUIRE_ACTION(flag_state_tuples != nullptr, done, retval = -EINVAL);

    retval = Detail::chkconfigFlagStateTupleDestroy(flag_state_tuples,
                                                    count);

 done:
    return (retval);
}
