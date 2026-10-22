#include "Shader_FullScreen.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_LightCommon.hlsli"
#include "Shader_PostCommon.hlsli"
#include "Shader_Spot.hlsli"
#include "Shader_RenderTuning.hlsli"

Texture2D g_DepthTexture : register(t23);

#define GODRAY_USE_SHADOW 1

static const int    GODRAY_STEPS     = 32;     // 스텝 수 (품질 - 비용 ) 

float3 Get_ViewRayDir(float2 uv)
{
    float4 vClip = float4(uv.x * 2.f - 1.f, uv.y * -2.f + 1.f, 1.f, 1.f);
    float4 vView = mul(vClip, ProjInvMatrix);
    vView /= vView.w;
    float4 vWorld = mul(float4(vView.xyz, 1.f), ViewInvMatrix);
    return normalize(vWorld.xyz - vCamPosition.xyz);
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;

    float3 O = vCamPosition.xyz;
    float3 D = Get_ViewRayDir(In.vTexcoord);

    float4 vDepthDesc = g_DepthTexture.Sample(g_PointSampler, In.vTexcoord);
    float fMarchEnd = g_RenderTuning[0].fGodRayMaxDist;
    if (vDepthDesc.x > 0.f && vDepthDesc.x < 1.f)
    {
        float3 vScenePos = Reconstruct_WorldPos(In.vTexcoord, vDepthDesc);
        fMarchEnd = min(length(vScenePos - O), g_RenderTuning[0].fGodRayMaxDist);
    }

    float fStep = fMarchEnd / GODRAY_STEPS;
    float fDither = IGN(In.vPosition.xy);

    float cosT = dot(normalize(vLight_Direction.xyz), -D);
    float fPhase = Henyey_GreeenStein(cosT, g_RenderTuning[0].fGodRayG);

    float fAccum = 0.f;
    [loop]
    for (int i = 0; i < GODRAY_STEPS; ++i)
    {
        float3 P = O + D * (fStep * (i + fDither));
        float fDist;
        float3 vL = Get_SurfaceToLight(P, fDist);
        float fAtt = Distance_Attenuation(fDist, fLight_Range);

        if (fAtt > 0)
        {
            float fSeg = fAtt * Spot_ConeFactor(vL);
#if GODRAY_USE_SHADOW
#if TAPS == 8
            fSeg *= Compute_Shadow_Spot(P, iShadowIndex, In.vPosition.xy);
#else
            fSeg *= Compute_Shadow_Spot_Hard(P, iShadowIndex, In.vPosition.xy);
#endif
#endif
            fAccum += fSeg;
        }
    }

    fAccum *= fStep * fPhase * g_RenderTuning[0].fGodRayDensity * g_RenderTuning[0].fGodRayIntensity;

    Out.vColor = float4(vLight_Diffuse.rgb * fAccum, 1.f);

    return Out;
}