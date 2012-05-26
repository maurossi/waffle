// Copyright 2012 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifdef __linux__
#   define _XOPEN_SOURCE 600
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <waffle/core/wcore_error.h>
#include <waffle_test/waffle_test.h>

TESTGROUP_SIMPLE(wcore_error)

TEST(wcore_error, code_unknown_error)
{
    wcore_error_reset();
    wcore_error(WAFFLE_UNKNOWN_ERROR);
    EXPECT_TRUE(wcore_error_get_code() == WAFFLE_UNKNOWN_ERROR);
    EXPECT_TRUE(!strcmp(wcore_error_get_info()->message, ""));

}

TEST(wcore_error, code_bad_attribute)
{
    wcore_error_reset();
    wcore_error(WAFFLE_BAD_ATTRIBUTE);
    EXPECT_TRUE(wcore_error_get_code() == WAFFLE_BAD_ATTRIBUTE);
    EXPECT_TRUE(!strcmp(wcore_error_get_info()->message, ""));

}

TEST(wcore_error, with_message)
{
    wcore_error_reset();
    wcore_errorf(WAFFLE_BAD_PARAMETER, "bad %s (0x%x)", "gl_api", 0x17);
    EXPECT_TRUE(wcore_error_get_code() == WAFFLE_BAD_PARAMETER);
    EXPECT_TRUE(!strcmp(wcore_error_get_info()->message, "bad gl_api (0x17)"));
}

TEST(wcore_error, internal_error)
{
    char error_location[1024];
    snprintf(error_location, 1024, "%s:%d:", __FILE__, __LINE__ + 3);

    wcore_error_reset();
    wcore_error_internal("%s zoroaster %d", "hello", 5);
    EXPECT_TRUE(wcore_error_get_code() == WAFFLE_INTERNAL_ERROR);
    EXPECT_TRUE(strstr(wcore_error_get_info()->message, "hello zoroaster 5"));
    EXPECT_TRUE(strstr(wcore_error_get_info()->message, error_location));
}

TEST(wcore_error, first_call_without_message_wins)
{
    wcore_error_reset();
    wcore_errorf(WAFFLE_UNKNOWN_ERROR, "cookies");
    wcore_error(WAFFLE_BAD_ATTRIBUTE);
    EXPECT_TRUE(wcore_error_get_code() == WAFFLE_UNKNOWN_ERROR);
    EXPECT_TRUE(!strcmp(wcore_error_get_info()->message, "cookies"));
}

TEST(wcore_error, first_call_with_message_wins)
{
    wcore_error_reset();
    wcore_errorf(WAFFLE_UNKNOWN_ERROR, "cookies");
    wcore_errorf(WAFFLE_NO_ERROR, "all is well");
    EXPECT_TRUE(wcore_error_get_code() == WAFFLE_UNKNOWN_ERROR);
    EXPECT_TRUE(!strcmp(wcore_error_get_info()->message, "cookies"));
}

TEST(wcore_error, disable_then_error)
{
    wcore_error_reset();
    wcore_error(WAFFLE_NOT_INITIALIZED);
    WCORE_ERROR_DISABLED(
        wcore_error(WAFFLE_BAD_ATTRIBUTE);
    );
    EXPECT_TRUE(wcore_error_get_code() == WAFFLE_NOT_INITIALIZED);
}

TEST(wcore_error, disable_then_errorf)
{
    wcore_error_reset();
    wcore_error(WAFFLE_NOT_INITIALIZED);
    WCORE_ERROR_DISABLED(
        wcore_errorf(WAFFLE_BAD_ATTRIBUTE, "i'm not here");
    );
    EXPECT_TRUE(wcore_error_get_code() == WAFFLE_NOT_INITIALIZED);
}

TEST(wcore_error, disable_then_error_internal)
{
    wcore_error_reset();
    wcore_error(WAFFLE_NOT_INITIALIZED);
    WCORE_ERROR_DISABLED({
        // Compilation fails with gcc when wcore_error_internal() appears
        // here. So directly the macro's implementing function.
        _wcore_error_internal(__FILE__, __LINE__, "this isn't happening");
    });
    EXPECT_TRUE(wcore_error_get_code() == WAFFLE_NOT_INITIALIZED);
}

/// Number of threads in test wcore_error.thread_local.
enum {
    NUM_THREADS = 3,
};

/// Given to pthread_create() in test wcore_error.thread_local.
///
/// A sub-thread, after calling wcore_error(), locks the mutex, increments
/// `num_threads_waiting`, and waits on `cond`. When all sub-threads are
/// waiting (that is, `num_threads_waiting == TOTAL_THREADS`), then the main
/// thread broadcasts on `cond`.
struct thread_arg {
    /// Has value in range `[0, TOTAL_THREADS)`.
    int thread_id;

    /// Protects `num_threads_waiting` and `cond`.
    pthread_mutex_t *mutex;

    /// Satisfied when `num_threads_waiting == TOTAL_THREADS`.
    pthread_cond_t *cond;

    /// Number of threads waiting on `cond`.
    int *num_threads_waiting;
};

/// The start routine given to threads in test wcore_error.thread_local.
static bool
thread_start(struct thread_arg *a)
{
    static const int error_codes[NUM_THREADS] = {
        WAFFLE_BAD_ATTRIBUTE,
        WAFFLE_UNKNOWN_ERROR,
        WAFFLE_ALREADY_INITIALIZED,
    };

    bool ok = true;
    int error_code = error_codes[a->thread_id];

    // Each thread begins in an error-free state.
    ok &= wcore_error_get_code() == WAFFLE_NO_ERROR;

    // Each thread sets its error code.
    wcore_error(error_code);

    // Wait for all threads to set their error codes, thus giving
    // the threads opportunity to clobber each other's codes.
    pthread_mutex_lock(a->mutex); {
        *a->num_threads_waiting += 1;
        pthread_cond_wait(a->cond, a->mutex);
        pthread_mutex_unlock(a->mutex);
    }

    // Verify that the threads did not clobber each other's
    // error codes.
    ok &= wcore_error_get_code() == error_code;

    return ok;
}

// Test that threads do not clobber each other's error codes.
TEST(wcore_error, thread_local)
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int num_threads_waiting = 0;

    pthread_t threads[NUM_THREADS];
    struct thread_arg thread_args[NUM_THREADS];
    bool exit_codes[NUM_THREADS];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    for (intptr_t i = 0; i < NUM_THREADS; ++i) {
        struct thread_arg *a = &thread_args[i];
        a->thread_id = i;
        a->mutex = &mutex;
        a->cond = &cond;
        a->num_threads_waiting = &num_threads_waiting;

        pthread_create(&threads[i], NULL,
                      (void* (*)(void*)) thread_start,
                      (void*) a);
    }

    // Wait for all threads to set their error codes, thus giving
    // the threads opportunity to clobber each other's codes.
    while (num_threads_waiting < NUM_THREADS)
        ;;
    pthread_mutex_lock(&mutex); {
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], (void**) &exit_codes[i]);
        EXPECT_TRUE(exit_codes[i] == true);
    }

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

void
testsuite_wcore_error(void)
{
    TEST_RUN(wcore_error, code_unknown_error);
    TEST_RUN(wcore_error, code_bad_attribute);
    TEST_RUN(wcore_error, code_unknown_error);
    TEST_RUN(wcore_error, with_message);
    TEST_RUN(wcore_error, internal_error);
    TEST_RUN(wcore_error, first_call_without_message_wins);
    TEST_RUN(wcore_error, first_call_with_message_wins);
    TEST_RUN(wcore_error, disable_then_error);
    TEST_RUN(wcore_error, disable_then_errorf);
    TEST_RUN(wcore_error, disable_then_error_internal);
    TEST_RUN(wcore_error, thread_local);
}
