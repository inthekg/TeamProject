#include "Shader_FullScreen.hlsli"
#include "Shader_RenderTuning.hlsli"

Texture2D g_Texture : register(t0);

// 멀리 있는 작은 픽셀 반짝임 억제용 휘도 상한.
// 낮출 수록 반짝임이 줄어들지만, 과하게 낮추면 밝은 부위 블룸이 저하됨
static const float BLOOM_MAX_LUM = 25.f;

float Bloom_Luminance(float3 vColor)
{
    return dot(vColor, float3(0.2126f, 0.7152f, 0.0722f));
}

// 평균 가중치 -> 밝은 샘플일수록 가중치를 낮춰 과도한 반짝임을 억제함.
float KarisWeight(float3 vColor)
{
    return 1.f / (1.f + Bloom_Luminance(vColor));
}

// 다운샘플 필터
float3 Bloom_Downsample13(float2 vUV, float2 vTexel, bool bKaris)
{
    float3 a = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(-2, -2)).rgb;
    float3 b = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(0, -2)).rgb;
    float3 c = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(2, -2)).rgb;

    float3 d = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(-2, 0)).rgb;
    float3 e = g_Texture.Sample(g_LinearClampSampler, vUV).rgb;
    float3 f = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(2, 0)).rgb;

    float3 g = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(-2, 2)).rgb;
    float3 h = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(0, 2)).rgb;
    float3 i = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(2, 2)).rgb;

    float3 j = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(-1, -1)).rgb;
    float3 k = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(1, -1)).rgb;
    float3 l = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(-1, 1)).rgb;
    float3 m = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(1, 1)).rgb;

    if (bKaris)
    {
        // 5개 4-tap 박스로 나눈 뒤 각 박스에 Karis 평균 가중치를 적용
        float3 b0 = (j + k + l + m) * 0.25f; // 중앙
        float3 b1 = (a + b + d + e) * 0.25f; // 좌상
        float3 b2 = (b + c + e + f) * 0.25f; // 우상
        float3 b3 = (d + e + g + h) * 0.25f; // 좌하
        float3 b4 = (e + f + h + i) * 0.25f; // 우하

        float w0 = KarisWeight(b0) * 0.5f;
        float w1 = KarisWeight(b1) * 0.125f;
        float w2 = KarisWeight(b2) * 0.125f;
        float w3 = KarisWeight(b3) * 0.125f;
        float w4 = KarisWeight(b4) * 0.125f;

        float3 vNum = b0 * w0 + b1 * w1 + b2 * w2 + b3 * w3 + b4 * w4;
        float fDen = w0 + w1 + w2 + w3 + w4;
        return vNum / max(fDen, 1e-4f);
    }

    // 표준 13-tap 가중합 (합계 = 1)
    float3 vResult = e * 0.125f;
    vResult += (a + c + g + i) * 0.03125f;
    vResult += (b + d + f + h) * 0.0625f;
    vResult += (j + k + l + m) * 0.125f;
    return vResult;
}

// 업샘플 필터
float3 Bloom_UpsampleTent(float2 vUV, float2 vTexel)
{
    float3 s = g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(-1, -1)).rgb * 1.f;
    s += g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(0, -1)).rgb * 2.f;
    s += g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(1, -1)).rgb * 1.f;

    s += g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(-1, 0)).rgb * 2.f;
    s += g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(0, 0)).rgb * 4.f;
    s += g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(1, 0)).rgb * 2.f;

    s += g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(-1, 1)).rgb * 1.f;
    s += g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(0, 1)).rgb * 2.f;
    s += g_Texture.Sample(g_LinearClampSampler, vUV + vTexel * float2(1, 1)).rgb * 1.f;

    return s * (1.f / 16.f);
}

PS_OUT PS_PREFILTER(PS_IN In)
{
    PS_OUT Out;
    float2 vDim;
    g_Texture.GetDimensions(vDim.x, vDim.y);
    float2 vTexel = 1.f / vDim;

    float3 vColor = Bloom_Downsample13(In.vTexcoord, vTexel, true);
    
    float fLumRaw = Bloom_Luminance(vColor);
    if (fLumRaw > BLOOM_MAX_LUM)
        vColor *= BLOOM_MAX_LUM / fLumRaw;
    
    float fThr = g_RenderTuning[0].fBloomThreshold;
    
    float fContrib = 1.f;
    if (fThr > 0.f)
    {
        float fLum = Bloom_Luminance(vColor);
        float fKnee = g_RenderTuning[0].fBloomKnee;
        float fSoft = clamp(fLum - fThr + fKnee, 0.f, 2.f * fKnee);
        fSoft = (fSoft * fSoft) / (4.f * fKnee + 1e-4f);
        fContrib = max(fSoft, fLum - fThr) / max(fLum, 1e-4f);
    }
    
    Out.vColor = float4(vColor * fContrib, 1.f);
    return Out;
}

PS_OUT PS_DOWNSAMPLE(PS_IN In)
{
    PS_OUT Out;
    
    float2 vDim;
    g_Texture.GetDimensions(vDim.x, vDim.y);
    
    float2 vTexel = 1.f / vDim;
    
    Out.vColor = float4(Bloom_Downsample13(In.vTexcoord, vTexel, false), 1.f);
    
    return Out;
}

PS_OUT PS_UPSAMPLE(PS_IN In)
{
    PS_OUT Out;
    
    float2 vDim;
    g_Texture.GetDimensions(vDim.x, vDim.y);
    float2 vTexel = 1.f / vDim;

    float3 vColor = Bloom_UpsampleTent(In.vTexcoord, vTexel);
    Out.vColor = float4(vColor, 1.f);
    
    return Out;
}

PS_OUT PS_COMPOSITE(PS_IN In)
{
    PS_OUT Out;
    float3 vBloom = g_Texture.Sample(g_LinearClampSampler, In.vTexcoord).rgb;
    Out.vColor = float4(vBloom * g_RenderTuning[0].fBloomIntensity, 0.f);
    return Out;
}
