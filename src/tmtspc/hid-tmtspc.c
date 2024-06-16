// SPDX-License-Identifier: GPL-2.0-or-later
#include <linux/usb.h>
#include <linux/hid.h>
#include "../hid-tmff2.h"

#define TSPC_MAX_EFFECTS 16
#define TSPC_BUFFER_LENGTH 63

static const u8 setup_0[64] = { 0x42, 0x01 };
static const u8 setup_1[64] = { 0x0a, 0x04, 0x90, 0x03 };
static const u8 setup_2[64] = { 0x0a, 0x04, 0x00, 0x0c };
static const u8 setup_3[64] = { 0x0a, 0x04, 0x12, 0x10 };
static const u8 setup_4[64] = { 0x0a, 0x04, 0x00, 0x06 };
static const u8 setup_5[64] = { 0x0a, 0x04, 0x00, 0x0e };
static const u8 setup_6[64] = { 0x0a, 0x04, 0x00, 0x0e, 0x01 };
static const u8 *const setup_arr[] = { setup_0, setup_1, setup_2, setup_3, setup_4, setup_5, setup_6 };
static const unsigned int setup_arr_sizes[] = {
	ARRAY_SIZE(setup_0),
	ARRAY_SIZE(setup_1),
	ARRAY_SIZE(setup_2),
	ARRAY_SIZE(setup_3),
	ARRAY_SIZE(setup_4),
	ARRAY_SIZE(setup_5),
	ARRAY_SIZE(setup_6)
};

static const unsigned long tspc_params =
	PARAM_SPRING_LEVEL
	| PARAM_DAMPER_LEVEL
	| PARAM_FRICTION_LEVEL
	| PARAM_RANGE
	| PARAM_GAIN
	;

static const signed short tspc_effects[] = {
	FF_CONSTANT,
	FF_RAMP,
	FF_SPRING,
	FF_DAMPER,
	FF_FRICTION,
	FF_INERTIA,
	FF_PERIODIC,
	FF_SINE,
	FF_TRIANGLE,
	FF_SQUARE,
	FF_SAW_UP,
	FF_SAW_DOWN,
	FF_AUTOCENTER,
	FF_GAIN,
	-1
};

static u8 tspc_pc_rdesc_fixed[] = {
	/* Some axes/buttons/dial may be used on other attachments, e.g. F1 wheel
	 * Report ID 7: 20 bytes
	 *  	2 bytes for X (Steering)
	 *		2 bytes for Y (Brake Pedal)
	 *		2 bytes for Rz (Throttle)
	 *		2 bytes for Slider (Clutch)
	 *		1 byte  for Vx (?)
	 *		1 byte  for Vy (?)
	 *		1 byte  for Rx (Wheel Buttons 3, 4, 5, 6, 7, 8, Shift Down, Shift Up)
	 *		1 byte  for Ry (Wheel Buttons 9, 10, 11, 12, 13)
	 *		1 byte  for Z  (?)
	 *		1 byte  for Dial (?)
	 *		4 bytes for Button (26 bits; not wheel buttons)
	 *		1 byte  for Padding (6 bits)
	 *		1 byte  for Hat (4 bits + 4 bits padding)
	 *	Report ID 96: 63 bytes
	 * 		63 bytes for Vendor Defined Data
	 *	Report ID 2:  1 byte
	 * 		1 byte  for Vendor Defined Data
	 *	Report ID 20: 1 byte
	 * 		1 byte  for Vendor Defined Data
	*/
	0x05, 0x01,                    // Usage Page (Generic Desktop)
	0x09, 0x04,                    // Usage (Joystick)
	0xa1, 0x01,                    // Collection (Application)
	0x09, 0x01,                    //  Usage (Pointer)
	0xa1, 0x00,                    //  Collection (Physical)
	0x85, 0x07,                    //   Report ID (7)
	0x09, 0x30,                    //   Usage (X) - Steering
	0x15, 0x00,                    //   Logical Minimum (0)
	0x27, 0xff, 0xff, 0x00, 0x00,  //   Logical Maximum (65535)
	0x35, 0x00,                    //   Physical Minimum (0)
	0x47, 0xff, 0xff, 0x00, 0x00,  //   Physical Maximum (65535)
	0x75, 0x10,                    //   Report Size (16)
	0x95, 0x01,                    //   Report Count (1)
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x09, 0x31,                    //   Usage (Y) - Brake Pedal
	0x26, 0xff, 0x03,              //   Logical Maximum (1023)
	0x46, 0xff, 0x03,              //   Physical Maximum (1023)
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x09, 0x35,                    //   Usage (Rz) - Throttle Pedal
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x09, 0x36,                    //   Usage (Slider) - Clutch?
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x75, 0x08,                    //   Report Size (8)
	0x26, 0xff, 0x00,              //   Logical Maximum (255)
	0x46, 0xff, 0x00,              //   Physical Maximum (255)
	0x09, 0x40,                    //   Usage (Vx)
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x09, 0x41,                    //   Usage (Vy)
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x09, 0x33,                    //   Usage (Rx) - Wheel Buttons 3, 4, 5, 6, 7, 8, Shift Down, Shift Up
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x09, 0x34,                    //   Usage (Ry) - Wheel Buttons 9, 10, 11, 12, 13
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x09, 0x32,                    //   Usage (Z)
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x09, 0x37,                    //   Usage (Dial)
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x05, 0x09,                    //   Usage Page (Button)
	0x19, 0x01,                    //   Usage Minimum (1)
	0x29, 0x1a,                    //   Usage Maximum (26)
	0x25, 0x01,                    //   Logical Maximum (1)
	0x45, 0x01,                    //   Physical Maximum (1)
	0x75, 0x01,                    //   Report Size (1)
	0x95, 0x1a,                    //   Report Count (26)
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x75, 0x06,                    //   Report Size (6)
	0x95, 0x01,                    //   Report Count (1)
	0x81, 0x03,                    //   Input (Cnst,Var,Abs)
	0x05, 0x01,                    //   Usage Page (Generic Desktop)
	0x09, 0x39,                    //   Usage (Hat switch)
	0x25, 0x07,                    //   Logical Maximum (7)
	0x46, 0x3b, 0x01,              //   Physical Maximum (315)
	0x55, 0x00,                    //   Unit Exponent (0)
	0x65, 0x14,                    //   Unit (EnglishRotation: deg)
	0x75, 0x04,                    //   Report Size (4)
	0x81, 0x42,                    //   Input (Data,Var,Abs,Null)
	0x65, 0x00,                    //   Unit (None)
	0x81, 0x03,                    //   Input (Cnst,Var,Abs)
	0x85, 0x60,                    //   Report ID (96)
	0x06, 0x00, 0xff,              //   Usage Page (Vendor Defined Page 1)
	0x09, 0x60,                    //   Usage (Vendor Usage 0x60)
	0x75, 0x08,                    //   Report Size (8)
	0x95, 0x3f,                    //   Report Count (63)
	0x26, 0xff, 0x00,              //   Logical Maximum (255)
	0x46, 0xff, 0x00,              //   Physical Maximum (255)
	0x91, 0x02,                    //   Output (Data,Var,Abs)
	0x85, 0x02,                    //   Report ID (2)
	0x09, 0x02,                    //   Usage (Vendor Usage 2)
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0x09, 0x14,                    //   Usage (Vendor Usage 0x14)
	0x85, 0x14,                    //   Report ID (20)
	0x81, 0x02,                    //   Input (Data,Var,Abs)
	0xc0,                          //  End Collection
	0xc0,                          // End Collection
};

static int tspc_interrupts(struct t300rs_device_entry *tspc)
{
	u8 *send_buf = kmalloc(256, GFP_KERNEL);
	struct usb_interface *usbif = to_usb_interface(tspc->hdev->dev.parent);
	struct usb_host_endpoint *ep;
	int ret, trans, b_ep, i;

	if (!send_buf) {
		hid_err(tspc->hdev, "failed allocating send buffer\n");
		return -ENOMEM;
	}

	ep = &usbif->cur_altsetting->endpoint[1];
	b_ep = ep->desc.bEndpointAddress;

	for (i = 0; i < ARRAY_SIZE(setup_arr); ++i) {
		memcpy(send_buf, setup_arr[i], setup_arr_sizes[i]);

		ret = usb_interrupt_msg(tspc->usbdev,
				usb_sndintpipe(tspc->usbdev, b_ep),
				send_buf, setup_arr_sizes[i],
				&trans,
				USB_CTRL_SET_TIMEOUT);

		if (ret) {
			hid_err(tspc->hdev, "setup data couldn't be sent\n");
			goto err;
		}
	}

err:
	kfree(send_buf);
	return ret;
}

int tspc_wheel_destroy(void *data)
{
	struct t300rs_device_entry *t300rs = data;

	if (!t300rs)
		return -ENODEV;

	kfree(t300rs->send_buffer);
	kfree(t300rs);
	return 0;
}

int tspc_set_range(void *data, uint16_t value)
{
	struct t300rs_device_entry *tspc = data;

	if (value < 140) {
		hid_info(tspc->hdev, "value %i too small, clamping to 140\n", value);
		value = 140;
	}

	if (value > 1080) {
		hid_info(tspc->hdev, "value %i too large, clamping to 1080\n", value);
		value = 1080;
	}

	return t300rs_set_range(data, value);
}

static int tspc_send_open(struct t300rs_device_entry *tspc)
{
	int r1, r2;
	tspc->send_buffer[0] = 0x01;
	tspc->send_buffer[1] = 0x04;
	if ((r1 = t300rs_send_int(tspc)))
		return r1;

	tspc->send_buffer[0] = 0x01;
	tspc->send_buffer[1] = 0x05;
	if ((r2 = t300rs_send_int(tspc)))
		return r2;

	return 0;
}

static int tspc_open(void *data, int open_mode)
{
	struct t300rs_device_entry *tspc = data;

	if (!tspc)
		return -ENODEV;

	if (open_mode)
		tspc_send_open(tspc);

	return tspc->open(tspc->input_dev);
}

static int tspc_send_close(struct t300rs_device_entry *tspc)
{
	int r1, r2;
	tspc->send_buffer[0] = 0x01;
	tspc->send_buffer[1] = 0x05;
	if ((r1 = t300rs_send_int(tspc)))
		return r1;

	tspc->send_buffer[0] = 0x01;
	tspc->send_buffer[1] = 0x00;
	if ((r2 = t300rs_send_int(tspc)))
		return r2;

	return 0;
}

static int tspc_close(void *data, int open_mode)
{
	struct t300rs_device_entry *tspc = data;

	if (!tspc)
		return -ENODEV;

	if (open_mode)
		tspc_send_close(tspc);

	tspc->close(tspc->input_dev);
	return 0;
}

int tspc_wheel_init(struct tmff2_device_entry *tmff2, int open_mode)
{
	struct t300rs_device_entry *tspc = kzalloc(sizeof(struct t300rs_device_entry), GFP_KERNEL);
	struct list_head *report_list;
	int ret;


	if (!tspc) {
		ret = -ENOMEM;
		goto tspc_err;
	}

	tspc->hdev = tmff2->hdev;
	tspc->input_dev = tmff2->input_dev;
	tspc->usbdev = to_usb_device(tmff2->hdev->dev.parent->parent);
	tspc->buffer_length = TSPC_BUFFER_LENGTH;

	tspc->send_buffer = kzalloc(tspc->buffer_length, GFP_KERNEL);
	if (!tspc->send_buffer) {
		ret = -ENOMEM;
		goto send_err;
	}

	report_list = &tspc->hdev->report_enum[HID_OUTPUT_REPORT].report_list;
	tspc->report = list_entry(report_list->next, struct hid_report, list);
	tspc->ff_field = tspc->report->field[0];

	tspc->open = tspc->input_dev->open;
	tspc->close = tspc->input_dev->close;

	if ((ret = tspc_interrupts(tspc)))
		goto interrupt_err;

	/* everything went OK */
	tmff2->data = tspc;
	tmff2->params = tspc_params;
	tmff2->max_effects = TSPC_MAX_EFFECTS;
	memcpy(tmff2->supported_effects, tspc_effects, sizeof(tspc_effects));

	if (!open_mode)
		tspc_send_open(tspc);

	hid_info(tspc->hdev, "force feedback for TSPC\n");
	return 0;

interrupt_err:
send_err:
	kfree(tspc);
tspc_err:
	hid_err(tmff2->hdev, "failed initializing TSPC\n");
	return ret;
}

static __u8 *tspc_wheel_fixup(struct hid_device *hdev, __u8 *rdesc,
		unsigned int *rsize)
{
	rdesc = tspc_pc_rdesc_fixed;
	*rsize = sizeof(tspc_pc_rdesc_fixed);
	return rdesc;
}

int tspc_populate_api(struct tmff2_device_entry *tmff2)
{
	tmff2->play_effect = t300rs_play_effect;
	tmff2->upload_effect = t300rs_upload_effect;
	tmff2->update_effect = t300rs_update_effect;
	tmff2->stop_effect = t300rs_stop_effect;

	tmff2->set_gain = t300rs_set_gain;
	tmff2->set_autocenter = t300rs_set_autocenter;
	/* TS-PC has 1080 degree range, just like T300RS 1080 */
	tmff2->set_range = tspc_set_range;
	tmff2->wheel_fixup = tspc_wheel_fixup;

	tmff2->open = tspc_open;
	tmff2->close = tspc_close;

	tmff2->wheel_init = tspc_wheel_init;
	tmff2->wheel_destroy = tspc_wheel_destroy;

	return 0;
}
