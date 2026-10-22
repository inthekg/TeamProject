#include "Compute_IBL_Common.hlsli"

RWTexture2D<float2> g_BRDFLut : register(u0);

[numthreads(8, 8, 1)]
void CS_Main(uint3 DTid : SV_DispatchThreadID)
{
    uint W, H;
    g_BRDFLut.GetDimensions(W, H);
    if (DTid.x >= W || DTid.y >= H)
        return;
    
    float NdotV = (DTid.x + 0.5f) / float(W);
    float roughness = (DTid.y + 0.5f) / float(H);
    
    float3 V = float3(sqrt(1.f - NdotV * NdotV), 0.f, NdotV);
    float3 N = float3(0.f, 0.f, 1.f);
    
    float A = 0.f;
    float B = 0.f;
    
    const uint SAMPLE_COUNT = 1024u;
    
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = Importance_SampleGGX(Xi, N, roughness);
        float3 L = normalize(2.f * dot(V, H) * H - V);
        
        float NdotL = saturate(L.z);
        float NdotH = saturate(H.z);
        float VdotH = saturate(dot(V, H));
        
        if(NdotL > 0.f)
        {
            float G = Geometry_Smith(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.f - VdotH, 5.f);
            A += (1.f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    g_BRDFLut[DTid.xy] = float2(A, B) / float(SAMPLE_COUNT);

}