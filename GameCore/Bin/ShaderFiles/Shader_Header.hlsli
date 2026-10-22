#ifndef SHADER_HEADER_HLSLI
#define SHADER_HEADER_HLSLI
static const int TextureMaxStack = 6;
static const float PI = 3.14159265359f;

cbuffer PerCameraBuffer : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjMatrix;
    float4x4 ViewInvMatrix;
    float4x4 ProjInvMatrix;
    float4 vCamPosition;
}

cbuffer PerLightBuffer : register(b1)
{
    float4 vLight_Diffuse;
    float4 vLight_Ambient;
    float4 vLight_Specular;
    float4 vLight_Direction;
    float4 vLight_Pos;
    float fLight_Range;
    float fLight_InnerCos;
    float fLight_OuterCos;
    int iShadowIndex;
}

cbuffer PerObjectBuffer : register(b2)
{
    float4x4 WorldMatrix;
    int iMaterialIndex;
    int iAlphaClip;
    float fEmissiveAmount;
    int iRimProfile;
    float fRimAmount;
    float fDissolveAmount;
    uint iBoneOffset;
    uint iBoneCount;
    float4 vDissolveColor;
}

SamplerState g_DefaultSampler : register(s0);
SamplerState g_PointSampler : register(s1);
SamplerState g_LinearClampSampler : register(s2);

#endif
