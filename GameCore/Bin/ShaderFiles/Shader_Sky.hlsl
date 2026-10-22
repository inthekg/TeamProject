#include "Shader_Header.hlsli"

TextureCube g_SkyTexture : register(t25);

struct VS_IN
{
    float3 vPosition : POSITIONT;
    float3 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition    : SV_Position;
    float3 vDirection   : TEXCOORD0;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    float4 vViewPosition = mul(float4(In.vPosition, 0.f), ViewMatrix);
    float4 vClipPosition = mul(float4(vViewPosition.xyz, 1.f), ProjMatrix);
    
    Out.vPosition = vClipPosition.xyww;
    Out.vDirection = In.vTexcoord;
    
    return Out;
}

float4 PS_MAIN(VS_OUT In) : SV_TARGET0
{
    return g_SkyTexture.Sample(g_DefaultSampler, normalize(In.vDirection));

}