#pragma once

#include <numbers>

class MathUtil
{
public:
    MathUtil() = delete;

    static constexpr double pi = std::numbers::pi;
    
    template<typename T>
    static constexpr T Lerp(T a, T b, float t) noexcept
    {
        return a + (b - a) * t;
    }
    
    template<typename T>
    static constexpr T Clamp(T value, T min, T max) noexcept
    {
        return value < min ? min : (value > max ? max : value);
    }
    
    static constexpr float LerpClamped(float a, float b, float t, float min = 0.0f, float max = 1.0f) noexcept
    {
        return Lerp(a, b, Clamp(t, min, max));
    }
    
    static constexpr float DegToRad(float deg) noexcept
    {
        return deg * pi / 180.0f;
    }

    static constexpr float RadToDeg(float rad) noexcept
    {
        return rad * 180.0f / pi;
    }
};
