#ifndef SHADER_BRDF_HLSLI
#define SHADER_BRDF_HLSLI
#include "Shader_Header.hlsli"

float _ndfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (cosLh * cosLh) * (alphaSq - 1.f) + 1.f;
    return alphaSq / (PI * denom * denom);
}

float _schlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.f - k) + k);
}

float _gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.f;
    float k = (r * r) * 0.125f;
    return _schlickG1(cosLi, k) * _schlickG1(cosLo, k);
}

float3 _FresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.f - F0) * pow(saturate(1.f - cosTheta), 5.f);
}

float3 _FresnelSchlickRoughness(float3 F0, float cosTheta, float roughness)
{
    float3 Fr = max(1.0 - roughness, F0);
    return F0 + (Fr - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

void Compute_PBR(
    float3 vNormal, float3 vLook, float3 vLight,
    float3 vAlbedo, float fAO, float fRoughness, float fMetallic,
    float3 radiance,
    out float3 vShade, out float3 vSpecular)
{
    float3 vHalf = normalize(vLight + vLook);
    float cosLo = max(0.f, dot(vNormal, vLook));
    float cosLi = max(0.f, dot(vNormal, vLight));
    float cosLh = max(0.f, dot(vNormal, vHalf));

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), vAlbedo, fMetallic);
    float3 F = _FresnelSchlick(F0, max(0.f, dot(vHalf, vLook)));
    float D = _ndfGGX(cosLh, fRoughness);
    float G = _gaSchlickGGX(cosLi, cosLo, fRoughness);

    float3 kd = (1.f - F) * (1.f - fMetallic);
    float3 vDiffBRDF = kd * vAlbedo / PI;
    float3 vSpecBRDF = (F * D * G) / max(1e-4f, 4.f * cosLi * cosLo);

    vShade = vDiffBRDF * (radiance * PI) * cosLi;
    vSpecular = vSpecBRDF * radiance * cosLi;
}

void Compute_PBR_HalfLambert(
    float3 vNormal, float3 vLook, float3 vLight,
    float3 vAlbedo, float fAO, float fRoughness, float fMetallic,
    float3 radiance,
    out float3 vShade, out float3 vSpecular)
{
    float3 vHalf = normalize(vLight + vLook);
    float cosLo = max(0.f, dot(vNormal, vLook));
    
    float cosLi_hard = max(0.f, dot(vNormal, vLight));
    float cosLh = max(0.f, dot(vNormal, vHalf));
    
    // 디퓨즈에만 하프램버트 ( 반대편 면 ~0.5 채움) . wrap = 0.5 와 유사
    float cosLi_diff = saturate(dot(vNormal, vLight) * 0.5f + 0.5f);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), vAlbedo, fMetallic);
    float3 F = _FresnelSchlick(F0, max(0.f, dot(vHalf, vLook)));
    float D = _ndfGGX(cosLh, fRoughness);
    float G = _gaSchlickGGX(cosLi_hard, cosLo, fRoughness);

    float3 kd = (1.f - F) * (1.f - fMetallic);
    float3 vDiffBRDF = kd * vAlbedo / PI;
    float3 vSpecBRDF = (F * D * G) / max(1e-4f, 4.f * cosLi_hard * cosLo);

    vShade = vDiffBRDF * (radiance * PI) * cosLi_diff;
    vSpecular = vSpecBRDF * radiance * cosLi_hard;
}

#endif