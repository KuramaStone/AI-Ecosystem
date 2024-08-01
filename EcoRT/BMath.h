#pragma once

#define _USE_MATH_DEFINES
#include <box2d/box2d.h>
#include <math.h>
#include <cmath>
#include <algorithm> 

class MyMath {
public:
    static float cosLookup[100];
    static float sinLookup[100];

    static void init();

    static float distance(b2Vec2& vec1, b2Vec2& vec2);
    static float normalizeAngle(float angle);
    static float pointToLineDistance(b2Vec2& lineStart, b2Vec2& lineEnd, b2Vec2& point);
    static float approxCos(float x);
    static float approxSin(float x);
    static float approxAtan2(float y, float x);
    static float approxAtan(float x);
    static float smoothstep(float edge0, float edge1, float x);
    static float cosine_interp(float a, float b, float t);
    static b2Vec3 rotateAroundAxis(const b2Vec3& point, const b2Vec3& axis, float angleRadians, const b2Vec3& center);
    static b2Vec3 equatorialToCartesian(float rightAscension, float declination);
    static float radians(float degrees);
    static float cubicInterpolation(float a, float b, float step);
};