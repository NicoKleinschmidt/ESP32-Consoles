#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include <N64Interface.hpp>

class ConsoleN64
{
private:
    uint8_t _report[4];
    gpio_num_t _data;

    N64Interface *_interface;
    TaskHandle_t _receiveTaskHandle;
    SemaphoreHandle_t _receiveSemaphore;
    SemaphoreHandle_t _waitFrameSemaphore;

    static void receiveTask(void *arg);
    static bool receiveHandler(const rmt_rx_done_event_data_t *edata, void *user_data);

public:
    ConsoleN64(uint8_t pinData);
    ~ConsoleN64();

    bool a();
    bool b();
    bool z();
    bool start();
    bool up();
    bool down();
    bool left();
    bool right();
    bool l();
    bool r();
    bool c_up();
    bool c_down();
    bool c_left();
    bool c_right();

    int8_t stick_x();
    int8_t stick_y();

    void set_a(bool pressed);
    void set_b(bool pressed);
    void set_z(bool pressed);
    void set_start(bool pressed);
    void set_up(bool pressed);
    void set_down(bool pressed);
    void set_left(bool pressed);
    void set_right(bool pressed);
    void set_l(bool pressed);
    void set_r(bool pressed);
    void set_c_up(bool pressed);
    void set_c_down(bool pressed);
    void set_c_left(bool pressed);
    void set_c_right(bool pressed);

    void set_stick_x(int8_t x);
    void set_stick_y(int8_t y);

    esp_err_t initialize();
    void stop();

    /// @brief Blocks until the console reads the current data.
    /// It waits for a maximum of 100ms.
    /// @return This returns false if the console did not update after 100ms.
    bool waitNextFrame();
};
