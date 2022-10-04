
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
 * Simple demo program showing how to read some of
 * the sensors on the XPLR-IOT-1 board
 *
 */

#include <zephyr.h>
#include <stdbool.h>
#include <stdio.h>
#include <device.h>
#include <drivers/sensor.h>


const struct device *gpBme280Dev;
const struct device *gpAdxl345Device;

#define INIT_SENSOR(sensor_name, p)                                                                             \
  {                                                                                                             \
    p = DEVICE_DT_GET_ANY(sensor_name);                                                                         \
    if (p && device_is_ready(p)) {                                                                              \
      printf("Found device \"%s\", on I2C address 0x%02x \n", p->name, DT_REG_ADDR(DT_INST(0, sensor_name)));   \
    }                                                                                                           \
  }

void pollSensors()
{
    if (gpBme280Dev && sensor_sample_fetch(gpBme280Dev) == 0) {
        struct sensor_value temp, press, humidity;
        sensor_channel_get(gpBme280Dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(gpBme280Dev, SENSOR_CHAN_PRESS, &press);
        sensor_channel_get(gpBme280Dev, SENSOR_CHAN_HUMIDITY, &humidity);
        printf("Temp: %3d.%02d  Press: %4d.%02d  Humidity: %3d.%02d\n",
               temp.val1, temp.val2 / 10000,
               press.val1, press.val2 / 10000,
               humidity.val1, humidity.val2 / 10000);
    }
    if (gpAdxl345Device && sensor_sample_fetch(gpAdxl345Device) >= 0) {
        struct sensor_value accel[3];
        if (sensor_channel_get(gpAdxl345Device, SENSOR_CHAN_ACCEL_XYZ, accel) == 0) {
            printf("Accel: X = %d Y = %d Z = %d\n",
                   accel[0].val1,
                   accel[1].val1,
                   accel[2].val1);
        }
    }
}

void main()
{
    INIT_SENSOR(bosch_bme280, gpBme280Dev);
    INIT_SENSOR(adi_adxl345, gpAdxl345Device);
    while (1) {
        pollSensors();
        k_msleep(2000);
    }
}
