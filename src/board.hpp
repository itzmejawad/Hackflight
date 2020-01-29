/*
   Class header for board-specific routines

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

#pragma once

#include <stdarg.h>
#include <stdint.h>

namespace hf {

    class Board {

        friend class HackflightBase;
        friend class Hackflight;
        friend class Debugger;
        friend class TimerTask;
        friend class SerialTask;
        friend class PidTask;

        protected:

            //------------------------------------ Core functionality ----------------------------------------------------
            virtual float getTime(void) = 0;

            //------------------------------- Serial communications via MSP ----------------------------------------------
            virtual uint8_t serialAvailableBytes(void) { return 0; }
            virtual uint8_t serialReadByte(void)  { return 1; }
            virtual void    serialWriteByte(uint8_t c) { (void)c; }

            // --------------------------- Adjust IMU readings based on IMU mounting ------------------------------------
            virtual void adjustGyrometer(float & gx, float & gy, float & gz) { (void)gx; (void)gy; (void)gz; }
            virtual void adjustQuaternion(float & qw, float & qx, float & qy, float & qz) { (void)qw; (void)qx; (void)qy; (void)qz; }
            virtual void adjustRollAndPitch(float & roll, float & pitch) { (void)roll; (void)pitch; }

            //------------------------------- Reboot for non-Arduino boards ---------------------------------------------
            virtual void reboot(void) { }

            //----------------------------------------- Safety -----------------------------------------------------------
            virtual void showArmedStatus(bool armed) { (void)armed; }
            virtual void flashLed(bool shouldflash) { (void)shouldflash; }

            //--------------------------------------- Debugging ----------------------------------------------------------
            static  void outbuf(char * buf);

    }; // class Board

} // namespace
