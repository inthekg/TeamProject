#include "Shader_Header.hlsli"

struct VS_IN
{
    float3 vPosition : POSITION;
    float4 vColor : COLOR;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vColor : COLOR;
};
    

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    float4 vPosition = mul(float4(In.vPosition, 1.f), ViewMatrix);
    vPosition = mul(vPosition, ProjMatrix);    
    Out.vPosition = vPosition;
    Out.vColor = In.vColor;
    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vColor : COLOR;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};
    

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    Out.vColor = In.vColor;
    return Out;
}