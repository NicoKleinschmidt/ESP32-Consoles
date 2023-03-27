#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include <N64Interface.hpp>

class ConsoleNGC
{
private:
    uint8_t _report[8];
    gpio_num_t _data;

    N64Interface *_interface;
    TaskHandle_t _receiveTaskHandle;
    SemaphoreHandle_t _receiveSemaphore;
    SemaphoreHandle_t _waitFrameSemaphore;

    static void receiveTask(void *arg);
    static bool receiveHandler(const rmt_rx_done_event_data_t *edata, void *user_data);

public:
    ConsoleNGC(uint8_t pinData);
    ~ConsoleNGC();

    bool a();
    bool b();
    bool x();
    bool y();
    bool z();
    bool start();
    bool up();
    bool down();
    bool left();
    bool right();
    uint8_t l();
    uint8_t r();
    
    int8_t c_stick_x();
    int8_t c_stick_y();
    int8_t stick_x();
    int8_t stick_y();

    void set_a(bool pressed);
    void set_b(bool pressed);
    void set_x(bool pressed);
    void set_y(bool pressed);
    void set_z(bool pressed);
    void set_start(bool pressed);
    void set_up(bool pressed);
    void set_down(bool pressed);
    void set_left(bool pressed);
    void set_right(bool pressed);
    void set_l(uint8_t value);
    void set_r(uint8_t value);
    void set_l(uint8_t value, bool endstop);
    void set_r(uint8_t value, bool endstop);
    
    void set_c_stick_x(int8_t x);
    void set_c_stick_y(int8_t y);
    void set_stick_x(int8_t x);
    void set_stick_y(int8_t y);

    esp_err_t initialize();
    void stop();

    /// @brief Blocks until the console reads the current data.
    /// It waits for a maximum of 100ms.
    /// @return This returns false if the console did not update after 100ms.
    bool waitNextFrame();
};
