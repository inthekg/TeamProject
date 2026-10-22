#ifndef SHADER_FULLSCREEN_HLSLI
#define SHADER_FULLSCREEN_HLSLI
#include "Shader_Header.hlsli"

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

// 일반 색 출력
struct PS_OUT
{
    float4 vColor : SV_TARGET0;    
};

// 라이트 누적 출력 ( 가산 HDR 타깃 ) 
struct PS_OUT_LIGHT
{
    float4 vLight : SV_TARGET0;
};

#endif