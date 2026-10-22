#include "Shader_Header.hlsli"
#include "Shader_MaterialTextures.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_Emissive.hlsli"


struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
};

struct MAP_INSTANCE_WORLD
{
    float4 vRight;
    float4 vUp;
    float4 vLook;
    float4 vTranslation;
};

StructuredBuffer<MAP_INSTANCE_WORLD> g_MapInstanceWorlds : register(t24);
StructuredBuffer<uint> g_MapVisibleInstanceIndices : register(t25);

cbuffer MapInstanceRenderBuffer : register(b8)
{
    int g_VisibleBase;
    float3 g_MapInstanceRenderPadding;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vProjPos : TEXCOORD1;
};

VS_OUT VS_MAIN(VS_IN In, uint iVisibleInstanceID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;


    // SV_InstanceID는 compact visable 순번으로 원본 instance Index를 한번 더 조회함
    uint iInstanceIndex = g_MapVisibleInstanceIndices[(uint) g_VisibleBase + iVisibleInstanceID];
    MAP_INSTANCE_WORLD Instance = g_MapInstanceWorlds[iInstanceIndex];


    float4x4 instanceWorld = float4x4(
        Instance.vRight,
        Instance.vUp,
        Instance.vLook,
        Instance.vTranslation
    );

    float4x4 finalWorld = mul(instanceWorld, WorldMatrix);

    float4 worldPos = mul(float4(In.vPosition, 1.f), finalWorld);
    float4 viewPos = mul(worldPos, ViewMatrix);
    float4 projPos = mul(viewPos, ProjMatrix);

    Out.vPosition = projPos;
    Out.vProjPos = projPos;
    Out.vTexcoord = In.vTexcoord;

    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), finalWorld).xyz);

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vProjPos : TEXCOORD1;
};


PS_OUT_GBUFFER PS_MAIN(PS_IN In)
{
    MaterialData mat = g_MaterialList[iMaterialIndex];
    PS_OUT_GBUFFER Out = (PS_OUT_GBUFFER) 0;


    float4 vAlbedo = g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord);
    if (vAlbedo.a < 0.1f)
        discard;

    if (iAlphaClip == 1)
    {
        float fOpacityMask = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord).a;
        clip(fOpacityMask - 0.3333f);
    }
    else if (iAlphaClip == 2)
    {
        float fOpacityMask = g_Mask2Texture[0].Sample(g_DefaultSampler, In.vTexcoord).g;
        clip(fOpacityMask - 0.3333f);
    }

    vAlbedo.rgb = pow(vAlbedo.rgb, 2.2f);

    float3 vNormal = normalize(In.vNormal);

    Out.vDiffuse = vAlbedo;
    Out.vNormal = Encode_Normal(vNormal);
    Out.vARM = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord);
    Out.vDepth = Pack_Depth(In.vProjPos);
    Out.vDepth.z = (float) iRimProfile;
    Out.vDepth.w = fRimAmount;

    //Out.vARM = float4(1.f, mat.Roughness, mat.Metallic_Factor, 1.f);
    Out.vEmissive = float4(Compute_Emissive(In.vTexcoord), 1.f);




    return Out;
}
