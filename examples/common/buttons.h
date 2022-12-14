
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

#include <stdint.h>

/**
 * Button callback function.
 * @param   button_no The index of the button pressed. First index is 0.
 * @param   hold_time Button press time in milliseconds. Will be zero
 *                    when the button is going down and contain the elapsed
 *                    time since button down when button going up.
 */
typedef void (*button_cb_t)(int button_no, uint32_t hold_time);

/** Initiate button handling
 * @param   cb Callback to be called when a button is pressed.
 * @return     Success or failure.
 */
bool init_buttons(button_cb_t cb);
