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
};

static const float HAIR_OPACITY_CLIP = 0.33f;

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

PS_OUT_GBUFFER PS_MAIN(PS_IN In, bool bIsFront : SV_IsFrontFace)
{
    PS_OUT_GBUFFER Out;

    float fOpacity = g_Mask2Texture[0].Sample(g_DefaultSampler, In.vTexcoord).g;
    if (fOpacity < HAIR_OPACITY_CLIP)
        discard;

    float3 vDissolveGlow = Compute_Dissolve(In.vWorldPos);
    
    float3 vBaseColor = pow(g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord).rgb, 2.2f);
    float3 vAlbedo = vBaseColor * RootColor_Tint.rgb;

    float3 vASR = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord).rgb;
    float fAO = vASR.r;
    float fRoughness = vASR.b;

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
