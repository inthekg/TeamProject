#include "Shader_Header.hlsli"

Texture2D g_AlbedoTexture[TextureMaxStack] : register(t0);

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};
    

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    
    float4 vPosition = mul(float4(In.vPosition, 1.f), WorldMatrix);
    vPosition = mul(vPosition, ViewMatrix);
    vPosition = mul(vPosition, ProjMatrix);
    
    Out.vPosition = vPosition;
    Out.vTexcoord = In.vTexcoord;
    
    return Out;
}


struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};
    

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    
    Out.vColor = g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord);
    if (Out.vColor.a < 0.1f)
        discard;
    
    return Out;
}

PS_OUT PS_MAIN_DEBUG(PS_IN In)
{
    PS_OUT Out;
    
    Out.vColor = g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord);
    if (Out.vColor.a < 0.1f)
        Out.vColor = float4(1.f, 1.f, 1.f, 1.f);
    
    return Out;
}
