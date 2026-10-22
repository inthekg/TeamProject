#ifndef SHADER_GBUFFER_HLSLI
#define SHADER_GBUFFER_HLSLI
#include "Shader_Header.hlsli"


float4 Encode_Normal(float3 vWorldNormal)
{
    return float4(vWorldNormal * 0.5f + 0.5f, 0.f);    
}

float3 Decode_Normal(float4 vEncode)
{
    return normalize(vEncode.xyz * 2.f - 1.f);

}

float3 Apply_NormalMap(float3 vNormalRGB, float3 T, float3 B, float3 N)
{
    float3 vNormal = vNormalRGB * 2.f - 1.f;
    float3x3 TBN = float3x3(T, B * -1.f, N);
    return normalize(mul(vNormal, TBN));
}

void Build_TBN(float3 vWorldN, float3 vWorldT, out float3 N, out float3 T, out float3 B)
{
    N = normalize(vWorldN);
    T = normalize(vWorldT);
    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);
}

float3 Reconstruct_WorldPos(float2 vTexcoord, float4 vDepthDesc)
{
    float fViewZ = vDepthDesc.y * vCamPosition.w;
    float4 vPos;
    vPos.x = vTexcoord.x * 2.f - 1.f;
    vPos.y = vTexcoord.y * -2.f + 1.f;
    vPos.z = vDepthDesc.x;
    vPos.w = 1.f;
    
    vPos *= fViewZ;
    vPos = mul(vPos, ProjInvMatrix);
    vPos = mul(vPos, ViewInvMatrix);
    return vPos.xyz;    
}

float4 Pack_Depth(float4 vProjPos)
{
    return float4(vProjPos.z / vProjPos.w, vProjPos.w / vCamPosition.w, 0.f, 0.f);
}

struct GBuffer_Surface
{
    float3 vAlbedo;
    float3 vNormal;
    float  fAO;
    float  fRoughness;
    float  fMetallic;
    float3 vWorldPos;
    bool   bValid;
};

struct PS_OUT_GBUFFER
{
    float4 vDiffuse  : SV_Target0;
    float4 vNormal   : SV_Target1;
    float4 vARM      : SV_Target2;
    float4 vDepth    : SV_Target3;
    float4 vEmissive : SV_Target4;
};
#endif