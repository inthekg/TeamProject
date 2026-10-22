#include "Shader_FullScreen.hlsli"

// WBOIT Composite Shader

Texture2D g_AccumTexture : register(t0);
Texture2D g_RevealTexture : register(t1);

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
    float fReveal = g_RevealTexture.Sample(g_PointSampler, In.vTexcoord).r;
    
    // 아무 파티클도 안 그려진 픽셀은 배경 그대로 둔다.
    if (fReveal > 1.f - 1e-5f)
        discard;
    
    float4 vAccum = g_AccumTexture.Sample(g_PointSampler, In.vTexcoord);
    
    // 누적 오버플로우 방어
    if (isinf(vAccum.a))
        vAccum.a = max(max(vAccum.r, vAccum.g), vAccum.b);
    if (any(isinf(vAccum.rgb)))
        vAccum.rgb = float3(vAccum.a, vAccum.a, vAccum.a);
    
    // 가중 평균색 복원
    float3 vAvg = vAccum.rgb / max(vAccum.a, 1e-5f);
    
    // 투명 레이어를 배경 위에 얹기
    PS_OUT Out;
    // 평균색과 Coverage를 내보냄.
    Out.vColor = float4(vAvg, 1.f - fReveal);
    return Out;
}