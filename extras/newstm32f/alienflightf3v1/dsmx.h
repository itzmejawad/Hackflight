/*
   dsmx.h Support for Spektrum DSMX receive on F3 boards

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

#include <receiver.hpp>
#include <hackflight.hpp>

class DSMX_Receiver : public hf::Receiver {

    public:

        DSMX_Receiver(const uint8_t channelMap[6], float trimRoll=.01, float trimPitch=0, float trimYaw=0);

        virtual void begin(void) override;

    protected:

        virtual bool gotNewFrame(void) override;

        virtual void readRawvals(void) override;

}; // DSMX_Receiver