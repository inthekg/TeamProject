#ifndef COMPUTE_IBL_COMMON_HLSLI
#define COMPUTE_IBL_COMMON_HLSLI


static const float PI = 3.14159265359f;

// 큐브면(face) + 0,1 uv -> 월드 방향
// Irradiance / Prefilter가 출력 방향 N 을 구할 때 사용
float3 Get_DirectionFromCube(uint face, float2 uv)
{
    float2 st = uv * 2.0f - 1.0f;
    float3 dir;
    switch (face)
    {
        case 0: dir = float3( 1.0f, -st.y, -st.x); break; // +X
        case 1: dir = float3(-1.0f, -st.y,  st.x); break; // -X
        case 2: dir = float3( st.x,  1.0f,  st.y); break; // +Y
        case 3: dir = float3( st.x, -1.0f, -st.y); break; // -Y
        case 4: dir = float3( st.x, -st.y,  1.0f); break; // +Z
        default:dir = float3(-st.x, -st.y, -1.0f); break; // -Z
    }
    return normalize(dir);
}

// Hammersley 저불일치 시퀀스
// Prefilter / BRDF LUT의 중요도 샘플 분포
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

//GGX 중요도 샘플 방향
// roughness에 맞춘 half-vector H 를 N 기준 탄젠트 공간에서 샘플

float3 Importance_SampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.f * PI * Xi.x;
    float cosTheta = sqrt((1.f - Xi.y) / (1.f + (a * a - 1.f) * Xi.y));
    float sinTheta = sqrt(1.f - cosTheta * cosTheta);
    
    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    float3 up = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, N));
    float3 biTangent = cross(N, tangent);

    return normalize(tangent * H.x + biTangent * H.y + N * H.z);    
}

//IBL 기하향 (Smith, K = a&2 /2 )
// BRDF LUT 의 split-sum 적분에서 사용 
float Geometry_SchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.f;
    return NdotV / (NdotV * (1.f - k) + k);
}

float Geometry_Smith(float NdotV, float NdotL, float roughness)
{
    return Geometry_SchlickGGX(NdotV, roughness) * Geometry_SchlickGGX(NdotL, roughness);
}

#endif