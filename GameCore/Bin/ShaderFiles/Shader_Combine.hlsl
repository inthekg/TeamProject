#include "Shader_FullScreen.hlsli"

Texture2D g_DiffuseTexture      : register(t20);
Texture2D g_LightTexture        : register(t24);
Texture2D g_EmissiveTexture     : register(t39);


PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    float4 vDiffuse = g_DiffuseTexture.Sample(g_PointSampler, In.vTexcoord);
    if (vDiffuse.a == 0.f)
        discard;
    float3 vLight = g_LightTexture.Sample(g_PointSampler, In.vTexcoord).rgb;
    float3 vEmissive = g_EmissiveTexture.Sample(g_PointSampler, In.vTexcoord).rgb;
    Out.vColor = float4(vLight + vEmissive, 1.f);
    return Out;
}


