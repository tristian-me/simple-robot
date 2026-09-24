#include "LVGL_Driver.h"

static const char *TAG = "LVGL_Driver";

static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(2);
}

static void lvgl_task(void *arg)
{
    while (true) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

lv_disp_t* LVGL_Driver::add_display(Display display,
                                    esp_lcd_panel_io_handle_t io,
                                    esp_lcd_panel_handle_t panel)
{
    if (!io || !panel) {
        ESP_LOGE(TAG, "Invalid io/panel handle");
        return nullptr;
    }

    // Initialise LVGL port only once
    static bool port_initialized = false;
    if (!port_initialized) {
        const lvgl_port_cfg_t cfg = ESP_LVGL_PORT_INIT_CONFIG();
        ESP_ERROR_CHECK(lvgl_port_init(&cfg));
        port_initialized = true;
        ESP_LOGI(TAG, "LVGL port initialized");
    }

    lvgl_port_display_cfg_t disp_cfg = _get_disp_cfg(display, io, panel);

    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to add display");
        return nullptr;
    }

    ESP_LOGI(TAG, "Display added to LVGL (%d)", display);
    return disp;
}

void LVGL_Driver::init_tick()
{
    esp_timer_create_args_t args = {};
    args.callback = &lvgl_tick_cb;
    args.arg = nullptr;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "lvgl_tick";
    args.skip_unhandled_events = false;

    esp_timer_handle_t timer = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 2 * 1000)); // 2 ms
}

void LVGL_Driver::start_task()
{
    xTaskCreate(lvgl_task, "lvgl", 4096, nullptr, 5, nullptr);
}

lvgl_port_display_cfg_t LVGL_Driver::_get_disp_cfg(Display display,
                                                  esp_lcd_panel_io_handle_t io,
                                                  esp_lcd_panel_handle_t panel,
                                                  int hres, int vres)
{
    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle     = io;
    disp_cfg.panel_handle  = panel;
    disp_cfg.control_handle = nullptr;
    disp_cfg.buffer_size   = (hres * 40);
    disp_cfg.double_buffer = true;
    disp_cfg.trans_size    = 0;
    disp_cfg.hres          = static_cast<uint32_t>(hres);
    disp_cfg.vres          = static_cast<uint32_t>(vres);
    disp_cfg.monochrome    = false;
    disp_cfg.flags.buff_dma     = true;
    disp_cfg.flags.buff_spiram  = false;
    disp_cfg.flags.sw_rotate    = false;
    disp_cfg.flags.full_refresh = false;
    disp_cfg.flags.direct_mode  = false;

    if (display == Display::PANEL_1)
    {
        disp_cfg.rotation.swap_xy  = true;
        disp_cfg.rotation.mirror_y = true;
        disp_cfg.rotation.mirror_x = true;
    }
    if (display == Display::PANEL_2)
    {
        disp_cfg.rotation.swap_xy = true;
    }

    return disp_cfg;
}
