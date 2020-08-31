#pragma once

#include <DirectXMath.h>
#include "Cereal/cereal.hpp"

// XMFLOAT
template<class Archive>
void serialize(Archive& archive,
    DirectX::XMFLOAT2& m)
{
    archive(m.x, m.y);
}

template<class Archive>
void serialize(Archive& archive,
    DirectX::XMFLOAT3& m)
{
    archive(m.x, m.y, m.z);
}

template<class Archive>
void serialize(Archive& archive,
    DirectX::XMFLOAT4& m)
{
    archive(m.x, m.y, m.z, m.w);
}

// XMINT
template<class Archive>
void serialize(Archive& archive,
    DirectX::XMINT2& m)
{
    archive(m.x, m.y);
}

template<class Archive>
void serialize(Archive& archive,
    DirectX::XMINT3& m)
{
    archive(m.x, m.y, m.z);
}

template<class Archive>
void serialize(Archive& archive,
    DirectX::XMINT4& m)
{
    archive(m.x, m.y, m.z, m.w);
}

// XMUINT
template<class Archive>
void serialize(Archive& archive,
    DirectX::XMUINT2& m)
{
    archive(m.x, m.y);
}

template<class Archive>
void serialize(Archive& archive,
    DirectX::XMUINT3& m)
{
    archive(m.x, m.y, m.z);
}

template<class Archive>
void serialize(Archive& archive,
    DirectX::XMUINT4& m)
{
    archive(m.x, m.y, m.z, m.w);
}