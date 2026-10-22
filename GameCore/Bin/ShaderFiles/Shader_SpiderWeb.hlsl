#include "Shader_Header.hlsli"
#include "Shader_MaterialTextures.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_RenderTuning.hlsli"

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
};

struct VS_IN_INSTANCE
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;

    float4 vRight : WORLD0;
    float4 vUp : WORLD1;
    float4 vLook : WORLD2;
    float4 vTranslation : WORLD3;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

VS_OUT Build_WebVertex(float3 vLocalPos, float3 vLocalNormal, float2 vTexcoord, float4x4 matWorld)
{
    VS_OUT Out;

    float4 vWorldPos = mul(float4(vLocalPos, 1.f), matWorld);
    Out.vWorldPos = vWorldPos.xyz;

    float4 vPosition = mul(vWorldPos, ViewMatrix);
    Out.vPosition = mul(vPosition, ProjMatrix);

    Out.vNormal = normalize(mul(float4(vLocalNormal, 0.f), matWorld).xyz);
    Out.vTexcoord = vTexcoord;

    return Out;
}

VS_OUT VS_MAIN(VS_IN In)
{
    return Build_WebVertex(In.vPosition, In.vNormal, In.vTexcoord, WorldMatrix);
}

VS_OUT VS_INSTANCE(VS_IN_INSTANCE In)
{
    float4x4 instanceWorld = float4x4(In.vRight, In.vUp, In.vLook, In.vTranslation);
    float4x4 finalWorld = mul(instanceWorld, WorldMatrix);

    return Build_WebVertex(In.vPosition, In.vNormal, In.vTexcoord, finalWorld);
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    MaterialData mat = g_MaterialList[iMaterialIndex];

    float fCoverage = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord).a;

    if (fCoverage < 0.004f)
        discard;

    float3 vAlbedo = mat.BaseColor_Tint.rgb;

    if (dot(vAlbedo, 1.f) < 1e-4f)
        vAlbedo = 0.5f;

    float3 N = normalize(In.vNormal);
    float3 L = normalize(-vLight_Direction.xyz);

    float fWrap = saturate(dot(N, L) * 0.5f + 0.5f);
    float3 vAmbient = saturate(vLight_Ambient.rgb) + g_RenderTuning[0].vAmbientFloor;

    float3 vLit = vAlbedo * (vLight_Diffuse.rgb * fWrap + vAmbient);

    Out.vColor = float4(vLit, fCoverage);
    return Out;
}
