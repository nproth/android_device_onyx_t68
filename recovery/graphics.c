/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
 * Copyright (C) 2021 nproth
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

// Freescale epdc code adapted from https://github.com/onyx-intl/platform_bootable/blob/master/recovery/minui/graphics.c

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

#include <pixelflinger/pixelflinger.h>

#ifdef BOARD_USE_CUSTOM_RECOVERY_FONT
#include BOARD_USE_CUSTOM_RECOVERY_FONT
#else
#include "roboto_15x24.h"
#endif

#include "minui.h"

//#include <linux/mxcfb.h>
#include "../kernel_headers/linux/mxcfb.h"

typedef struct {
    GGLSurface texture;
    unsigned cwidth;
    unsigned cheight;
    unsigned ascent;
} GRFont;

typedef struct {
    int r, g, b, a;
} GRColor;

#define THEME_TEXT_COLOR    0, 0, 0, 255
#define END_OF_THEME        0, 0, 0, 0    

/* Hack color theming into recovery for better legibility: 
 * replace specific colors in gr_color() */
static const GRColor THEME_COLORS[] = {
    { 0, 191, 255, 255 },   { THEME_TEXT_COLOR }, // MENU_TEXT_COLOR
    { 200, 200, 200, 255 }, { THEME_TEXT_COLOR }, // NORMAL_TEXT_COLOR
    { END_OF_THEME }
};

//static const int THEME_ENTRIES = sizeof(THEME_COLORS) / (8 * sizeof(int));

static GRFont *gr_font = 0;
static GGLContext *gr_context = 0;
static GGLSurface gr_font_texture;
static GGLSurface gr_framebuffer;
static GGLSurface gr_mem_surface;

static int gr_fb_fd = -1;


static struct fb_fix_screeninfo fi;
static struct fb_var_screeninfo vi;

static unsigned int epdc_update_marker = 0;

static void epdc_queue_update(int left, int top, int width, int height)
{
	struct mxcfb_update_data upd_data = {
        .update_mode = UPDATE_MODE_PARTIAL,
        .waveform_mode = WAVEFORM_MODE_AUTO,
        .temp = TEMP_USE_AMBIENT,
        .flags = 0
    };

    /* Crop to display, else driver complains */
    if(left >= (int) vi.xres || top >= (int) vi.yres) {
        return;
    }
    if(left < 0) {
        width += left;
        left = 0;
    }
    if(top < 0) {
        height += top;
        top = 0;
    }
    if(width < 0 || height < 0) {
        return;
    }
    if(left + width > (int) vi.xres) {
        width = vi.xres - left;
    }
    if(top + height > (int) vi.yres) {
        height = vi.yres - top;
    }

    upd_data.update_region.left = left;
    upd_data.update_region.top = top;
    upd_data.update_region.width = width;
    upd_data.update_region.height = height;

    /* Get unique marker value */
	upd_data.update_marker = ++epdc_update_marker;
	
    int retries = 0;
    int retval = ioctl(gr_fb_fd, MXCFB_SEND_UPDATE, &upd_data);

    while(retval < 0 && retries < 10) {
        sleep(1);
        retval = ioctl(gr_fb_fd, MXCFB_SEND_UPDATE, &upd_data);
        retries++;
    }
    if(retval < 0) {
        fprintf(stderr, "Epdc update failed\n");
    }
}

static void epdc_flush_updates() {
    if(epdc_update_marker > 0) {
        if(0 > ioctl(gr_fb_fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &epdc_update_marker)) {
            fprintf(stderr, "Epdc wait for update complete failed\n");
        }
        /*  Queue now empty */
        epdc_update_marker = 0;
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
    vi.grayscale = GRAYSCALE_8BIT;
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

    if (ioctl(fd, MXCFB_SET_AUTO_UPDATE_MODE, &auto_update_mode) < 0) {
        perror("set auto update mode failed\n");
        return -1;
    }

    if (ioctl(fd, MXCFB_SET_WAVEFORM_MODES, &wv_modes) < 0) {
        perror("set waveform modes failed\n");
        return -1;
    }

    if (ioctl(fd, MXCFB_SET_UPDATE_SCHEME, &scheme) < 0) {
        perror("set update scheme failed\n");
        return -1;
    }

    return fd;
}

static void get_memory_surface(GGLSurface* ms) {
  ms->version = sizeof(*ms);
  ms->width = vi.xres;
  ms->height = vi.yres;
  ms->stride = fi.line_length;
  ms->data = malloc(fi.line_length * vi.yres);
  ms->format = GGL_PIXEL_FORMAT_L_8;
}

int getFbXres(void) {
    return vi.xres;
}

int getFbYres (void) {
    return vi.yres;
}

void gr_flip(void)
{
    /* Wait for last update to finish */
    epdc_flush_updates();

    /* Copy new data into fb */
    memcpy(gr_framebuffer.data, gr_mem_surface.data, fi.line_length * vi.yres);

    /* And schedule next screen update */
    epdc_queue_update(0, 0, vi.xres, vi.yres);
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    /* Apply theme colors */
    int i;
    for(i = 0; THEME_COLORS[i].a > 0; i += 2) {
        
        if(r == THEME_COLORS[i].r && g == THEME_COLORS[i].g
           && b == THEME_COLORS[i].g && a == THEME_COLORS[i].a) {

            r = THEME_COLORS[i + 1].r;
            g = THEME_COLORS[i + 1].g;
            b = THEME_COLORS[i + 1].b;
            a = THEME_COLORS[i + 1].a;
            break;
        }
    }

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