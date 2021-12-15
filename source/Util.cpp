#include "Util.h"

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
