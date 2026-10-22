#include "Shader_Header.hlsli"
#include "Shader_Skinning.hlsli"
#include "Shader_Dissolve.hlsli"

Texture2D g_Mask1Texture[TextureMaxStack] : register(t12);

static const float EYELASH_CLIP = 0.125f;
static const float3 EYELASH_COLOR = float3(0.f, 0.f, 0.f);

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

// 맵에 직접 배치된 정적 속눈썹 메시 입력 데이터입니다.
struct VS_IN_STATIC
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
};

// WORLD0~3 입력을 사용하는 맵 인스턴스 속눈썹 메시 입력 데이터입니다.
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
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

// 메시 종류와 관계없이 동일한 속눈썹 정점 출력을 생성합니다.
VS_OUT Build_EyelashVertex(float4 vLocalPosition, float2 vTexcoord, float4x4 matWorld)
{
    VS_OUT Out;
    float4 vWorldPosition = mul(vLocalPosition, matWorld);

    Out.vWorldPos = vWorldPosition.xyz;
    Out.vPosition = mul(mul(vWorldPosition, ViewMatrix), ProjMatrix);
    Out.vTexcoord = vTexcoord;
    return Out;
}

// 애니메이션 캐릭터의 속눈썹 메시를 처리합니다.
VS_OUT VS_MAIN(VS_IN In)
{
    float4x4 BoneMatrix = Compute_BoneMatrix(In.vBlendIndex, In.vBlendWeight);
    float4 vSkinnedPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);

    return Build_EyelashVertex(vSkinnedPosition, In.vTexcoord, WorldMatrix);
}

// 맵에 직접 배치된 정적 속눈썹 메시를 처리합니다.
VS_OUT VS_STATIC(VS_IN_STATIC In)
{
    return Build_EyelashVertex(float4(In.vPosition, 1.f), In.vTexcoord, WorldMatrix);
}

// CPU Visible Matrix 인스턴스 경로의 속눈썹 메시를 처리합니다.
VS_OUT VS_INSTANCE(VS_IN_INSTANCE In)
{
    float4x4 matInstanceWorld = float4x4(In.vRight, In.vUp, In.vLook, In.vTranslation);
    float4x4 matFinalWorld = mul(matInstanceWorld, WorldMatrix);

    return Build_EyelashVertex(float4(In.vPosition, 1.f), In.vTexcoord, matFinalWorld);
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float3 vWorldPos : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_Target0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    
    float4 vMask = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord);
    float fOpacity = vMask.r;
    if (fOpacity < EYELASH_CLIP)
        discard;

    float3 vDissolveGlow = Compute_Dissolve(In.vWorldPos);
    
    Out.vColor = float4(EYELASH_COLOR + vDissolveGlow, 1.f);
    return Out;
}
