#pragma once

// https://gist.github.com/fairlight1337/4935ae72bcbcc1ba5c72
// Copyright (c) 2014, Jan Winkler <winkler@cs.uni-bremen.de>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Universitat Bremen nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/* Author: Jan Winkler */

#include <DirectXMath.h>
#include <algorithm>

namespace KGL
{
    inline namespace COLOR
    {
        inline DirectX::XMFLOAT3 ConvertToHSL(const DirectX::XMFLOAT3& rgb)
        {
            const float r = rgb.x, g = rgb.y, b = rgb.z;
            float h, s, v;
            const float rgb_max = (std::max)((std::max)(r, g), b);
            const float rgb_min = (std::min)((std::min)(r, g), b);
            const float delta = rgb_max - rgb_min;

            if (delta > 0)
            {
                // H
                if (rgb_max == r)
                    h = 60.f * fmodf(((g - b) / delta), 6.f);
                else if(rgb_max == g)
                    h = 60.f * (((b - r) / delta) + 2.f);
                else if (rgb_max == b)
                    h = 60.f * (((r - g) / delta) + 4.f);

                // S
                if (rgb_max > 0.f)
                {
                    s = delta / rgb_max;
                }
                else
                {
                    s = 0.f;
                }

                // V
                v = rgb_max;
            }
            else
            {
                h = 0.f;
                s = 0.f;
                v = rgb_max;
            }
            
            if (h < 0)
            {
                h = 360.f + h;
            }

            return { h, s, v };
        }
        inline DirectX::XMFLOAT4 ConvertToHSL(const DirectX::XMFLOAT4& rgb)
        {
            DirectX::XMFLOAT3 result = ConvertToHSL({ rgb.x, rgb.y, rgb.z });
            return { result.x, result.y, result.z, rgb.w };
        }

        inline DirectX::XMFLOAT3 ConvertToRGB(const DirectX::XMFLOAT3& hsv)
        {
            const float h = hsv.x, s = hsv.y, v = hsv.z;
            float r, g, b;

            const float chroma = v * s;
            const float h_prime = fmodf(h / 60.f, 6.f);
            const float x = chroma * (1.f - fabs(fmod(h_prime, 2.f) - 1.f));
            const float m = v - chroma;

            if (0.f <= h_prime && h_prime < 1.f)
            {
                r = chroma;
                g = x;
                b = 0.f;
            }
            else if (1.f <= h_prime && h_prime < 2.f)
            {
                r = x;
                g = chroma;
                b = 0.f;
            }
            else if (2.f <= h_prime && h_prime < 3.f)
            {
                r = 0.f;
                g = chroma;
                b = x;
            }
            else if (3.f <= h_prime && h_prime < 4.f)
            {
                r = 0.f;
                g = x;
                b = chroma;
            }
            else if (4.f <= h_prime && h_prime < 5.f)
            {
                r = x;
                g = 0;
                b = chroma;
            }
            else if (5.f <= h_prime && h_prime < 6.f)
            {
                r = chroma;
                g = 0;
                b = x;
            }
            else
            {
                r = g = b = 0.f;
            }

            r += m;
            g += m;
            b += m;

            return { r, g, b };
        }
        inline DirectX::XMFLOAT4 ConvertToRGB(const DirectX::XMFLOAT4& hsv)
        {
            DirectX::XMFLOAT3 result = ConvertToRGB({ hsv.x, hsv.y, hsv.z });
            return { result.x, result.y, result.z, hsv.w };
        }
    }
}