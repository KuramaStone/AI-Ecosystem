#include "SmoothNoise.h"

SmoothNoise::SmoothNoise() {
    seed = time(NULL);
}

void SmoothNoise::setSeed(int newseed) {
    seed = newseed;
}

float SmoothNoise::dot(const b2Vec3& a, const b2Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

b2Vec3 SmoothNoise::floor(const b2Vec3& a) {
    return b2Vec3(floorf(a.x), floorf(a.y), floorf(a.z));
}

float SmoothNoise::hash(b2Vec3 p3) {

    // This is from https://www.shadertoy.com/view/4djSRW
    p3 = b2Vec3(fmod(p3.x * 0.1031, 1.0), fmod(p3.y * 0.1030, 1.0), fmod(p3.z * 0.0973, 1.0));
    p3 = b2Vec3(fmod(p3.x + dot(p3, b2Vec3(p3.y + 33.33, p3.z + 33.33, p3.x + 33.33)), 1.0),
        fmod(p3.y + dot(p3, b2Vec3(p3.z + 33.33, p3.x + 33.33, p3.y + 33.33)), 1.0),
        fmod(p3.z + dot(p3, b2Vec3(p3.x + 33.33, p3.y + 33.33, p3.z + 33.33)), 1.0));

    // Ensure intermediate values are positive
    p3 = b2Vec3((p3.x + 1.0) / 2.0, (p3.y + 1.0) / 2.0, (p3.z + 1.0) / 2.0);

    return fmod(fmod(p3.x + p3.y, p3.z) + fmod(p3.x + p3.z, p3.y) + fmod(p3.y + p3.z, p3.x), 1.0);
}

float SmoothNoise::mix(float x, float y, float a) {
    return x * (1.0 - a) + y * a;
}

float SmoothNoise::smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0 - 2.0 * t);
}

float SmoothNoise::fract(float x) {
    return x - std::floor(x);
}

float SmoothNoise::achnoise(b2Vec3 x) {
    b2Vec3 p = floor(x);
    b2Vec3 fr = b2Vec3(smoothstep(0.0, 1.0, fract(x.x)), smoothstep(0.0, 1.0, fract(x.y)), smoothstep(0.0, 1.0, fract(x.z)));
    b2Vec3 LBZ = p + b2Vec3(0.0, 0.0, 0.0);
    b2Vec3 LTZ = p + b2Vec3(0.0, 1.0, 0.0);
    b2Vec3 RBZ = p + b2Vec3(1.0, 0.0, 0.0);
    b2Vec3 RTZ = p + b2Vec3(1.0, 1.0, 0.0);
    b2Vec3 LBF = p + b2Vec3(0.0, 0.0, 1.0);
    b2Vec3 LTF = p + b2Vec3(0.0, 1.0, 1.0);
    b2Vec3 RBF = p + b2Vec3(1.0, 0.0, 1.0);
    b2Vec3 RTF = p + b2Vec3(1.0, 1.0, 1.0);

    float l0candidate1 = hash(LBZ);
    float l0candidate2 = hash(RBZ);
    float l0candidate3 = hash(LTZ);
    float l0candidate4 = hash(RTZ);

    float l0candidate5 = hash(LBF);
    float l0candidate6 = hash(RBF);
    float l0candidate7 = hash(LTF);
    float l0candidate8 = hash(RTF);

    float l1candidate1 = mix(l0candidate1, l0candidate2, fr.x);
    float l1candidate2 = mix(l0candidate3, l0candidate4, fr.x);
    float l1candidate3 = mix(l0candidate5, l0candidate6, fr.x);
    float l1candidate4 = mix(l0candidate7, l0candidate8, fr.x);

    float l2candidate1 = mix(l1candidate1, l1candidate2, fr.y);
    float l2candidate2 = mix(l1candidate3, l1candidate4, fr.y);

    float l3candidate1 = mix(l2candidate1, l2candidate2, fr.z);

    return l3candidate1;
}

float SmoothNoise::snoise(b2Vec3 p) {
    float a = achnoise(p);
    float b = achnoise(b2Vec3(p.x + 120.5, p.y + 120.5, p.z + 120.5));
    return (a + b) * 0.5;
}

float SmoothNoise::getSmoothNoiseAt(float x, float y, float z, int layers, float periodExpo, float persistance) {
    b2Vec3 pos = b2Vec3(x, y, z);
    float total = 0;
    float frequency = 1;
    float amplitude = 1;
    float maxValue = 0;

    for (int i = 0; i < layers; i++) {
        total += snoise(b2Vec3(30000 + pos.x * frequency, 30000 + pos.y * frequency, 30000 + pos.z * frequency)) * amplitude;
        maxValue += amplitude;
        amplitude *= persistance;
        frequency *= periodExpo;
    }

    return (total / maxValue) * 2 - 1;
}
