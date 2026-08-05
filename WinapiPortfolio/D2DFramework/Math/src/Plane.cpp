#include "../include/Plane.h"

Plane::Plane(Vector3 normal, float distance)
    :normalVector(normal), d(distance){}

PlaneSide Plane::CheckSide(const Vector3& point) const
{
    float result = d + point.Dot(normalVector);
    if (result < 0) return PlaneSide::Inside;
    else if (result > 0) return PlaneSide::Outside;
    else return PlaneSide::Intersecting;
}
