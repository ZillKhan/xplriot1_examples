/*

  Example showing how to use the buttons (and leds) in the XPLR-IOT-1

*/

#include <stdio.h>

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

// Button handling

typedef void (*button_cb_t)(int button_no, uint32_t hold_time);

static struct k_sem button_semaphore;
static const struct gpio_dt_spec buttons[] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0}),
};
static struct gpio_callback button_cb_data;
static button_cb_t button_cb = NULL;

static void button_thread(void);
K_THREAD_DEFINE(button_thread_id, 1024, button_thread, NULL, NULL, NULL, 7, 0, K_TICKS_FOREVER);

void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
  gpio_remove_callback(buttons[0].port, &button_cb_data);
  gpio_remove_callback(buttons[1].port, &button_cb_data);
  k_sem_give(&button_semaphore);
}

bool init_buttons(button_cb_t cb) {
  bool ok = true;
  gpio_init_callback(&button_cb_data, button_isr,
                     BIT(buttons[0].pin) | BIT(buttons[1].pin));
  for (int i = 0; i < 2 && ok; i++) {
    ok = device_is_ready(buttons[i].port) &&
              gpio_pin_configure_dt(&buttons[i], GPIO_INPUT) == 0 &&
              gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_TO_ACTIVE) == 0;
    gpio_add_callback(buttons[i].port, &button_cb_data);
  }
  if (ok) {
    button_cb = cb;
    k_sem_init(&button_semaphore, 0, 1);
    k_thread_start(button_thread_id);
  }
  return ok;
}

static void button_thread(void) {
  while (true) {
    k_sem_take(&button_semaphore, K_FOREVER);
    int button_no = gpio_pin_get_dt(&buttons[0]) ? 0 : 1;
    uint32_t start = k_uptime_get_32();
    k_sleep(K_MSEC(100)); // Debounce

    button_cb(button_no, 0);
    while (gpio_pin_get_dt(&buttons[button_no])) {
      k_sleep(K_MSEC(10));
    }

    button_cb(button_no, k_uptime_get_32() - start);

    gpio_add_callback(buttons[0].port, &button_cb_data);
    gpio_add_callback(buttons[1].port, &button_cb_data);
  }
}

// LEDs and main

#define LED_ON  0
#define LED_OFF 1
static struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0}),
};

const int led_cnt = sizeof(leds) / sizeof(leds[0]);
int curr_led = 0;

void button_pressed(int button_no, uint32_t hold_time) {
  if (!hold_time) {
    printf("Button %d down\n", button_no);
    if (button_no == 0) {
      gpio_pin_set_dt(&leds[curr_led], LED_OFF);
    } else {
      gpio_pin_set_dt(&leds[curr_led], LED_OFF);
      curr_led = (curr_led + 1) % led_cnt;
      gpio_pin_set_dt(&leds[curr_led], LED_ON);
    }
  } else {
    printf("Button %d up. Hold time: %u ms\n", button_no, hold_time);
    if (button_no == 0) {
      gpio_pin_set_dt(&leds[curr_led], LED_ON);
    }
  }
}

void main(void) {
  if (!init_buttons(button_pressed)) {
    printf("* Failed to initiate buttons\n");
  }

  for (int i = 0; i < led_cnt; i++) {
    if (device_is_ready(leds[i].port) && gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT) == 0) {
      if (i == curr_led) {
        gpio_pin_set_dt(&leds[i], LED_ON);
      }
    } else {
      printf("* Failed to setup led %d\n", i);
    }
  }

  printf("Press the buttons!\n");

}