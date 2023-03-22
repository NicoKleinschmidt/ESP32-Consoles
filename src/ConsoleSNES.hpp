#pragma once

#include "ConsoleNESBase.hpp"

class ConsoleSNES : public NESConsoleWriter
{
private:
    uint16_t _report;

public:
    ConsoleSNES(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock);

    bool a();
    bool b();
    bool x();
    bool y();
    bool l();
    bool r();
    bool select();
    bool start();
    bool up();
    bool down();
    bool left();
    bool right();

    void set_a(bool pressed);
    void set_b(bool pressed);
    void set_x(bool pressed);
    void set_y(bool pressed);
    void set_l(bool pressed);
    void set_r(bool pressed);
    void set_select(bool pressed);
    void set_start(bool pressed);
    void set_up(bool pressed);
    void set_down(bool pressed);
    void set_left(bool pressed);
    void set_right(bool pressed);

    uint16_t raw();
    void setRaw(uint16_t report);
};
