#include "Util.h"
#include <chrono>

guVector operator+(const guVector& left, const guVector& right)
{
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

guVector operator-(const guVector& left, const guVector& right)
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

guVector operator/(const guVector& left, const float right)
{
    return {left.x / right, left.y / right, left.z / right};
}

guVector makeGuVector(float x, float y, float z)
{
    return {x, y, z};
}

int millis()
{
    auto time = std::chrono::system_clock::now();
    auto since_epoch = time.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);

    return millis.count();
}
