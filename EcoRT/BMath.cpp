#include "BMath.h"

float MyMath::cosLookup[100] = {}; // initialize array
float MyMath::sinLookup[100] = {}; // initialize array
void MyMath::init() {
    float size = static_cast<int>(std::size(cosLookup));
    for (int i = 0; i < size; i++) {
        float angle = (static_cast<float>(i) / size) * M_PI * 2 - M_PI;
        cosLookup[i] = std::cos(angle); 
        sinLookup[i] = std::sin(angle);
    }

    //float error = 0;
    //BRandom rndEngine;
    //for (int i = 0; i < 10000; i++) {
    //    float x = (rndEngine.randomFloat() * 2 - 1) * 100;
    //    float y = (rndEngine.randomFloat() * 2 - 1) * 100;

    //    float acc = atan2(y, x);
    //    float app = approxAtan2(y, x);
    //    error += abs(acc - app);
    //}
    //std::cout << "Atan2 approx error: " << error << std::endl;

}

float MyMath::cubicInterpolation(float b, float a, float step) {
    // Ensure that step is in the range [0, 1]
    step = std::max(0.0f, std::min(1.0f, step));

    // Cubic interpolation formula
    float t = step;
    float tt = t * t;
    float ttt = tt * t;

    // Coefficients for the cubic polynomial
    float c0 = 2 * ttt - 3 * tt + 1;
    float c1 = ttt - 2 * tt + t;
    float c2 = -2 * ttt + 3 * tt;
    float c3 = ttt - tt;

    // Perform the interpolation
    float result = a * c0 + b * c1 + a * c2 + b * c3;

    //result = a * (1 - step) + b * step;

    return result;
}

float MyMath::distance(b2Vec2& vec1, b2Vec2& vec2) {
    return (vec1 - vec2).Length();
}

float MyMath::normalizeAngle(float angle) {
    if (!isfinite(angle)) {
        return 0;
    }

    while (angle > M_PI) {
        angle -= 2 * M_PI;
    }

    while (angle < -M_PI) {
        angle += 2 * M_PI;
    }

    return angle;
}

float MyMath::approxAtan(float x) {
    float a1 = 0.99997726f;
    float a3 = -0.33262347f;
    float a5 = 0.19354346f;
    float a7 = -0.11643287f;
    float a9 = 0.05265332f;
    float a11 = -0.01172120f;

    float x_sq = x * x;
    return
        x * (a1 + x_sq * (a3 + x_sq * (a5 + x_sq * (a7 + x_sq * (a9 + x_sq * a11)))));
}

float MyMath::approxAtan2(float y, float x) {
    float pi = M_PI;
    float pi_2 = M_PI_2;

    // Ensure input is in [-1, +1]
    bool swap = fabs(x) < fabs(y);
    float atan_input = (swap ? x : y) / (swap ? y : x);

    // Approximate atan
    float res = approxAtan(atan_input);

    // If swapped, adjust atan output
    res = swap ? (atan_input >= 0.0f ? pi_2 : -pi_2) - res : res;
    // Adjust the result depending on the input quadrant
    if (x < 0.0f) {
        res = (y >= 0.0f ? pi : -pi) + res;
    }

    // return result
    return res;
}

float MyMath::approxCos(float x) {
    float sizeHalf = std::size(cosLookup) / 2.0f;
    int index = int((x / M_PI) * sizeHalf + sizeHalf);
    return cosLookup[index];
}

float MyMath::approxSin(float x) {
    float sizeHalf = std::size(sinLookup) / 2.0f;
    int index = int((x / M_PI) * sizeHalf + sizeHalf);
    return sinLookup[index];
}

// Function to calculate distance between a point and a line formed by two points
float MyMath::pointToLineDistance(b2Vec2& lineStart, b2Vec2& lineEnd, b2Vec2& point) {
    float A = point.x - lineStart.x;
    float B = point.y - lineStart.y;
    float C = lineEnd.x - lineStart.x;
    float D = lineEnd.y - lineStart.y;

    float dot = A * C + B * D;
    float len_sq = C * C + D * D;
    float param = dot / len_sq;

    float xx, yy;

    if (param < 0) {
        xx = lineStart.x;
        yy = lineStart.y;
    }
    else if (param > 1) {
        xx = lineEnd.x;
        yy = lineEnd.y;
    }
    else {
        xx = lineStart.x + param * C;
        yy = lineStart.y + param * D;
    }

    float dx = point.x - xx;
    float dy = point.y - yy;
    return std::sqrt(dx * dx + dy * dy);
}

float MyMath::smoothstep(float edge0, float edge1, float x) {
    // Scale, and clamp x to 0..1 range
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    // Evaluate polynomial
    return x * x * (3.0f - 2.0f * x);
}

float MyMath::cosine_interp(float a, float b, float t) {
    float cos_t = (1 - std::cos(t * M_PI)) * 0.5f;
    return a + cos_t * (b - a);
}

b2Vec3 MyMath::equatorialToCartesian(float rightAscension, float declination) {
    float cosDeclination = cos(declination);

    float x = cosDeclination * cos(rightAscension);
    float y = cosDeclination * sin(rightAscension);
    float z = sin(declination);

    return b2Vec3(x, y, z);
}

float MyMath::radians(float degrees) {
    return normalizeAngle(degrees * M_PI / 180);
}

b2Vec3 MyMath::rotateAroundAxis(const b2Vec3& point, const b2Vec3& axis, float angleRadians, const b2Vec3& center) {
    // Translate the point and center to the origin
    b2Vec3 translatedPoint = point - center;

    // Construct the rotation matrix
    float cosAngle = cos(angleRadians);
    float sinAngle = sin(angleRadians);

    float ux = axis.x;
    float uy = axis.y;
    float uz = axis.z;

    float rotationMatrix[9] = {
        cosAngle + ux * ux * (1 - cosAngle),
        ux * uy * (1 - cosAngle) - uz * sinAngle,
        ux * uz * (1 - cosAngle) + uy * sinAngle,
        uy * ux * (1 - cosAngle) + uz * sinAngle,
        cosAngle + uy * uy * (1 - cosAngle),
        uy * uz * (1 - cosAngle) - ux * sinAngle,
        uz * ux * (1 - cosAngle) - uy * sinAngle,
        uz * uy * (1 - cosAngle) + ux * sinAngle,
        cosAngle + uz * uz * (1 - cosAngle)
    };

    // Apply the rotation matrix to the translated point
    b2Vec3 rotatedPoint;
    rotatedPoint.x = rotationMatrix[0] * translatedPoint.x + rotationMatrix[1] * translatedPoint.y + rotationMatrix[2] * translatedPoint.z;
    rotatedPoint.y = rotationMatrix[3] * translatedPoint.x + rotationMatrix[4] * translatedPoint.y + rotationMatrix[5] * translatedPoint.z;
    rotatedPoint.z = rotationMatrix[6] * translatedPoint.x + rotationMatrix[7] * translatedPoint.y + rotationMatrix[8] * translatedPoint.z;

    // Translate the rotated point back to its original position
    return rotatedPoint + center;
}