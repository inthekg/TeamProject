#include "Shader_Header.hlsli"
#include "Shader_Skinning.hlsli"
#include "Shader_CSM.hlsli"
#include "Shader_ShadowData.hlsli"
#include "Shader_Dissolve.hlsli"

Texture2D g_HairOpacity : register(t18);
static const float SHADOW_ALPHA_REF = 0.1f;

struct VS_IN_STATIC
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
};
struct VS_IN_SKINNED
{
    float3  vPosition   : POSITION;
    float3 vNormal      : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    uint4 vBlendIndex : BLENDINDEX;
    float4 vBlendWeight : BLENDWEIGHT;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

VS_OUT VS_MAIN_STATIC(VS_IN_STATIC In)
{
    VS_OUT Out;
    
    float4 vWorld = mul(float4(In.vPosition, 1.f), WorldMatrix);
    Out.vPosition = mul(vWorld, g_ShadowViews[g_CurView.x].ViewProj);
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = vWorld.xyz;
    return Out;    
}

VS_OUT VS_MAIN_SKINNED(VS_IN_SKINNED In)
{
    VS_OUT Out;
    float4x4 BoneMatrix = Compute_BoneMatrix(In.vBlendIndex, In.vBlendWeight);
    float4 vLocal = mul(float4(In.vPosition, 1.f), BoneMatrix);
    float4 vWorld = mul(vLocal, WorldMatrix);
    Out.vPosition = mul(vWorld, g_ShadowViews[g_CurView.x].ViewProj);
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = vWorld.xyz;
    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};


float PS_MAIN(PS_IN In) : SV_TARGET
{
    if (iAlphaClip != 0)
    {
        float a = g_HairOpacity.Sample(g_DefaultSampler, In.vTexcoord).g;
        clip(a - SHADOW_ALPHA_REF);
    }
    
    Compute_Dissolve(In.vWorldPos);
    
    return In.vPosition.z;
}