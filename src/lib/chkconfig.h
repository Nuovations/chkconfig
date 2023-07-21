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
 *      This file defines interfaces for the chkconfig configuruation
 *      management library.
 *
 */

#ifndef CHKCONFIG_H
#define CHKCONFIG_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 *  The request or operatin completed successfully.
 */
#define CHKCONFIG_STATUS_SUCCESS  0

// MARK: Type Declarations

/**
 *  @typedef int32_t chkconfig_status_t
 *
 *  @brief
 *    Type used for all enumerated success or failure status.
 *
 *  @sa chkconfig_error_t
 *
 */
typedef int32_t                            chkconfig_status_t;

/**
 *  @typedef chkconfig_status_t chkconfig_error_t
 *
 *  @brief
 *    Type used for all enumerated failure status.
 *
 *  @sa chkconfig_status_t
 *
 */
typedef chkconfig_status_t                 chkconfig_error_t;

/**
 *  A convenience type for a state flag.
 *
 */
typedef const char *                       chkconfig_flag_t;

/**
 *  A convenience type for a state value.
 *
 */
typedef bool                               chkconfig_state_t;

/**
 *  An enumeration indicating the origin of a flag state.
 *
 */
typedef enum
{
    CHKCONFIG_ORIGIN_UNKNOWN = 0, //!< The flag state origin is unknown.

    CHKCONFIG_ORIGIN_NONE    = 1, //!< The flag state origin does not exist.

    CHKCONFIG_ORIGIN_DEFAULT = 2, //!< The flag state origin was from the
                                  //!< default backing store.

    CHKCONFIG_ORIGIN_STATE   = 3  //!< The flag state origin was from the
                                  //!< state backing store.
} chkconfig_origin_t;

/**
 *  A structure for manipulating a state flag and value as a pair.
 *
 */
struct chkconfig_flag_state_tuple
{
    chkconfig_flag_t   m_flag;   //!< The flag.
    chkconfig_state_t  m_state;  //!< The state value associated with the flag.
    chkconfig_origin_t m_origin; //!< The origin of the flag state.
};

/**
 *  A convenience type for manipulating a state flag and value as a pair.
 *
 */
typedef struct chkconfig_flag_state_tuple  chkconfig_flag_state_tuple_t;

struct _chkconfig_context;

/**
 *  A convenience type for chkconfig library context.
 *
 */
typedef struct _chkconfig_context          chkconfig_context_t;

/**
 *  A convenience type for a pointer to chkconfig library context.
 *
 *  Most chkconfig library interfaces take a pointer to this context.
 *
 */
typedef chkconfig_context_t *              chkconfig_context_pointer_t;

/**
 *  A forward declaration for an opaque type for chkconfig library
 *  runtime options.
 *
 *  @private
 *
 */
struct _chkconfig_options;

/**
 *  A convenience type for chkconfig library runtime options.
 *
 */
typedef struct _chkconfig_options          chkconfig_options_t;

/**
 *  A convenience type for a pointer to chkconfig library runtime
 *  options.
 *
 */
typedef chkconfig_options_t *              chkconfig_options_pointer_t;

/**
 *  A set of enumerations used to encode chkconfig option key/value
 *  pair keys.
 *
 *  @private
 *
 */
enum _chkconfig_option_type {
    _CHKCONFIG_OPTION_TYPE_NONE    = 0x000,

    _CHKCONFIG_OPTION_TYPE_INT8    = 0x001,
    _CHKCONFIG_OPTION_TYPE_UINT8   = 0x002,
    _CHKCONFIG_OPTION_TYPE_INT16   = 0x004,
    _CHKCONFIG_OPTION_TYPE_UINT16  = 0x008,
    _CHKCONFIG_OPTION_TYPE_INT32   = 0x010,
    _CHKCONFIG_OPTION_TYPE_UINT32  = 0x020,
    _CHKCONFIG_OPTION_TYPE_INT64   = 0x040,
    _CHKCONFIG_OPTION_TYPE_UINT64  = 0x080,

    _CHKCONFIG_OPTION_TYPE_POINTER = 0x100,

    _CHKCONFIG_OPTION_TYPE_BOOLEAN = _CHKCONFIG_OPTION_TYPE_UINT8,
    _CHKCONFIG_OPTION_TYPE_CSTRING = _CHKCONFIG_OPTION_TYPE_POINTER
};

/**
 *  The bitmask for the option key type encoding.
 *
 *  @private
 *
 */
#define _CHKCONFIG_OPTION_TYPE_MASK          0xFFF00000

/**
 *  The bit shift for the option key type encoding.
 *
 *  @private
 *
 */
#define _CHKCONFIG_OPTION_TYPE_SHIFT         20

/**
 *  The bitmask for the option key value encoding.
 *
 *  @private
 *
 */
#define _CHKCONFIG_OPTION_VALUE_MASK         0x000FFFFF

/**
 *  The bit shift for the option key value encoding.
 *
 *  @private
 *
 */
#define _CHKCONFIG_OPTION_VALUE_SHIFT         0

/**
 *  Encode an option key type and value.
 *
 *  @param[in]  type   The type of value associated with the option key.
 *  @param[in]  value  The actual value to encode for the option key.
 *
 *  @private
 *
 */
#define _CHKCONFIG_OPTION_ENCODE(type, value)                                         \
        (((( type) << _CHKCONFIG_OPTION_TYPE_SHIFT ) & _CHKCONFIG_OPTION_TYPE_MASK) | \
         (((value) << _CHKCONFIG_OPTION_VALUE_SHIFT) & _CHKCONFIG_OPTION_VALUE_MASK))

/**
 *  An enumeration key for specifying a chkconfig option key/value pair.
 *
 */
enum {
    /**
     *  An option key whose immutable null-terminated C string value
     *  is the read/write flag state backing file directory.
     *
     */
    CHKCONFIG_OPTION_STATE_DIRECTORY        = _CHKCONFIG_OPTION_ENCODE(_CHKCONFIG_OPTION_TYPE_CSTRING, 1),

    /**
     *  An option key whose Boolean value, when asserted, indicates
     *  that backing state files that do not already exist should be
     *  created.
     *
     */
    CHKCONFIG_OPTION_FORCE_STATE            = _CHKCONFIG_OPTION_ENCODE(_CHKCONFIG_OPTION_TYPE_BOOLEAN, 2),

    /**
     *  An option key whose immutable null-terminated C string value
     *  is the read-only flag state fallback 'default' backing file
     *  directory to use when a flag does not exist in the 'state'
     *  directory.
     *
     */
    CHKCONFIG_OPTION_DEFAULT_DIRECTORY      = _CHKCONFIG_OPTION_ENCODE(_CHKCONFIG_OPTION_TYPE_CSTRING, 3),

    /**
     *  An option key whose Boolean value, when asserted, indicates
     *  that the read-only flag state fallback default directory
     *  should be used when a flag does not exist in the state
     *  directory.
     *
     */
    CHKCONFIG_OPTION_USE_DEFAULT_DIRECTORY  = _CHKCONFIG_OPTION_ENCODE(_CHKCONFIG_OPTION_TYPE_BOOLEAN, 4)
};

/**
 *  A type for a chkconfig option key/value pair key.
 *
 */
typedef uint32_t chkconfig_option_t;

// MARK: Function Prototypes

// MARK: Utility

extern chkconfig_status_t chkconfig_origin_get_origin_string(chkconfig_origin_t origin,
                                                             const char **origin_string);

extern chkconfig_status_t chkconfig_state_string_get_state(const char *state_string,
                                                           chkconfig_state_t *state);
extern chkconfig_status_t chkconfig_state_get_state_string(chkconfig_state_t state,
                                                           const char **state_string);

extern chkconfig_status_t chkconfig_flag_state_tuple_init(chkconfig_flag_state_tuple_t **flag_state_tuples,
                                                          size_t count);
extern chkconfig_status_t chkconfig_flag_state_tuple_destroy(chkconfig_flag_state_tuple_t *flag_state_tuples,
                                                             size_t count);
extern int chkconfig_flag_state_tuple_flag_compare_function(const void *first_tuple,
                                                            const void *second_tuple);
extern int chkconfig_flag_state_tuple_state_compare_function(const void *first_tuple,
                                                             const void *second_tuple);

// MARK: Lifetime Management

extern chkconfig_status_t chkconfig_init(chkconfig_context_pointer_t *context_pointer);
extern chkconfig_status_t chkconfig_options_init(chkconfig_context_pointer_t context_pointer,
                                                 chkconfig_options_pointer_t *options_pointer);
extern chkconfig_status_t chkconfig_options_destroy(chkconfig_context_pointer_t context_pointer,
                                                    chkconfig_options_pointer_t *options_pointer);
extern chkconfig_status_t chkconfig_destroy(chkconfig_context_pointer_t *context_pointer);

// MARK: Option Management

extern chkconfig_status_t chkconfig_options_set(chkconfig_context_pointer_t context_pointer,
                                                chkconfig_options_pointer_t options_pointer,
                                                chkconfig_option_t option,
                                                ...);

// MARK: Observers

extern chkconfig_status_t chkconfig_state_get(chkconfig_context_pointer_t context_pointer,
                                              chkconfig_flag_t flag,
                                              chkconfig_state_t *state);
extern chkconfig_status_t chkconfig_state_get_with_origin(chkconfig_context_pointer_t context_pointer,
                                                          chkconfig_flag_t flag,
                                                          chkconfig_state_t *state,
                                                          chkconfig_origin_t *origin);

extern chkconfig_status_t chkconfig_state_get_multiple(chkconfig_context_pointer_t context_pointer,
                                                       chkconfig_flag_state_tuple_t *flag_state_tuples,
                                                       size_t count);
extern chkconfig_status_t chkconfig_state_get_count(chkconfig_context_pointer_t context_pointer,
                                                    size_t *count);
extern chkconfig_status_t chkconfig_state_copy_all(chkconfig_context_pointer_t context_pointer,
                                                   chkconfig_flag_state_tuple_t **flag_state_tuples,
                                                   size_t *count);

// MARK: Mutators

extern chkconfig_status_t chkconfig_state_set(chkconfig_context_pointer_t context_pointer,
                                              chkconfig_flag_t flag,
                                              chkconfig_state_t state);
extern chkconfig_status_t chkconfig_state_set_multiple(chkconfig_context_pointer_t context_pointer,
                                                       const chkconfig_flag_state_tuple_t *flag_state_tuples,
                                                       size_t count);

#ifdef __cplusplus
}
#endif

#endif // CHKCONFIG_H
