#ifndef SHADER_SMAADEFINES_HLSLI
#define SHADER_SMAADEFINES_HLSLI
#include "Shader_PostGlobals.hlsli"

#define SMAA_RT_METRICS g_SMAA_RT_METRICS
#define SMAA_PRESET_HIGH 1

#define SMAA_CUSTOM_SL 1
#define SMAATexture2D(tex) Texture2D tex
#define SMAATexturePass2D(tex) tex
#define SMAASampleLevelZero(tex, coord) tex.SampleLevel(g_LinearClampSampler, coord, 0)
#define SMAASampleLevelZeroPoint(tex, coord) tex.SampleLevel(g_PointSampler, coord, 0)
#define SMAASampleLevelZeroOffset(tex, coord, offset) tex.SampleLevel(g_LinearClampSampler, coord, 0, offset)
#define SMAASample(tex, coord) tex.Sample(g_LinearClampSampler, coord)
#define SMAASamplePoint(tex, coord) tex.Sample(g_PointSampler, coord)
#define SMAASampleOffset(tex, coord, offset) tex.Sample(g_LinearClampSampler, coord, offset)
#define SMAA_FLATTEN [flatten]
#define SMAA_BRANCH [branch]
#define SMAATexture2DMS2(tex) Texture2DMS<float4, 2> tex
#define SMAALoad(tex, pos, sample) tex.Load(pos, sample)
#define SMAAGather(tex, coord) tex.Gather(g_LinearClampSampler, coord, 0)


#endif
