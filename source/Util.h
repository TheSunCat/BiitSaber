#ifndef UTIL_H
#define UTIL_H

#include <gccore.h>

guVector operator+(const guVector& left, const guVector& right);
guVector operator-(const guVector& left, const guVector& right);
guVector operator/(const guVector& left, const float right);

#endif // UTIL_H