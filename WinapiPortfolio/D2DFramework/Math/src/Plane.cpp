#include "../include/Plane.h"

#include <cmath>

Plane::Plane(Vector3 normal, float distance)
    :normalVector(normal), d(distance){}

PlaneSide Plane::CheckSide(const Vector3& point) const
{
    float result = d + point.Dot(normalVector);
    if (result < 0) return PlaneSide::Inside;
    else if (result > 0) return PlaneSide::Outside;
    else return PlaneSide::Intersecting;
}

void Plane::RotatePlane(const float radian)
{
    float c = std::cos(radian);
    float s = std::sin(radian);
    normalVector = Vector3(c * normalVector.x + s * normalVector.z, normalVector.y, -s * normalVector.x + c * normalVector.z);
}
