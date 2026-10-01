#include <zmk/display/widgets/output_status.h>
#include <zmk/display/widgets/peripheral_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/status_screen.h>
#include <zmk/events/position_state_changed.h>

#include <lvgl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/*
 * Small 32x22 Bongo Cat animation.
 * Frame 0 = idle, frame 1 = tapping.
 */
static const uint8_t bongo_idle_map[] = {
    0x00,0x00,0x00,0x00,0x00,0x38,0x1c,0x00,
    0x00,0x44,0x22,0x00,0x00,0x82,0x41,0x00,
    0x01,0x01,0x80,0x80,0x02,0x00,0x00,0x40,
    0x04,0x00,0x00,0x20,0x08,0x24,0x24,0x10,
    0x10,0x00,0x00,0x08,0x10,0x00,0x00,0x08,
    0x10,0x42,0x42,0x08,0x10,0x3c,0x3c,0x08,
    0x08,0x00,0x00,0x10,0x08,0x00,0x00,0x10,
    0x04,0x80,0x01,0x20,0x02,0x7f,0xfe,0x40,
    0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t bongo_tap_map[] = {
    0x00,0x00,0x00,0x00,0x00,0x38,0x1c,0x00,
    0x00,0x44,0x22,0x00,0x00,0x82,0x41,0x00,
    0x01,0x01,0x80,0x80,0x02,0x00,0x00,0x40,
    0x04,0x00,0x00,0x20,0x08,0x24,0x24,0x10,
    0x10,0x00,0x00,0x08,0x10,0x00,0x00,0x08,
    0x10,0x42,0x42,0x08,0x10,0x3c,0x3c,0x08,
    0x08,0x00,0x00,0x10,0x08,0x00,0x00,0x10,
    0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,
    0x01,0xff,0xff,0x80,0x00,0x3c,0x3c,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const lv_img_dsc_t bongo_idle = {
    .header.always_zero = 0,
    .header.w = 32,
    .header.h = 22,
    .data_size = sizeof(bongo_idle_map),
    .header.cf = LV_IMG_CF_ALPHA_1BIT,
    .data = bongo_idle_map,
};

static const lv_img_dsc_t bongo_tap = {
    .header.always_zero = 0,
    .header.w = 32,
    .header.h = 22,
    .data_size = sizeof(bongo_tap_map),
    .header.cf = LV_IMG_CF_ALPHA_1BIT,
    .data = bongo_tap_map,
};

static struct zmk_widget_battery_status battery_status_widget;
static struct zmk_widget_output_status output_status_widget;
static struct zmk_widget_peripheral_status peripheral_status_widget;
static struct zmk_widget_layer_status layer_status_widget;

static lv_obj_t *bongo_cat;
static bool tap_pending;
static uint32_t tap_until;

static void bongo_timer_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);

    if (!bongo_cat) {
        return;
    }

    uint32_t now = (uint32_t)k_uptime_get();

    if (tap_pending) {
        tap_pending = false;
        lv_img_set_src(bongo_cat, &bongo_tap);
        tap_until = now + 120U;
    } else if (tap_until != 0U && (int32_t)(now - tap_until) >= 0) {
        lv_img_set_src(bongo_cat, &bongo_idle);
        tap_until = 0U;
    }
}

static int on_key_event(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev =
        as_zmk_position_state_changed(eh);

    if (ev && ev->state) {
        tap_pending = true;
    }

    return 0;
}

ZMK_LISTENER(bongo_cat, on_key_event);
ZMK_SUBSCRIPTION(bongo_cat, zmk_position_state_changed);

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    /* Match the stock ZMK layout. */
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget),
                 LV_ALIGN_TOP_RIGHT, 0, 0);

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget),
                 LV_ALIGN_TOP_LEFT, 0, 0);
#else
    zmk_widget_peripheral_status_init(&peripheral_status_widget, screen);
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_status_widget),
                 LV_ALIGN_TOP_LEFT, 0, 0);
#endif
#else
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget),
                 LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                               lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget),
                 LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /*
     * Put Bongo Cat in the lower middle area so it does not cover the
     * stock connectivity or battery widgets.
     */
    bongo_cat = lv_img_create(screen);
    lv_img_set_src(bongo_cat, &bongo_idle);
    lv_obj_set_style_img_recolor(bongo_cat, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(bongo_cat, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_align(bongo_cat, LV_ALIGN_BOTTOM_MID, 0, 0);

    tap_pending = false;
    tap_until = 0U;
    lv_timer_create(bongo_timer_cb, 50, NULL);

    return screen;
}
