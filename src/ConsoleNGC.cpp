#include "ConsoleNGC.hpp"
#include "internal/bits.h"
#include <string.h>
#include <esp_log.h>
#include <esp_timer.h>

#define RMT_MAX_RX_BYTES 34 // Maximum bytes received in a single read.

const static rmt_receive_config_t n64_rmt_rx_config = {
    .signal_range_min_ns = 1000000000 / N64_RMT_RESOLUTION_HZ,
    .signal_range_max_ns = 6 * 1000,
};

void ConsoleNGC::receiveTask(void *arg)
{
    ConsoleNGC *console = (ConsoleNGC *)arg;

    for(;;)
    {
        uint8_t buf[3];
        console->_interface->read(sizeof(buf) * 8 + 1);

        if(!xSemaphoreTake(console->_receiveSemaphore, 40 / portTICK_PERIOD_MS))
        {
            // esp_rom_printf("ERR ERR ERR\n");
            continue;
        }

        gpio_set_level(GPIO_NUM_19, 1);
        console->_interface->waitForTx();
        // esp_rom_delay_us(100);
        gpio_set_level(GPIO_NUM_19, 0);

        xSemaphoreGive(console->_waitFrameSemaphore);
    }
}

bool ConsoleNGC::receiveHandler(const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t taskWoken = pdFALSE;
    uint8_t decoded[3];
    size_t bytes = N64Interface::n64_rmt_decode_data(edata->received_symbols, edata->num_symbols, decoded, sizeof(decoded));

    if(bytes == 0) {
        ESP_LOGE("NGC", "shit");
        return false;
    }

    ConsoleNGC *console = (ConsoleNGC *)user_data;
    switch (decoded[0])
    {
    case 0xFF: // Reset Fallthrough
    case 0x00: // Status
    {
        uint8_t response[] = { 0x09, 0x00, 0x20 };
        console->_interface->write(response, sizeof(response));
        break;
    }
    case 0x40: // Poll
        console->_interface->write(console->_report, sizeof(console->_report));
        break;

    case 0x41: // Fallthrough
    case 0x42: // Calibrate
    {
        uint8_t response[] = { 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00 };
        console->_interface->write(response, sizeof(response));
        break;
    }
    default:
        break;
    }

    xSemaphoreGiveFromISR(console->_receiveSemaphore, &taskWoken);
        
    return taskWoken;
}

ConsoleNGC::ConsoleNGC(uint8_t pinData)
{
    _data = static_cast<gpio_num_t>(pinData);
    _interface = new N64Interface(_data, RMT_MAX_RX_BYTES, receiveHandler, this, n64_rmt_rx_config);
    _receiveTaskHandle = nullptr;
    _waitFrameSemaphore = xSemaphoreCreateBinary();
    _receiveSemaphore = xSemaphoreCreateBinary();
    memset(_report, 0, sizeof(_report));
}

ConsoleNGC::~ConsoleNGC()
{
    stop();

    if(_interface != nullptr)
    {
        delete _interface;
    }

    vSemaphoreDelete(_waitFrameSemaphore);
    vSemaphoreDelete(_receiveSemaphore);
}

esp_err_t ConsoleNGC::initialize()
{
    stop();

    esp_err_t ret = _interface->initialize();

    if (ret != ESP_OK)
        return ret;

    uint8_t initByte = 0;
    _interface->write(&initByte, sizeof(initByte));

    xTaskCreate(receiveTask, "NGC Rec Task", 2048, this, 10, &_receiveTaskHandle);

    return ret;
}

void ConsoleNGC::stop()
{
    if(_receiveTaskHandle != nullptr)
    {
        vTaskDelete(_receiveTaskHandle);
        _receiveTaskHandle = nullptr;
    }
}

bool ConsoleNGC::waitNextFrame()
{
    return xSemaphoreTake(_waitFrameSemaphore, 100 / portTICK_PERIOD_MS);
}

bool ConsoleNGC::a()      { return _report[0] & (1 << 0); }
bool ConsoleNGC::b()      { return _report[0] & (1 << 1); }
bool ConsoleNGC::x()      { return _report[0] & (1 << 2); }
bool ConsoleNGC::y()      { return _report[0] & (1 << 3); }
bool ConsoleNGC::start()  { return _report[0] & (1 << 4); }

bool ConsoleNGC::left()   { return _report[0] & (1 << 0); }
bool ConsoleNGC::right()  { return _report[0] & (1 << 1); }
bool ConsoleNGC::up()     { return _report[0] & (1 << 2); }
bool ConsoleNGC::down()   { return _report[0] & (1 << 3); }
bool ConsoleNGC::z()      { return _report[0] & (1 << 4); }

int8_t ConsoleNGC::stick_x() 
{ 
    int8_t signedValue;
    memcpy(&signedValue, &_report[2], sizeof(uint8_t));
    return signedValue;
}

int8_t ConsoleNGC::stick_y() 
{ 
    int8_t signedValue;
    memcpy(&signedValue, &_report[3], sizeof(uint8_t));
    return signedValue; 
}

int8_t ConsoleNGC::c_stick_x()
{
    int8_t signedValue;
    memcpy(&signedValue, &_report[4], sizeof(uint8_t));
    return signedValue;
}

int8_t ConsoleNGC::c_stick_y()
{
    int8_t signedValue;
    memcpy(&signedValue, &_report[5], sizeof(uint8_t));
    return signedValue;
}

uint8_t ConsoleNGC::l() { return _report[6]; }
uint8_t ConsoleNGC::r() { return _report[7]; }

void ConsoleNGC::set_a(bool pressed)     { SET_BIT(_report[0], 7, pressed); }
void ConsoleNGC::set_b(bool pressed)     { SET_BIT(_report[0], 6, pressed); }
void ConsoleNGC::set_x(bool pressed)     { SET_BIT(_report[0], 6, pressed); }
void ConsoleNGC::set_y(bool pressed)     { SET_BIT(_report[0], 6, pressed); }
void ConsoleNGC::set_start(bool pressed) { SET_BIT(_report[0], 4, pressed); }

void ConsoleNGC::set_z(bool pressed)     { SET_BIT(_report[0], 5, pressed); }
void ConsoleNGC::set_up(bool pressed)    { SET_BIT(_report[0], 3, pressed); }
void ConsoleNGC::set_down(bool pressed)  { SET_BIT(_report[0], 2, pressed); }
void ConsoleNGC::set_left(bool pressed)  { SET_BIT(_report[0], 1, pressed); }
void ConsoleNGC::set_right(bool pressed) { SET_BIT(_report[0], 0, pressed); }

void ConsoleNGC::set_l(uint8_t value) 
{ 
    _report[6] = value;
    SET_BIT(_report[1], 5, value == 255);
}

void ConsoleNGC::set_r(uint8_t value) 
{
    _report[7] = value;
    SET_BIT(_report[1], 6, value == 255);
}

void ConsoleNGC::set_l(uint8_t value, bool endstop)
{
    _report[6] = value;
    SET_BIT(_report[1], 5, endstop);
}

void ConsoleNGC::set_r(uint8_t value, bool endstop)
{
    _report[7] = value;
    SET_BIT(_report[1], 6, endstop);
}

void ConsoleNGC::set_stick_x(int8_t x) 
{
    memcpy(&_report[2], &x, sizeof(int8_t)); 
}

void ConsoleNGC::set_stick_y(int8_t y) 
{
    memcpy(&_report[3], &y, sizeof(int8_t));
}

void ConsoleNGC::set_c_stick_x(int8_t x)
{
    memcpy(&_report[4], &x, sizeof(int8_t));
}

void ConsoleNGC::set_c_stick_y(int8_t y)
{
    memcpy(&_report[5], &y, sizeof(int8_t));
}
