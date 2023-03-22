#include "ConsoleNESBase.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <soc/gpio_reg.h>

static const char *TAG = "NES_WRITER";

void NESConsoleWriter::latchISR(void *arg)
{
    NESConsoleWriter *writer = static_cast<NESConsoleWriter *>(arg);

    memcpy(writer->_latchedReport, writer->_reportBuf, writer->_reportLen);

    // Get first bit value of report, active low.
    uint8_t firstBit = !(writer->_latchedReport[0] & 1);
    
    uint32_t regValue = REG_READ(writer->_dataOutRegister);

    // Set data pin to first bit value.
    regValue &= ~(1 << writer->_dataOutBitPosition);
    regValue |= (firstBit << writer->_dataOutBitPosition);
    REG_WRITE(writer->_dataOutRegister, regValue);

    writer->_currentBit = 1;
}

void NESConsoleWriter::clockISR(void *arg)
{
    NESConsoleWriter *writer = static_cast<NESConsoleWriter *>(arg);

    if(writer->_currentBit >= writer->_reportLen * 8)
    {
        writer->_currentBit = 0;

        uint32_t regValue = REG_READ(writer->_dataOutRegister);
        regValue &= ~(1 << writer->_dataOutBitPosition);
        REG_WRITE(writer->_dataOutRegister, regValue);

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        xSemaphoreGiveFromISR(writer->_frameSemaphore, &xHigherPriorityTaskWoken);

        if(xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }
    }
    else
    {
        int32_t start = esp_timer_get_time();
        uint8_t byte = writer->_latchedReport[writer->_currentBit / 8];
        uint8_t bit = writer->_currentBit % 8;

        // Get value of current bit, active low.
        uint8_t bitValue = !(byte & (1 << bit));
        
        uint32_t regValue = REG_READ(writer->_dataOutRegister);

        // Set data pin to current bit value.
        regValue &= ~(1 << writer->_dataOutBitPosition);
        regValue |= (bitValue << writer->_dataOutBitPosition);
        REG_WRITE(writer->_dataOutRegister, regValue);

        writer->_currentBit++;
    }
}

NESConsoleWriter::NESConsoleWriter(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock, uint8_t *reportBuf, size_t reportLen)
{
    _data = static_cast<gpio_num_t>(pinData);
    _latch = static_cast<gpio_num_t>(pinLatch);
    _clock = static_cast<gpio_num_t>(pinClock);
    _dataOutRegister = _data < 32 ? GPIO_OUT_REG : GPIO_OUT1_REG;
    _dataOutBitPosition = _data % 32;

    _reportBuf = reportBuf;
    _reportLen = reportLen;

    _currentBit = 0;
    _latchedReport = new uint8_t[reportLen];
    _frameSemaphore = xSemaphoreCreateBinary();
    _interruptsActive = false;
}

NESConsoleWriter::~NESConsoleWriter() 
{ 
    stop();

    if(_latchedReport != nullptr)
    {
        delete[] _latchedReport;
        _latchedReport = nullptr;
    }

    vSemaphoreDelete(_frameSemaphore);
}

esp_err_t NESConsoleWriter::initialize()
{
    stop();

    gpio_set_direction(_data, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_direction(_latch, gpio_mode_t::GPIO_MODE_INPUT);
    gpio_set_direction(_clock, gpio_mode_t::GPIO_MODE_INPUT);

    esp_err_t ret = gpio_install_isr_service(0);
    if(ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) 
    {
        ESP_LOGE(TAG, "gpio install isr service failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = gpio_isr_handler_add(_latch, latchISR, this);
    if(ret != ESP_OK)
        return ret;
    ret = gpio_intr_enable(_latch);
    if(ret != ESP_OK)
        return ret;
    gpio_set_intr_type(_latch, gpio_int_type_t::GPIO_INTR_POSEDGE); // Actual controller outputs only on falling edge, but this makes timing easier.

    ret = gpio_isr_handler_add(_clock, clockISR, this);
    if(ret != ESP_OK)
        return ret;
    ret = gpio_intr_enable(_clock);
    if(ret != ESP_OK)
        return ret;
    gpio_set_intr_type(_clock, gpio_int_type_t::GPIO_INTR_POSEDGE);

    _interruptsActive = true;
    
    gpio_set_pull_mode(_latch, gpio_pull_mode_t::GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(_clock, gpio_pull_mode_t::GPIO_PULLUP_ONLY);

    return ESP_OK;
}

void NESConsoleWriter::stop()
{
    if(_interruptsActive)
    {
        gpio_isr_handler_remove(_latch);
        gpio_isr_handler_remove(_clock);
        _interruptsActive = false;
    }
}

bool NESConsoleWriter::waitNextFrame()
{
    return xSemaphoreTake(_frameSemaphore, 100 / portTICK_PERIOD_MS);
}
