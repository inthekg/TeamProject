struct MAP_INSTANCE_BOUNDS
{
    float3 vCenter;
    float fPadding0;
    float3 vExtents;
    float fPadding1;
};

struct MAP_INSTANCE_RENDER_META
{
    int iGroupID;
    int iVisibleBase;
    int iPadding0;
    int iPadding1;
};

cbuffer MapCullCSBuffer : register(b0)
{
    float4 g_FrustumPlanes[6];
    float4 g_CameraPosition;
    float g_CullDistance;
    int g_CandidateCount;
    float g_DepthBias;
    float g_ScreenMargin;
    float4x4 g_PreviousViewProjection;
    float2 g_HiZTextureSize;
    int g_HiZMipCount;
    int g_UseHiZ;
};

StructuredBuffer<MAP_INSTANCE_BOUNDS> g_InstanceBounds : register(t26);
StructuredBuffer<uint> g_CandidateIndices : register(t27);
StructuredBuffer<MAP_INSTANCE_RENDER_META> g_InstanceMeta : register(t28);
Texture2D<float> g_HiZTexture : register(t29);

RWStructuredBuffer<uint> g_GroupVisibleIndices : register(u0);
RWByteAddressBuffer g_GroupIndirectArgs : register(u1);

static const uint MAP_INDIRECT_ARGS_STRIDE = 20;
static const uint MAP_INDIRECT_INSTANCE_COUNT_OFFSET = 4;

bool IsVisible(MAP_INSTANCE_BOUNDS Bounds)
{
    float3 vCameraToCenter = Bounds.vCenter - g_CameraPosition.xyz;
    float fRadius = length(Bounds.vExtents);
    float fCullDistance = g_CullDistance + fRadius;

    if (dot(vCameraToCenter, vCameraToCenter) > fCullDistance * fCullDistance)
        return false;

    [unroll]
    for (uint i = 0; i < 6; ++i)
    {
        float3 vPlaneNormal = g_FrustumPlanes[i].xyz;
        float fPlaneDistance = dot(vPlaneNormal, Bounds.vCenter) + g_FrustumPlanes[i].w;
        float fPlaneRadius = dot(abs(vPlaneNormal), Bounds.vExtents);

        if (fPlaneDistance < -fPlaneRadius)
            return false;
    }

    return true;
}
bool IsHiZOccluded(MAP_INSTANCE_BOUNDS Bounds)
{
    float2 vMinimumUV = float2(1.f, 1.f);
    float2 vMaximumUV = float2(0.f, 0.f);
    float fNearestDepth = 1.f;

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        float3 vSign = float3((i & 1) ? 1.f : -1.f, (i & 2) ? 1.f : -1.f, (i & 4) ? 1.f : -1.f);
        float3 vCorner = Bounds.vCenter + Bounds.vExtents * vSign;
        float4 vClip = mul(float4(vCorner, 1.f), g_PreviousViewProjection);

        // 이전 프레임 near plane과 교차할 가능성이 있으면 제거하지 않는다.
        if (vClip.w <= 0.0001f)
            return false;

        float3 vNDC = vClip.xyz / vClip.w;
        float2 vUV = float2(vNDC.x * 0.5f + 0.5f, vNDC.y * -0.5f + 0.5f);

        vMinimumUV = min(vMinimumUV, vUV);
        vMaximumUV = max(vMaximumUV, vUV);
        fNearestDepth = min(fNearestDepth, vNDC.z);
    }

    if (fNearestDepth <= 0.f || fNearestDepth >= 1.f)
        return false;

    // 이전 화면 바깥이나 경계에 걸린 객체는 보수적으로 유지한다.
    if (vMinimumUV.x <= g_ScreenMargin || vMinimumUV.y <= g_ScreenMargin || vMaximumUV.x >= 1.f - g_ScreenMargin || vMaximumUV.y >= 1.f - g_ScreenMargin)
        return false;

    float2 vRectPixels = (vMaximumUV - vMinimumUV) * g_HiZTextureSize;
    float fMaximumSize = max(1.f, max(vRectPixels.x, vRectPixels.y));

    // float log2 경계 오차를 피하도록 실제 픽셀 점유 크기를 정수로 올림 처리한다.
    uint iMaximumPixelSpan = max(1u, (uint) ceil(fMaximumSize));
    int iMip = 0;

    // ceil(log2(iMaximumPixelSpan))을 정수 연산으로 계산한다.
    if (iMaximumPixelSpan > 1u)
        iMip = (int) firstbithigh(iMaximumPixelSpan - 1u) + 1;

    iMip = clamp(iMip, 0, g_HiZMipCount - 1);
    uint2 vMipSize = max(uint2(1, 1), (uint2) g_HiZTextureSize >> iMip);

    uint2 vCoord0 = min((uint2) (vMinimumUV * vMipSize), vMipSize - 1);
    uint2 vCoord1 = min((uint2) (float2(vMaximumUV.x, vMinimumUV.y) * vMipSize), vMipSize - 1);
    uint2 vCoord2 = min((uint2) (float2(vMinimumUV.x, vMaximumUV.y) * vMipSize), vMipSize - 1);
    uint2 vCoord3 = min((uint2) (vMaximumUV * vMipSize), vMipSize - 1);

    float fDepth0 = g_HiZTexture.Load(int3(vCoord0, iMip));
    float fDepth1 = g_HiZTexture.Load(int3(vCoord1, iMip));
    float fDepth2 = g_HiZTexture.Load(int3(vCoord2, iMip));
    float fDepth3 = g_HiZTexture.Load(int3(vCoord3, iMip));
    float fHiZDepth = max(max(fDepth0, fDepth1), max(fDepth2, fDepth3));

    return fNearestDepth > fHiZDepth + g_DepthBias;
}


[numthreads(64, 1, 1)]
void CS_Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    int iCandidateID = DispatchThreadID.x;

    if (iCandidateID >= g_CandidateCount)
        return;

    uint iInstanceIndex = g_CandidateIndices[iCandidateID];
    MAP_INSTANCE_BOUNDS Bounds = g_InstanceBounds[iInstanceIndex];

    if (!IsVisible(Bounds))
        return;
    
    if (g_UseHiZ != 0 && IsHiZOccluded(Bounds))
        return;

    MAP_INSTANCE_RENDER_META Meta = g_InstanceMeta[iInstanceIndex];

    uint iVisibleSlot = 0;
    uint iArgsByteOffset = (uint) Meta.iGroupID * MAP_INDIRECT_ARGS_STRIDE;

    // group별 indirect args의 InstanceCount만 증가시킨다.
    g_GroupIndirectArgs.InterlockedAdd(iArgsByteOffset + MAP_INDIRECT_INSTANCE_COUNT_OFFSET, 1, iVisibleSlot);

    // visibleBase + 증가된 slot에 global instance index를 저장한다.
    g_GroupVisibleIndices[(uint) Meta.iVisibleBase + iVisibleSlot] = iInstanceIndex;
}
