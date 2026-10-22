#ifndef SHADER_EMISSIVE_HLSLI
#define SHADER_EMISSIVE_HLSLI
#include "Shader_Header.hlsli"
#include "Shader_MaterialTextures.hlsli"
#include "Shader_RenderTuning.hlsli"


Texture2D g_EmissiveSRVTexture : register(t38);


float3 Compute_Emissive(float2 vUV)
{
    MaterialData tData = g_MaterialList[iMaterialIndex];
    float3 tex = g_EmissiveSRVTexture.Sample(g_DefaultSampler, vUV).rgb;

    float3 Out;
    if (tData.EmissiveMode > 0.5f) 
    {
        float3 vEye = tex.r * tData.EmissiveEyeColor.rgb * tData.EmissiveEyeColor.a;
        float3 vNeck = tex.g * tData.EmissiveColor.rgb * tData.EmissiveColor.a;
        Out = vEye + vNeck;
    }
    else
    {
        Out = tex.rgb * tData.EmissiveColor.rgb * tData.EmissiveColor.a;
    }

    Out *= g_RenderTuning[0].fEmissiveScale * fEmissiveAmount;
    return Out;
}
#endif