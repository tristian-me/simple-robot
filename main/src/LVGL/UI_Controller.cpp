#include "UI_Controller.h"
#include <stdlib.h>
#include <math.h>
#include "esp_task_wdt.h"
#include "esp_heap_caps.h"

///
// Eye data
///
typedef struct {
    lv_obj_t * canvas;
    lv_point_t pupil_pos;
} eye_t;

static eye_t eye1, eye2;
static lv_point_t global_target_pos = {0, 0};
static uint32_t last_move_time = 0;

#define CANVAS_SIZE      140
#define MOVE_INTERVAL    2000
#define MOVE_STEP        3
#define MAX_OFFSET       28      // how far pupil can travel from center
#define PUPIL_R          28

/**********************
 * Draw one eye
 **********************/
static void draw_eye(const eye_t * eye)
{
    lv_canvas_fill_bg(eye->canvas, lv_color_hex(0x000000), LV_OPA_COVER);

    constexpr lv_coord_t cx = CANVAS_SIZE / 2;
    constexpr lv_coord_t cy = CANVAS_SIZE / 2;

    const lv_coord_t px = LV_CLAMP(cx - MAX_OFFSET, cx + eye->pupil_pos.x, cx + MAX_OFFSET);
    const lv_coord_t py = LV_CLAMP(cy - MAX_OFFSET, cy + eye->pupil_pos.y, cy + MAX_OFFSET);

    // Pupil only
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = LV_RADIUS_CIRCLE;
    d.bg_color = lv_color_hex(0xFFFFFF);   // white “pupil” on black (or swap to black on white)
    lv_canvas_draw_rect(eye->canvas,
                        px - PUPIL_R, py - PUPIL_R,
                        PUPIL_R * 2, PUPIL_R * 2,
                        &d);
}

static void eye_anim_timer(lv_timer_t * t)
{
    LV_UNUSED(t);
    uint32_t now = lv_tick_get();

    if (now - last_move_time > MOVE_INTERVAL) {
        last_move_time = now;
        global_target_pos.x = (rand() % (MAX_OFFSET * 2)) - MAX_OFFSET;
        global_target_pos.y = (rand() % (MAX_OFFSET * 2)) - MAX_OFFSET;
    }

    auto step = [](lv_coord_t & cur, lv_coord_t tgt) {
        if (cur < tgt) { cur = (lv_coord_t)(cur + MOVE_STEP); if (cur > tgt) cur = tgt; }
        else if (cur > tgt) { cur = (lv_coord_t)(cur - MOVE_STEP); if (cur < tgt) cur = tgt; }
    };
    step(eye1.pupil_pos.x, global_target_pos.x);
    step(eye1.pupil_pos.y, global_target_pos.y);
    eye2.pupil_pos = eye1.pupil_pos;

    draw_eye(&eye1);
    draw_eye(&eye2);
}

static void init_eye(eye_t * eye, lv_disp_t * disp)
{
    lv_disp_set_default(disp);

    esp_task_wdt_reset();

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    eye->canvas = lv_canvas_create(lv_scr_act());
    eye->pupil_pos = {0, 0};

    lv_obj_set_size(eye->canvas, CANVAS_SIZE, CANVAS_SIZE);
    lv_obj_center(eye->canvas);

    // Static – no heap, no WDT risk (~20 KB each)
    static lv_color_t buf1[CANVAS_SIZE * CANVAS_SIZE];
    static lv_color_t buf2[CANVAS_SIZE * CANVAS_SIZE];
    lv_color_t * buf = (eye == &eye1) ? buf1 : buf2;

    lv_canvas_set_buffer(eye->canvas, buf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);
    draw_eye(eye);
}

///
/// Public
///

void UI_Controller::init()
{
    // Setup display panel 1
    _disp1 = LVGL_Driver::add_display(
        Display::PANEL_1,
        LCD_Driver::get_io1(),
        LCD_Driver::get_panel1()
    );

    // Setup display panel 2
    _disp2 = LVGL_Driver::add_display(
        Display::PANEL_2,
        LCD_Driver::get_io2(),
        LCD_Driver::get_panel2()
    );

    _show_eyes();
}

///
/// Private
///

void UI_Controller::_show_loading()
{
    // Simple test on display 1
    if (_disp1) {
        lv_disp_set_default(_disp1);
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x97EB97), 0);
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Smells");
        lv_obj_set_style_text_color(label, lv_color_black(), 0);
        lv_obj_center(label);
    }

    // Simple test on display 2
    if (_disp2) {
        lv_disp_set_default(_disp2);
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x00E4FF), 0);
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Leah");
        lv_obj_set_style_text_color(label, lv_color_black(), 0);
        lv_obj_center(label);
    }
}

void UI_Controller::_show_eyes()
{
    srand(12345);

    if (_disp1) {
        init_eye(&eye1, _disp1);
    }
    if (_disp2) {
        init_eye(&eye2, _disp2);
    }

    lv_timer_create(eye_anim_timer, 50, nullptr);
}
