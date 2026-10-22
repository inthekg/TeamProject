#include "Shader_Header.hlsli"
#include "Shader_MaterialTextures.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_PostGlobals.hlsli"

Texture2D g_DepthTexture : register(t23);
Texture2D g_ReflectionTexture : register(t42);

cbuffer PerMaterialBuffer : register(b3)
{
    float4 Water_Color;
    float4 Fluid_BaseColor;
    float4 Water_DepthFade_Color;

    float Wave_Speed;
    float Wave_Size;
    float Wave_Intensity;
    float Fluid_Normal_Intensity;

    float Opacity;
    float Fluid_Opacity;
    float Color_Depth;
    float Color_Contrast;

    float Water_Roughness;
    float CubeMap_Itensity;
    float Refraction;
    float Overall_Intensity;

    float Reflect_Min;
    float Reflect_Power;
    float Reflect_Tint;
    float Shore_Fade;

    float Moss_Range;
    float Moss_Scale;
    float Moss_BlendPower;
    float Moss_NoiseScale;

    float Foam_Radius;
    float Foam_Opacity;
    float Foam_Speed;
    float FoamSize;

    float Foam_Cutoff;
    float Foam_Distort;
    float Foam_Pad0;
    float Foam_Pad1;
};

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
    float4 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

VS_OUT Build_WaterVertex(float3 vLocalPos, float3 vLocalNormal, float3 vLocalTangent, float2 vTexcoord, float4x4 matWorld)
{
    VS_OUT Out;

    float4 vWorldPos = mul(float4(vLocalPos, 1.f), matWorld);
    Out.vWorldPos = vWorldPos.xyz;

    float4 vPosition = mul(vWorldPos, ViewMatrix);
    vPosition = mul(vPosition, ProjMatrix);

    Out.vPosition = vPosition;
    Out.vProjPos = vPosition;
    Out.vTexcoord = vTexcoord;

    float3 vWorldN = normalize(mul(float4(vLocalNormal, 0.f), matWorld).xyz);
    float3 vWorldT = normalize(mul(float4(vLocalTangent, 0.f), matWorld).xyz);

    float3 N, T, B;
    Build_TBN(vWorldN, vWorldT, N, T, B);
    Out.vNormal = float4(N, 0.f);
    Out.vTangent = T;
    Out.vBinormal = B;

    return Out;
}

VS_OUT VS_MAIN(VS_IN In)
{
    return Build_WaterVertex(In.vPosition, In.vNormal, In.vTangent, In.vTexcoord, WorldMatrix);
}

VS_OUT VS_INSTANCE(VS_IN_INSTANCE In)
{
    float4x4 instanceWorld = float4x4(In.vRight, In.vUp, In.vLook, In.vTranslation);
    float4x4 finalWorld = mul(instanceWorld, WorldMatrix);

    return Build_WaterVertex(In.vPosition, In.vNormal, In.vTangent, In.vTexcoord, finalWorld);
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

float3 Get_WaveNormal(float3 vWorldPos, float3 T, float3 B, float3 N)
{
    float fScale = max(Wave_Size, 1.f);
    float fFlow = fTime * Wave_Speed;

    float2 vUV0 = vWorldPos.xz / fScale + float2(0.0060f, 0.0042f) * fFlow;
    float2 vUV1 = vWorldPos.xz / fScale * 1.7f + float2(-0.0034f, 0.0066f) * fFlow;

    float3 vN0 = g_NormalTexture[0].Sample(g_DefaultSampler, vUV0).xyz * 2.f - 1.f;
    float3 vN1 = g_NormalTexture[0].Sample(g_DefaultSampler, vUV1).xyz * 2.f - 1.f;

    float3 vTangentN = float3(vN0.xy + vN1.xy, vN0.z * vN1.z);
    vTangentN.xy *= max(Fluid_Normal_Intensity, 0.f);
    vTangentN = normalize(vTangentN);

    return normalize(mul(vTangentN, float3x3(T, B * -1.f, N)));
}

float Get_Fresnel(float fNdotV)
{
    float fMin = Reflect_Min;
    float fPower = Reflect_Power;

    if (fPower < 0.01f)
    {
        fMin = 0.25f;
        fPower = 2.5f;
    }

    fMin = saturate(fMin);

    return saturate(fMin + (1.f - fMin) * pow(1.f - fNdotV, fPower));
}

float Get_ShoreFade(float fDepthDiff)
{
    float fDistance = (Shore_Fade > 0.f) ? Shore_Fade : 0.2f;

    return saturate(fDepthDiff / fDistance);
}

float Get_MossMask(float3 vWorldPos, float fDepthDiff)
{
    if (Moss_Range <= 0.f)
        return 0.f;

    float fMask = 1.f - saturate(fDepthDiff / Moss_Range);

    if (fMask <= 0.001f)
        return 0.f;

    float fNoiseScale = max(Moss_NoiseScale, 0.01f);
    float fNoise = g_AlbedoTexture[0].Sample(g_DefaultSampler, vWorldPos.xz / fNoiseScale).g;

    fMask *= saturate(fNoise * 1.6f);
    fMask *= saturate(Moss_BlendPower);

    return saturate(fMask);
}

float Get_FoamNoise(float2 vUV)
{
    float fN0 = g_NormalTexture[0].Sample(g_DefaultSampler, vUV).r;
    float fN1 = g_NormalTexture[0].Sample(g_DefaultSampler, vUV * 2.3f + float2(0.37f, 0.71f)).g;

    return saturate(abs(fN0 - 0.5f) * abs(fN1 - 0.5f) * 9.f);
}

float Get_Foam(float3 vWorldPos, float fDepthDiff)
{
    if (Foam_Radius <= 0.f)
        return 0.f;

    float fSize = max(FoamSize, 0.01f);
    float2 vFlow = fTime * Foam_Speed * float2(0.013f, 0.009f);

    float fWobble = Get_FoamNoise(vWorldPos.xz / (fSize * 3.f) + vFlow * 0.4f);

    float fDistortAmount = (Foam_Distort > 0.f) ? Foam_Distort : 0.6f;
    float fDistort = (fWobble - 0.5f) * fDistortAmount * Foam_Radius;

    float fEdge = 1.f - saturate((fDepthDiff + fDistort) / Foam_Radius);

    if (fEdge <= 0.001f)
        return 0.f;

    float fNoise = Get_FoamNoise(vWorldPos.xz / fSize + vFlow);
    float fMask = fEdge * fNoise * 1.6f;

    float fCutoff = (Foam_Cutoff > 0.f) ? Foam_Cutoff : 0.35f;
    fMask = smoothstep(fCutoff, fCutoff + 0.18f, fMask);

    return fMask * saturate(Foam_Opacity * 5.f);
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    float3 T = normalize(In.vTangent);
    float3 B = normalize(In.vBinormal);
    float3 vFlatN = normalize(In.vNormal.xyz);

    float3 N = Get_WaveNormal(In.vWorldPos, T, B, vFlatN);
    float3 V = normalize(vCamPosition.xyz - In.vWorldPos);

    float2 vScreenUV = In.vProjPos.xy / In.vProjPos.w;
    vScreenUV = vScreenUV * float2(0.5f, -0.5f) + 0.5f;

    float4 vSceneDepthDesc = g_DepthTexture.Sample(g_PointSampler, vScreenUV);
    float fSceneViewZ = vSceneDepthDesc.y * vCamPosition.w;
    float fWaterViewZ = In.vProjPos.w;

    float fDepthDiff = max(0.f, fSceneViewZ - fWaterViewZ);
    float fDepthFade = saturate(fDepthDiff / max(Color_Depth, 1e-3f));
    fDepthFade = pow(fDepthFade, max(1.f - Color_Contrast, 0.05f));

    float fMoss = Get_MossMask(In.vWorldPos, fDepthDiff);

    if (fMoss > 0.f)
    {
        float fMossScale = max(Moss_Scale, 0.01f);
        float3 vMossN = g_DetailNormalTexture.Sample(g_DefaultSampler, In.vWorldPos.xz / fMossScale).xyz * 2.f - 1.f;
        float3 vMossWorldN = normalize(mul(vMossN, float3x3(T, B * -1.f, vFlatN)));

        N = normalize(lerp(N, vMossWorldN, fMoss));
    }

    float2 vDistort = N.xz * Wave_Intensity * 0.05f;
    float2 vReflectUV = saturate(vScreenUV + vDistort);
    float3 vReflectColor = g_ReflectionTexture.Sample(g_LinearClampSampler, vReflectUV).rgb;

    vReflectColor *= (Reflect_Tint > 0.f) ? Reflect_Tint : 1.f;

    float fNdotV = saturate(dot(N, V));
    float fFresnel = Get_Fresnel(fNdotV);

    float3 vBody = lerp(Fluid_BaseColor.rgb, Water_DepthFade_Color.rgb, fDepthFade);

    float3 vColor = lerp(vBody, vReflectColor, fFresnel);

    if (fMoss > 0.f)
    {
        float fMossScale = max(Moss_Scale, 0.01f);
        float3 vMossColor = g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vWorldPos.xz / fMossScale).rgb;

        vColor = lerp(vColor, vMossColor, fMoss);
    }

    float3 L = normalize(-vLight_Direction.xyz);
    float3 H = normalize(L + V);
    float fSpecPower = lerp(512.f, 16.f, saturate(Water_Roughness));
    float fSpec = pow(saturate(dot(N, H)), fSpecPower);
    vColor += vLight_Specular.rgb * fSpec * (1.f - fMoss);

    vColor *= max(Overall_Intensity, 0.f);

    float fAlpha = saturate(lerp(Fluid_Opacity, Opacity, fDepthFade));
    fAlpha = saturate(fAlpha + fFresnel * 0.5f);
    fAlpha = saturate(fAlpha + fMoss);

    fAlpha *= Get_ShoreFade(fDepthDiff);

    float fFoam = Get_Foam(In.vWorldPos, fDepthDiff);

    vColor = lerp(vColor, float3(0.85f, 0.88f, 0.90f), fFoam);
    fAlpha = saturate(fAlpha + fFoam);

    Out.vColor = float4(vColor, fAlpha);
    return Out;
}
