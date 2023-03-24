#include "ConsoleN64.hpp"
#include "internal/bits.h"
#include <string.h>
#include <esp_log.h>
#include <esp_timer.h>

#define RMT_MAX_RX_BYTES 34 // Maximum bytes received in a single read.

const static rmt_receive_config_t n64_rmt_rx_config = {
    .signal_range_min_ns = 1000000000 / N64_RMT_RESOLUTION_HZ,
    .signal_range_max_ns = 6 * 1000,
};

void ConsoleN64::receiveTask(void *arg)
{
    ConsoleN64 *console = (ConsoleN64 *)arg;

    for(;;)
    {
        uint8_t buf[1];
        console->_interface->read(sizeof(buf) * 8 + 1);

        if(xSemaphoreTake(console->_receiveSemaphore, 20 / portTICK_PERIOD_MS))
        {
            xSemaphoreGive(console->_waitFrameSemaphore);
        }
        else
        {
            esp_rom_printf("ERR ERR ERR\n");
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

bool ConsoleN64::receiveHandler(const rmt_rx_done_event_data_t *edata, void *user_data)
{
    // gpio_set_level(GPIO_NUM_12, 1);
    BaseType_t taskWoken = pdFALSE;
    uint8_t decoded[1];
    size_t bytes = N64Interface::n64_rmt_decode_data(edata->received_symbols, edata->num_symbols, decoded, sizeof(decoded));

    // gpio_set_level(GPIO_NUM_12, 0);

    if(bytes == 0) {
        return false;
    }

    ConsoleN64 *console = (ConsoleN64 *)user_data;
    switch (decoded[0])
    {
    case 0xFF: // Reset Fallthrough
    case 0x00: // Status
    {
        uint8_t response[] = { 0x05, 0x00, 0x00 };
        console->_interface->write(response, sizeof(response));
        break;
    }
    case 0x01: // Poll
        console->_interface->write(console->_report, sizeof(console->_report));
        break;
    
    default:
        break;
    }

    xSemaphoreGiveFromISR(console->_receiveSemaphore, &taskWoken);
        
    return taskWoken;
}

ConsoleN64::ConsoleN64(uint8_t pinData)
{
    _data = static_cast<gpio_num_t>(pinData);
    _interface = new N64Interface(_data, RMT_MAX_RX_BYTES, receiveHandler, this, n64_rmt_rx_config);
    _receiveTaskHandle = nullptr;
    _waitFrameSemaphore = xSemaphoreCreateBinary();
    _receiveSemaphore = xSemaphoreCreateBinary();
    memset(_report, 0, sizeof(_report));
}

ConsoleN64::~ConsoleN64()
{
    stop();

    if(_interface != nullptr)
    {
        delete _interface;
    }

    vSemaphoreDelete(_waitFrameSemaphore);
    vSemaphoreDelete(_receiveSemaphore);
}

esp_err_t ConsoleN64::initialize()
{
    stop();

    esp_err_t ret = _interface->initialize();

    if (ret != ESP_OK)
        return ret;

    uint8_t initByte = 0;
    _interface->write(&initByte, sizeof(initByte));

    xTaskCreate(receiveTask, "N64 Rec Task", 2048, this, 10, &_receiveTaskHandle);

    return ret;
}

void ConsoleN64::stop()
{
    if(_receiveTaskHandle != nullptr)
    {
        vTaskDelete(_receiveTaskHandle);
        _receiveTaskHandle = nullptr;
    }
}

bool ConsoleN64::waitNextFrame()
{
    return xSemaphoreTake(_waitFrameSemaphore, 100 / portTICK_PERIOD_MS);
}

bool ConsoleN64::a()      { return _report[0] & (1 << 7); }
bool ConsoleN64::b()      { return _report[0] & (1 << 6); }
bool ConsoleN64::z()      { return _report[0] & (1 << 5); }
bool ConsoleN64::start()  { return _report[0] & (1 << 4); }
bool ConsoleN64::up()     { return _report[0] & (1 << 3); }
bool ConsoleN64::down()   { return _report[0] & (1 << 2); }
bool ConsoleN64::left()   { return _report[0] & (1 << 1); }
bool ConsoleN64::right()  { return _report[0] & (1 << 0); }

bool ConsoleN64::l()        { return _report[1] & (1 << 5); }
bool ConsoleN64::r()        { return _report[1] & (1 << 4); }
bool ConsoleN64::c_up()     { return _report[1] & (1 << 3); }
bool ConsoleN64::c_down()   { return _report[1] & (1 << 2); }
bool ConsoleN64::c_left()   { return _report[1] & (1 << 1); }
bool ConsoleN64::c_right()  { return _report[1] & (1 << 0); }

int8_t ConsoleN64::stick_x() 
{ 
    int8_t signedValue;
    memcpy(&signedValue, &_report[2], sizeof(uint8_t));
    return signedValue; 
}

int8_t ConsoleN64::stick_y() 
{ 
    int8_t signedValue;
    memcpy(&signedValue, &_report[3], sizeof(uint8_t));
    return signedValue; 
}

void ConsoleN64::set_a(bool pressed)       { SET_BIT(_report[0], 7, pressed); }
void ConsoleN64::set_b(bool pressed)       { SET_BIT(_report[0], 6, pressed); }
void ConsoleN64::set_z(bool pressed)       { SET_BIT(_report[0], 5, pressed); }
void ConsoleN64::set_start(bool pressed)   { SET_BIT(_report[0], 4, pressed); }
void ConsoleN64::set_up(bool pressed)      { SET_BIT(_report[0], 3, pressed); }
void ConsoleN64::set_down(bool pressed)    { SET_BIT(_report[0], 2, pressed); }
void ConsoleN64::set_left(bool pressed)    { SET_BIT(_report[0], 1, pressed); }
void ConsoleN64::set_right(bool pressed)   { SET_BIT(_report[0], 0, pressed); }

void ConsoleN64::set_l(bool pressed)       { SET_BIT(_report[1], 5, pressed); }
void ConsoleN64::set_r(bool pressed)       { SET_BIT(_report[1], 4, pressed); }
void ConsoleN64::set_c_up(bool pressed)    { SET_BIT(_report[1], 3, pressed); }
void ConsoleN64::set_c_down(bool pressed)  { SET_BIT(_report[1], 2, pressed); }
void ConsoleN64::set_c_left(bool pressed)  { SET_BIT(_report[1], 1, pressed); }
void ConsoleN64::set_c_right(bool pressed) { SET_BIT(_report[1], 0, pressed); }

void ConsoleN64::set_stick_x(int8_t x) 
{
    memcpy(&_report[2], &x, sizeof(int8_t)); 
}

void ConsoleN64::set_stick_y(int8_t y) 
{
    memcpy(&_report[3], &y, sizeof(int8_t));
}
