#include "Sprite.hlsli"

#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 24

#include "FXAA3_11.hlsli"

cbuffer cbFXAA : register(b0)
{
    // 0.333 - 少なすぎます（高速になります）。
    // 0.250 - 低品質
    // 0.166 - デフォルト
    // 0.125 - 高品質 
    // 0.063 - オーバーキル(遅い)
    float quality_edge_threshold;

    // 0.0833 - 上限値（デフォルトでは，フィルタリングされていない可視のエッジの開始点）．
    // 0.0625 - 高品質(高速)
    // 0.0312 - 可視限界（遅い）。
    float quality_edge_threshold_min;

    // 0.125はエイリアシングを少なくしますが、よりソフトにします（デフォルト!
    // 0.25はより多くのエイリアスを残し、よりシャープになります。
    float console_edge_threshold;

    // 0.06 - 速くなったが、暗いところでのエイリアシングが多くなった。
    // 0.05 - デフォルト
    // 0.04 - 暗いところでのエイリアシングが少なく、遅くなりました。
    float console_edge_threshold_min;

    // 8.0はシャープ（デフォルト!
    // 4.0はソフトです。
    // 2.0 は本当にソフトです (ベクターグラフィックス入力にのみ有効)
    float edge_sharpness;

    // 1.00 - 上限（ソフト）。
    // 0.75 - デフォルトのフィルタリング量
    // 0.50 - 下限（よりシャープで、サブピクセルのエイリアシング除去が少ない）。
    // 0.25 - ほぼオフ
    // 0.00 - 完全にオフ
    float subpix;

    // {x_} = 1.0/screenWidthInPixels
    // {_y} = 1.0/screenHeightInPixels
    float2 rcp_frame;

    // N = 0.50 (デフォルト)
    // N = 0.33 (シャープ)
    // {x___} = -N / screenWidthInPixels
    // {_y__} = -N / screenHeightInPixels
    // {__z_} = N / screenWidthInPixels
    // {___w} = N / screenHeightInPixels
    float4 rcp_frame_opt;

    // {x___} = -2.0 /screenWidthInPixels  
    // {_y__} = -2.0 / screenHeightInPixels
    // {__z_} = 2.0 / screenWidthInPixels  
    // {___w} = 2.0 / screenHeightInPixels 
    float4 rcp_frame_opt2;
}

Texture2D<float4> tex : register(t0);
SamplerState smp : register (s0);

float4 PSMain(PSInput input) : SV_TARGET
{
    FxaaTex fxaa_tex = { smp, tex };
    return FxaaPixelShader(
        input.uv,                               // FxaaFloat2 pos,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),     // FxaaFloat4 fxaaConsolePosPos,
        fxaa_tex,                               // FxaaTex tex,
        fxaa_tex,                               // FxaaTex fxaaConsole360TexExpBiasNegOne,
        fxaa_tex,                               // FxaaTex fxaaConsole360TexExpBiasNegTwo,
        rcp_frame,                              // FxaaFloat2 fxaaQualityRcpFrame,
        rcp_frame_opt,                          // FxaaFloat4 fxaaConsoleRcpFrameOpt,
        rcp_frame_opt2,                         // FxaaFloat4 fxaaConsoleRcpFrameOpt2,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),     // FxaaFloat4 fxaaConsole360RcpFrameOpt2,
        subpix,                                 // FxaaFloat fxaaQualitySubpix,
        quality_edge_threshold,                 // FxaaFloat fxaaQualityEdgeThreshold,
        quality_edge_threshold_min,             // FxaaFloat fxaaQualityEdgeThresholdMin,
        edge_sharpness,                         // FxaaFloat fxaaConsoleEdgeSharpness,
        console_edge_threshold,                 // FxaaFloat fxaaConsoleEdgeThreshold,
        console_edge_threshold_min,             // FxaaFloat fxaaConsoleEdgeThresholdMin,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f)      // FxaaFloat fxaaConsole360ConstDir,
    );
}

float4 AlphaGray(PSInput input) : SV_TARGET
{
    const float3 Convert_YUV = { 0.299f, 0.578f, 0.114f };
    float4 col = tex.Sample(smp, input.uv);
    float Y = dot(col.rgb, Convert_YUV);
    return float4(col.rgb, Y);
}