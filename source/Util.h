#ifndef UTIL_H
#define UTIL_H

#include <gccore.h>

const double PI = 3.14159265358979323846;

guVector operator+(const guVector& left, const guVector& right);
guVector operator-(const guVector& left, const guVector& right);
guVector operator/(const guVector& left, const float right);

guVector makeGuVector(float x, float y, float z);

int millis();

void sleep_for(int ms);

// Block until A is pressed
void pressA(int wiimote = 0);

/* Nunchuk accelerometer value to radian conversion.   Normalizes the measured value
 * into the range specified by hi-lo.  Most of the time lo should be 0, and hi should be 1023.
 * lo         the lowest possible reading from the Nunchuk
 * hi         the highest possible reading from the Nunchuk
 * measured   the actual measurement */
float angleInRadians(int lo, int hi, long measured);

#endif // UTIL_H
