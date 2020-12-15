#pragma once

#include <DirectXMath.h>
#include <Cereal/cereal.hpp>

namespace DirectX
{
    // XMFLOAT
    template<class Archive>
    void serialize(Archive& archive,
        XMFLOAT2& m)
    {
        archive(m.x, m.y);
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMFLOAT3& m)
    {
        archive(m.x, m.y, m.z);
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMFLOAT4& m)
    {
        archive(m.x, m.y, m.z, m.w);
    }

    // XMINT
    template<class Archive>
    void serialize(Archive& archive,
        XMINT2& m)
    {
        archive(m.x, m.y);
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMINT3& m)
    {
        archive(m.x, m.y, m.z);
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMINT4& m)
    {
        archive(m.x, m.y, m.z, m.w);
    }

    // XMUINT
    template<class Archive>
    void serialize(Archive& archive,
        XMUINT2& m)
    {
        archive(m.x, m.y);
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMUINT3& m)
    {
        archive(m.x, m.y, m.z);
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMUINT4& m)
    {
        archive(m.x, m.y, m.z, m.w);
    }
}