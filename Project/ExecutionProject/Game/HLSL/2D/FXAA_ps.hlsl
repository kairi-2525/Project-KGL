#include "Sprite.hlsli"

#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 24

#include "FXAA3_11.hlsli"

cbuffer cbFXAA : register(b0)
{
    // 0.333 - ���Ȃ����܂��i�����ɂȂ�܂��j�B
    // 0.250 - ��i��
    // 0.166 - �f�t�H���g
    // 0.125 - ���i�� 
    // 0.063 - �I�[�o�[�L��(�x��)
    float quality_edge_threshold;

    // 0.0833 - ����l�i�f�t�H���g�ł́C�t�B���^�����O����Ă��Ȃ����̃G�b�W�̊J�n�_�j�D
    // 0.0625 - ���i��(����)
    // 0.0312 - �����E�i�x���j�B
    float quality_edge_threshold_min;

    // 0.125�̓G�C���A�V���O�����Ȃ����܂����A���\�t�g�ɂ��܂��i�f�t�H���g!
    // 0.25�͂�葽���̃G�C���A�X���c���A���V���[�v�ɂȂ�܂��B
    float console_edge_threshold;

    // 0.06 - �����Ȃ������A�Â��Ƃ���ł̃G�C���A�V���O�������Ȃ����B
    // 0.05 - �f�t�H���g
    // 0.04 - �Â��Ƃ���ł̃G�C���A�V���O�����Ȃ��A�x���Ȃ�܂����B
    float console_edge_threshold_min;

    // 8.0�̓V���[�v�i�f�t�H���g!
    // 4.0�̓\�t�g�ł��B
    // 2.0 �͖{���Ƀ\�t�g�ł� (�x�N�^�[�O���t�B�b�N�X���͂ɂ̂ݗL��)
    float edge_sharpness;

    // 1.00 - ����i�\�t�g�j�B
    // 0.75 - �f�t�H���g�̃t�B���^�����O��
    // 0.50 - �����i���V���[�v�ŁA�T�u�s�N�Z���̃G�C���A�V���O���������Ȃ��j�B
    // 0.25 - �قڃI�t
    // 0.00 - ���S�ɃI�t
    float subpix;

    // {x_} = 1.0/screenWidthInPixels
    // {_y} = 1.0/screenHeightInPixels
    float2 rcp_frame;

    // N = 0.50 (�f�t�H���g)
    // N = 0.33 (�V���[�v)
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