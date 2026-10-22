#include "Shader_Header.hlsli"
#include "Shader_MaterialTextures.hlsli"
#include "Shader_Skinning.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_Emissive.hlsli"
#include "Shader_Dissolve.hlsli"

struct VS_IN
{
    float3 vPosition    : POSITION;
    float3 vNormal      : NORMAL;    
    float2 vTexcoord    : TEXCOORD0;
    float3 vTangent     : TANGENT;
    float3 vBinormal    : BINORMAL;    
    uint4  vBlendIndex  : BLENDINDEX;
    float4 vBlendWeight : BLENDWEIGHT;    
};

struct VS_OUT
{
	float4 vPosition    : SV_POSITION;
	float4 vNormal      : NORMAL;
	float2 vTexcoord    : TEXCOORD0;
	float3 vTangent     : TANGENT;
	float3 vBinormal    : BINORMAL;
    float4 vProjPos     : TEXCOORD1;
    float3 vWorldPos    : TEXCOORD2;
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
    Out.vProjPos  = vPosition;    
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
    float4 vNormal   : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent  : TANGENT;
    float3 vBinormal : BINORMAL;
    float4 vProjPos  : TEXCOORD1;
    float3 vWorldPos : TEXCOORD2;
};

PS_OUT_GBUFFER PS_MAIN(PS_IN In)
{
    PS_OUT_GBUFFER Out;
    
    float4 vAlbedo = pow(g_AlbedoTexture[0].Sample(g_DefaultSampler, In.vTexcoord), 2.2f);
    if (vAlbedo.a < 0.1f)
        discard;
    
    float3 vDissolveGlow = Compute_Dissolve(In.vWorldPos);
    float3 vNormalMap = g_NormalTexture[0].Sample(g_DefaultSampler, In.vTexcoord).xyz;
    float3 vNormal = Apply_NormalMap(vNormalMap, In.vTangent, In.vBinormal, In.vNormal.xyz);    
            
    Out.vDiffuse = vAlbedo;
    Out.vNormal  = Encode_Normal(vNormal);
    Out.vARM     = g_Mask1Texture[0].Sample(g_DefaultSampler, In.vTexcoord);
    Out.vDepth = Pack_Depth(In.vProjPos);
    Out.vDepth.z = (float) iRimProfile;
    Out.vDepth.w = fRimAmount;
    Out.vEmissive = float4(Compute_Emissive(In.vTexcoord) + vDissolveGlow, 1.f);          
    return Out;
}