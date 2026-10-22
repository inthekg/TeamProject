#include "Shader_FullScreen.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_PostCommon.hlsli"
#include "Shader_FogCommon.hlsli"

Texture2D g_DepthTexture : register(t23);


float3 Get_ViewRayDir(float2 vUV)
{
    float4 vClip = float4(vUV.x * 2.f - 1.f, vUV.y * -2.f + 1.f, 1.f, 1.f);
    float4 vView = mul(vClip, ProjInvMatrix);
    vView /= vView.w;
    float4 vWorld = mul(float4(vView.xyz, 1.f), ViewInvMatrix);
    return normalize(vWorld.xyz - vCamPosition.xyz);
}



PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;

    float4 vDepthDesc = g_DepthTexture.Sample(g_PointSampler, In.vTexcoord);
    bool bSky = !(vDepthDesc.x > 0.f && vDepthDesc.x < 1.f);

    float3 vWorldPos = bSky
        ? (vCamPosition.xyz + Get_ViewRayDir(In.vTexcoord) * 100000.f)
        : Reconstruct_WorldPos(In.vTexcoord, vDepthDesc);

    float3 vFogColorOut;
    float fFog = Compute_SceneFog(vWorldPos, vFogColorOut);

    Out.vColor = float4(vFogColorOut, fFog);
    return Out;
}
