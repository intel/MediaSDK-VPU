/*
 * Copyright (c) 2019-2020 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <va\va.h>
#include "va_backend.h"
#include <stdlib.h>

extern "C" VADisplay vaGetDisplayDRM(int fd);
extern "C" VAStatus vaInternalTerminateLight(VADriverContextP ctx);
extern "C" void InitGlobalMutex();

class vaDRM {
    VADisplay dpy = 0;
public:
    
    bool is_global_mutex_inited = false;

    vaDRM() {
    }

    virtual ~vaDRM() {
        /*VADisplayContextP pDisplayContext = (VADisplayContextP)dpy;
        if (!vaDisplayIsValid(dpy))
            return;

        VADriverContextP old_ctx = (((VADisplayContextP)dpy)->pDriverContext);
        vaInternalTerminateLight(old_ctx);*/
    }

    VADisplay createDisplay() {
        if (!is_global_mutex_inited)
        {
            is_global_mutex_inited = true;
            InitGlobalMutex();
        }

        VADisplay d = vaGetDisplayDRM(0);
        return d;
    }

    VADisplay init() {
        if (dpy)
            return dpy;

        if (!is_global_mutex_inited)
        {
            is_global_mutex_inited = true;
            InitGlobalMutex();
        }

        dpy = vaGetDisplayDRM(0);
        return dpy;
    }

    static vaDRM* getDRM() {
        static vaDRM drm;
        return &drm;
    }
};

extern "C" void va_DisplayContextDestroy(VADisplayContextP pDisplayContext)
{
    /*if (!pDisplayContext)
        return;

    free(pDisplayContext->pDriverContext);
    free(pDisplayContext);*/
}

extern "C" VADisplay vaGetDisplayWindowsDRM() {
    return vaDRM::getDRM()->createDisplay();
    //return vaDRM::getDRM()->init();
}

