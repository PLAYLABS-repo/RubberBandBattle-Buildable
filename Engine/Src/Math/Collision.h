#pragma once

#include "Math/Vector.h"

namespace Absolut
{

// ============================================================
// 2D AABB
// ============================================================

class Collision2D
{
public:
    Vec2 min;
    Vec2 max;

    Collision2D()
        : min(0.0f, 0.0f),
          max(0.0f, 0.0f)
    {
    }

    Collision2D(const Vec2& minimum, const Vec2& maximum)
        : min(minimum),
          max(maximum)
    {
    }

    static Collision2D FromCenter(
        const Vec2& center,
        const Vec2& halfSize)
    {
        return Collision2D(
            Vec2(
                center.x - halfSize.x,
                center.y - halfSize.y
            ),
            Vec2(
                center.x + halfSize.x,
                center.y + halfSize.y
            )
        );
    }

    bool Intersects(const Collision2D& other) const
    {
        return
            min.x <= other.max.x &&
            max.x >= other.min.x &&
            min.y <= other.max.y &&
            max.y >= other.min.y;
    }

    bool Contains(const Vec2& point) const
    {
        return
            point.x >= min.x &&
            point.x <= max.x &&
            point.y >= min.y &&
            point.y <= max.y;
    }

    Vec2 GetCenter() const
    {
        return Vec2(
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f
        );
    }

    Vec2 GetSize() const
    {
        return Vec2(
            max.x - min.x,
            max.y - min.y
        );
    }

    Vec2 GetHalfSize() const
    {
        return Vec2(
            (max.x - min.x) * 0.5f,
            (max.y - min.y) * 0.5f
        );
    }

    void Translate(const Vec2& offset)
    {
        min.x += offset.x;
        min.y += offset.y;

        max.x += offset.x;
        max.y += offset.y;
    }

    void SetPosition(const Vec2& position)
    {
        Vec2 halfSize = GetHalfSize();

        min.x = position.x - halfSize.x;
        min.y = position.y - halfSize.y;

        max.x = position.x + halfSize.x;
        max.y = position.y + halfSize.y;
    }
};


// ============================================================
// 3D AABB
// ============================================================

class Collision
{
public:
    Vec3 min;
    Vec3 max;

    Collision()
        : min(0.0f, 0.0f, 0.0f),
          max(0.0f, 0.0f, 0.0f)
    {
    }

    Collision(const Vec3& minimum, const Vec3& maximum)
        : min(minimum),
          max(maximum)
    {
    }

    static Collision FromCenter(
        const Vec3& center,
        const Vec3& halfSize)
    {
        return Collision(
            Vec3(
                center.x - halfSize.x,
                center.y - halfSize.y,
                center.z - halfSize.z
            ),
            Vec3(
                center.x + halfSize.x,
                center.y + halfSize.y,
                center.z + halfSize.z
            )
        );
    }

    bool Intersects(const Collision& other) const
    {
        return
            min.x <= other.max.x &&
            max.x >= other.min.x &&

            min.y <= other.max.y &&
            max.y >= other.min.y &&

            min.z <= other.max.z &&
            max.z >= other.min.z;
    }

    bool Contains(const Vec3& point) const
    {
        return
            point.x >= min.x &&
            point.x <= max.x &&

            point.y >= min.y &&
            point.y <= max.y &&

            point.z >= min.z &&
            point.z <= max.z;
    }

    Vec3 GetCenter() const
    {
        return Vec3(
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f
        );
    }

    Vec3 GetSize() const
    {
        return Vec3(
            max.x - min.x,
            max.y - min.y,
            max.z - min.z
        );
    }

    Vec3 GetHalfSize() const
    {
        return Vec3(
            (max.x - min.x) * 0.5f,
            (max.y - min.y) * 0.5f,
            (max.z - min.z) * 0.5f
        );
    }

    void Translate(const Vec3& offset)
    {
        min.x += offset.x;
        min.y += offset.y;
        min.z += offset.z;

        max.x += offset.x;
        max.y += offset.y;
        max.z += offset.z;
    }

    void SetPosition(const Vec3& position)
    {
        Vec3 halfSize = GetHalfSize();

        min.x = position.x - halfSize.x;
        min.y = position.y - halfSize.y;
        min.z = position.z - halfSize.z;

        max.x = position.x + halfSize.x;
        max.y = position.y + halfSize.y;
        max.z = position.z + halfSize.z;
    }
};

}
