#pragma once
#include "lvgl.h"
#include "hal/lv_hal_disp.h"
#include "../LCD//LCD_Driver.h"
#include "LVGL_Driver.h"

class UI_Controller
{
public:
    static void init();

private:
    inline static lv_disp_t* _disp1;
    inline static lv_disp_t* _disp2;

    static void _show_loading();

    static void _show_eyes();
};
