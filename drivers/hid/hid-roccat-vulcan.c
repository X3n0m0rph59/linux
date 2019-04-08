// SPDX-License-Identifier: GPL-2.0+
/*
 * Roccat Vulcan 100/120 driver for Linux
 *
 * Copyright (c) 2019 Tobias Buettner <t.linux@spiritcroc.de>
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/device.h>
#include <linux/input.h>
#include <linux/hid.h>
#include <linux/module.h>
#include "hid-ids.h"

struct vulcan_drvdata {
	struct input_dev *input;
};

static int vulcan_probe(struct hid_device *hdev,
		const struct hid_device_id *id)
{
	int retval;
	struct vulcan_drvdata *drvdata;

	drvdata = devm_kzalloc(&hdev->dev, sizeof(*drvdata), GFP_KERNEL);
	if (drvdata == NULL)
		return -ENOMEM;

	hid_set_drvdata(hdev, drvdata);

	retval = hid_parse(hdev);
	if (retval) {
		hid_err(hdev, "parse failed\n");
		goto exit;
	}

	retval = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (retval) {
		hid_err(hdev, "hw start failed\n");
		goto exit;
	}

	if (!drvdata->input) {
		hid_err(hdev, "Roccat vulcan input not registered\n");
		retval = -ENOMEM;
		goto exit_stop;
	}

	return 0;
exit_stop:
	hid_hw_stop(hdev);
exit:
	return retval;
}

static void vulcan_remove(struct hid_device *hdev)
{
	hid_hw_stop(hdev);
}

/*
 * As soon as the init sequence is sent to the keyboard,
 * we get following data instead of the original scancodes
 */
static const u8 vulcan_media_key_seq1[] = { 0x03, 0x00, 0x0b };
static const u8 vulcan_media_key_seq2_map[][2] = {
	{ 0x21, KEY_PREVIOUSSONG },
	{ 0x22, KEY_NEXTSONG },
	{ 0x23, KEY_PLAYPAUSE },
	{ 0x24, KEY_STOPCD },
	{ 0, 0 },
};

static int vulcan_raw_event(struct hid_device *hdev,
		struct hid_report *report, u8 *data, int size)
{
	struct vulcan_drvdata *drvdata = hid_get_drvdata(hdev);
	int i;

	if (size == 5 && memcmp(data, vulcan_media_key_seq1, 3) == 0) {
		for (i = 0; vulcan_media_key_seq2_map[i][0]; i++) {
			if (data[3] == vulcan_media_key_seq2_map[i][0]) {
				input_event(drvdata->input, EV_KEY,
						vulcan_media_key_seq2_map[i][1],
						!data[4]);
				input_sync(drvdata->input);
				return 1;
			}
		}
	}

	return 0;
}

static int vulcan_input_configured(struct hid_device *hdev,
		struct hid_input *hi)
{
	struct vulcan_drvdata *drvdata = hid_get_drvdata(hdev);

	drvdata->input = hi->input;

	return 0;
}

static const struct hid_device_id vulcan_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_ROCCAT,
		USB_DEVICE_ID_ROCCAT_VULCAN_100) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_ROCCAT,
		USB_DEVICE_ID_ROCCAT_VULCAN_120) },
	{ }

};
MODULE_DEVICE_TABLE(hid, vulcan_devices);

static struct hid_driver vulcan_driver = {
	.name = "roccat-vulcan",
	.id_table = vulcan_devices,
	.probe = vulcan_probe,
	.remove = vulcan_remove,
	.raw_event = vulcan_raw_event,
	.input_configured = vulcan_input_configured
};
module_hid_driver(vulcan_driver);

MODULE_AUTHOR("Tobias Buettner");
MODULE_DESCRIPTION("USB Roccat Vulcan 100/120 driver");
MODULE_LICENSE("GPL v2");
