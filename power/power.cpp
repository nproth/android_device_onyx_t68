/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define LOG_TAG "i.MXPowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>
#include <utils/StrongPointer.h>
#include <cutils/properties.h>

#define GOVERNOR_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define BOOSTPULSE_PATH "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"
#define PROP_CPUFREQGOV "sys.interactive"
#define PROP_VAL "active"

#define CONSERVATIVE "conservative"
//#define INTERACTIVE  "interactive"    Not available on prebuilt kernel
#define ONDEMAND     "ondemand"
#define POWERSAVE    "powersave"
#define PERFORMANCE  "performance"

static int interactive_mode = 0;

static void getcpu_gov(char *s, int size)
{
    int len;
    int fd = open(GOVERNOR_PATH, O_RDONLY);

    if (fd < 0) {
        ALOGE("Error opening %s: %s\n", GOVERNOR_PATH, strerror(errno));
        return;
    }

    len = read(fd, s, size);
    if (len < 0) {
        ALOGE("Error read %s: %s\n", GOVERNOR_PATH, strerror(errno));
    }
    close(fd);
}

static void sysfs_write(const char *path, const char *s)
{
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        ALOGE("Error opening %s: %s\n", path, strerror(errno));
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        ALOGE("Error writing to %s: %s\n", path, strerror(errno));
    }

    close(fd);
}

static ssize_t sysfs_read(char *path, char *s, int num_bytes)
{
    ssize_t n;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        ALOGE("Error reading from %s: %s\n", path, strerror(errno));
        return -1;
    }

    if ((n = read(fd, s, (num_bytes - 1))) < 0) {
        ALOGE("Error reading from  %s: %s\n", path, strerror(errno));
    } else {
        if ((n >= 1) && (s[n-1] == '\n')) {
            s[n-1] = '\0';
        } else {
            s[n] = '\0';
        }
    }

    close(fd);
    ALOGV("read '%s' from %s", s, path);

    return n;
}
int do_setproperty(const char *propname, const char *propval)
{
    char prop_cpugov[PROPERTY_VALUE_MAX];
    int ret;

    property_set(propname, propval);
    if ( property_get(propname, prop_cpugov, NULL) &&
                     (strcmp(prop_cpugov, propval) == 0) ) {
	ret = 0;
        ALOGV("setprop: %s = %s ", propname, propval);
    } else {
        ret = -1;
        ALOGE("setprop: %s = %s fail\n", propname, propval);
    }
    return ret;
}

int do_changecpugov(const char *gov)
{
    int fd = open(GOVERNOR_PATH, O_WRONLY);
    if (fd < 0) {
        ALOGE("Error opening %s: %s\n", GOVERNOR_PATH, strerror(errno));
        return -1;
    }

    if (write(fd, gov, strlen(gov)) < 0) {
        ALOGE("Error writing to %s: %s\n", GOVERNOR_PATH, strerror(errno));
        close(fd);
        return -1;
    }
    /*if (strncmp(INTERACTIVE, gov, strlen(INTERACTIVE)) == 0) {
        do_setproperty(PROP_CPUFREQGOV, PROP_VAL);
        interactive_mode = 1;
    } else {
        interactive_mode = 0;
    }*/

    close(fd);
    return 0;
}

static void fsl_power_init(struct power_module *module)
{
    /*
     * cpufreq interactive governor: timer 40ms, min sample 60ms,
     * hispeed at cpufreq MAX freq in freq_table at load 40% 
     * the params is initialized in init.rc
     */
    (void)module;

    // TODO adjust parameters
    do_changecpugov(ONDEMAND);
}

static void fsl_power_set_interactive(struct power_module *module, int on)
{
    /* swich to conservative when system in early_suspend or
     * suspend mode.
     */
    (void)module;

    if (on)
        do_changecpugov(ONDEMAND);
    else
        do_changecpugov(CONSERVATIVE);
}

static void fsl_power_hint(struct power_module *module, power_hint_t hint,
                            void *data)
{
    (void)module;

    switch (hint) {
    case POWER_HINT_VSYNC:
        break;

    case POWER_HINT_INTERACTION:
    case POWER_HINT_CPU_BOOST:
            do_changecpugov(PERFORMANCE);
        break;
    
    default:
        break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "FSL i.MX Power HAL",
        .author = "Freescale Semiconductor, Inc.",
        .methods = &power_module_methods,
        .dso = NULL,
        .reserved = {0}
    },

    .init = fsl_power_init,
    .setInteractive = fsl_power_set_interactive,
    .powerHint = fsl_power_hint,
};
