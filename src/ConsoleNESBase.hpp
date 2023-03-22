#pragma once

#include "stdint.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class NESConsoleWriter
{
private:
    SemaphoreHandle_t _frameSemaphore;
    bool _interruptsActive;

    static void latchISR(void *arg);
    static void clockISR(void *arg);

    uint8_t *_latchedReport;
    uint8_t *_reportBuf;
    size_t _reportLen;
    size_t _currentBit;

    gpio_num_t _data;
    gpio_num_t _latch;
    gpio_num_t _clock;

    int _dataOutRegister;
    int _dataOutBitPosition;

public:
    NESConsoleWriter(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock, uint8_t *reportBuf, size_t reportLen);
    ~NESConsoleWriter();

    esp_err_t initialize();
    void stop();

    /// @brief Blocks until the console reads the current data.
    /// It waits for a maximum of 100ms.
    /// @return This returns false if the console did not update after 100ms.
    bool waitNextFrame();
};
