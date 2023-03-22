#include "ConsoleNES.hpp"
#include <esp_timer.h>
#include "internal/bits.h"

ConsoleNES::ConsoleNES(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock) : 
    NESConsoleWriter(pinData, pinLatch, pinClock, &_report, sizeof(_report))
{
    _report = 0;
}

ConsoleNES::~ConsoleNES() { }

bool ConsoleNES::a() { return _report & (1 << 0); }
bool ConsoleNES::b()      { return _report & (1 << 1); }
bool ConsoleNES::select() { return _report & (1 << 2); }
bool ConsoleNES::start()  { return _report & (1 << 3); }
bool ConsoleNES::up()     { return _report & (1 << 4); }
bool ConsoleNES::down()   { return _report & (1 << 5); }
bool ConsoleNES::left()   { return _report & (1 << 6); }
bool ConsoleNES::right()  { return _report & (1 << 7); }

void ConsoleNES::set_a(bool pressed)      { SET_BIT(_report, 0, pressed); }
void ConsoleNES::set_b(bool pressed)      { SET_BIT(_report, 1, pressed); }
void ConsoleNES::set_select(bool pressed) { SET_BIT(_report, 2, pressed); }
void ConsoleNES::set_start(bool pressed)  { SET_BIT(_report, 3, pressed); }
void ConsoleNES::set_up(bool pressed)     { SET_BIT(_report, 4, pressed); }
void ConsoleNES::set_down(bool pressed)   { SET_BIT(_report, 5, pressed); }
void ConsoleNES::set_left(bool pressed)   { SET_BIT(_report, 6, pressed); }
void ConsoleNES::set_right(bool pressed)  { SET_BIT(_report, 7, pressed); }

uint8_t ConsoleNES::raw() { return _report; }
void ConsoleNES::setRaw(uint8_t report) { _report = report; }
