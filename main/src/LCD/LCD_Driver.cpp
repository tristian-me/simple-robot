#include "LCD_Driver.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_panel_ops.h"
#include "hal/lv_hal_disp.h"

static const char *TAG = "LCD_Controller";

// ===================== Pin & config =====================
constexpr spi_host_device_t LCD_SPI_HOST     = SPI2_HOST;
constexpr int               LCD_PIXEL_CLOCK = 20 * 1000 * 1000; // 40 MHz is safe
constexpr int               LCD_WIDTH       = 240;
constexpr int               LCD_HEIGHT      = 240;
constexpr int               LCD_BPP         = 16;

constexpr gpio_num_t PIN_MOSI = GPIO_NUM_42;
constexpr gpio_num_t PIN_MISO = GPIO_NUM_40;
constexpr gpio_num_t PIN_SCLK = GPIO_NUM_41;
constexpr gpio_num_t PIN_DC   = GPIO_NUM_45;

constexpr gpio_num_t PIN_LCD1_CS  = GPIO_NUM_47;
constexpr gpio_num_t PIN_LCD1_RST = GPIO_NUM_48;
constexpr gpio_num_t PIN_LCD1_BL  = GPIO_NUM_46;

constexpr gpio_num_t PIN_LCD2_CS  = GPIO_NUM_38;
constexpr gpio_num_t PIN_LCD2_RST = GPIO_NUM_8;
constexpr gpio_num_t PIN_LCD2_BL  = GPIO_NUM_39;

// Backlight PWM
constexpr ledc_mode_t      BL_MODE       = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t     BL_TIMER      = LEDC_TIMER_0;
constexpr ledc_timer_bit_t BL_RESOLUTION = LEDC_TIMER_13_BIT;
constexpr uint32_t         BL_FREQ       = 5000;
constexpr uint32_t         BL_MAX_DUTY   = ((1 << LEDC_TIMER_13_BIT) - 1); //(1u << 8) - 1;

static ledc_channel_config_t bl_ch1{};
static ledc_channel_config_t bl_ch2{};

// ===================== Implementation =====================

void LCD_Driver::init_spi_bus()
{
    ESP_LOGI(TAG, "Initialize SPI bus");

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num     = PIN_MOSI;
    buscfg.miso_io_num     = PIN_MISO;
    buscfg.sclk_io_num     = PIN_SCLK;
    buscfg.quadwp_io_num   = -1;
    buscfg.quadhd_io_num   = -1;
    buscfg.max_transfer_sz = LCD_WIDTH * 80 * sizeof(uint16_t);

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
}

void LCD_Driver::init_panel(gpio_num_t cs, gpio_num_t rst,
                                bool mirror_x, bool mirror_y, bool swap_xy,
                                esp_lcd_panel_io_handle_t *io,
                                esp_lcd_panel_handle_t *panel)
{
    ESP_LOGI(TAG, "Init panel CS=%d RST=%d", (int)cs, (int)rst);

    // Panel IO
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num       = cs;
    io_config.dc_gpio_num       = PIN_DC;
    io_config.spi_mode          = 0;
    io_config.pclk_hz           = LCD_PIXEL_CLOCK;
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits      = 8;
    io_config.lcd_param_bits    = 8;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &io_handle));
    *io = io_handle;

    // Panel
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = rst;
    panel_config.rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.bits_per_pixel = LCD_BPP;

    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(*panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(*panel, mirror_x, mirror_y));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(*panel, swap_xy));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*panel, true));
}

void LCD_Driver::init_backlight()
{
    ESP_LOGI(TAG, "Init backlight");

    ledc_timer_config_t timer = {};
    timer.speed_mode       = BL_MODE;
    timer.duty_resolution  = BL_RESOLUTION;
    timer.timer_num        = BL_TIMER;
    timer.freq_hz          = BL_FREQ;
    timer.clk_cfg          = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    auto setup_channel = [](gpio_num_t gpio, ledc_channel_t ch, ledc_channel_config_t *cfg) {
        gpio_config_t io = {};
        io.pin_bit_mask = 1ULL << gpio;
        io.mode = GPIO_MODE_OUTPUT;
        gpio_config(&io);

        cfg->channel    = ch;
        cfg->duty       = 0;
        cfg->gpio_num   = gpio;
        cfg->speed_mode = BL_MODE;
        cfg->timer_sel  = BL_TIMER;
        ledc_channel_config(cfg);
    };

    setup_channel(PIN_LCD1_BL, LEDC_CHANNEL_0, &bl_ch1);
    setup_channel(PIN_LCD2_BL, LEDC_CHANNEL_1, &bl_ch2);

    ledc_fade_func_install(0);
    set_backlight_brightness(70); // default 70%
}

void LCD_Driver::set_backlight_brightness(uint8_t percent)
{
    if (percent > 100) percent = 100;
    uint32_t duty = (BL_MAX_DUTY * percent) / 100;

    ledc_set_duty(BL_MODE, bl_ch1.channel, duty);
    ledc_update_duty(BL_MODE, bl_ch1.channel);

    ledc_set_duty(BL_MODE, bl_ch2.channel, duty);
    ledc_update_duty(BL_MODE, bl_ch2.channel);
}

void LCD_Driver::init()
{
    ESP_LOGI(TAG, "LCD_Controller::init()");

    init_spi_bus();

    // Display 1 – default orientation
    init_panel(PIN_LCD1_CS, PIN_LCD1_RST,
               false, false, false,
               &io_1, &panel_1);

    // Display 2 – mirrored version (adjust if needed)
    init_panel(PIN_LCD2_CS, PIN_LCD2_RST,
               false, false, false,
               &io_2, &panel_2);

    init_backlight();

    ESP_LOGI(TAG, "Both panels ready");
}