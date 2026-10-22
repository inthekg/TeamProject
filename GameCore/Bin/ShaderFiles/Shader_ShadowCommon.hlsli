#ifndef SHADER_SHADOWCOMMON_HLSLI
#define SHADER_SHADOWCOMMON_HLSLI
#include "Shader_Header.hlsli"
#include "Shader_RenderTuning.hlsli"


#define SHADOW_DITHER_ROTATE 0
#define TAPS 8

// iGN - 픽셀별 회전각 PCF 계단을 노이즈로 분산
float Shadow_IGN(float2 vPixel)
{
    return frac(52.9829189f * frac(dot(vPixel, float2(0.06711056f, 0.00583715f))));
}

#if TAPS == 8
// Poisson 디스크 8탭( 반경 ~ 0.7 정규화 ) 회전시켜 사용
static const int SHADOW_TAPS = 8;
static const float2 POISSON8[8] =
{
    float2(-0.7071f, 0.0000f), float2(0.7071f, 0.0000f),
    float2(0.0000f, -0.7071f), float2(0.0000f, 0.7071f),
    float2(-0.5000f, -0.5000f), float2(0.5000f, -0.5000f),
    float2(-0.5000f, 0.5000f), float2(0.5000f, 0.5000f),
};

float Shadow_PCF_Atlas(Texture2D tMap, SamplerState tSamp,
                       float2 vTileUV, float4 vAtlasRect,
                       float fReceiver, float fTileSize, float fFilterTexels, float2 vPixel)
{
    float fTexel = 1.0f / fTileSize;

#if SHADOW_DITHER_ROTATE
    float  fAng = Shadow_IGN(vPixel) * 6.2831853f;
    float2 vRot = float2(cos(fAng), sin(fAng));
#endif

    float fLit = 0.0f;
    [unroll]
    for (int i = 0; i < SHADOW_TAPS; ++i)
    {
        float2 o = POISSON8[i];
#if SHADOW_DITHER_ROTATE
        o = float2(o.x * vRot.x - o.y * vRot.y,
                   o.x * vRot.y + o.y * vRot.x);
#endif
        o *= (fFilterTexels * fTexel);

        float2 vTap = clamp(vTileUV + o, fTexel * 0.5f, 1.0f - fTexel * 0.5f);
        float2 vUV = vAtlasRect.xy + vTap * vAtlasRect.zw;
        fLit += (fReceiver <= tMap.Sample(tSamp, vUV).r) ? 1.0f : 0.0f;
    }
    fLit /= (float) SHADOW_TAPS;
    //return lerp(0.0f, 1.0f, fLit);
    return lerp(1.0f - g_RenderTuning[0].fShadowStrength, 1.0f, fLit);
}

#else
    static const int SHADOW_TAPS = 16;
    static const float2 POISSON16[16] =
    {
        float2(-0.94201624f, -0.39906216f), float2( 0.94558609f, -0.76890725f),
        float2(-0.09418410f, -0.92938870f), float2( 0.34495938f,  0.29387760f),
        float2(-0.91588581f,  0.45771432f), float2(-0.81544232f, -0.87912464f),
        float2(-0.38277543f,  0.27676845f), float2( 0.97484398f,  0.75648379f),
        float2( 0.44323325f, -0.97511554f), float2( 0.53742981f, -0.47373420f),
        float2(-0.26496911f, -0.41893023f), float2( 0.79197514f,  0.19090188f),
        float2(-0.24188840f,  0.99706507f), float2(-0.81409955f,  0.91437590f),
        float2( 0.19984126f,  0.78641367f), float2( 0.14383161f, -0.14100790f),
    };

    float Shadow_PCF_Atlas(Texture2D tMap, SamplerState tSamp,
                           float2 vTileUV, float4 vAtlasRect,
                           float fReceiver, float fTileSize, float fFilterTexels, float2 vPixel)
    {
        float fTexel = 1.0f / fTileSize;

#if SHADOW_DITHER_ROTATE
        float  fAng = Shadow_IGN(vPixel) * 6.2831853f;
        float2 vRot = float2(cos(fAng), sin(fAng));
#endif

        float fLit = 0.0f;
        [unroll]
        for (int i = 0; i < SHADOW_TAPS; ++i)
        {
            float2 o = POISSON16[i];
#if SHADOW_DITHER_ROTATE
            o = float2(o.x * vRot.x - o.y * vRot.y,
                       o.x * vRot.y + o.y * vRot.x);
#endif
            o *= (fFilterTexels * fTexel);

            float2 vTap = clamp(vTileUV + o, fTexel * 0.5f, 1.0f - fTexel * 0.5f);
            float2 vUV = vAtlasRect.xy + vTap * vAtlasRect.zw;
            fLit += (fReceiver <= tMap.Sample(tSamp, vUV).r) ? 1.0f : 0.0f;
        }
        fLit /= (float) SHADOW_TAPS;

        return lerp(1.0f - g_RenderTuning[0].fShadowStrength, 1.0f, fLit);
    }
#endif
#endif
