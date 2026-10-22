#include "Shader_Header.hlsli"
#include "Shader_MaterialTextures.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_RenderTuning.hlsli"

static const float GLASS_F0 = 0.04f;

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};


VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4 vPosition = mul(float4(In.vPosition, 1.f), WorldMatrix);
    Out.vWorldPos = vPosition;

    vPosition = mul(vPosition, ViewMatrix);
    vPosition = mul(vPosition, ProjMatrix);
    Out.vPosition = vPosition;
    Out.vTexcoord = In.vTexcoord;

    float3 vWorldN = normalize(mul(float4(In.vNormal, 0.f), WorldMatrix).xyz);
    float3 vWorldT = normalize(mul(float4(In.vTangent, 0.f), WorldMatrix).xyz);

    float3 N, T, B;
    Build_TBN(vWorldN, vWorldT, N, T, B);
    Out.vNormal = float4(N, 0.f);
    Out.vTangent = T;
    Out.vBinormal = B;
    return Out;
}

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

VS_OUT VS_INSTANCE(VS_IN_INSTANCE In)
{
    VS_OUT Out;

    float4x4 instanceWorld = float4x4(In.vRight, In.vUp, In.vLook, In.vTranslation);
    float4x4 finalWorld = mul(instanceWorld, WorldMatrix);

    float4 vPosition = mul(float4(In.vPosition, 1.f), finalWorld);
    Out.vWorldPos = vPosition.xyz;

    vPosition = mul(vPosition, ViewMatrix);
    vPosition = mul(vPosition, ProjMatrix);
    Out.vPosition = vPosition;
    Out.vTexcoord = In.vTexcoord;

    float3 vWorldN = normalize(mul(float4(In.vNormal, 0.f), finalWorld).xyz);
    float3 vWorldT = normalize(mul(float4(In.vTangent, 0.f), finalWorld).xyz);

    float3 N, T, B;
    Build_TBN(vWorldN, vWorldT, N, T, B);
    Out.vNormal = float4(N, 0.f);
    Out.vTangent = T;
    Out.vBinormal = B;
    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};


// Shlick 프레넬
float Fresnel_Schlick(float fCosTheta, float fF0)
{
    return fF0 + (1.f - fF0) * pow(saturate(1.f - fCosTheta), 5.f);
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    MaterialData mat = g_MaterialList[iMaterialIndex];
    RenderTuning rt = g_RenderTuning[0];

    float4 vMask = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord);
    float fDirt = vMask.r;
    float fRough = vMask.g;
    float fCoverage = vMask.a;

    float3 N = normalize(In.vNormal.xyz);
    float3 V = normalize(vCamPosition.xyz - In.vWorldPos);
    float3 L = normalize(-vLight_Direction.xyz);
    float3 H = normalize(V + L);

    float fNdotV = saturate(dot(N, V));
    float fFresnel = Fresnel_Schlick(fNdotV, GLASS_F0);

    float fSpecPow = lerp(160.0f, 8.0f, saturate(fRough));
    float fSpec = pow(saturate(dot(N, H)), fSpecPow);
    float3 vSpecular = saturate(vLight_Specular.rgb) * fSpec * (1.0f - saturate(fRough)) * rt.fGlassSpec;

    float3 vGlassTint = mat.GlassColor.rgb;
    float3 vDirtTint = mat.DirtyColor.rgb;

    if (dot(vGlassTint + vDirtTint, 1.f) < 1e-4f)
    {
        vGlassTint = mat.BaseColor_Tint.rgb;
        vDirtTint = mat.BaseColor_Tint.rgb;
    }

    float3 vTint = lerp(vGlassTint, vDirtTint, saturate(fDirt));
    float3 vReflect = saturate(vLight_Ambient.rgb) * fFresnel * rt.fGlassReflect;

    float3 vFinal = vTint * rt.fGlassTint + vReflect + vSpecular;

    float fAlpha = saturate(rt.fGlassBaseAlpha + fFresnel * rt.fGlassFresnel
                          + fDirt * rt.fGlassDirtAlpha + fSpec * 0.15f);

    fAlpha *= fCoverage;

    if (fAlpha < 0.004f)
        discard;

    Out.vColor = float4(vFinal, fAlpha);
    return Out;
}
