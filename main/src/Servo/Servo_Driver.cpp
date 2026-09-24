#include "Servo_Driver.h"

static const char *TAG = "Servo_Controller";

///
/// Ports and GPIO settings
///
constexpr i2c_port_t I2C_PORT = I2C_NUM_0;
constexpr gpio_num_t SDA_GPIO = GPIO_NUM_11;
constexpr gpio_num_t SCL_GPIO = GPIO_NUM_10;
constexpr uint8_t PCA_ADDR = PCA9685_ADDR_BASE;


///
/// Servo settings
///
constexpr uint32_t I2C_FREQ_HZ = 400000;
constexpr uint16_t SERVO_FREQ_HZ = 50;
constexpr uint16_t SERVO_MIN =  102;
constexpr uint16_t SERVO_MAX = 512;

/**
 * @public
 */
void Servo_Driver::init()
{
    ESP_LOGI(TAG, "Servo Controller Init");
    ESP_LOGI(TAG, "Using SDA=%d, SCL=%d", (int)SDA_GPIO, (int)SCL_GPIO);

    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialise descriptor (library sets 1 MHz by default)
    ESP_ERROR_CHECK(pca9685_init_desc(&dev,
        PCA_ADDR,
        I2C_PORT,
        SDA_GPIO,
        SCL_GPIO));

    // Optional but useful while debugging
    ESP_LOGI(TAG, "Descriptor pins: SDA=%d SCL=%d",
        (int)dev.cfg.sda_io_num,
        (int)dev.cfg.scl_io_num);

    dev.cfg.master.clk_speed = I2C_FREQ_HZ;

    ESP_ERROR_CHECK(pca9685_init(&dev));
    ESP_ERROR_CHECK(pca9685_restart(&dev));

    // Make sure the device is awake
    ESP_ERROR_CHECK(pca9685_sleep(&dev, false));

    ESP_ERROR_CHECK(pca9685_set_pwm_frequency(&dev, SERVO_FREQ_HZ));

    uint16_t actual = 0;
    ESP_ERROR_CHECK(pca9685_get_pwm_frequency(&dev, &actual));
    ESP_LOGI(TAG, "Requested %u Hz → actual %u Hz", SERVO_FREQ_HZ, actual);

    // Abort if the frequency is far from expected
    if (actual < 40 || actual > 60) {
        ESP_LOGE(TAG, "Frequency far from expected – check wiring / pull-ups");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

    // Centre all channels
    ESP_LOGI(TAG, "Centre all channels");
    for (int i = 0; i < 16; ++i) {
        auto ch = static_cast<Channel>(i);
        ESP_ERROR_CHECK(set_servo(ch, 90.0f));
    }

    vTaskDelay(pdMS_TO_TICKS(500));
}

/**
 * @public
 * @param channel
 */
void Servo_Driver::test(Channel channel)
{
    // Check the channel is valid
    if (bool is_valid = static_cast<int>(channel) < static_cast<int>(Channel::COUNT); !is_valid) {
        ESP_LOGE(TAG, "Invalid channel");
        return;
    }

    // Check each [pwn].
    for (uint16_t pwm = SERVO_MIN; pwm <= SERVO_MAX; pwm += 10) {
        ESP_LOGI(TAG, "PWM = %u", pwm);
        pca9685_set_pwm_value(&dev, static_cast<uint8_t>(channel), pwm);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

/**
 * @public
 * @param channel
 * @param angle_deg
 */
esp_err_t Servo_Driver::set_servo(Channel channel, float angle_deg)
{
    // Prevents the head from going over the top
    if (channel == Channel::SERVO_1 && angle_deg > 90)
    {
        angle_deg = 90;
    }

    // Prevents the thing from tipping over :/
    if (channel == Channel::SERVO_0 && angle_deg < 20)
    {
        angle_deg = 20;
    }
    else if (angle_deg > 160)
    {
        angle_deg = 160;
    }

    const uint16_t pwm = _angle_to_pwm(angle_deg);

    return pca9685_set_pwm_value(&dev, static_cast<uint8_t>(channel), pwm);
}

esp_err_t Servo_Driver::set_servo_random(Channel channel)
{
    const float angle = (esp_random() % 18001) / 100.0f;
    return set_servo(channel, angle);
}

/**
 * @private
 * @param angle_deg
 * @return
 */
uint16_t Servo_Driver::_angle_to_pwm(float angle_deg)
{
    // Clamp to valid range
    if (angle_deg < 0.0f)   angle_deg = 0.0f;
    if (angle_deg > 180.0f) angle_deg = 180.0f;

    const float ratio = angle_deg / 180.0f;
    return static_cast<uint16_t>(SERVO_MIN + ratio * (SERVO_MAX - SERVO_MIN) + 0.5f);
}
