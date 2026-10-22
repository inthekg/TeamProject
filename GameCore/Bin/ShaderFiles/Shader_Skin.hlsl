#include "Shader_Header.hlsli"
#include "Shader_Skinning.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_Dissolve.hlsli"

Texture2D g_AlbedoTexture[TextureMaxStack] : register(t0);
Texture2D g_NormalTexture[TextureMaxStack] : register(t6);
Texture2D g_Mask1Texture[TextureMaxStack] : register(t12);
Texture2D g_Mask2Texture : register(t18);
Texture2D g_DetailNormalTexture : register(t19);

cbuffer PerMaterialBuffer : register(b3)
{
    float Normal_Intensity;
    float DetailNormal_Tiling;
    float DetailNormal_Intensity;
    float Roughness_Factor;
    float Skin_Sheen;
};

static const float SKIN_OPACITY_CLIP = 0.3333f;
static const float SKIN_ROUGHNESS_FACTOR = 0.8f;
static const float SKIN_ROUGHNESS_FLOOR = 0.45f;
static const float SKIN_NORMAL_INTENSITY = 0.85f;
#define USE_DETAIL_NORMAL 1
static const float SKIN_DETAIL_TILING = 15.0f;
static const float SKIN_DETAIL_INTENSITY = 0.4f;
static const float SKIN_SHEEN_FLOOR = 0.24f;
static const float SKIN_SHEEN_SCALE = 0.6f;

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

// 맵에 직접 배치된 정적 Skin 메시 입력 데이터입니다.
struct VS_IN_STATIC
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
};

// WORLD0~3 입력을 사용하는 맵 인스턴스 Skin 메시 입력 데이터입니다.
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
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float4 vProjPos : TEXCOORD1;
    float3 vWorldPos : TEXCOORD2;
};

// 메시 종류와 관계없이 동일한 Skin 정점 출력을 생성합니다.
VS_OUT Build_SkinVertex(float4 vLocalPosition, float3 vLocalNormal, float3 vLocalTangent, float2 vTexcoord, float4x4 matWorld)
{
    VS_OUT Out;
    float4 vWorldPosition = mul(vLocalPosition, matWorld);

    Out.vWorldPos = vWorldPosition.xyz;
    Out.vPosition = mul(mul(vWorldPosition, ViewMatrix), ProjMatrix);
    Out.vProjPos = Out.vPosition;
    Out.vTexcoord = vTexcoord;

    float3 vWorldN = mul(float4(vLocalNormal, 0.f), matWorld).xyz;
    float3 vWorldT = mul(float4(vLocalTangent, 0.f), matWorld).xyz;
    float3 N, T, B;
    Build_TBN(vWorldN, vWorldT, N, T, B);
    Out.vNormal = float4(N, 0.f);
    Out.vTangent = T;
    Out.vBinormal = B;
    return Out;
}

// 애니메이션 캐릭터의 Skin 메시를 처리합니다.
VS_OUT VS_MAIN(VS_IN In)
{
    float4x4 BoneMatrix = Compute_BoneMatrix(In.vBlendIndex, In.vBlendWeight);
    float4 vSkinnedPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
    float3 vSkinnedNormal = mul(float4(In.vNormal, 0.f), BoneMatrix).xyz;
    float3 vSkinnedTangent = mul(float4(In.vTangent, 0.f), BoneMatrix).xyz;

    return Build_SkinVertex(vSkinnedPosition, vSkinnedNormal, vSkinnedTangent, In.vTexcoord, WorldMatrix);
}

// 맵에 직접 배치된 정적 Skin 메시를 처리합니다.
VS_OUT VS_STATIC(VS_IN_STATIC In)
{
    return Build_SkinVertex(float4(In.vPosition, 1.f), In.vNormal, In.vTangent, In.vTexcoord, WorldMatrix);
}

// CPU Visible Matrix 인스턴스 경로의 Skin 메시를 처리합니다.
VS_OUT VS_INSTANCE(VS_IN_INSTANCE In)
{
    float4x4 matInstanceWorld = float4x4(In.vRight, In.vUp, In.vLook, In.vTranslation);
    float4x4 matFinalWorld = mul(matInstanceWorld, WorldMatrix);

    return Build_SkinVertex(float4(In.vPosition, 1.f), In.vNormal, In.vTangent, In.vTexcoord, matFinalWorld);
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
    
    float fNormalIntensity = (Normal_Intensity > 0.f) ? Normal_Intensity : SKIN_NORMAL_INTENSITY;
    float fDetailTiling = (DetailNormal_Tiling > 0.f) ? DetailNormal_Tiling : SKIN_DETAIL_TILING;
    float fDetailIntensity = (DetailNormal_Intensity > 0.f) ? DetailNormal_Intensity : SKIN_DETAIL_INTENSITY;
    float fRoughnessFactor = (Roughness_Factor > 0.f) ? Roughness_Factor : SKIN_ROUGHNESS_FACTOR;

    float4 vAlbedo = pow(g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord), 2.2f);
    if (vAlbedo.a < SKIN_OPACITY_CLIP)
        discard;

    float3 vNormalMap = g_NormalTexture[0].Sample(g_DefaultSampler, In.vTexcoord).xyz;
    vNormalMap = vNormalMap * 2.f - 1.f;
    vNormalMap.xy *= fNormalIntensity;
    float3 N = normalize(In.vNormal.xyz);
    float3 T = normalize(In.vTangent);
    float3 B = normalize(In.vBinormal);
    float3x3 TBN = float3x3(T, B * -1.f, N);
    float3 vWorldNormal = normalize(mul(vNormalMap, TBN));

#if USE_DETAIL_NORMAL
    float3 vDetail = g_DetailNormalTexture.Sample(g_DefaultSampler, In.vTexcoord * fDetailTiling).xyz;
    vDetail = vDetail * 2.f - 1.f;
    float3 vDetailWorld = normalize(mul(vDetail, TBN));
    vWorldNormal = normalize(vWorldNormal + vDetailWorld * fDetailIntensity);
#endif

    float3 vARD = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord).rgb;
    float fAO = vARD.r;
    float fMetallic = 0.f;

    float fCavity = g_Mask2Texture.Sample(g_DefaultSampler, In.vTexcoord).b;
    fAO = saturate(fAO * lerp(1.f, fCavity, 0.5f));

    float fSheen = saturate(Skin_Sheen);
    float fSheenMask = lerp(0.5f, 1.f, fCavity);
    float fSheenAmt = fSheen * fSheenMask;

    float fRoughness = saturate(vARD.g * fRoughnessFactor);
    fRoughness *= lerp(1.f, SKIN_SHEEN_SCALE, fSheenAmt);
    fRoughness = max(fRoughness, lerp(SKIN_ROUGHNESS_FLOOR, SKIN_SHEEN_FLOOR, fSheenAmt));

    Out.vDiffuse = float4(vAlbedo.rgb, 1.f);
    Out.vNormal = Encode_Normal(vWorldNormal);
    Out.vARM = float4(fAO, fRoughness, fMetallic, 1.f);
    Out.vDepth = Pack_Depth(In.vProjPos);
    Out.vDepth.z = (float) iRimProfile;
    Out.vDepth.w = fRimAmount;
    Out.vEmissive = float4(vDissolveGlow, 0.f);
    return Out;
}
