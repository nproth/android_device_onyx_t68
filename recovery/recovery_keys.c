/*
 * Copyright (C) 2005-2008, The Android Open Source Project
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

#include <linux/input.h>

#include "common.h"
#include "extendedcommands.h"
#include "recovery_ui.h"


int device_handle_key(int key_code, int visible) {
    if (visible) {
        switch (key_code) {
            case KEY_CAPSLOCK:
            case KEY_DOWN:
            case KEY_VOLUMEDOWN:
            case KEY_MENU:
            case KEY_PAGEDOWN:
                return HIGHLIGHT_DOWN;

            case KEY_LEFTSHIFT:
            case KEY_UP:
            case KEY_VOLUMEUP:
            case KEY_HOME:
            case KEY_PAGEUP:
                return HIGHLIGHT_UP;

            case KEY_POWER:
            case KEY_LEFTBRACE:
            case KEY_ENTER:
            case BTN_MOUSE:
            case KEY_CAMERA:
            case KEY_F21:
            case KEY_SEND:
            case KEY_HOMEPAGE:
            case KEY_SEARCH:
                return SELECT_ITEM;
            
            case KEY_END:
            case KEY_BACKSPACE:
            case KEY_BACK:
                if (!ui_root_menu) {
                    return GO_BACK;
                } else {
                    return NO_ACTION;
                }
        }
    }

    return NO_ACTION;
}
