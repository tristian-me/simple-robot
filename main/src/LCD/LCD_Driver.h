#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"

class LCD_Driver {
public:
    static void init();

    // Accessors for LVGL
    static esp_lcd_panel_io_handle_t get_io1()    { return io_1; }
    static esp_lcd_panel_handle_t   get_panel1() { return panel_1; }
    static esp_lcd_panel_io_handle_t get_io2()    { return io_2; }
    static esp_lcd_panel_handle_t   get_panel2() { return panel_2; }

    static void set_backlight_brightness(uint8_t percent); // 0-100

private:
    // Handles
    inline static esp_lcd_panel_io_handle_t io_1    = nullptr;
    inline static esp_lcd_panel_handle_t   panel_1 = nullptr;
    inline static esp_lcd_panel_io_handle_t io_2    = nullptr;
    inline static esp_lcd_panel_handle_t   panel_2 = nullptr;

    // Internal helpers
    static void init_spi_bus();
    static void init_panel(gpio_num_t cs, gpio_num_t rst,
                           bool mirror_x, bool mirror_y, bool swap_xy,
                           esp_lcd_panel_io_handle_t *io,
                           esp_lcd_panel_handle_t *panel);
    static void init_backlight();
};