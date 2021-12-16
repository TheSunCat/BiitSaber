#include "Wiimote.h"
#include <wiiuse/wpad.h>

#include "Util.h"

Wiimote::Wiimote(int channel) : chan(channel)
{
    // get a first reading jic
    receiveData();

    initGyroKalman(&rollData,Q_angle,Q_gyro,R_angle);
    initGyroKalman(&pitchData,Q_angle,Q_gyro,R_angle);
    initGyroKalman(&yawData,Q_angle,Q_gyro,R_angle);

    yawRK.val_i_3 = 0;  yawRK.val_i_2 = 0;  yawRK.val_i_1 = 0;  yawRK.previous = 0;
    lastread = millis();
}


void Wiimote::calibrateZeroes()
{
    long y0 = 0, p0 = 0, r0 = 0;
    long xa0 = 0, ya0 = 0, za0 = 0;
    int avg = 300;

    for (int i=0; i < avg; i++) {

        y0 += yaw;
        r0 += roll;
        p0 += pitch;

        xa0 += ax_m;
        ya0 += ay_m;
        za0 += az_m;
    }

    yaw0 = y0 / (avg/2); roll0 = r0 / (avg/2); pitch0 = p0 / (avg/2);
    xAcc0 = xa0 / (avg/2); yAcc0 = ya0 / (avg/2); zAcc0 = za0 / (avg/2);
}

void Wiimote::initGyroKalman(struct GyroKalman *kalman, const float Q_angle, const float Q_gyro, const float R_angle)
{
    kalman->Q_angle = Q_angle; kalman->Q_gyro  = Q_gyro;  kalman->R_angle = R_angle;
    kalman->P_00 = 0; kalman->P_01 = 0; kalman->P_10 = 0; kalman->P_11 = 0;
}


void Wiimote::receiveData() {
    WPADData* wd = WPAD_Data(chan);

    // MotionPlus
    yaw =   wd->exp.mp.ry;
    roll =  wd->exp.mp.rz;
    pitch = wd->exp.mp.rx;

    /*slowPitch = data[3] & 1;
    slowYaw = data[3] & 2;
    slowRoll = data[4] & 2;*/

    // This function processes the byte array from the wii nunchuck.
    ax_m = wd->accel.x;
    ay_m = wd->accel.y;
    az_m = wd->accel.z;
}
