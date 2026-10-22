#include "Compute_IBL_Common.hlsli"
TextureCube               g_SkyTexture    : register(t0);
RWTexture2DArray<float4>  g_Prefiltered   : register(u0);
SamplerState              g_LinearSampler : register(s0);

cbuffer PrefilterCB : register(b0)
{
    float   fRoughness;
    uint    iMipSize;
    float2  padding;
}

[numthreads(8,8,1)]
void CS_Main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= iMipSize || DTid.y >= iMipSize)
        return;
    
    float2 uv = (DTid.xy + 0.5f) / float(iMipSize);
    float3 N = Get_DirectionFromCube(DTid.z, uv);
    float3 R = N;
    float3 V = N;
    
    const uint SAMPLE_COUNT = 128u;
    float3 prefiltered = float3(0.f, 0.f, 0.f);
    float totalWeight = 0.f;
    
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = Importance_SampleGGX(Xi, N, fRoughness);
        float3 L = normalize(2.f * dot(V, H) * H - V);
        
        float NdotL = saturate(dot(N, L));
        if(NdotL > 0.0f)
        {
            prefiltered += g_SkyTexture.SampleLevel(g_LinearSampler, L, 0).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    
    prefiltered = totalWeight > 0.f ? prefiltered / totalWeight : prefiltered;
    g_Prefiltered[DTid] = float4(prefiltered, 1.f);
}
