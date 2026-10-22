#ifndef SHADER_RENDERTUNING_HLSLI
#define SHADER_RENDERTUNING_HLSLI

struct RenderTuning
{
    float fCSMBiasBase;
    float fCSMFilterTexels;
    float fSpotBias;
    float fSpotFilterTexels;

    float fPointBias;
    float fPointFilterTexels;
    float fShadowStrength;
    float fSSAODiffuseMin;

    float3 vAmbientFloor;
    float fAmbientDesat;

    float fSSAORadius;
    float fSSAOBias;
    float fSSAOPower;
    float fSSAOStrength;

    float fSSRMaxDistance;
    float fSSRThickness;
    float fSSRRoughCutoff;
    float fSSRBias;

    float fGodRayG;
    float fGodRayDensity;
    float fGodRayIntensity;
    float fGodRayMaxDist;

    float fExposure;

    float fGamma;

    float fBloomThreshold;
    float fBloomKnee;
    float fBloomIntensity;

    float fEmissiveScale;
    float fLampEmissiveScale;

    int iGlobalFogEnable;

    float fGlassBaseAlpha;
    float fGlassTint;
    float fGlassFresnel;
    float fGlassReflect;
    float fGlassSpec;
    float fGlassDirtAlpha;

    float2 vPad;

    float3 vFogColor;
    float fFogDensity;

    float fFogStart;
    float fFogHeight;
    float fFogHeightFalloff;
    float fFogFeather;
};

StructuredBuffer<RenderTuning> g_RenderTuning : register(t36);
#endif
