#include "ConsoleGenesis.hpp"
#include "freertos/FreeRTOS.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <string.h>
#include <soc/gpio_reg.h>

const static char *TAG = "CON_GENESIS";

ConsoleGenesis::ConsoleGenesis(uint8_t pinSelect, uint8_t pinData0, uint8_t pinData1, uint8_t pinData2, uint8_t pinData3, uint8_t pinData4, uint8_t pinData5)
{
    _select = static_cast<gpio_num_t>(pinSelect);
    _data0 = static_cast<gpio_num_t>(pinData0);
    _data1 = static_cast<gpio_num_t>(pinData1);
    _data2 = static_cast<gpio_num_t>(pinData2);
    _data3 = static_cast<gpio_num_t>(pinData3);
    _data4 = static_cast<gpio_num_t>(pinData4);
    _data5 = static_cast<gpio_num_t>(pinData5);

    _waitFrameSemaphore = xSemaphoreCreateBinary();
    _select_gpio_register = _select < 32 ? GPIO_IN_REG : GPIO_IN1_REG;
}

ConsoleGenesis::~ConsoleGenesis() 
{
    stop();
 
    if(_waitFrameSemaphore)
    {
        vSemaphoreDelete(_waitFrameSemaphore);
    }
}

esp_err_t ConsoleGenesis::initialize()
{
    gpio_set_direction(_select, gpio_mode_t::GPIO_MODE_INPUT);
    gpio_set_direction(_data0, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_direction(_data1, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_direction(_data2, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_direction(_data3, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_direction(_data4, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_direction(_data5, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(_select, gpio_pull_mode_t::GPIO_PULLUP_ONLY);

    esp_err_t ret = gpio_install_isr_service(0);
    if(ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) 
    {
        ESP_LOGE(TAG, "gpio install isr service failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = gpio_isr_handler_add(_select, selectISR, this);
    if(ret != ESP_OK)
        return ret;
    ret = gpio_intr_enable(_select);
    if(ret != ESP_OK)
        return ret;
    gpio_set_intr_type(_select, gpio_int_type_t::GPIO_INTR_ANYEDGE);

    _lastTickUs = 0;
    _tickCounter = 0;

    return ESP_OK;
}

void ConsoleGenesis::stop()
{
    gpio_isr_handler_remove(_select);
}

void ConsoleGenesis::selectISR(void *arg)
{
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL_ISR(&mux);

    ConsoleGenesis *console = static_cast<ConsoleGenesis *>(arg);
   
    bool select = REG_READ(console->_select_gpio_register) & (1 << (console->_select % 32));
    
    portEXIT_CRITICAL_ISR(&mux);
    if(!select) {
        // Select LOW

        gpio_set_level(console->_data0, !console->_report.up);
        gpio_set_level(console->_data1, !console->_report.down);
        gpio_set_level(console->_data2, 0);
        gpio_set_level(console->_data3, 0);
        gpio_set_level(console->_data4, !console->_report.a);
        gpio_set_level(console->_data5, !console->_report.start);

        return;
    }

    // Select HIGH
    int32_t currentTime = esp_timer_get_time();
    bool thirdPulse = console->_tickCounter++ == 2;
    bool fastClock = (currentTime - console->_lastTickUs) < 100;

    console->_lastTickUs = currentTime;
    if(thirdPulse) {
        console->_tickCounter = 0;
    }

    if(thirdPulse && fastClock) {
        gpio_set_level(console->_data0, !console->_report.x);
        gpio_set_level(console->_data1, !console->_report.y);
        gpio_set_level(console->_data2, !console->_report.z);
        gpio_set_level(console->_data3, 0);
        gpio_set_level(console->_data4, !console->_report.b);
        gpio_set_level(console->_data5, !console->_report.c);
    } else {
        gpio_set_level(console->_data0, !console->_report.up);
        gpio_set_level(console->_data1, !console->_report.down);
        gpio_set_level(console->_data2, !console->_report.left);
        gpio_set_level(console->_data3, !console->_report.right);
        gpio_set_level(console->_data4, !console->_report.b);
        gpio_set_level(console->_data5, !console->_report.c);
    }

    if(!fastClock || (fastClock && thirdPulse)) {
        BaseType_t taskWoken = pdFALSE;
        xSemaphoreGiveFromISR(console->_waitFrameSemaphore, &taskWoken);

        if(taskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}

bool ConsoleGenesis::waitNextFrame()
{
    return xSemaphoreTake(_waitFrameSemaphore, 100 / portTICK_PERIOD_MS);
}

bool ConsoleGenesis::a() { return _report.a; }
bool ConsoleGenesis::b()     { return _report.b; }
bool ConsoleGenesis::c()     { return _report.c; }
bool ConsoleGenesis::x()     { return _report.x; }
bool ConsoleGenesis::y()     { return _report.y; }
bool ConsoleGenesis::z()     { return _report.z; }
bool ConsoleGenesis::start() { return _report.start; }
bool ConsoleGenesis::mode()  { return _report.mode; }
bool ConsoleGenesis::up()    { return _report.up; }
bool ConsoleGenesis::down()  { return _report.down; }
bool ConsoleGenesis::left()  { return _report.left; }
bool ConsoleGenesis::right() { return _report.right; }
bool ConsoleGenesis::isSixButton() { return _report.is_six_button; }

uint16_t ConsoleGenesis::raw()
{
    uint16_t raw = 0;
    memcpy(&raw, &_report, sizeof(_report));
    return raw;
}

void ConsoleGenesis::set_isSixButton(bool sixButton) { _report.is_six_button = sixButton; }
void ConsoleGenesis::set_a(bool pressed)     { _report.a = pressed; }
void ConsoleGenesis::set_b(bool pressed)     { _report.b = pressed; }
void ConsoleGenesis::set_c(bool pressed)     { _report.c = pressed; }
void ConsoleGenesis::set_x(bool pressed)     { _report.x = pressed; }
void ConsoleGenesis::set_y(bool pressed)     { _report.y = pressed; }
void ConsoleGenesis::set_z(bool pressed)     { _report.z = pressed; }
void ConsoleGenesis::set_start(bool pressed) { _report.start = pressed; }
void ConsoleGenesis::set_mode(bool pressed)  { _report.mode = pressed; }
void ConsoleGenesis::set_up(bool pressed)    { _report.up = pressed; }
void ConsoleGenesis::set_down(bool pressed)  { _report.down = pressed; }
void ConsoleGenesis::set_left(bool pressed)  { _report.left = pressed; }
void ConsoleGenesis::set_right(bool pressed) { _report.right = pressed; }