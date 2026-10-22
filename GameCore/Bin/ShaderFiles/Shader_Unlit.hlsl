#include "Shader_Header.hlsli"
#include "Shader_MaterialTextures.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_RenderTuning.hlsli"
#include "Shader_Skinning.hlsli"


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
    float3 vNormal : NORMAL;
    float4 vProjPos : TEXCOORD1;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    
    float4 vPosition = mul(float4(In.vPosition, 1.f), WorldMatrix);
    vPosition = mul(mul(vPosition, ViewMatrix), ProjMatrix);
    
    Out.vPosition = vPosition;
    Out.vProjPos = vPosition;
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), WorldMatrix).xyz);
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

    float4 worldPos = mul(float4(In.vPosition, 1.f), finalWorld);
    float4 viewPos = mul(worldPos, ViewMatrix);
    float4 projPos = mul(viewPos, ProjMatrix);

    Out.vPosition = projPos;
    Out.vProjPos = projPos;
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), finalWorld).xyz);
    return Out;
}

struct VS_IN_ANIM
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    uint4 vBlendIndex : BLENDINDEX;
    float4 vBlendWeight : BLENDWEIGHT;
};

VS_OUT VS_ANIM(VS_IN_ANIM In)
{
    VS_OUT Out;

    float4x4 BoneMatrix = Compute_BoneMatrix(In.vBlendIndex, In.vBlendWeight);
    float4 vPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
    float3 vNormal = mul(float4(In.vNormal, 0.f), BoneMatrix).xyz;

    float4x4 matWVP = mul(mul(WorldMatrix, ViewMatrix), ProjMatrix);
    vPosition = mul(vPosition, matWVP);

    Out.vPosition = vPosition;
    Out.vProjPos = vPosition;
    Out.vNormal = normalize(mul(float4(vNormal, 0.f), WorldMatrix).xyz);
    return Out;
}


struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float4 vProjPos : TEXCOORD1;
};

PS_OUT_GBUFFER PS_MAIN(PS_IN In)
{
    MaterialData mat = g_MaterialList[iMaterialIndex];
    PS_OUT_GBUFFER Out = (PS_OUT_GBUFFER) 0;
    
    if (mat.BaseColor_Tint.a < 0.1f)
        discard;
    
    Out.vDiffuse = float4(mat.BaseColor_Tint.rgb, 1.f);
    Out.vNormal = Encode_Normal(normalize(In.vNormal));
    Out.vARM = float4(1.f, mat.Roughness, mat.Metallic_Factor, 1.f);
    Out.vDepth = Pack_Depth(In.vProjPos);
    Out.vDepth.z = (float) iRimProfile;
    Out.vDepth.w = fRimAmount;
    
    Out.vEmissive = float4(mat.EmissiveColor.rgb * mat.Emissive, 1.f) * g_RenderTuning[0].fLampEmissiveScale;
    return Out;
}
