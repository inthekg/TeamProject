#ifndef SHADER_SHADOWDATA_HLSLI
#define SHADER_SHADOWDATA_HLSLI
#include "Shader_Header.hlsli"

struct ShadowView
{
    float4x4 ViewProj;
    float4   AtlasRect;
};

StructuredBuffer<ShadowView> g_ShadowViews : register(t35);

cbuffer ShadowGlobals : register(b10)
{
    int4   g_SpotInfo;
    int4   g_CSMInfo;
    int4   g_PointInfo;
    int4   g_CurView;
    float4 g_CSMSplitViewZ;
    float4 g_PointLightPoseRange[2];

}

#endif