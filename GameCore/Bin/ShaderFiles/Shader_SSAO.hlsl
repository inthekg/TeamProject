#include "Shader_FullScreen.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_PostCommon.hlsli"
#include "Shader_RenderTuning.hlsli"

Texture2D   g_NormalTexture     : register(t21);
Texture2D   g_DepthTexture      : register(t23);
Texture2D   g_AOTexture         : register(t28);

static const int   SSAO_SAMPLES  = 16;

float3 ViewPos(float2 uv)
{
    float4 vDepth = g_DepthTexture.Sample(g_PointSampler, uv);
    float3 vWorld = Reconstruct_WorldPos(uv, vDepth);
    return mul(float4(vWorld, 1.f), ViewMatrix).xyz;
}

float3 Kernel(int i)
{
    float a = frac(sin(i * 12.9898f) * 43758.5453f);
    float b = frac(sin(i * 78.2330f) * 43758.5453f);
    float phi = a * 6.2831853f;
    float cosT = b;
    float sinT = sqrt(saturate(1.f - cosT * cosT));
    float3 dir = float3(cos(phi) * sinT, sin(phi) * sinT, cosT);
    float t = (float) i / (float) SSAO_SAMPLES;
    return dir * lerp(0.1f, 1.0f, t * t);
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;

    float4 vDepth = g_DepthTexture.Sample(g_PointSampler, In.vTexcoord);
    if (!(vDepth.x > 0.f && vDepth.x < 1.f))
    {
        Out.vColor = 1.f;
        return Out;
    }

    float3 P = ViewPos(In.vTexcoord);
    float3 Nworld = Decode_Normal(g_NormalTexture.Sample(g_PointSampler, In.vTexcoord));
    float3 N = normalize(mul(float4(Nworld, 0.f), ViewMatrix).xyz);

    float rnd = IGN(In.vPosition.xy) * 6.2831853f;
    float3 rv = float3(cos(rnd), sin(rnd), 0.f);
    float3 T = normalize(rv - N * dot(rv, N));
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);

    float radius = g_RenderTuning[0].fSSAORadius * P.z;
    float bias = g_RenderTuning[0].fSSAOBias * P.z;

    float occlusion = 0.f;
    [loop]
    for (int i = 0; i < SSAO_SAMPLES; ++i)
    {
        float3 samplePos = P + mul(Kernel(i), TBN) * radius;

        float4 clip = mul(float4(samplePos, 1.f), ProjMatrix);
        if (clip.w <= 0.f)
            continue;
        float2 uv = clip.xy / clip.w * float2(0.5f, -0.5f) + 0.5f;
        if (any(uv < 0.f) || any(uv > 1.f))
            continue;

        float sceneZ = ViewPos(uv).z;

        float rangeCheck = smoothstep(0.f, 1.f, radius / max(abs(P.z - sceneZ), 1e-4f));
        occlusion += (sceneZ < samplePos.z - bias ? 1.f : 0.f) * rangeCheck;
    }

    float ao = 1.f - (occlusion / SSAO_SAMPLES) * g_RenderTuning[0].fSSAOStrength;
    ao = pow(saturate(ao), g_RenderTuning[0].fSSAOPower);
    Out.vColor = float4(ao, ao, ao, 1.f);
    return Out;
}

// SSAO 노이즈 제거 
PS_OUT PS_BLUR(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;
    float2 texel;
    g_AOTexture.GetDimensions(texel.x, texel.y);
    texel = 1.f / texel;

    float sum = 0.f;
    [unroll]
    for (int x = -2; x < 2; ++x)
    [unroll]
        for (int y = -2; y < 2; ++y)
            sum += g_AOTexture.Sample(g_PointSampler, In.vTexcoord + float2(x, y) * texel).r;

    float ao = sum / 16.f;
    Out.vColor = float4(ao, ao, ao, 1.f);
    return Out;
}
