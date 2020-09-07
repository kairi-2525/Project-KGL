#pragma once

#include <DirectXMath.h>
#include "Cereal/cereal.hpp"

#ifdef KGL_USE_NVP_NAME
#define KGL_NVP(name, param) cereal::make_nvp(name, param)
#else
#define KGL_NVP(name, param) param
#endif

namespace DirectX
{
    // XMFLOAT
    template<class Archive>
    void serialize(Archive& archive,
        XMFLOAT2& m)
    {
        archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y));
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMFLOAT3& m)
    {
        archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y), KGL_NVP("z", m.z));
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMFLOAT4& m)
    {
        archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y), KGL_NVP("z", m.z), KGL_NVP("w", m.w));
    }

    // XMINT
    template<class Archive>
    void serialize(Archive& archive,
        XMINT2& m)
    {
        archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y));
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMINT3& m)
    {
        archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y), KGL_NVP("z", m.z));
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMINT4& m)
    {
        archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y), KGL_NVP("z", m.z), KGL_NVP("w", m.w));
    }

    // XMUINT
    template<class Archive>
    void serialize(Archive& archive,
        XMUINT2& m)
    {
        archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y));
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMUINT3& m)
    {
        archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y), KGL_NVP("z", m.z));
    }

    template<class Archive>
    void serialize(Archive& archive,
        XMUINT4& m)
    {
        archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y), KGL_NVP("z", m.z), KGL_NVP("w", m.w));
    }
}