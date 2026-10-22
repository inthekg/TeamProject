#include "Shader_Header.hlsli"
#include "Shader_MaterialTextures.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_Emissive.hlsli"
#include "Shader_Dissolve.hlsli"

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
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float4 vProjPos : TEXCOORD1;
    float3 vWorldPos : TEXCOORD2;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4 vWorldPos = mul(float4(In.vPosition, 1.f), WorldMatrix);
    Out.vWorldPos = vWorldPos.xyz;

    float4 vPosition = mul(vWorldPos, ViewMatrix);
    vPosition = mul(vPosition, ProjMatrix);

    Out.vPosition = vPosition;
    Out.vProjPos = vPosition;
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

    MaterialData mat = g_MaterialList[iMaterialIndex];
    PS_OUT_GBUFFER Out = (PS_OUT_GBUFFER) 0;

    float4 vAlbedo = g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord);
    if (vAlbedo.a < 0.1f)
        discard;

    float3 vDissolveGlow = Compute_Dissolve(In.vWorldPos);

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

    float3 vNormalMap = g_NormalTexture[0].Sample(g_DefaultSampler, In.vTexcoord).xyz;
    float3 vNormal = Apply_NormalMap(vNormalMap, In.vTangent, In.vBinormal, In.vNormal.xyz);


    Out.vDiffuse = vAlbedo;
    Out.vNormal = Encode_Normal(vNormal);
    Out.vARM = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord);
    //Out.vARM = float4(1.f, mat.Roughness, mat.Metallic_Factor, 1.f);
    Out.vDepth = Pack_Depth(In.vProjPos);
    Out.vDepth.z = (float) iRimProfile;
    Out.vDepth.w = fRimAmount;

    Out.vEmissive = float4(Compute_Emissive(In.vTexcoord) + vDissolveGlow, 1.f);

    return Out;
}
