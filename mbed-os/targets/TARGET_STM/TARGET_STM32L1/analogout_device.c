/* mbed Microcontroller Library
 * Copyright (c) 2014, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "mbed_assert.h"
#include "analogout_api.h"

#if DEVICE_ANALOGOUT

#include "cmsis.h"
#include "pinmap.h"
#include "mbed_error.h"
#include "PeripheralPins.h"

// These variables are used for the "free" function
static int pa4_used = 0;
static int pa5_used = 0;

void analogout_init(dac_t *obj, PinName pin) {
    DAC_ChannelConfTypeDef sConfig = {0};

    // Get the peripheral name (DAC_1, ...) from the pin and assign it to the object
    obj->dac = (DACName)pinmap_peripheral(pin, PinMap_DAC);
    MBED_ASSERT(obj->dac != (DACName)NC);
    // Get the pin function and assign the used channel to the object
    uint32_t function = pinmap_function(pin, PinMap_DAC);
    switch (STM_PIN_CHANNEL(function)) {
        case 1:
            obj->channel = DAC_CHANNEL_1;
            break;
#if defined(DAC_CHANNEL_2)
        case 2:
            obj->channel = DAC_CHANNEL_2;
            break;
#endif
        default:
            error("Unknown DAC channel");
            break;
    }

    // Configure GPIO
    pinmap_pinout(pin, PinMap_DAC);

    // Save the channel for future use
    obj->pin = pin;

    obj->handle.Instance = DAC;
    obj->handle.State = HAL_DAC_STATE_RESET;

    if (HAL_DAC_Init(&obj->handle) != HAL_OK ) {
        error("HAL_DAC_Init failed");
    }

    // Enable DAC clock
    __DAC_CLK_ENABLE();

    // Configure DAC
    sConfig.DAC_Trigger      = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    if (pin == PA_4) {
        pa4_used = 1;
    } else { // PA_5
        pa5_used = 1;
    }

    if (HAL_DAC_ConfigChannel(&obj->handle, &sConfig, obj->channel) != HAL_OK) {
        error("Cannot configure DAC channel\n");
    }

    analogout_write_u16(obj, 0);
}

void analogout_free(dac_t *obj) {
    // Reset DAC and disable clock
    if (obj->pin == PA_4) pa4_used = 0;
    if (obj->pin == PA_5) pa5_used = 0;
    if ((pa4_used == 0) && (pa5_used == 0)) {
        __DAC_FORCE_RESET();
        __DAC_RELEASE_RESET();
        __DAC_CLK_DISABLE();
    }

    // Configure GPIO
    pin_function(obj->pin, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, 0));
}

#endif // DEVICE_ANALOGOUT
