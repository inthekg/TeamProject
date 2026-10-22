#include "Shader_Header.hlsli"
#include "Shader_Skinning.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_Dissolve.hlsli"

Texture2D g_AlbedoTexture[TextureMaxStack] : register(t0);
Texture2D g_Mask1Texture[TextureMaxStack] : register(t12);
Texture2D g_Mask2Texture[TextureMaxStack] : register(t18);

cbuffer PerMaterialBuffer : register(b3)
{
    float4 RootColor_Tint;
    float4 TipColor_Tint;
};

static const float HAIR_OPACITY_CLIP = 0.1f;

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

// 맵에 직접 배치된 정적 Hair 메시 입력 데이터입니다.
struct VS_IN_STATIC
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
};

// WORLD0~3 입력을 사용하는 맵 인스턴스 Hair 메시 입력 데이터입니다.
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

struct VS_OUT_UNLIT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

// 메시 종류와 관계없이 동일한 Hair 정점 출력을 생성합니다.
VS_OUT Build_HairVertex(float4 vLocalPosition, float3 vLocalNormal, float3 vLocalTangent, float2 vTexcoord, float4x4 matWorld)
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

// 애니메이션 캐릭터의 Hair 메시를 처리합니다.
VS_OUT VS_MAIN(VS_IN In)
{
    float4x4 BoneMatrix = Compute_BoneMatrix(In.vBlendIndex, In.vBlendWeight);
    float4 vSkinnedPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
    float3 vSkinnedNormal = mul(float4(In.vNormal, 0.f), BoneMatrix).xyz;
    float3 vSkinnedTangent = mul(float4(In.vTangent, 0.f), BoneMatrix).xyz;

    return Build_HairVertex(vSkinnedPosition, vSkinnedNormal, vSkinnedTangent, In.vTexcoord, WorldMatrix);
}

// 맵에 직접 배치된 정적 Hair 메시를 처리합니다.
VS_OUT VS_STATIC(VS_IN_STATIC In)
{
    return Build_HairVertex(float4(In.vPosition, 1.f), In.vNormal, In.vTangent, In.vTexcoord, WorldMatrix);
}

// CPU Visible Matrix 인스턴스 경로의 Hair 메시를 처리합니다.
VS_OUT VS_INSTANCE(VS_IN_INSTANCE In)
{
    float4x4 matInstanceWorld = float4x4(In.vRight, In.vUp, In.vLook, In.vTranslation);
    float4x4 matFinalWorld = mul(matInstanceWorld, WorldMatrix);

    return Build_HairVertex(float4(In.vPosition, 1.f), In.vNormal, In.vTangent, In.vTexcoord, matFinalWorld);
}

VS_OUT_UNLIT VS_MAIN_UNLIT(VS_IN In)
{
    VS_OUT_UNLIT Out;
    float4x4 BoneMatrix = Compute_BoneMatrix(In.vBlendIndex, In.vBlendWeight);
    float4 vPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);

    float4x4 matWVP = mul(mul(WorldMatrix, ViewMatrix), ProjMatrix);
    Out.vPosition = mul(vPosition, matWVP);
    Out.vTexcoord = In.vTexcoord;
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

struct PS_IN_UNLIT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

PS_OUT_GBUFFER PS_MAIN(PS_IN In, bool bIsFront : SV_IsFrontFace)
{
    PS_OUT_GBUFFER Out;

    float fOpacity = g_Mask2Texture[0].Sample(g_DefaultSampler, In.vTexcoord).g;
    if (fOpacity < HAIR_OPACITY_CLIP)
        discard;
    
    float3 vDissolveGlow = Compute_Dissolve(In.vWorldPos);

    float3 vBaseColor = pow(g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord).rgb, 2.2f);

    float3 vASR = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord).rgb;
    float fAO = vASR.r;
    float fRoughness = vASR.b;

    float fRootTip = g_Mask2Texture[0].Sample(g_DefaultSampler, In.vTexcoord).r;
    float3 vTint = lerp(RootColor_Tint.rgb, TipColor_Tint.rgb, fRootTip);
    float3 vAlbedo = vBaseColor * vTint;

    float3 N = normalize(In.vNormal.xyz);
    if (!bIsFront)
        N = -N;

    Out.vDiffuse = float4(vAlbedo, 1.f);
    Out.vNormal = Encode_Normal(N);
    Out.vARM = float4(fAO, fRoughness, 0.f, 0.f);
    Out.vDepth = Pack_Depth(In.vProjPos);
    Out.vDepth.z = 0.f;
    Out.vDepth.w = 0.f;
    Out.vEmissive = float4(vDissolveGlow, 1.f);
    return Out;
}
