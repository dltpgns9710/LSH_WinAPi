#pragma once
#include "Vector3.h"

enum class PlaneSide
{
    Inside,      //내부
    Outside,     //외부
    Intersecting //평면에 포함
};

struct Plane
{
    Plane(Vector3 normal, float distance);
    
    Vector3 normalVector;
    float d;
    
    PlaneSide CheckSide(const Vector3& point) const;
};
