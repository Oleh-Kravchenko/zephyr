/*
 * Copyright (c) 2025 Paul Timke <ptimkec@live.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/paj7620.h>

#define GESTURE_POLL_TIME_MS 100

#ifdef CONFIG_PAJ7620_TRIGGER
K_SEM_DEFINE(sem, 0, 1); /* starts off "not available" */

static void trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	ARG_UNUSED(trigger);

	if (sensor_sample_fetch(dev) < 0) {
		printf("sensor_sample_fetch failed\n");
		return;
	}

	k_sem_give(&sem);
}
#endif

static void print_hand_gesture(uint16_t gest_flags)
{
	if ((gest_flags & PAJ7620_FLAG_GES_UP) != 0) {
		printf("Gesture UP\n");
	}
	if ((gest_flags & PAJ7620_FLAG_GES_DOWN) != 0) {
		printf("Gesture DOWN\n");
	}
	if ((gest_flags & PAJ7620_FLAG_GES_LEFT) != 0) {
		printf("Gesture LEFT\n");
	}
	if ((gest_flags & PAJ7620_FLAG_GES_RIGHT) != 0) {
		printf("Gesture RIGHT\n");
	}
	if ((gest_flags & PAJ7620_FLAG_GES_FORWARD) != 0) {
		printf("Gesture FORWARD\n");
	}
	if ((gest_flags & PAJ7620_FLAG_GES_BACKWARD) != 0) {
		printf("Gesture BACKWARD\n");
	}
	if ((gest_flags & PAJ7620_FLAG_GES_CLOCKWISE) != 0) {
		printf("Gesture CLOCKWISE\n");
	}
	if ((gest_flags & PAJ7620_FLAG_GES_COUNTERCLOCKWISE) != 0) {
		printf("Gesture COUNTER CLOCKWISE\n");
	}
}

#ifdef CONFIG_PAJ7620_TRIGGER_OWN_THREAD
static void trigger_main_loop(const struct device *dev)
{
	struct sensor_value data;

	while (1) {
		k_sem_take(&sem, K_FOREVER);

		(void)sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_PAJ7620_GESTURES,
					 &data);

		print_hand_gesture(data.val1);
	}
}
#endif

#ifdef CONFIG_PAJ7620_TRIGGER_NONE
static void polling_main_loop(const struct device *dev)
{
	struct sensor_value data;

	while (1) {
		(void)sensor_sample_fetch(dev);
		(void)sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_PAJ7620_GESTURES,
					 &data);

		print_hand_gesture(data.val1);
		k_msleep(GESTURE_POLL_TIME_MS);
	}
}
#endif

int main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(pixart_paj7620);

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return -ENODEV;
	}

#ifdef CONFIG_PAJ7620_TRIGGER
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_MOTION,
		.chan = (enum sensor_channel)SENSOR_CHAN_PAJ7620_GESTURES,
	};
	int ret;

	printf("PAJ7620 gesture sensor sample - trigger mode\n");

	ret = sensor_trigger_set(dev, &trig, trigger_handler);
	if (ret < 0) {
		printf("Could not set trigger\n");
		return ret;
	}

	trigger_main_loop(dev);
#else
	printf("PAJ7620 gesture sensor sample - polling mode\n");
	polling_main_loop(dev);
#endif
}
