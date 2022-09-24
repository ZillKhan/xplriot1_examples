/*

  A simple demo application showing how to start a
  subprocess and toggle a gpio for led blink using ubxlib.

*/

#include "ubxlib.h"

#define LED_PIN        39 // This is for XPLR-IOT-1
#define BLINK_TIME_MS 500

static void blinkTask(void *pParameters)
{
    uPortGpioConfig_t gpioConfig;
    U_PORT_GPIO_SET_DEFAULT(&gpioConfig);
    gpioConfig.pin = LED_PIN;
    gpioConfig.direction = U_PORT_GPIO_DIRECTION_OUTPUT;
    uPortGpioConfig(&gpioConfig);
    bool on = false;
    while (1) {
        uPortGpioSet(LED_PIN, on);
        on = !on;
        uPortTaskBlock(BLINK_TIME_MS);
    }
}

void main()
{
    uPortInit();
    uPortTaskHandle_t taskHandle;
    uPortTaskCreate(blinkTask,
                    "twinkle",
                    1024,
                    NULL,
                    5,
                    &taskHandle);
}
