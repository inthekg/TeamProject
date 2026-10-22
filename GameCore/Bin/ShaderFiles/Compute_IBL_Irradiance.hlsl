#include "Compute_IBL_Common.hlsli"

TextureCube                 g_SkyTexture    : register(t0);
RWTexture2DArray<float4>    g_Irradiance    : register(u0);
SamplerState                g_LinearSampler : register(s0);

[numthreads(8,8,1)]
void CS_Main(uint3 DTid : SV_DispatchThreadID)
{
    uint W, H, E;
    g_Irradiance.GetDimensions(W, H, E);
    if (DTid.x >= W || DTid.y >= H)
        return;
    
    float2 uv = (DTid.xy + 0.5f) / float2(W, H);
    float3 vNormal = Get_DirectionFromCube(DTid.z, uv);
    
    float3 vUp = abs(vNormal.y) < 0.999f ? float3(0, 1, 0) : float3(0, 0, 1);
    float3 vRight = normalize(cross(vUp, vNormal));
    
    float3 vIrradiance = float3(0.f, 0.f, 0.f);
    float fnrSamples = 0.f;
    const float dPhi = 0.025f;
    const float dTheta = 0.025f;
    
    for (float phi = 0.f; phi < 2.f * PI; phi += dPhi)
    {
        for (float theta = 0.0f; theta < 0.5f * PI; theta += dTheta)
        {
            float3 tangentSample = float3(sin(theta) * cos(phi),
                                          sin(theta) * sin(phi),
                                          cos(theta));
            float3 sampleVec = tangentSample.x * vRight
                             + tangentSample.y * vUp
                             + tangentSample.z * vNormal;

            vIrradiance += g_SkyTexture.SampleLevel(g_LinearSampler, sampleVec, 0).rgb
                        * cos(theta) * sin(theta);
            fnrSamples += 1.0f;
        }
    }

    vIrradiance = PI * vIrradiance / max(fnrSamples, 1.f);
    g_Irradiance[DTid] = float4(vIrradiance, 1.f);
}