/*
 * Copyright 2022 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *
 * Example showing how to use the buttons and leds in the XPLR-IOT-1
 *
 */

#include <stdio.h>

#include "leds.h"
#include "buttons.h"

int curr_led = 0;

void button_pressed(int button_no, uint32_t hold_time)
{
    if (!hold_time) {
        printf("Button %d down\n", button_no);
        if (button_no == 0) {
            // Led off on down
            set_led(curr_led, false);
        } else {
            // Second button, switch color
            set_led(curr_led, false);
            curr_led = (curr_led + 1) % 3;
            set_led(curr_led, true);
        }
    } else {
        printf("Button %d up. Hold time: %u ms\n", button_no, hold_time);
        if (button_no == 0) {
            // Led on on up
            set_led(curr_led, true);
        }
    }
}

void main(void)
{
    if (!init_buttons(button_pressed)) {
        printf("* Failed to initiate buttons\n");
    }
    if (!init_leds()) {
        printf("* Failed to initiate leds\n");
    }
    set_led(curr_led, true);
    printf("Press the buttons!\n");

}
