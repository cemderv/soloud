#pragma once

namespace SoLoud
{
struct vec3
{
    float mX = 0.0f;
    float mY = 0.0f;
    float mZ = 0.0f;

    vec3() = default;

    vec3(float x, float y, float z)
        : mX(x)
        , mY(y)
        , mZ(z)
    {
    }

    [[nodiscard]] bool null() const
    {
        if (mX == 0 && mY == 0 && mZ == 0)
            return true;
        return false;
    }

    [[nodiscard]] float mag() const
    {
        return std::sqrt(mX * mX + mY * mY + mZ * mZ);
    }

    [[nodiscard]] float dot(const vec3& a) const
    {
        return mX * a.mX + mY * a.mY + mZ * a.mZ;
    }

    [[nodiscard]] vec3 cross(const vec3& a) const
    {
        return {
            mY * a.mZ - a.mY * mZ,
            mZ * a.mX - a.mZ * mX,
            mX * a.mY - a.mX * mY,
        };
    }
};

static vec3 operator-(const vec3& v)
{
    return {-v.mX, -v.mY, -v.mZ};
}

static vec3 operator-(const vec3& lhs, const vec3 rhs)
{
    return {
        lhs.mX - rhs.mX,
        lhs.mY - rhs.mY,
        lhs.mZ - rhs.mZ,
    };
}

static vec3 normalize(const vec3& v)
{
    const float m = v.mag();
    return m == 0.0f ? vec3{} : vec3{v.mX / m, v.mY / m, v.mZ / m};
}

} // namespace SoLoud