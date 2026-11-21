/* nrf52_bsim stand-in for USB (usb.c) and the USB HID class: reports are logged to the
 * host and completed one USB frame (1 ms poll) later. Also stubs receiver OTA and starts
 * pairing at boot so simulated trackers can join. */
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/class/usbd_hid.h>

#include "connection/esb.h"
#include "receiver_ota.h"
#include "usb.h"

void slime_rx_log_report(uint64_t t_us, const uint8_t *report, uint16_t len);

static receiver_usb_state_cb_t state_cb;
static const struct device *hid_dev;
static const struct hid_device_ops *hid_ops;
static const uint8_t *in_flight;

bool receiver_usb_is_enabled(void)
{
	return true;
}

bool receiver_usb_is_configured(void)
{
	return true;
}

void receiver_usb_set_state_callback(receiver_usb_state_cb_t callback)
{
	state_cb = callback;
}

DEVICE_DT_DEFINE(DT_NODELABEL(hid_dev_0), NULL, NULL, NULL, NULL, POST_KERNEL, 99, NULL);

static void usb_up(struct k_work *work)
{
	ARG_UNUSED(work);
	if (hid_ops && hid_ops->iface_ready) {
		hid_ops->iface_ready(hid_dev, true);
	}
	if (state_cb) {
		state_cb(true);
	}
}
static K_WORK_DELAYABLE_DEFINE(usb_up_work, usb_up);

int hid_device_register(const struct device *dev, const uint8_t *const rdesc, const uint16_t rsize,
			const struct hid_device_ops *const ops)
{
	ARG_UNUSED(rdesc);
	ARG_UNUSED(rsize);
	hid_dev = dev;
	hid_ops = ops;
	k_work_schedule(&usb_up_work, K_MSEC(50));
	return 0;
}

static void report_done(struct k_work *work)
{
	ARG_UNUSED(work);
	const uint8_t *report = in_flight;
	in_flight = NULL;
	if (hid_ops && hid_ops->input_report_done) {
		hid_ops->input_report_done(hid_dev, report);
	}
}
static K_WORK_DELAYABLE_DEFINE(report_done_work, report_done);

int hid_device_submit_report(const struct device *dev, const uint16_t size, const uint8_t *const report)
{
	ARG_UNUSED(dev);
	if (in_flight) {
		return -EBUSY;
	}
	slime_rx_log_report(k_ticks_to_us_floor64(k_uptime_ticks()), report, size);
	in_flight = report;
	k_work_schedule(&report_done_work, K_USEC(1000)); /* host polls every 1 ms */
	return 0;
}

bool receiver_ota_is_active(void)
{
	return false;
}

void receiver_ota_process_hid(const uint8_t *data, size_t len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(len);
}

static void pair(struct k_work *work)
{
	ARG_UNUSED(work);
	esb_start_pairing();
}
static K_WORK_DELAYABLE_DEFINE(pair_work, pair);

static int sim_init(void)
{
	k_work_schedule(&pair_work, K_MSEC(200));
	return 0;
}
SYS_INIT(sim_init, APPLICATION, 99);

/* CMSIS byte reverse used by NCS esb.c; missing from the bsim CMSIS replacement. */
uint32_t __REV(uint32_t value)
{
	return __builtin_bswap32(value);
}
