#include "Shader_Header.hlsli"
#include "Shader_Skinning.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_Dissolve.hlsli"

Texture2D g_AlbedoTexture[TextureMaxStack] : register(t0);
Texture2D g_NormalTexture[TextureMaxStack] : register(t6);
Texture2D g_Mask1Texture[TextureMaxStack] : register(t12);

cbuffer PerMaterialBuffer : register(b3)
{
    float4 Eye_IrisColor;
    float4 Eye_ScleraColor;
    float4 EyePoint_Control;
    float Eye_IrisRadius;
    float Eye_LimbusPow;
    float Eye_LimbusDarkScale;
    float Eye_FlattenNormal;
    float Eye_IrisRoughness;
    float Eye_ScleraRoughness;
};

static const float2 EYE_UV_CENTER = float2(0.5f, 0.5f);
static const float EYE_UV_ASPECT = 1.0f;
static const float EYE_IRIS_RADIUS = 0.19f;
static const float EYE_FLATTEN_N = 0.75f;
static const float EYE_IRIS_ROUGH = 0.17f;
static const float EYE_SCLERA_ROUGH = 0.069f;
static const float EYE_LIMBUS_POW = 8.0f;
static const float EYE_LIMBUS_DARK = 2.15f;
static const float EYE_IRIS_GLOW = 3.0f;


struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    uint4 vBlendIndex : BLENDINDEX;
    float4 vBlendWeight : BLENDWEIGHT;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float4 vProjPos : TEXCOORD1;
    float3 vWorldPos : TEXCOORD2;
};


VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    float4x4 BoneMatrix = Compute_BoneMatrix(In.vBlendIndex, In.vBlendWeight);
    float4 vPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
    float3 vNormal = mul(float4(In.vNormal, 0.f), BoneMatrix).xyz;
    float3 vTangent = mul(float4(In.vTangent, 0.f), BoneMatrix).xyz;
    
    Out.vWorldPos = mul(vPosition, WorldMatrix).xyz;
    float4x4 matWVP = mul(mul(WorldMatrix, ViewMatrix), ProjMatrix);
    vPosition = mul(vPosition, matWVP);
    Out.vPosition = vPosition;
    Out.vProjPos = vPosition;
    Out.vTexcoord = In.vTexcoord;

    float3 vWorldN = mul(float4(vNormal, 0.f), WorldMatrix).xyz;
    float3 vWorldT = mul(float4(vTangent, 0.f), WorldMatrix).xyz;
    float3 N, T, B;
    Build_TBN(vWorldN, vWorldT, N, T, B);
    Out.vNormal = float4(N, 0.f);
    Out.vTangent = T;
    Out.vBinormal = B;
    return Out;
}

// 맵에 배치된 비애니메이션 Eye 메시를 기존 Eye 픽셀 셰이더로 전달한다.
struct VS_IN_STATIC
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
};

VS_OUT VS_STATIC(VS_IN_STATIC In)
{
    VS_OUT Out;
    float4 vWorldPosition = mul(float4(In.vPosition, 1.f), WorldMatrix);
    float4 vPosition = mul(mul(vWorldPosition, ViewMatrix), ProjMatrix);

    Out.vPosition = vPosition;
    Out.vProjPos = vPosition;
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = vWorldPosition.xyz;

    float3 vWorldN = mul(float4(In.vNormal, 0.f), WorldMatrix).xyz;
    float3 vWorldT = mul(float4(In.vTangent, 0.f), WorldMatrix).xyz;
    float3 N, T, B;
    Build_TBN(vWorldN, vWorldT, N, T, B);
    Out.vNormal = float4(N, 0.f);
    Out.vTangent = T;
    Out.vBinormal = B;
    return Out;
}

// 맵 인스턴스의 WORLD0~3 행렬을 적용한 Eye 메시를 기존 Eye 픽셀 셰이더로 전달한다.
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
    float4 vWorldPosition = mul(float4(In.vPosition, 1.f), finalWorld);
    float4 vPosition = mul(mul(vWorldPosition, ViewMatrix), ProjMatrix);

    Out.vPosition = vPosition;
    Out.vProjPos = vPosition;
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = vWorldPosition.xyz;

    float3 vWorldN = mul(float4(In.vNormal, 0.f), finalWorld).xyz;
    float3 vWorldT = mul(float4(In.vTangent, 0.f), finalWorld).xyz;
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
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float4 vProjPos : TEXCOORD1;
    float3 vWorldPos : TEXCOORD2;
};


PS_OUT_GBUFFER PS_MAIN(PS_IN In)
{
    PS_OUT_GBUFFER Out;

    float3 vDissolveGlow = Compute_Dissolve(In.vWorldPos);
    
    float fIrisRadius = (Eye_IrisRadius > 0.f) ? Eye_IrisRadius : EYE_IRIS_RADIUS;
    float fLimbusPow = (Eye_LimbusPow > 0.f) ? Eye_LimbusPow : EYE_LIMBUS_POW;
    float fLimbusDark = (Eye_LimbusDarkScale > 0.f) ? Eye_LimbusDarkScale : EYE_LIMBUS_DARK;
    float fFlattenN = (Eye_FlattenNormal > 0.f) ? Eye_FlattenNormal : EYE_FLATTEN_N;
    float fIrisRough = (Eye_IrisRoughness > 0.f) ? Eye_IrisRoughness : EYE_IRIS_ROUGH;
    float fScleraRough = (Eye_ScleraRoughness > 0.f) ? Eye_ScleraRoughness : EYE_SCLERA_ROUGH;

    float2 vEyeCenter = (EyePoint_Control.x > 0.01f && EyePoint_Control.y > 0.01f) ? EyePoint_Control.xy : EYE_UV_CENTER;

    float3 vIrisTint = (dot(Eye_IrisColor.rgb, 1.f.xxx) > 1e-4f) ? Eye_IrisColor.rgb : 1.f.xxx;
    float3 vScleraTint = (dot(Eye_ScleraColor.rgb, 1.f.xxx) > 1e-4f) ? Eye_ScleraColor.rgb : 1.f.xxx;

    float2 vDir = (In.vTexcoord - vEyeCenter) * float2(EYE_UV_ASPECT, 1.f);
    float fR = length(vDir);
    float fAA = fwidth(fR) + 1e-4f;
    float fIrisMask = 1.f - smoothstep(fIrisRadius - fAA, fIrisRadius + fAA, fR);
    float fIrisT = saturate(fR / max(fIrisRadius, 1e-4f));

    float3 vSclera = pow(g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord).rgb, 2.2f);
    vSclera *= vScleraTint;

    float2 vIrisUV = (vDir / fIrisRadius) * 0.5f + 0.5f;
    float3 vIrisTex = pow(g_Mask1Texture[0].Sample(g_DefaultSampler, vIrisUV).rgb, 2.2f);

    float fLimbus = pow(fIrisT, fLimbusPow) * fLimbusDark;
    float fLimbusFade = saturate(1.f - fLimbus * 0.6f);

    float3 vIrisFull = vIrisTex * vIrisTint * fLimbusFade;
    float3 vIrisAlbedo = saturate(vIrisFull);

    float3 vColor = lerp(vSclera, vIrisAlbedo, fIrisMask);

    float3 N = normalize(In.vNormal.xyz);
    float3 vNormalMap = g_NormalTexture[0].Sample(g_DefaultSampler, In.vTexcoord).xyz * 2.f - 1.f;
    float3 T = normalize(In.vTangent);
    float3 B = normalize(In.vBinormal);
    float3x3 TBN = float3x3(T, B * -1.f, N);
    float3 vWetN = normalize(mul(vNormalMap, TBN));
    float3 vWorldNormal = normalize(lerp(vWetN, N, fFlattenN));

    float fRoughness = lerp(fScleraRough, fIrisRough, fIrisMask);

    float fEmiss = (fEmissiveAmount > 0.f) ? fEmissiveAmount : 1.f;
    float3 vIrisGlowTint = max(vIrisTint - 1.f, 0.f);
    float3 vGlow = vIrisTex * vIrisGlowTint * fLimbusFade * fIrisMask * fEmiss * EYE_IRIS_GLOW;

    Out.vDiffuse = float4(vColor, 1.f);
    Out.vNormal = Encode_Normal(vWorldNormal);
    Out.vARM = float4(1.f, fRoughness, 0.f, 1.f);
    Out.vDepth = Pack_Depth(In.vProjPos);
    Out.vDepth.z = (float) iRimProfile;
    Out.vDepth.w = fRimAmount;
    Out.vEmissive = float4(vGlow + vDissolveGlow, 1.f);
    return Out;
}
