/*
   hackflight.hpp : general header, plus init and update methods

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Hackflight is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MEReceiverHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with Hackflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "config.hpp"
#include "board.hpp"
#include "mixer.hpp"
#include "model.hpp"
#include "msp.hpp"
#include "receiver.hpp"
#include "stabilize.hpp"
#include "altitude.hpp"
#include "timedtask.hpp"
#include "model.hpp"

// For logical combinations of stick positions (low, center, high)
#define ROL_LO (1 << (2 * DEMAND_ROLL))
#define ROL_CE (3 << (2 * DEMAND_ROLL))
#define ROL_HI (2 << (2 * DEMAND_ROLL))
#define PIT_LO (1 << (2 * DEMAND_PITCH))
#define PIT_CE (3 << (2 * DEMAND_PITCH))
#define PIT_HI (2 << (2 * DEMAND_PITCH))
#define YAW_LO (1 << (2 * DEMAND_YAW))
#define YAW_CE (3 << (2 * DEMAND_YAW))
#define YAW_HI (2 << (2 * DEMAND_YAW))
#define THR_LO (1 << (2 * DEMAND_THROTTLE))
#define THR_CE (3 << (2 * DEMAND_THROTTLE))
#define THR_HI (2 << (2 * DEMAND_THROTTLE))

namespace hf {

class Hackflight {

    public:

        void init(Board * _board, Receiver *_receiver, Model * _model);
        void update(void);

    private:

        void blinkLedForTilt(void);
        void flashLeds(const InitConfig& config);
        void updateRc(void);
        void updateImu(void);
        void updateReadyState(void);

        Mixer      mixer;
        MSP        msp;
        Stabilize  stab;
        Altitude   alti;

        Board    * board;
        Receiver * receiver;

        TimedTask imuTask;
        TimedTask rcTask;
        TimedTask angleCheckTask;
        TimedTask altitudeTask;

        bool     armed;
        uint8_t  auxState;
        float    eulerAnglesDegrees[3];
        bool     safeToArm;
        uint16_t maxArmingAngle;
};

/********************************************* CPP ********************************************************/

void Hackflight::init(Board * _board, Receiver * _receiver, Model * _model)
{  
    board = _board;
    receiver = _receiver;

    // Do hardware initialization for board
    board->init();

    // Modify default board configuration as needed
    Config config;
    board->modifyConfig(config);

    // Flash the LEDs to indicate startup
    flashLeds(config.init);

    // Get particulars for board
    LoopConfig loopConfig = config.loop;
    ImuConfig imuConfig = config.imu;

    // Store some for later
    maxArmingAngle = imuConfig.maxArmingAngle;

    // Sleep  a bit to allow IMU to catch up
    board->delayMilliseconds(config.init.delayMilli);

    // Initialize timing tasks
    imuTask.init(loopConfig.imuLoopMicro);
    rcTask.init(loopConfig.rcLoopMilli * 1000);
    angleCheckTask.init(loopConfig.angleCheckMilli * 1000);
    altitudeTask.init(loopConfig.altHoldLoopMilli * 1000);

    // Initialize the receiver
    receiver->init(config.rc, config.pwm);

    // Initialize our stabilization, mixing, and MSP (serial comms)
    stab.init(config.stabilize, config.imu, _model);
    mixer.init(config.pwm, receiver, &stab, board); 
    msp.init(&mixer, receiver, board);

    // Initialize altitude estimator, which will be used if there's a barometer
    alti.init(config.altitude, board, _model);

    // Initialize state varriables
    for (int k=0; k<3; ++k) {
        eulerAnglesDegrees[k] = 0;
    }
    
    // Start in unarmed mode, except for simulator
    armed = board->skipArming();
    safeToArm = board->skipArming();

} // init

void Hackflight::update(void)
{
    // Grab current time for various loops
    uint32_t currentTime = (uint32_t)board->getMicros();

    // Outer (slow) loop: update Receiver
    if (rcTask.checkAndUpdate(currentTime)) {
        updateRc();
    }

    // Altithude-PID task (never called in same loop iteration as Receiver update)
    else if (board->extrasHaveBaro() && altitudeTask.checkAndUpdate(currentTime)) {
        alti.computePid(armed);
    }

    // Polling for EM7180 SENtral Sensor Fusion IMU
    board->extrasImuPoll();

    // Inner (fast) loop: update IMU
    if (imuTask.checkAndUpdate(currentTime)) {
        updateImu();
    }

} // update

void Hackflight::updateRc(void)
{
    // Update Receiver channels
    receiver->update();

   // When landed, reset integral component of PID
    if (receiver->throttleIsDown()) {
        stab.resetIntegral();
    }

    // Certain actions (arming, disarming) need checking every time
    if (receiver->changed()) {

        // actions during armed
        if (armed) {      

            // Disarm on throttle down + yaw
            if (receiver->sticks == THR_LO + YAW_LO + PIT_CE + ROL_CE) {
                if (armed) {
                    armed = board->skipArming();
                }
            }

        // Actions during not armed
        } else {         

            // Arm via throttle-low / yaw-right
            if (receiver->sticks == THR_LO + YAW_HI + PIT_CE + ROL_CE) {
                if (safeToArm) {
                    auxState = receiver->getAuxState();
                    if (!auxState) // aux switch must be in zero position
                        if (!armed) {
                            armed = true;
                        }
                }
            }

        } // not armed

    } // receiver->changed()

    // Detect aux switch changes for altitude-hold
    if (receiver->getAuxState() != auxState) {
        auxState = receiver->getAuxState();
        if (board->extrasHaveBaro()) {
            if (auxState > 0) {
                alti.start(receiver->command[DEMAND_THROTTLE]);
            }
            else {
                alti.stop();
            }
        }
    }
}

void Hackflight::updateImu(void)
{
    // Compute exponential Receiver commands
    receiver->computeExpo();

    // Get Euler angles from board
    float eulerAnglesRadians[3];
    board->imuGetEuler(eulerAnglesRadians);

    // Convert angles from radians to degrees
    for (int k=0; k<3; ++k) {
        eulerAnglesDegrees[k]  = eulerAnglesRadians[k]  * 180.0f / (float)M_PI;
    }

    // Convert heading from [-180,+180] to [0,360]
    if (eulerAnglesDegrees[AXIS_YAW] < 0) {
        eulerAnglesDegrees[AXIS_YAW] += 360;
    }

    // Update status using Euler angles
    updateReadyState();

    // If barometer avaialble, update accelerometer for altitude fusion, then modify throttle demand
    if (board->extrasHaveBaro()) {
        alti.updateAccelerometer(eulerAnglesRadians, armed);
        alti.modifyThrottleDemand(receiver->command[DEMAND_THROTTLE]);
    }

    // Get raw gyro values from board
    int16_t gyroRaw[3];
    board->imuGetGyro(gyroRaw);

    // Stabilization, mixing, and MSP are synced to IMU update.  Stabilizer also uses raw gyro values.
    stab.update(receiver->command, gyroRaw, eulerAnglesDegrees);
    mixer.update(armed);
    msp.update(eulerAnglesDegrees, armed);
} 

void Hackflight::updateReadyState(void)
{
    if (safeToArm)
        board->ledSet(0, false);
    if (armed)
        board->ledSet(1, true);
    else
        board->ledSet(1, false);

    // If angle too steep, flash LED
    uint32_t currentTime = (uint32_t)board->getMicros();
    if (angleCheckTask.ready(currentTime)) {
        if (!(abs(eulerAnglesDegrees[0]) < maxArmingAngle && abs(eulerAnglesDegrees[1]) < maxArmingAngle)) {
            safeToArm = false; 
            blinkLedForTilt();
            angleCheckTask.update(currentTime);
        } else {
            safeToArm = true;
        }
    }
}

void Hackflight::blinkLedForTilt(void)
{
    static bool on;

    if (on) {
        board->ledSet(0, false);
        on = false;
    }
    else {
        board->ledSet(0, true);
        on = true;
    }
}

void Hackflight::flashLeds(const InitConfig& config)
{
    uint32_t pauseMilli = config.ledFlashMilli / config.ledFlashCount;
    board->ledSet(0, false);
    board->ledSet(1, false);
    for (uint8_t i = 0; i < config.ledFlashCount; i++) {
        board->ledSet(0, true);
        board->ledSet(1, false);
        board->delayMilliseconds(pauseMilli);
        board->ledSet(0, false);
        board->ledSet(1, true);
        board->delayMilliseconds(pauseMilli);
    }
    board->ledSet(0, false);
    board->ledSet(1, false);
}

} // namespace
