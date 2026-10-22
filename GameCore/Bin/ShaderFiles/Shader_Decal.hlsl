#include "Shader_Header.hlsli"
#include "Shader_GBuffer.hlsli"

cbuffer DecalBuffer : register(b13)
{
    float4x4    DecalWorld;
    float4x4    DecalInWorld;
    float4      vDecalTint;
    float2      vDecalTiling;       float2  vDecalUVOffset;
    float       fAngleFadeStart;    float   fAngleFadeEnd;      float   fAlbedoWeight;      float   fNormalWeight;
    float       fORMSWeight;        float   fEmissiveWeight;    float   fEmissiveIntensity; int     iHasAlbedo;
    int         iHashNormal;        int     iHasORMS;           int     iHasEmissive;       int     iHasMask;
}

Texture2D g_AlbedoTex   : register(t0);
Texture2D g_NormalTex   : register(t1);
Texture2D g_ORMSTex     : register(t2);
Texture2D g_EmissiveTex : register(t3);
Texture2D g_MaskTex     : register(t4);
Texture2D g_DepthTex    : register(t23);

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vClip : TEXCOORD0;
};

VS_OUT VS_Main(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;
    
    float4 worldPos = mul(float4(In.vPosition, 1.f), DecalWorld);
    float4 viewPos = mul(worldPos, ViewMatrix);
    float4 projPos = mul(viewPos, ProjMatrix);
    
    Out.vPosition = projPos;
    Out.vClip = projPos;
    
    return Out;
}

struct PS_OUT
{
    float4 vDiffuse : SV_Target0;
    float4 vNormal : SV_Target1;
    float4 vORMS : SV_Target2;
    float4 vEmissive : SV_Target3;
};

PS_OUT PS_Main(VS_OUT In)
{
    PS_OUT Out = (PS_OUT) 0;
    
    float2 screenUV = (In.vClip.xy / In.vClip.w) * float2(0.5f, -0.5f) + 0.5f;
    
    float4 depthDesc = g_DepthTex.Sample(g_PointSampler, screenUV);
    if (depthDesc.y <= 0.f)
        discard;
    
    float3 worldPos = Reconstruct_WorldPos(screenUV, depthDesc);
    
    float3 vLocal = mul(float4(worldPos, 1.f), DecalInWorld).xyz;
    if (any(abs(vLocal) > 0.5f))
        discard;
    
    float2 uv = (vLocal.xz + 0.5f) * vDecalTiling + vDecalUVOffset;
    
    float3 T = normalize(mul(float4(1.f, 0.f, 0.f, 0.f), DecalWorld).xyz);
    float3 N = normalize(mul(float4(0.f, 1.f, 0.f, 0.f), DecalWorld).xyz);
    float3 B = normalize(mul(float4(0.f, 0.f, 1.f, 0.f), DecalWorld).xyz);
   
    float3 surfN = normalize(cross(ddx(worldPos), ddy(worldPos)));
    if (dot(surfN, vCamPosition.xyz - worldPos) < 0.f)
        surfN = -surfN;
    
    float cosA = dot(surfN, N);
    if (cosA <= 0.f)
        discard;
    
    float fAngle = degrees(acos(saturate(cosA)));
    float fAngleFade = 1.f - saturate((fAngle - fAngleFadeStart) / max(1e-3f, fAngleFadeEnd - fAngleFadeStart));
    
    float fAlpha = vDecalTint.a * fAngleFade;
    
    if (iHasMask)
        fAlpha *= g_MaskTex.Sample(g_DefaultSampler, uv).r;
    
    if (fAlpha <= 0.001f)
        discard;
    
    float3 vAlbedo = vDecalTint.rgb;
    float fAlbedoAlpha = fAlpha;
    if (iHasAlbedo)
    {
        float4 a = g_AlbedoTex.Sample(g_DefaultSampler, uv);
        vAlbedo *= pow(abs(a.rgb), 2.2f);
        fAlbedoAlpha *= a.a;
    }
    Out.vDiffuse = float4(vAlbedo, fAlbedoAlpha * fAlbedoWeight);
    
    float3 outN = N;
    if (iHashNormal)
    {
        float3 nT = g_NormalTex.Sample(g_DefaultSampler, uv).xyz * 2.f - 1.f;
        outN = normalize(nT.x * T + nT.y * B + nT.z * N);
    }
    Out.vNormal = float4(Encode_Normal(outN).xyz, fAlpha * fNormalWeight);
    
    float4 vORMS = float4(1.f, 1.f, 0.f, 1.f);
    if (iHasORMS)
        vORMS = g_ORMSTex.Sample(g_DefaultSampler, uv);
    Out.vORMS = float4(vORMS.rgb, fAlpha * fORMSWeight);
    
    float3 vEmi = float3(0.f, 0.f, 0.f);
    if (iHasEmissive)
        vEmi = g_EmissiveTex.Sample(g_DefaultSampler, uv).rgb * fEmissiveIntensity * vDecalTint.rgb;
    Out.vEmissive = float4(vEmi, fAlpha * fEmissiveWeight);
    
    return Out;
}