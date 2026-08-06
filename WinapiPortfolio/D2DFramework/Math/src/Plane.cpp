#include "../include/Plane.h"

#include <cmath>

#include "../include/MathUtil.h"

Plane::Plane(Vector3 normal, float distance)
    :normalVector(normal),  baseNormalVector(normal), d(distance){}

EPlaneSide Plane::CheckSide(const Vector3& point) const
{
    float result = d + point.Dot(normalVector);
    if (result < 0) return EPlaneSide::Inside;
    else if (result > 0) return EPlaneSide::Outside;
    else return EPlaneSide::Intersecting;
}

void Plane::RotatePlane(const float radian)
{
    float c = std::cos(radian);
    float s = std::sin(radian);
    normalVector = Vector3(c * normalVector.x + s * normalVector.z, normalVector.y, -s * normalVector.x + c * normalVector.z);
}

void Plane::RotateFromBaseWithRadian(const float radian)
{
    float c = std::cos(radian);
    float s = std::sin(radian);
    normalVector = Vector3(c * baseNormalVector.x + s * baseNormalVector.z, baseNormalVector.y, -s * baseNormalVector.x + c * baseNormalVector.z);
}

void Plane::RotateFromBaseWithDegree(const float degree)
{
    RotateFromBaseWithRadian(MathUtil::DegToRad(degree));
}
