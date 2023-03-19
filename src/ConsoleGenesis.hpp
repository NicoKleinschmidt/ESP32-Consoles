#pragma once

#include "stdint.h"
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class ConsoleGenesis
{
private:
    struct GenesisReport
    {
        bool up   : 1;
        bool down : 1;
        bool left : 1;
        bool right: 1;
        bool a    : 1;
        bool b    : 1;
        bool c    : 1;
        bool x    : 1;
        bool y    : 1;
        bool z    : 1;
        bool start: 1;
        bool mode : 1;

        bool is_six_button: 1;
    };

    GenesisReport _report;
    SemaphoreHandle_t _waitFrameSemaphore;
    
    gpio_num_t _data0;
    gpio_num_t _data1;
    gpio_num_t _data2;
    gpio_num_t _data3; 
    gpio_num_t _data4;
    gpio_num_t _data5;
    gpio_num_t _select;

    int64_t _lastTickUs;
    uint8_t _tickCounter;
    int _select_gpio_register;

    static void selectISR(void *arg);

public:
    ConsoleGenesis(uint8_t pinSelect, uint8_t pinData0, uint8_t pinData1, uint8_t pinData2, uint8_t pinData3, uint8_t pinData4, uint8_t pinData5);
    ~ConsoleGenesis();

    esp_err_t initialize();
    void stop();

    /// @brief Blocks until the console reads the current data.
    /// It waits for a maximum of 100ms.
    /// @return This returns false if the console did not update after 100ms.
    bool waitNextFrame();

    bool a();
    bool b();
    bool c();
    bool x();
    bool y();
    bool z();
    bool up();
    bool down();
    bool left();
    bool right();
    bool start();
    bool mode();
    bool isSixButton();
    uint16_t raw();

    void set_a(bool pressed);
    void set_b(bool pressed);
    void set_c(bool pressed);
    void set_x(bool pressed);
    void set_y(bool pressed);
    void set_z(bool pressed);
    void set_up(bool pressed);
    void set_down(bool pressed);
    void set_left(bool pressed);
    void set_right(bool pressed);
    void set_start(bool pressed);
    void set_mode(bool pressed);
    void set_isSixButton(bool sixButton);
};
