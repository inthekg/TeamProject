

cbuffer HiZBuildBuffer : register(b0)
{
    int g_SourceWidth;
    int g_SourceHeight;
    int g_DestinationWidth;
    int g_DestinationHeight;
};

Texture2D<float4> g_DepthSource : register(t0);
Texture2D<float> g_HiZSource : register(t1);
RWTexture2D<float> g_HiZDestination : register(u0);

float ConvertDepth(float fDepth)
{
    // Target_Depth의 미기록 영역은 0이므로 far depth인 1로 변환한다.
    return fDepth > 0.f && fDepth < 1.f ? fDepth : 1.f;
}

[numthreads(8, 8, 1)]
void CS_CopyDepth(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    int2 vDestination = int2(DispatchThreadID.xy);

    if (vDestination.x >= g_DestinationWidth ||
        vDestination.y >= g_DestinationHeight)
        return;

    int2 vSourceSize = int2(g_SourceWidth, g_SourceHeight);
    int2 vDestinationSize = int2(g_DestinationWidth, g_DestinationHeight);

    // Destination texel이 담당하는 정규화 영역을 실제 Source 좌표 범위로 변환한다.
    int2 vSourceBegin = (vDestination * vSourceSize) / vDestinationSize;
    int2 vSourceEnd = ((vDestination + int2(1, 1)) * vSourceSize + vDestinationSize - int2(1, 1)) / vDestinationSize;
    vSourceEnd = min(vSourceEnd, vSourceSize);

    float fMaximumDepth = 0.f;

    // 홀수 해상도에서도 누락이 없도록 최대 3x3 Source 범위를 보수적으로 축소한다.
    [loop]
    for (int y = vSourceBegin.y; y < vSourceEnd.y; ++y)
    {
        [loop]
        for (int x = vSourceBegin.x; x < vSourceEnd.x; ++x)
        {
            float fDepth = ConvertDepth(g_DepthSource.Load(int3(x, y, 0)).x);
            fMaximumDepth = max(fMaximumDepth, fDepth);
        }
    }

    g_HiZDestination[vDestination] = fMaximumDepth;
}

[numthreads(8, 8, 1)]
void CS_Downsample(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    int2 vDestination = int2(DispatchThreadID.xy);

    if (vDestination.x >= g_DestinationWidth ||
        vDestination.y >= g_DestinationHeight)
        return;

    int2 vSourceSize = int2(g_SourceWidth, g_SourceHeight);
    int2 vDestinationSize = int2(g_DestinationWidth, g_DestinationHeight);

    // Destination texel이 담당하는 정규화 영역을 실제 Source mip 좌표 범위로 변환한다.
    int2 vSourceBegin = (vDestination * vSourceSize) / vDestinationSize;
    int2 vSourceEnd = ((vDestination + int2(1, 1)) * vSourceSize + vDestinationSize - int2(1, 1)) / vDestinationSize;
    vSourceEnd = min(vSourceEnd, vSourceSize);

    float fMaximumDepth = 0.f;

    // 홀수 mip에서도 마지막 행과 열을 포함하도록 최대 3x3 범위를 max reduction 한다.
    [loop]
    for (int y = vSourceBegin.y; y < vSourceEnd.y; ++y)
    {
        [loop]
        for (int x = vSourceBegin.x; x < vSourceEnd.x; ++x)
        {
            float fDepth = g_HiZSource.Load(int3(x, y, 0));
            fMaximumDepth = max(fMaximumDepth, fDepth);
        }
    }

    g_HiZDestination[vDestination] = fMaximumDepth;
}

