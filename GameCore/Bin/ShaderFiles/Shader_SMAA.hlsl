#include "Shader_Header.hlsli"
#include "Shader_SMAADefines.hlsli"
#include "SMAA.hlsl"

Texture2D g_ColorTex  : register(t0);
Texture2D g_EdgesTex  : register(t1);
Texture2D g_BlendTex  : register(t2);
Texture2D g_AreaTex   : register(t3);
Texture2D g_SearchTex : register(t4);

float2 Make_FullScreenUV(uint iVertexID, out float4 vPosition)
{
    float2 vUV = float2((iVertexID << 1) & 2, iVertexID & 2);
    vPosition = float4(vUV * float2(2.f, -2.f) + float2(-1.f, 1.f), 0.f, 1.f);
    return vUV;
}

struct VS_EDGE_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vOffset[3] : TEXCOORD1;
};

VS_EDGE_OUT VS_EDGE(uint iVertexID : SV_VertexID)
{
    VS_EDGE_OUT Out;
    Out.vTexcoord = Make_FullScreenUV(iVertexID, Out.vPosition);
    SMAAEdgeDetectionVS(Out.vTexcoord, Out.vOffset);
    return Out;
}

float2 PS_EDGE(VS_EDGE_OUT In) : SV_TARGET
{
    return SMAALumaEdgeDetectionPS(In.vTexcoord, In.vOffset, g_ColorTex);
}

struct VS_WEIGHT_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vPixcoord : TEXCOORD1;
    float4 vOffset[3] : TEXCOORD2;
};

VS_WEIGHT_OUT VS_WEIGHT(uint iVertexID : SV_VertexID)
{
    VS_WEIGHT_OUT Out;
    Out.vTexcoord = Make_FullScreenUV(iVertexID, Out.vPosition);
    SMAABlendingWeightCalculationVS(Out.vTexcoord, Out.vPixcoord, Out.vOffset);
    return Out;
}

float4 PS_WEIGHT(VS_WEIGHT_OUT In) : SV_TARGET
{
    return SMAABlendingWeightCalculationPS(In.vTexcoord, In.vPixcoord, In.vOffset, g_EdgesTex, g_AreaTex, g_SearchTex, float4(0.f, 0.f, 0.f, 0.f));
}

struct VS_BLEND_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vOffset : TEXCOORD1;
};

VS_BLEND_OUT VS_BLEND(uint iVertexID : SV_VertexID)
{
    VS_BLEND_OUT Out;
    Out.vTexcoord = Make_FullScreenUV(iVertexID, Out.vPosition);
    SMAANeighborhoodBlendingVS(Out.vTexcoord, Out.vOffset);
    return Out;
}

float4 PS_BLEND(VS_BLEND_OUT In) : SV_TARGET
{
    return SMAANeighborhoodBlendingPS(In.vTexcoord, In.vOffset, g_ColorTex, g_BlendTex);
}
