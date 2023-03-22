#include "ConsoleSNES.hpp"
#include "freertos/task.h"
#include "internal/bits.h"
#include <esp_timer.h>
#include <esp_log.h>

ConsoleSNES::ConsoleSNES(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock) : 
    NESConsoleWriter(pinData, pinLatch, pinClock, (uint8_t *) &_report, sizeof(_report))
{
    _report = 0;
}

bool ConsoleSNES::b()      { return _report & (1 << 0); }
bool ConsoleSNES::y()      { return _report & (1 << 1); }
bool ConsoleSNES::select() { return _report & (1 << 2); }
bool ConsoleSNES::start()  { return _report & (1 << 3); }
bool ConsoleSNES::up()     { return _report & (1 << 4); }
bool ConsoleSNES::down()   { return _report & (1 << 5); }
bool ConsoleSNES::left()   { return _report & (1 << 6); }
bool ConsoleSNES::right()  { return _report & (1 << 7); }
bool ConsoleSNES::a()      { return _report & (1 << 8); }
bool ConsoleSNES::x()      { return _report & (1 << 9); }
bool ConsoleSNES::l()      { return _report & (1 << 10); }
bool ConsoleSNES::r()      { return _report & (1 << 11); }

void ConsoleSNES::set_b(bool pressed)      { SET_BIT(_report, 0, pressed); }
void ConsoleSNES::set_y(bool pressed)      { SET_BIT(_report, 1, pressed); }
void ConsoleSNES::set_select(bool pressed) { SET_BIT(_report, 2, pressed); }
void ConsoleSNES::set_start(bool pressed)  { SET_BIT(_report, 3, pressed); }
void ConsoleSNES::set_up(bool pressed)     { SET_BIT(_report, 4, pressed); }
void ConsoleSNES::set_down(bool pressed)   { SET_BIT(_report, 5, pressed); }
void ConsoleSNES::set_left(bool pressed)   { SET_BIT(_report, 6, pressed); }
void ConsoleSNES::set_right(bool pressed)  { SET_BIT(_report, 7, pressed); }
void ConsoleSNES::set_a(bool pressed)      { SET_BIT(_report, 8, pressed); }
void ConsoleSNES::set_x(bool pressed)      { SET_BIT(_report, 9, pressed); }
void ConsoleSNES::set_l(bool pressed)      { SET_BIT(_report, 10, pressed); }
void ConsoleSNES::set_r(bool pressed)      { SET_BIT(_report, 11, pressed); }

uint16_t ConsoleSNES::raw() { return _report; }
void ConsoleSNES::setRaw(uint16_t report) { _report = report; }
