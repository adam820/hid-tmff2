/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __HID_TMFF2_H
#define __HID_TMFF2_H

#include <linux/fixp-arith.h>
#include <linux/ktime.h>
#include <linux/input.h>

extern int timer_msecs;
extern int spring_level;
extern int damper_level;
extern int friction_level;
extern int range;
extern int alt_mode;

#define USB_VENDOR_ID_THRUSTMASTER 0x044f

/* the wheel seems to only be capable of processing a certain number of
 * interrupts per second, and if this value is too low the kernel urb buffer(or
 * some buffer at least) fills up. Optimally I would figure out some way to
 * space out the interrupts so that they all leave at regular intervals, but
 * for now this is good enough, go slow enough that everything works.
 */
#define DEFAULT_TIMER_PERIOD	8

#define FF_EFFECT_QUEUE_UPLOAD	0
#define FF_EFFECT_QUEUE_START	1
#define FF_EFFECT_QUEUE_STOP	2
#define FF_EFFECT_QUEUE_UPDATE	3
#define FF_EFFECT_PLAYING	4

#define PARAM_SPRING_LEVEL	(1 << 0)
#define PARAM_DAMPER_LEVEL	(1 << 1)
#define PARAM_FRICTION_LEVEL	(1 << 2)
#define PARAM_RANGE		(1 << 3)
#define PARAM_ALT_MODE		(1 << 4)

#undef fixp_sin16
#define fixp_sin16(v) (((v % 360) > 180) ?\
		-(fixp_sin32((v % 360) - 180) >> 16)\
		: fixp_sin32(v) >> 16)

#define JIFFIES2MS(jiffies) ((jiffies) * 1000 / HZ)

struct tmff2_effect_state {
	struct ff_effect effect;
	struct ff_effect old;

	unsigned long flags;
	unsigned long count;
	unsigned long start_time;
};

struct tmff2_device_entry {
	struct hid_device *hdev;
	struct input_dev *input_dev;

	/* pointer to array */
	struct tmff2_effect_state *states;

	struct delayed_work work;

	spinlock_t lock;

	/* fields relevant to each actual device (T300, T150...) */
	void *data;
	unsigned long params;
	unsigned long max_effects;
	signed short supported_effects[FF_CNT];

	/* obligatory callbacks */
	int (*play_effect)(void *data, struct tmff2_effect_state *state);
	int (*upload_effect)(void *data, struct tmff2_effect_state *state);
	int (*update_effect)(void *data, struct tmff2_effect_state *state);
	int (*stop_effect)(void *data, struct tmff2_effect_state *state);

	int (*wheel_init)(struct tmff2_device_entry *tmff2);
	int (*wheel_destroy)(void *data);

	/* optional callbacks */
	int (*open)(void *data);
	int (*close)(void *data);
	int (*set_gain)(void *data, uint16_t gain);
	int (*set_range)(void *data, uint16_t range);
	int (*switch_mode)(void *data, uint16_t mode);
	int (*set_autocenter)(void *data, uint16_t autocenter);
	__u8 *(*wheel_fixup)(struct hid_device *hdev, __u8 *rdesc, unsigned int *rsize);

	/* void pointers are dangerous, I know, but in this case likely the best option... */
};

/* external */
int t300rs_populate_api(struct tmff2_device_entry *tmff2);

#define TMT300RS_PS3_NORM_ID 0xb66e
#define TMT300RS_PS3_ADV_ID 0xb66f
#define TMT300RS_PS4_NORM_ID 0xb66d

#endif /* __HID_TMFF2_H */