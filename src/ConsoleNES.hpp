#pragma once

#include "ConsoleNESBase.hpp"

class ConsoleNES : public NESConsoleWriter
{
private:
    uint8_t _report;

public:
    ConsoleNES(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock);
    ~ConsoleNES();

    bool a();
    bool b();
    bool select();
    bool start();
    bool up();
    bool down();
    bool left();
    bool right();

    void set_a(bool pressed);
    void set_b(bool pressed);
    void set_select(bool pressed);
    void set_start(bool pressed);
    void set_up(bool pressed);
    void set_down(bool pressed);
    void set_left(bool pressed);
    void set_right(bool pressed);

    uint8_t raw();
    void setRaw(uint8_t report);
};
