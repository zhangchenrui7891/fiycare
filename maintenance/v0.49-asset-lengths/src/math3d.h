#pragma once

#include <algorithm>
#include <cmath>

namespace sim {

constexpr double pi = 3.14159265358979323846;

inline double wrapAngle(double angle) {
    return std::remainder(angle, 2.0 * pi);
}

// World/body convention: X right, Y up, Z forward. Units are SI.
struct V3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    V3 operator+(V3 other) const { return {x + other.x, y + other.y, z + other.z}; }
    V3 operator-(V3 other) const { return {x - other.x, y - other.y, z - other.z}; }
    V3 operator-() const { return {-x, -y, -z}; }
    V3 operator*(double scale) const { return {x * scale, y * scale, z * scale}; }
    V3 operator/(double scale) const { return *this * (1.0 / scale); }

    V3& operator+=(V3 other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    double dot(V3 other) const { return x * other.x + y * other.y + z * other.z; }
    double lengthSquared() const { return dot(*this); }
    double length() const { return std::sqrt(lengthSquared()); }

    V3 cross(V3 other) const {
        return {y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x};
    }

    V3 unit() const {
        const double magnitude = length();
        return magnitude > 1e-8 ? *this / magnitude : V3{0, 0, 1};
    }

    V3 normalizedOrZero() const {
        const double magnitude = length();
        return magnitude > 1e-8 ? *this / magnitude : V3{};
    }

    bool finite() const {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
};

struct Quaternion {
    double w = 1.0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Quaternion operator*(const Quaternion& b) const {
        return {w*b.w - x*b.x - y*b.y - z*b.z,
                w*b.x + x*b.w + y*b.z - z*b.y,
                w*b.y - x*b.z + y*b.w + z*b.x,
                w*b.z + x*b.y - y*b.x + z*b.w};
    }

    Quaternion conjugate() const { return {w, -x, -y, -z}; }
    double normSquared() const { return w*w + x*x + y*y + z*z; }

    bool finite() const {
        return std::isfinite(w) && std::isfinite(x) &&
               std::isfinite(y) && std::isfinite(z);
    }

    void normalize() {
        const double n = std::sqrt(normSquared());
        if (!std::isfinite(n) || n < 1e-12) {
            *this = {};
            return;
        }
        w /= n;
        x /= n;
        y /= n;
        z /= n;
    }

    static Quaternion axisAngle(V3 axis, double angle) {
        axis = axis.normalizedOrZero();
        const double half = angle * 0.5;
        const double s = std::sin(half);
        return {std::cos(half), axis.x*s, axis.y*s, axis.z*s};
    }

    static Quaternion fromEuler(double pitch, double roll, double yaw) {
        // Match the existing DirectX visual attitude exactly.
        auto q = axisAngle({0, 1, 0}, yaw) *
                 axisAngle({1, 0, 0}, -pitch) *
                 axisAngle({0, 0, 1}, -roll);
        q.normalize();
        return q;
    }

    V3 rotate(V3 value) const {
        const V3 u{x, y, z};
        const V3 t = u.cross(value) * 2.0;
        return value + t*w + u.cross(t);
    }

    V3 inverseRotate(V3 value) const { return conjugate().rotate(value); }

    // Quaternion exponential: integrate angular velocity expressed in body axes.
    void integrateBodyRate(V3 omega, double dt) {
        const double speed = omega.length();
        if (speed > 1e-10) {
            *this = *this * axisAngle(omega / speed, speed * dt);
            normalize();
        }
    }

    V3 euler() const {
        const V3 forward = rotate({0, 0, 1});
        const V3 right = rotate({1, 0, 0});
        const V3 up = rotate({0, 1, 0});
        return {std::asin(std::clamp(forward.y, -1.0, 1.0)),
                std::atan2(-right.y, up.y),
                std::atan2(forward.x, forward.z)};
    }
};

struct ForceMoment {
    V3 force;
    V3 moment;

    void addAt(V3 appliedForce, V3 positionFromCG) {
        force += appliedForce;
        moment += positionFromCG.cross(appliedForce);
    }

    void add(const ForceMoment& other) {
        force += other.force;
        moment += other.moment;
    }
};

} // namespace sim
