/*
 * Power-aware OLED blanking for Corne42, compatible with ZMK v0.2.
 *
 * Each split half evaluates its own USB power state and local activity.
 * USB-powered halves keep the display on; battery-powered halves blank
 * after CONFIG_ZMK_IDLE_TIMEOUT.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

LOG_MODULE_REGISTER(power_aware_display, CONFIG_ZMK_LOG_LEVEL);

static const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static enum zmk_activity_state activity_state = ZMK_ACTIVITY_ACTIVE;

static void update_display(void) {
    if (!device_is_ready(display)) {
        return;
    }

    if (zmk_usb_is_powered() || activity_state == ZMK_ACTIVITY_ACTIVE) {
        display_blanking_off(display);
    } else {
        display_blanking_on(display);
    }
}

static int activity_state_listener(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    activity_state = ev->state;
    update_display();
    return 0;
}

static int usb_state_listener(const zmk_event_t *eh) {
    if (as_zmk_usb_conn_state_changed(eh) == NULL) {
        return -ENOTSUP;
    }

    update_display();
    return 0;
}

ZMK_LISTENER(power_aware_display_activity, activity_state_listener);
ZMK_SUBSCRIPTION(power_aware_display_activity, zmk_activity_state_changed);

ZMK_LISTENER(power_aware_display_usb, usb_state_listener);
ZMK_SUBSCRIPTION(power_aware_display_usb, zmk_usb_conn_state_changed);
