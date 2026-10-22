#include "Particle_Header.hlsli"

AppendStructuredBuffer<uint> g_DeadList : register(u0);
RWStructuredBuffer<Particle> g_Pool : register(u1);

cbuffer SimCB : register(b0)
{
    float   g_dt;
    float3  g_gravity;
    uint    g_maxCount;
    float3  fPadding;
};

[numthreads(128, 1, 1)]
void CS_Main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_maxCount)
        return;

    g_Pool[id.x].Life = 0.f;
    g_DeadList.Append(id.x);
}
