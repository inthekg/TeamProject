#include "Shader_FullScreen.hlsli"
#include "Shader_PostCommon.hlsli"
#include "Shader_RenderTuning.hlsli"
#include "Shader_PostGlobals.hlsli"

Texture2D g_Texture : register(t0);

VS_OUT VS_MAIN(uint iVertexID : SV_VertexID)
{
    VS_OUT Out;
    float2 vUV = float2((iVertexID << 1) & 2, iVertexID & 2);
    Out.vTexcoord = vUV;
    Out.vPosition = float4(vUV * float2(2.f, -2.f) + float2(-1.f, 1.f), 0.f, 1.f);
    return Out;
}


PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    Out.vColor = g_Texture.Sample(g_DefaultSampler, In.vTexcoord);
    return Out;
}


PS_OUT PS_FINAL(PS_IN In)
{
    PS_OUT Out;

    float4 vColor = g_Texture.Sample(g_PointSampler, In.vTexcoord);

    vColor.rgb *= g_RenderTuning[0].fExposure * fExposureScale;
    vColor.rgb = ACES_Film(vColor.rgb);
    vColor.rgb = pow(vColor.rgb, 1.f / g_RenderTuning[0].fGamma);
    Out.vColor = float4(vColor.rgb, vColor.a);
    return Out;
}
