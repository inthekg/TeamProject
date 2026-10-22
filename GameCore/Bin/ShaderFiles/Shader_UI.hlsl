#include "Shader_Header.hlsli"

Texture2D g_AlbedoTexture[TextureMaxStack] : register(t0);

cbuffer PerUIBuffer : register(b11)
{
    float4 g_UIColor;
    
    float4 g_UIColorLT;
    float4 g_UIColorRT;
    float4 g_UIColorRB;
    float4 g_UIColorLB;

    float4 g_UIUVRect;

    float g_UIFillAmount;
    int g_UIUseTexture;
    int g_UIBlendMode;
    
    float g_UIRevealProgress;
    float g_UIRevealSoftness;
    float2 g_UIRevealCenter;

    int g_UIRevealType;
}

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4 vPosition = mul(float4(In.vPosition, 1.f), WorldMatrix);
    vPosition = mul(vPosition, ViewMatrix);
    vPosition = mul(vPosition, ProjMatrix);

    Out.vPosition = vPosition;
    Out.vTexcoord = In.vTexcoord;

    return Out;
}

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

// 현재 픽셀의 Reveal 표시 비율을 계산한다.
// 반환값 0은 완전히 숨김, 1은 완전히 표시를 의미한다.
float Calculate_UIRevealMask(float2 vLocalUV)
{
    if (g_UIRevealType == 0)
        return 1.f;

    float fProgress = saturate(g_UIRevealProgress);

    // 진행률 양 끝에서는 경계 오차 없이 완전히 숨기거나 표시한다.
    if (fProgress <= 0.f)
        return 0.f;

    if (fProgress >= 1.f)
        return 1.f;

    float fSoftness = max(g_UIRevealSoftness, 0.0001f);
    float fDistance = 0.f;

    if (g_UIRevealType == 1)
    {
        // RADIAL: 지정한 중심에서 가장 먼 모서리까지의 거리를 1로 정규화한다.
        float fCornerLT = length(float2(0.f, 0.f) - g_UIRevealCenter);
        float fCornerRT = length(float2(1.f, 0.f) - g_UIRevealCenter);
        float fCornerRB = length(float2(1.f, 1.f) - g_UIRevealCenter);
        float fCornerLB = length(float2(0.f, 1.f) - g_UIRevealCenter);

        float fMaxDistance = max(max(fCornerLT, fCornerRT), max(fCornerRB, fCornerLB));
        fDistance = length(vLocalUV - g_UIRevealCenter) / max(fMaxDistance, 0.0001f);
    }
    else if (g_UIRevealType == 2)
    {
    // DIAGONAL: DirectX UI 좌표 기준 우측 상단에서 좌측 하단으로 펼쳐진다.
        fDistance = ((1.f - vLocalUV.x) + vLocalUV.y) * 0.5f;
    }
    else if (g_UIRevealType == 3)
    {
    // DISSOLVE: Material의 두 번째 BASE_COLOR 텍스처를 노이즈 마스크로 사용한다.
    // 노이즈는 아틀라스 UV가 아니라 UI 전체의 0~1 로컬 UV로 샘플링한다.
        fDistance = g_AlbedoTexture[1].Sample(g_DefaultSampler, vLocalUV).r;
    }
    else
    {
        return 1.f;
    }

    // 진행 경계에 Softness를 적용해 딱 잘리는 현상을 줄인다.
    return 1.f - smoothstep(fProgress, min(fProgress + fSoftness, 1.f), fDistance);
}

PS_OUT PS_MAIN(VS_OUT In)
{
    PS_OUT Out;

    if (In.vTexcoord.x > g_UIFillAmount)
        discard;
    
    float2 vUV = lerp(g_UIUVRect.xy, g_UIUVRect.zw, In.vTexcoord);

    float4 vTextureColor = float4(1.f, 1.f, 1.f, 1.f);

    if (g_UIUseTexture != 0)
        vTextureColor = g_AlbedoTexture[0].Sample(g_DefaultSampler, vUV);
    
    float4 vTopColor = lerp(g_UIColorLT, g_UIColorRT, In.vTexcoord.x);
    float4 vBottomColor = lerp(g_UIColorLB, g_UIColorRB, In.vTexcoord.x);
    float4 vVertexColor = lerp(vTopColor, vBottomColor, In.vTexcoord.y);

    float4 vFinalColor = vTextureColor * g_UIColor * vVertexColor;
    
    // Reveal 계산에는 아틀라스 변환 전의 UI 로컬 UV를 사용한다.
    float fRevealMask = Calculate_UIRevealMask(In.vTexcoord);

    if (fRevealMask <= 0.001f)
        discard;

    // 경계 부분은 알파값으로 부드럽게 표시한다.
    vFinalColor.a *= fRevealMask;
    
    if (g_UIBlendMode == 1)
    {
        float fBrightness = max(max(vFinalColor.r, vFinalColor.g), vFinalColor.b);
        
        if (fBrightness < 0.03f)
            discard;
    }
    
    //if (vFinalColor.a < 0.1f)
    //    discard;

    Out.vColor = vFinalColor;
    
    return Out;
}