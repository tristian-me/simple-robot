#ifndef HELLO_WORLD_SERVO_CONTROLLER_H
#define HELLO_WORLD_SERVO_CONTROLLER_H

#include <pca9685.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_random.h"
#include <cstring>

enum class Channel
{
    SERVO_0 = 0,
    SERVO_1 = 1,
    SERVO_2 = 2,
    SERVO_3 = 3,
    SERVO_4 = 4,
    SERVO_5 = 5,
    SERVO_6 = 6,
    SERVO_7 = 7,
    SERVO_8 = 8,
    SERVO_9 = 9,
    SERVO_10 = 10,
    SERVO_11 = 11,
    SERVO_12 = 12,
    SERVO_13 = 13,
    SERVO_14 = 14,
    SERVO_15 = 15,
    COUNT   // used for counting
};

class Servo_Driver
{
public:
    inline static i2c_dev_t dev{};

    static void init();
    static void test(Channel channel);
    static esp_err_t set_servo(Channel channel, float angle_deg);
    static esp_err_t set_servo_random(Channel channel);

private:
    i2c_master_bus_handle_t _bus_handle{};

    static uint16_t _angle_to_pwm(float angle_deg);
};

#endif // HELLO_WORLD_SERVO_CONTROLLER_H