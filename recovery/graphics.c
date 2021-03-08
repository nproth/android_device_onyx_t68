/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
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

// Freescale epdc code sourced from https://github.com/onyx-intl/platform_bootable/blob/master/recovery/minui/graphics.c

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>

// #include <linux/mxcfb.h>
#include "mxcfb.h"


// ioctl() function numbers from mxcfb.h do not match
// From strace /system/bin/recovery
#define ONYX_SET_AUTO_UPDATE_MODE      0x4004462d
#define ONYX_SET_WAVEFORM_MODES        0x4018462b
#define ONYX_SET_UPDATE_SCHEME         0x40044632

#define ONYX_SEND_UPDATE               0x4044462e
#define ONYX_WAIT_FOR_UPDATE_COMPLETE  0xc008462f

#include <pixelflinger/pixelflinger.h>

#include "roboto_15x24.h"

#include "minui.h"

typedef struct {
    GGLSurface texture;
    unsigned cwidth;
    unsigned cheight;
    unsigned ascent;
} GRFont;

static GRFont *gr_font = 0;
static GGLContext *gr_context = 0;
static GGLSurface gr_font_texture;
static GGLSurface gr_framebuffer;
static GGLSurface gr_mem_surface;

static int gr_fb_fd = -1;


static struct fb_fix_screeninfo fi;
static struct fb_var_screeninfo vi;

static unsigned int marker_val = 0;

bool target_has_overlay(char *version)
{
    return false;
}

bool isTargetMdp5() 
{
    return false;
}

int free_ion_mem(void)
{
    return -EINVAL;
}

int alloc_ion_mem(unsigned int size)
{
    return -EINVAL;
}

int allocate_overlay(int fd, GGLSurface gr_fb[])
{
    return -EINVAL;
}

int free_overlay(int fd)
{
    return -EINVAL;
}

int overlay_display_frame(int fd, GGLubyte* data, size_t size)
{
    return -EINVAL;
}

void setDisplaySplit(void) {
    fprintf(stderr, "Display split not supported\n");
}

int getLeftSplit(void) {
   return 0;
}

int getRightSplit(void) {
   return 0;
}

bool isDisplaySplit(void) {
    return false;
}

static void update_to_display(int left, int top, int width, int height, int update_mode,
	int wait_for_complete, uint flags)
{
	int retval;
	struct mxcfb_update_data upd_data = {
        .update_mode = update_mode,
        .waveform_mode = WAVEFORM_MODE_AUTO,
        .temp = TEMP_USE_AMBIENT,
        .flags = flags,
        .update_region = {
            .left = left,
            .top = top,
            .width = width,
            .height = height
        }
    };

    /* Crop to display, else driver complains */
    if(left >= (int) vi.xres || top >= (int) vi.yres) {
        return;
    }
    if(left + width > (int) vi.xres) {
        upd_data.update_region.width = vi.xres - left;
    }
    if(top + height > (int) vi.yres) {
        upd_data.update_region.height = vi.yres - top;
    }

	if (wait_for_complete) {
		/* Get unique marker value */
		upd_data.update_marker = ++marker_val;
	} else {
		upd_data.update_marker = 0;
	}

	retval = ioctl(gr_fb_fd, ONYX_SEND_UPDATE, &upd_data);
	
    int i;
    for(i = 0; retval < 0 && i < 10; i++) {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
        printf("Update failed, retry\n");
		sleep(1);
		retval = ioctl(gr_fb_fd, ONYX_SEND_UPDATE, &upd_data);
	}
    if(retval < 0) {
        printf("Update failed finally. Error = 0x%x\n", retval);
        return;
    }

	if (wait_for_complete) {
		/* Wait for update to complete */
		retval = ioctl(gr_fb_fd, ONYX_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
		if (retval < 0) {
			printf("Wait for update complete failed.  Error = 0x%x\n", retval);
		}
	}
}

static int get_framebuffer(GGLSurface *fb)
{
    int fd;
    void *bits;
    int auto_update_mode = AUTO_UPDATE_MODE_REGION_MODE;
    int scheme = UPDATE_SCHEME_QUEUE;
    struct mxcfb_waveform_modes wv_modes = {
        .mode_init = 0,
        .mode_du = 1,
        .mode_gc4 = 3,
        .mode_gc8 = 2,
        .mode_gc16 = 2,
        .mode_gc32 = 2
    };

    fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        perror("cannot open fb0");
        return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
        perror("failed to get fb0 info");
        close(fd);
        return -1;
    }

    vi.bits_per_pixel = 8;
    vi.grayscale = GRAYSCALE_8BIT_INVERTED;
    vi.rotate = FB_ROTATE_CW;
    vi.activate = FB_ACTIVATE_FORCE;

    if (ioctl(fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("failed to put fb0 info");
        close(fd);
        return -1;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
        perror("failed to get fb0 info");
        close(fd);
        return -1;
    }

    bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        close(fd);
        return -1;
    }

    fb->version = sizeof(*fb);
    fb->width = vi.xres;
    fb->height = vi.yres;
    fb->stride = fi.line_length;
    fb->format = GGL_PIXEL_FORMAT_L_8;
    fb->data = bits;

    memset(fb->data, 0, fi.line_length * vi.yres);

    if (ioctl(fd, ONYX_SET_AUTO_UPDATE_MODE, &auto_update_mode) < 0) {
        perror("set auto update mode failed\n");
        return -1;
    }

    if (ioctl(fd, ONYX_SET_WAVEFORM_MODES, &wv_modes) < 0) {
        perror("set waveform modes failed\n");
        return -1;
    }

    if (ioctl(fd, ONYX_SET_UPDATE_SCHEME, &scheme) < 0) {
        perror("set update scheme failed\n");
        return -1;
    }

    return fd;
}

int getFbXres(void) {
    return vi.xres;
}

int getFbYres (void) {
    return vi.yres;
}

static void get_memory_surface(GGLSurface* ms) {
  ms->version = sizeof(*ms);
  ms->width = vi.xres;
  ms->height = vi.yres;
  ms->stride = fi.line_length;
  ms->data = malloc(fi.line_length * vi.yres);
  ms->format = GGL_PIXEL_FORMAT_L_8;
}

void gr_flip(void)
{
    /* Copy new data into fb */
    memcpy(gr_framebuffer.data, gr_mem_surface.data, fi.line_length * vi.yres);

    /* And apply screen update */
   update_to_display(0, 0, vi.xres, vi.yres, UPDATE_MODE_PARTIAL, true, 0);
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    GGLContext *gl = gr_context;
    GGLint color[4];
    color[0] = ((r << 8) | r) + 1;
    color[1] = ((g << 8) | g) + 1;
    color[2] = ((b << 8) | b) + 1;
    color[3] = ((a << 8) | a) + 1;
    gl->color4xv(gl, color);
}

int gr_measure(const char *s)
{
    return gr_font->cwidth * strlen(s);
}

void gr_font_size(int *x, int *y)
{
    *x = gr_font->cwidth;
    *y = gr_font->cheight;
}

int gr_text(int x, int y, const char *s, int bold)
{
    GGLContext *gl = gr_context;
    GRFont *font = gr_font;
    unsigned off;

    y -= font->ascent;

    int left = x, top = y;

    gl->bindTexture(gl, &font->texture);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);

    while((off = *s++)) {
        off -= 32;
        if (off < 96) {
            gl->texCoord2i(gl, (off * font->cwidth) - x, 0 - y);
            gl->recti(gl, x, y, x + font->cwidth, y + font->cheight);
        }
        x += font->cwidth;
    }

    return x;
}

void gr_texticon(int x, int y, gr_surface icon) {
    if (gr_context == NULL || icon == NULL) {
        return;
    }
    GGLContext* gl = gr_context;

    gl->bindTexture(gl, (GGLSurface*) icon);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);

    int w = gr_get_width(icon);
    int h = gr_get_height(icon);

    gl->texCoord2i(gl, -x, -y);
    gl->recti(gl, x, y, x+gr_get_width(icon), y+gr_get_height(icon));
}

void gr_fill(int x1, int y1, int x2, int y2)
{
    GGLContext *gl = gr_context;
    gl->disable(gl, GGL_TEXTURE_2D);
    gl->recti(gl, x1, y1, x2, y2);
}

void gr_blit(gr_surface source, int sx, int sy, int w, int h, int dx, int dy) {
    if (gr_context == NULL || source == NULL) {
        return;
    }
    GGLContext *gl = gr_context;

    gl->bindTexture(gl, (GGLSurface*) source);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);
    gl->texCoord2i(gl, sx - dx, sy - dy);
    gl->recti(gl, dx, dy, dx + w, dy + h);
}

unsigned int gr_get_width(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->width;
}

unsigned int gr_get_height(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->height;
}

static void gr_init_font(void)
{
    GGLSurface *ftex;
    unsigned char *bits, *rle;
    unsigned char *in, data;

    gr_font = calloc(sizeof(*gr_font), 1);
    ftex = &gr_font->texture;

    bits = malloc(font.width * font.height);

    ftex->version = sizeof(*ftex);
    ftex->width = font.width;
    ftex->height = font.height;
    ftex->stride = font.width;
    ftex->data = (void*) bits;
    ftex->format = GGL_PIXEL_FORMAT_A_8;

    in = font.rundata;
    while((data = *in++)) {
        memset(bits, (data & 0x80) ? 255 : 0, data & 0x7f);
        bits += (data & 0x7f);
    }

    gr_font->cwidth = font.cwidth;
    gr_font->cheight = font.cheight;
    gr_font->ascent = font.cheight - 2;
}

int gr_init(void)
{
    gglInit(&gr_context);
    GGLContext *gl = gr_context;

    gr_init_font();

    gr_fb_fd = get_framebuffer(&gr_framebuffer);
    if (gr_fb_fd < 0) {
        gr_exit();
        return -1;
    }

    get_memory_surface(&gr_mem_surface);

    fprintf(stderr, "framebuffer: fd %d (%d x %d)\n",
            gr_fb_fd, gr_framebuffer.width, gr_framebuffer.height);

    gl->colorBuffer(gl, &gr_mem_surface);
    gl->activeTexture(gl, 0);
    gl->enable(gl, GGL_BLEND);
    gl->blendFunc(gl, GGL_SRC_ALPHA, GGL_ONE_MINUS_SRC_ALPHA);

    gr_fb_blank(true);
    gr_fb_blank(false);

    return 0;
}

void gr_exit(void)
{
    close(gr_fb_fd);
    gr_fb_fd = -1;

    free(gr_mem_surface.data);
}

int gr_fb_width(void)
{
    return gr_framebuffer.width;
}

int gr_fb_height(void)
{
    return gr_framebuffer.height;
}

gr_pixel *gr_fb_data(void)
{
    return (unsigned short *) gr_mem_surface.data;
}

void gr_fb_blank(bool blank)
{
    int ret = ioctl(gr_fb_fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
    if (ret < 0)
        perror("ioctl(): blank");
}
