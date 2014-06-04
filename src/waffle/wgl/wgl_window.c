// Copyright 2014 Emil Velikov
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "wcore_error.h"

#include "wgl_config.h"
#include "wgl_window.h"

bool
wgl_window_destroy(struct wcore_window *wc_self)
{
    struct wgl_window *self = wgl_window(wc_self);
    bool ok;

    if (!self)
        return true;

    ok = wcore_window_teardown(wc_self);
    free(self);
    return ok;
}

struct wcore_window*
wgl_window_create(struct wcore_platform *wc_plat,
                  struct wcore_config *wc_config,
                  int width,
                  int height)
{
    struct wgl_window *self;
    bool ok;

    self = wcore_calloc(sizeof(*self));
    if (!self)
        return NULL;

    ok = wcore_window_init(&self->wcore, wc_config);
    if (!ok)
        goto error;

    return &self->wcore;

error:
    wgl_window_destroy(&self->wcore);
    return NULL;
}

bool
wgl_window_show(struct wcore_window *wc_self)
{
    return false;
}

bool
wgl_window_resize(struct wcore_window *wc_self,
                  int32_t width, int32_t height)
{
    return false;
}

bool
wgl_window_swap_buffers(struct wcore_window *wc_self)
{
    return false;

}
