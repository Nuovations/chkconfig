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
 *      This file defines a chkconfig project-specific wrapper header
 *      for the Nest Labs Assert package that defines desired default
 *      behavior of that package for this project.
 *
 */

#ifndef CHKCONFIG_ASSERT_H
#define CHKCONFIG_ASSERT_H


#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @def NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  Indicate that the package desires the default Nest Labs Assert
 *  behavior flags.
 *
 */
#define NL_ASSERT_USE_FLAGS_DEFAULT 1

/**
 *  @def NL_ASSERT_LOG
 *
 *  Indicate that the package will use standard I/O, as defined.
 *
 */
#define NL_ASSERT_LOG(aPrefix, aName, aCondition, aLabel, aFile, aLine, aMessage)       \
    do                                                                                  \
    {                                                                                   \
        fprintf(stderr,                                                                 \
                NL_ASSERT_LOG_FORMAT_DEFAULT,                                           \
                aPrefix,                                                                \
                (((aName) == 0) || (*(aName) == '\0')) ? "" : aName,                    \
                (((aName) == 0) || (*(aName) == '\0')) ? "" : ": ",                     \
                aCondition,                                                             \
                ((aMessage == 0) ? "" : aMessage),                                      \
                ((aMessage == 0) ? "" : ", "),                                          \
                aFile,                                                                  \
                aLine);                                                                 \
    } while (0)

#include <nlassert.h>

#ifdef __cplusplus
}
#endif

#endif // CHKCONFIG_ASSERT_H
