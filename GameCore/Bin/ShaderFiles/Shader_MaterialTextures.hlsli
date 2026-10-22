#ifndef SHADER_MATERIALTEXTURES_HLSLI
#define SHADER_MATERIALTEXTURES_HLSLI
#include "Shader_Header.hlsli"


struct MaterialData
{
    float4 EmissiveColor;
    float4 BaseColor_Tint;
    float4 GlassColor;
    float4 DirtyColor;
    float4 EmissiveEyeColor;
    
    float Roughness;
    float Roughness_Factor;
    float Emissive;
    float Specular;
    
    float AO_Pow;
    float Metallic_Factor;
    float RefractionDepthBias;
    float EmissiveMode;
};

StructuredBuffer<MaterialData> g_MaterialList : register(t30);

Texture2D g_AlbedoTexture[TextureMaxStack] : register(t0);
Texture2D g_NormalTexture[TextureMaxStack] : register(t6);
Texture2D g_Mask1Texture[TextureMaxStack] : register(t12);
Texture2D g_Mask2Texture[TextureMaxStack] : register(t18);
Texture2D g_DetailNormalTexture : register(t19);

#endif