#include "main.h"

static void Driver_Init()
{
    ESP_LOGI(TAG, "Driver Init");

    ESP_ERROR_CHECK(i2cdev_init());

    // Init i2C
    i2c_dev_t dev{};
    std::memset(&dev, 0, sizeof(dev));

    Servo_Driver::init();

    // Initialise LCDs
    LCD_Driver::init();

    // Initialise LVGL
    LVGL_Driver::init_tick();

    // Start the UI
    UI_Controller::init();
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting...");

    // Load drivers
    Driver_Init();

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));    // prevents brownouts
        Servo_Driver::set_servo_random(Channel::SERVO_0);
        Servo_Driver::set_servo_random(Channel::SERVO_1);
    }
}
