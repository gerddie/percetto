/*
 * Copyright (C) 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PERCETTO_H
#define PERCETTO_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
#include <atomic>
using std::atomic_uint_fast32_t;
using std::atomic_uint_fast64_t;
using std::atomic_uintptr_t;
using std::atomic_load_explicit;
using std::memory_order_relaxed;
using std::memory_order_acquire;
using std::size_t;
#else
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PERCETTO_MAX_CATEGORIES 256
#define PERCETTO_MAX_GROUP_CATEGORIES 32
#define PERCETTO_MAX_GROUP_SIZE 4
#define PERCETTO_MAX_CATEGORY_TAGS 4
#define PERCETTO_MAX_TRACKS 32

#ifdef NPERCETTO
#define PERCETTO_CATEGORY_DECLARE(MACRO)
#define PERCETTO_CATEGORY_DEFINE(MACRO)
#define PERCETTO_CATEGORY_IS_ENABLED(category) 0
#define PERCETTO_TRACK_DEFINE(track, track_type)
#define PERCETTO_INIT(clock_id) 0
#define PERCETTO_REGISTER_TRACK(track) 0
#else

/** Optionally declare categories in a header. */
#define PERCETTO_CATEGORY_DECLARE(MACRO) \
    MACRO(I_PERCETTO_CATEGORY_DECLARE_SEMICOLON, I_PERCETTO_BLANK) \
    MACRO(I_PERCETTO_BLANK, I_PERCETTO_CATEGORY_DECLARE_SEMICOLON)

/** Define categories in the compilation file where PERCETTO_INIT is called. */
#define PERCETTO_CATEGORY_DEFINE(MACRO) \
    MACRO(I_PERCETTO_CATEGORY_EXT_DEFINE_SEMICOLON, I_PERCETTO_BLANK) \
    MACRO(I_PERCETTO_BLANK, I_PERCETTO_GROUP_CATEGORY_EXT_DEFINE) \
    MACRO(I_PERCETTO_CATEGORY_DEFINE_SEMICOLON, I_PERCETTO_BLANK) \
    MACRO(I_PERCETTO_BLANK, I_PERCETTO_GROUP_CATEGORY_DEFINE) \
    static struct percetto_category* g_percetto_categories[] = { \
        MACRO(I_PERCETTO_CATEGORY_PTR_COMMA, I_PERCETTO_BLANK) \
        MACRO(I_PERCETTO_BLANK, I_PERCETTO_CATEGORY_PTR_COMMA) \
    };

/** Efficiently check if the category is enabled. */
#define PERCETTO_CATEGORY_IS_ENABLED(category) \
    (!!I_PERCETTO_LOAD_MASK(category))

/** Optionally declare tracks in a header. */
#define PERCETTO_TRACK_DECLARE(track) \
    extern struct percetto_track g_percetto_track_##track

/**
 * Define each track in a compilation file. For track_type see
 * percetto_track_type.
 */
#define PERCETTO_TRACK_DEFINE(track, track_type) \
    struct percetto_track g_percetto_track_##track = \
        { ATOMIC_VAR_INIT(0), ATOMIC_VAR_INIT(0), \
        #track, (int32_t)track_type, 0, NULL }

/**
 * Initialize the Percetto library.
 * Not thread safe. Only one call is allowed.
 * PERCETTO_CATEGORY_DEFINE must be called earlier.
 *
 * @param clock_id The clock that will be used for all trace event timestamps,
 *   whether passed through the API or retrieved internally.
 *   BUILTIN_CLOCK_BOOTTIME is the recommended choice, but some systems may not
 *   support it. PERCETTO_CLOCK_DONT_CARE will try to use BOOTTIME and fall
 *   back onto MONOTONIC. If you plan to manually set timestamps on any events,
 *   you must confirm that the clock works (ie: by checking the result of
 *   clock_gettime) and then pass in the corresponding PERCETTO_CLOCK enum
 *   rather than using PERCETTO_CLOCK_DONT_CARE.
 *
 * @return 0 on success or negative error code on failure. After failure, it is
 * safe to continue the application and the calls to trace macros below will
 * behave as if tracing is always disabled.
 * TODO(jbates): document all error codes.
 */
#define PERCETTO_INIT(clock_id) percetto_init( \
    sizeof(g_percetto_categories) / sizeof(g_percetto_categories[0]), \
    g_percetto_categories, clock_id)

/**
 * Up to PERCETTO_MAX_TRACKS tracks can be added for counters or events that
 * are not associated with the calling thread.
 * Tracks can never be removed for the lifetime of the process.
 * Thread safe. Can be called from any thread after percetto_init.
 */
#define PERCETTO_REGISTER_TRACK(track) \
    percetto_register_track(I_PERCETTO_TRACK_PTR(track))

/* Internal macros prefixed with I_. */

#define I_PERCETTO_BLANK(...)

#define I_PERCETTO_NINTH(a1, a2, a3, a4, a5, a6, a7, a8, a9, ...) a9
#define I_PERCETTO_COUNT_ARGS(...) \
    I_PERCETTO_NINTH(dummy, __VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0)
#define I_PERCETTO_CONCAT(a, b) a ## b
#define I_PERCETTO_CONCAT2(a, b) I_PERCETTO_CONCAT(a, b)

#define I_PERCETTO_LIST_CAT_PTRS2(C1, C2) \
    I_PERCETTO_CATEGORY_PTR(C1), I_PERCETTO_CATEGORY_PTR(C2)

#define I_PERCETTO_LIST_CAT_PTRS3(C1, C2, C3) \
    I_PERCETTO_CATEGORY_PTR(C1), I_PERCETTO_LIST_CAT_PTRS2(C2, C3)

#define I_PERCETTO_LIST_CAT_PTRS4(C1, C2, C3, C4) \
    I_PERCETTO_LIST_CAT_PTRS2(C1, C2), I_PERCETTO_CATEGORY_PTR(C3, C4)

#define I_PERCETTO_GROUP_CATEGORY_EXT_DEFINE(category, ...) \
    struct percetto_category_ext g_percetto_category_ext_##category = \
        { NULL, { NULL }, { I_PERCETTO_CONCAT2(I_PERCETTO_LIST_CAT_PTRS, \
          I_PERCETTO_COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__) }, NULL };

#define I_PERCETTO_GROUP_CATEGORY_DEFINE(category, ...) \
    struct percetto_category g_percetto_category_##category = \
        { ATOMIC_VAR_INIT(0), 0, \
          &g_percetto_category_ext_##category };

#define I_PERCETTO_CATEGORY_DECLARE(category) \
    extern struct percetto_category g_percetto_category_##category

#define I_PERCETTO_CATEGORY_DEFINE(category, ...) \
    struct percetto_category g_percetto_category_##category = \
        { ATOMIC_VAR_INIT(0), 0, \
          &g_percetto_category_ext_##category }

#define I_PERCETTO_CATEGORY_EXT_DEFINE_SEMICOLON(category, ...) \
    struct percetto_category_ext g_percetto_category_ext_##category = \
        { #category, {__VA_ARGS__}, { NULL }, NULL };

#define I_PERCETTO_TRACK_PTR(track) (&g_percetto_track_##track)

#define I_PERCETTO_CATEGORY_PTR(category) (&g_percetto_category_##category)

#define I_PERCETTO_UID3(a, b) percetto_uid_##a##b
#define I_PERCETTO_UID2(a, b) I_PERCETTO_UID3(a, b)
#define I_PERCETTO_UID(prefix) I_PERCETTO_UID2(prefix, __LINE__)

#define I_PERCETTO_CATEGORY_DECLARE_SEMICOLON(category, ...) \
    I_PERCETTO_CATEGORY_DECLARE(category);

#define I_PERCETTO_CATEGORY_DEFINE_SEMICOLON(category, ...) \
    I_PERCETTO_CATEGORY_DEFINE(category, __VA_ARGS__);

#define I_PERCETTO_CATEGORY_PTR_COMMA(category, ...) \
    I_PERCETTO_CATEGORY_PTR(category),

#define I_PERCETTO_LOAD_MASK_PTR(category) \
    (atomic_load_explicit(&(category)->sessions, memory_order_relaxed))

#define I_PERCETTO_LOAD_MASK(category) \
    I_PERCETTO_LOAD_MASK_PTR(&g_percetto_category_##category)

#endif /* NPERCETTO */

#define PERCETTO_LIKELY(x) __builtin_expect(!!(x), 1)
#define PERCETTO_UNLIKELY(x) __builtin_expect(!!(x), 0)

enum percetto_clock {
  /* Same as perfetto BuiltinClock. */
  PERCETTO_CLOCK_DONT_CARE = 0,
  PERCETTO_CLOCK_REALTIME = 1,
  PERCETTO_CLOCK_REALTIME_COARSE = 2,
  PERCETTO_CLOCK_MONOTONIC = 3,
  PERCETTO_CLOCK_MONOTONIC_COARSE = 4,
  PERCETTO_CLOCK_MONOTONIC_RAW = 5,
  PERCETTO_CLOCK_BOOTTIME = 6,
};

enum percetto_event_type {
  /* Same as perfetto TrackEvent_Type. */
  PERCETTO_EVENT_UNSPECIFIED = 0,
  PERCETTO_EVENT_BEGIN = 1,
  PERCETTO_EVENT_END = 2,
  PERCETTO_EVENT_INSTANT = 3,
  PERCETTO_EVENT_COUNTER = 4,
};

enum percetto_event_extended_type {
  PERCETTO_EVENT_EXTENDED_DEBUG_DATA = 0,
  PERCETTO_EVENT_EXTENDED_END,
};

enum percetto_event_debug_data_type {
  PERCETTO_EVENT_DEBUG_DATA_BOOL = 0,
  PERCETTO_EVENT_DEBUG_DATA_UINT,
  PERCETTO_EVENT_DEBUG_DATA_INT,
  PERCETTO_EVENT_DEBUG_DATA_DOUBLE,
  PERCETTO_EVENT_DEBUG_DATA_STRING,
  PERCETTO_EVENT_DEBUG_DATA_POINTER,
  PERCETTO_EVENT_DEBUG_DATA_END,
};

enum percetto_track_type {
  PERCETTO_TRACK_EVENTS = 0,
  PERCETTO_TRACK_COUNTER,
};

struct percetto_category {
  atomic_uint_fast32_t sessions;
  /* Category name id or 0 for group categories. */
  uint64_t name_iid;
  struct percetto_category_ext* ext;
};

struct percetto_category_ext {
  /* Category name or null for group categories. */
  const char* name;
  /* First string is description, followed by tags. */
  const char* strings[PERCETTO_MAX_CATEGORY_TAGS + 1];
  /* Only used for group categories. Two or more can be non-null for groups. */
  const struct percetto_category* group[PERCETTO_MAX_GROUP_SIZE];
  void* _reserved;
};

struct percetto_track {
  /* uuids are set during the register call. */
  atomic_uint_fast64_t uuid;
  atomic_uint_fast64_t parent_uuid;
  const char* name;
  /* See percetto_track_type */
  int32_t type;
  uint32_t _pad;
  void* _reserved;
};

struct percetto_event_data {
  /* Non-NULL implies target track, otherwise thread track is used. */
  const struct percetto_track* track;
  int64_t extra;
  /* Non-zero implies provided timestamp. */
  uint64_t timestamp;
  const char* name;
};

struct percetto_event_extended {
  /* See percetto_event_extended_type. */
  int32_t type;
  const struct percetto_event_extended* next;
};

struct percetto_event_debug_data {
  struct percetto_event_extended extended;
  /* See percetto_event_debug_data_type. */
  int32_t type;
  const char* name;
  union {
    uint32_t bool_value;
    uint64_t uint_value;
    int64_t int_value;
    double double_value;
    const char* string_value;
    uintptr_t pointer_value;
  };
};

/**
 * See PERCETTO_INIT.
 */
int percetto_init(size_t category_count,
                  struct percetto_category** categories,
                  enum percetto_clock clock_id);

/**
 * See PERCETTO_REGISTER_TRACK.
 */
int percetto_register_track(struct percetto_track* track);

/**
 * Registers a group category.
 * Up to PERCETTO_MAX_GROUP_CATEGORIES can be added.
 * Categories can never be removed for the lifetime of the process.
 * Thread safe. Can be called from any thread after percetto_init.
 *
 * @param category Category data in persistent memory.
 */
int percetto_register_group_category(struct percetto_category* category);

/** See TRACE_EVENT macros. */
void percetto_event_begin(struct percetto_category* category,
                          uint32_t sessions,
                          const char* name);

/** See TRACE_EVENT macros. */
void percetto_event_end(struct percetto_category* category,
                        uint32_t sessions);

/** See TRACE_INSTANT, TRACE_COUNTER, TRACE_ANY_WITH_ARGS macros. */
void percetto_event(struct percetto_category* category,
                    uint32_t sessions,
                    int32_t type,
                    const struct percetto_event_data* data);

/** See TRACE_DEBUG macros. */
void percetto_event_extended(struct percetto_category* category,
                             uint32_t sessions,
                             int32_t type,
                             const struct percetto_event_data* data,
                             const struct percetto_event_extended* extended);

/** See TRACE_EVENT macros. */
static inline void percetto_cleanup_end(struct percetto_category** category) {
  const uint32_t mask = I_PERCETTO_LOAD_MASK_PTR(*category);
  if (PERCETTO_UNLIKELY(mask))
    percetto_event_end(*category, mask);
}

#ifdef NPERCETTO
#define TRACE_EVENT(category, str_name)
#define TRACE_ANY_WITH_ARGS_PTR(type, category, ptrack, ts, str_name, extra)
#else

/**
 * Trace the current scope.
 *
 * @param category Category identifier.
 * @param str_name Must evaluate to a const char*. It is only evaluated when
 * tracing is enabled, but must outlive its own scope. Ie, do not do this:
 * str::string(compute_string()).c_str(), because the string will be destructed
 * too soon.
 */
#define TRACE_EVENT(category, str_name) \
    const uint32_t I_PERCETTO_UID(mask) = I_PERCETTO_LOAD_MASK(category); \
    if (PERCETTO_UNLIKELY(I_PERCETTO_UID(mask))) \
      percetto_event_begin(&g_percetto_category_##category, \
          I_PERCETTO_UID(mask), (str_name)); \
    struct percetto_category* I_PERCETTO_UID(trace_scoped) \
        __attribute__((cleanup(percetto_cleanup_end), unused)) = \
            &g_percetto_category_##category

#define TRACE_ANY_WITH_ARGS_PTR(type, category, ptrack, ts, str_name, \
                                extra_value) \
    do { \
      const uint32_t I_PERCETTO_UID(mask) = I_PERCETTO_LOAD_MASK_PTR(category); \
      if (PERCETTO_UNLIKELY(I_PERCETTO_UID(mask))) { \
        struct percetto_event_data I_PERCETTO_UID(data) = { \
          .track = ptrack, \
          .extra = (int64_t)(extra_value), \
          .timestamp = (ts), \
          .name = (str_name) \
        }; \
        percetto_event(category, I_PERCETTO_UID(mask), \
            (int32_t)(type), &I_PERCETTO_UID(data)); \
      } \
    } while(0)

#endif /* NPERCETTO */

#define TRACE_ANY_WITH_ARGS(type, category, track, ts, str_name, \
                            extra_value) \
    TRACE_ANY_WITH_ARGS_PTR(type, &g_percetto_category_##category, track, \
        ts, str_name, extra_value)

#define TRACE_EVENT_BEGIN(category, str_name) \
    TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_BEGIN, category, NULL, 0, str_name, 0)

#define TRACE_EVENT_END(category) \
    TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_END, category, NULL, 0, NULL, 0)

#define TRACE_EVENT_BEGIN_ON_TRACK(category, track, timestamp, str_name) \
    TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_BEGIN, category, \
        &g_percetto_track_##track, timestamp, str_name, 0)

#define TRACE_EVENT_END_ON_TRACK(category, track, timestamp) \
    TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_END, category, \
        &g_percetto_track_##track, timestamp, NULL, 0)

#define TRACE_INSTANT(category, str_name) \
    TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_INSTANT, category, NULL, 0, str_name, 0)

#define TRACE_INSTANT_ON_TRACK(category, track, str_name) \
    TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_INSTANT, category, \
        &g_percetto_track_##track, 0, str_name, 0)

#define TRACE_COUNTER(category, track, i64_value) \
    TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_COUNTER, category, \
        &g_percetto_track_##track, 0, NULL, i64_value)

#define TRACE_COUNTER_TS(category, track, timestamp, i64_value) \
    TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_COUNTER, category, \
        &g_percetto_track_##track, timestamp, NULL, i64_value)

#define TRACE_FLOW(category, str_name, i64_cookie) \
    TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_INSTANT, category, \
        NULL, 0, str_name, i64_cookie)

/* Debug data annotation macros */

#define PERCETTO_BOOL(str_name, value) { \
    .extended = { .type = PERCETTO_EVENT_EXTENDED_DEBUG_DATA, .next = NULL }, \
    .type = PERCETTO_EVENT_DEBUG_DATA_BOOL, \
    .name = str_name, .bool_value = (uint32_t)!!(value) \
  }

#define PERCETTO_UINT(str_name, value) { \
    .extended = { .type = PERCETTO_EVENT_EXTENDED_DEBUG_DATA, .next = NULL }, \
    .type = PERCETTO_EVENT_DEBUG_DATA_UINT, \
    .name = str_name, .uint_value = (value) \
  }

#define PERCETTO_INT(str_name, value) { \
    .extended = { .type = PERCETTO_EVENT_EXTENDED_DEBUG_DATA, .next = NULL }, \
    .type = PERCETTO_EVENT_DEBUG_DATA_INT, \
    .name = str_name, .int_value = (value) \
  }

#define PERCETTO_DOUBLE(str_name, value) { \
    .extended = { .type = PERCETTO_EVENT_EXTENDED_DEBUG_DATA, .next = NULL }, \
    .type = PERCETTO_EVENT_DEBUG_DATA_DOUBLE, \
    .name = str_name, .double_value = (value) \
  }

/**
 * @param str_value Only evaluated when tracing is enabled, but must outlive
 * its own scope. Ie, do not do this: str::string(compute_string()).c_str(),
 * because the string will be destructed too soon.
 */
#define PERCETTO_STRING(str_name, str_value) { \
    .extended = { .type = PERCETTO_EVENT_EXTENDED_DEBUG_DATA, .next = NULL }, \
    .type = PERCETTO_EVENT_DEBUG_DATA_STRING, \
    .name = str_name, .string_value = (str_value) \
  }

#define PERCETTO_POINTER(str_name, ptr) { \
    .extended = { .type = PERCETTO_EVENT_EXTENDED_DEBUG_DATA, .next = NULL }, \
    .type = PERCETTO_EVENT_DEBUG_DATA_POINTER, \
    .name = str_name, .pointer_value = (uintptr_t)(ptr) \
  }

#if (defined(NPERCETTO) || defined(NPERCETTODEBUG))
#define TRACE_DEBUG_DATA(category, data)
#define TRACE_DEBUG_DATA2(category, str_name, data1, data2)
#define TRACE_DEBUG_DATA3(category, str_name, data1, data2, data3)
#else

#define TRACE_DEBUG_DATA(category, data) \
    do { \
      const uint32_t I_PERCETTO_UID(mask) = I_PERCETTO_LOAD_MASK(category); \
      if (PERCETTO_UNLIKELY(I_PERCETTO_UID(mask))) { \
        struct percetto_event_debug_data I_PERCETTO_UID(dbg1) = data; \
        struct percetto_event_data I_PERCETTO_UID(evdata) = { \
          .track = NULL, \
          .extra = 0, \
          .timestamp = 0, \
          .name = I_PERCETTO_UID(dbg1).name \
        }; \
        percetto_event_extended(&g_percetto_category_##category, \
            I_PERCETTO_UID(mask), (int32_t)PERCETTO_EVENT_INSTANT, \
            &I_PERCETTO_UID(evdata), &I_PERCETTO_UID(dbg1).extended); \
      } \
    } while(0)

#define TRACE_DEBUG_DATA2(category, str_name, data1, data2) \
    do { \
      const uint32_t I_PERCETTO_UID(mask) = I_PERCETTO_LOAD_MASK(category); \
      if (PERCETTO_UNLIKELY(I_PERCETTO_UID(mask))) { \
        struct percetto_event_data I_PERCETTO_UID(evdata) = { \
          .track = NULL, \
          .extra = 0, \
          .timestamp = 0, \
          .name = (str_name) \
        }; \
        struct percetto_event_debug_data I_PERCETTO_UID(dbg1) = data1; \
        struct percetto_event_debug_data I_PERCETTO_UID(dbg2) = data2; \
        I_PERCETTO_UID(dbg1).extended.next = &I_PERCETTO_UID(dbg2).extended; \
        percetto_event_extended(&g_percetto_category_##category, \
            I_PERCETTO_UID(mask), (int32_t)PERCETTO_EVENT_INSTANT, \
            &I_PERCETTO_UID(evdata), &I_PERCETTO_UID(dbg1).extended); \
      } \
    } while(0)

#define TRACE_DEBUG_DATA3(category, str_name, data1, data2, data3) \
    do { \
      const uint32_t I_PERCETTO_UID(mask) = I_PERCETTO_LOAD_MASK(category); \
      if (PERCETTO_UNLIKELY(I_PERCETTO_UID(mask))) { \
        struct percetto_event_data I_PERCETTO_UID(evdata) = { \
          .track = NULL, \
          .extra = 0, \
          .timestamp = 0, \
          .name = (str_name) \
        }; \
        struct percetto_event_debug_data I_PERCETTO_UID(dbg1) = data1; \
        struct percetto_event_debug_data I_PERCETTO_UID(dbg2) = data2; \
        struct percetto_event_debug_data I_PERCETTO_UID(dbg3) = data3; \
        I_PERCETTO_UID(dbg1).extended.next = &I_PERCETTO_UID(dbg2).extended; \
        I_PERCETTO_UID(dbg2).extended.next = &I_PERCETTO_UID(dbg3).extended; \
        percetto_event_extended(&g_percetto_category_##category, \
            I_PERCETTO_UID(mask), (int32_t)PERCETTO_EVENT_INSTANT, \
            &I_PERCETTO_UID(evdata), &I_PERCETTO_UID(dbg1).extended); \
      } \
    } while(0)

#endif /* NPERCETTO || NPERCETTODEBUG */

#ifdef __cplusplus
}
#endif

#endif /* PERCETTO_H */
