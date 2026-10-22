#include "Shader_Header.hlsli"
#include "Shader_MaterialTextures.hlsli"
#include "Shader_RenderTuning.hlsli"
#include "Shader_PostGlobals.hlsli"

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
    float fWorldY : TEXCOORD1;
};

VS_OUT Build_ReflectVertex(float3 vLocalPos, float3 vLocalNormal, float2 vTexcoord, float4x4 matWorld)
{
    VS_OUT Out;

    float4 vWorldPos = mul(float4(vLocalPos, 1.f), matWorld);
    float4 vPosition = mul(vWorldPos, ViewMatrix);
    vPosition = mul(vPosition, ProjMatrix);

    Out.vPosition = vPosition;
    Out.vTexcoord = vTexcoord;
    Out.vNormal = normalize(mul(float4(vLocalNormal, 0.f), matWorld).xyz);
    Out.fWorldY = vWorldPos.y;

    return Out;
}

VS_OUT VS_MAIN(VS_IN In)
{
    return Build_ReflectVertex(In.vPosition, In.vNormal, In.vTexcoord, WorldMatrix);
}

VS_OUT VS_INSTANCE(VS_IN_INSTANCE In)
{
    float4x4 instanceWorld = float4x4(In.vRight, In.vUp, In.vLook, In.vTranslation);
    float4x4 finalWorld = mul(instanceWorld, WorldMatrix);

    return Build_ReflectVertex(In.vPosition, In.vNormal, In.vTexcoord, finalWorld);
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float fWorldY : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    clip(In.fWorldY - fReflectClipHeight + 0.05f);

    float4 vAlbedo = g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord);
    if (vAlbedo.a < 0.1f)
        discard;

    vAlbedo.rgb = pow(vAlbedo.rgb, 2.2f);

    float3 N = normalize(In.vNormal);
    float3 L = normalize(-vLight_Direction.xyz);
    float fNdotL = saturate(dot(N, L));

    float fWrap = saturate(dot(N, L) * 0.5f + 0.5f);
    float fDiffuse = lerp(fWrap, fNdotL, 0.5f);

    float3 vAmbient = saturate(vLight_Ambient.rgb) + g_RenderTuning[0].vAmbientFloor;

    float3 vLit = vAlbedo.rgb * (vLight_Diffuse.rgb * fDiffuse + vAmbient);

    Out.vColor = float4(vLit, 1.f);
    return Out;
}
