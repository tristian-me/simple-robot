#pragma once

#include "lvgl.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port_disp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lvgl_port.h"

enum class Display
{
    PANEL_1 = 1,
    PANEL_2 = 2,
};

class LVGL_Driver {
public:
    // Call after LCD_Controller::init()
    static lv_disp_t* add_display(Display display_no,
                                  esp_lcd_panel_io_handle_t io,
                                  esp_lcd_panel_handle_t panel);

    static void init_tick();          // starts the 2 ms tick timer
    static void start_task();         // starts the lv_timer_handler task

private:
    static lvgl_port_display_cfg_t _get_disp_cfg(Display display,
                                                esp_lcd_panel_io_handle_t io,
                                                esp_lcd_panel_handle_t panel,
                                                int hres = 240,
                                                int vres = 240);
};