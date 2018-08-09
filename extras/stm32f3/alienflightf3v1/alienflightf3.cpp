/*
   alienflightf3.cpp Board class implementation for AlienflightF3

   Copyright (c) 2018 Simon D. Levy

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Hackflight is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with Hackflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "alienflightf3.h"

// This code has to talk to the C code supporing USB Virtual COM Port ------------------------------------------

extern "C" {

#include "platform.h"
#include "dma.h"
#include "gpio.h"
#include "serial.h"
#include "system.h"
#include "serial_uart.h"
#include "serial_usb_vcp.h"

    static serialPort_t * serial0;

    AlienflightF3::AlienflightF3(void)
    {
        serial0 = usbVcpOpen();

        init();
    }

    uint8_t AlienflightF3::serialAvailableBytes(void)
    {
        return usbVcpAvailable(serial0);
    }

    uint8_t AlienflightF3::serialReadByte(void)
    {
        return usbVcpRead(serial0);
    }

    void AlienflightF3::serialWriteByte(uint8_t c)
    {
        usbVcpWrite(serial0, c);
        while (!isSerialTransmitBufferEmpty(serial0));
    }

    void F3Board::outchar(char c)
    {
        usbVcpWrite(serial0, c);
    }

    USART_TypeDef * getDsmUart()
    {
        return USART2;
    }

} // extern "C"

// This code is ordinary C++ -----------------------------------------------------------------------------------

#include <f3board.h>
#include <hackflight.hpp>

bool AlienflightF3::getGyrometer(float gyroRates[3])
{
    (void)gyroRates;

    int16_t gyroCount[3];
    int16_t accelCount[3];

    if (getImu(gyroCount, accelCount)) {

        return true;
    }

    return false;
}

bool AlienflightF3::getQuaternion(float quat[4])
{
    (void)quat; // XXX

    static uint32_t _time;
    uint32_t time = micros();
    if (time-_time > 10000) {
        _time = time;
        return true;
    }
    return false;
}

void AlienflightF3::writeMotor(uint8_t index, float value)
{
    (void)index; // XXX
    (void)value;
}

