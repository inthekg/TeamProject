#ifndef SHADER_SKINNING_HLSLI
#define SHADER_SKINNING_HLSLI
#include "Shader_Header.hlsli"

StructuredBuffer<float4x4> g_BoneMatrices : register(t41);

float4x4 Compute_BoneMatrix(uint4 vBlendIndex, float4 vBlendWeight)
{
    float fWeightW = 1.f - (vBlendWeight.x + vBlendWeight.y + vBlendWeight.z);

    uint i0 = iBoneOffset + vBlendIndex.x;
    uint i1 = iBoneOffset + vBlendIndex.y;
    uint i2 = iBoneOffset + vBlendIndex.z;
    uint i3 = iBoneOffset + vBlendIndex.w;

    return g_BoneMatrices[i0] * vBlendWeight.x +
            g_BoneMatrices[i1] * vBlendWeight.y +
            g_BoneMatrices[i2] * vBlendWeight.z +
            g_BoneMatrices[i3] * fWeightW;
}

#endif
