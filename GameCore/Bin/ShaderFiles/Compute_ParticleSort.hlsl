#include "Particle_Header.hlsli"

// 알파 파티클 정렬을 위한 Compute Shader
// - AliveList + Pool -> 뷰 공간 깊이를 키로 하여 정렬 한 뒤 SortList로 반환
// - 2048개 이상일 경우 정렬하지 않음.

#define SORT_N 2048
#define SORT_THREADS 1024
#define SORT_SENTINEL 3.402823466e38f

StructuredBuffer<Particle> g_Pool : register(t0);
StructuredBuffer<uint> g_AliveList : register(t1);
Buffer<uint>            g_AliveCount : register(t2);

RWStructuredBuffer<uint> g_SortedList : register(u0);

cbuffer SortCamCB : register(b0)
{
    float4x4 g_matVP;
};

groupshared float gs_key[SORT_N];
groupshared uint gs_idx[SORT_N];

[numthreads(SORT_THREADS, 1, 1)]
void CS_Main(uint3 gtid : SV_GroupThreadID)
{
    uint tid = gtid.x;
    uint aliveCount = g_AliveCount.Load(0);
    
    // 각 스레드가 2개 원소를 적재함 (tid, tid + 1024)
    [unroll]
    for (uint e = 0; e < 2; ++e)
    {
        uint j = tid + e * SORT_THREADS;
        if (j < aliveCount)
        {
            uint idx = g_AliveList[j];
            float viewZ = mul(float4(g_Pool[idx].Pos, 1.f), g_matVP).w;
            gs_key[j] = -viewZ; // 먼 파티클일 수록 키가 작음 -> 앞으로 정렬
            gs_idx[j] = idx;
        }
        else
        {
            gs_key[j] = SORT_SENTINEL; // 유효 범위 밖이면 맨 뒤로 정렬
            gs_idx[j] = 0;
        }
    }

    GroupMemoryBarrierWithGroupSync();
    
    // Bitonic 정렬 (오름차순)
    for (uint k = 2; k <= SORT_N; k <<= 1)
    {
        for (uint step = k >> 1; step > 0; step >>= 1)
        {
            [unroll]
            for (uint e = 0; e < 2; ++e)
            {
                uint i = tid + e * SORT_THREADS;
                uint ixj = i ^ step;
                if (ixj >i)
                {
                    bool ascending = ((i & k) == 0);
                    if ((gs_key[i] > gs_key[ixj]) == ascending)
                    {
                        float tk = gs_key[i];
                        gs_key[i] = gs_key[ixj];
                        gs_key[ixj] = tk;
                        
                        uint ti = gs_idx[i];
                        gs_idx[i] = gs_idx[ixj];
                        gs_idx[ixj] = ti;

                    }
                }
            }
            GroupMemoryBarrierWithGroupSync();
        }

    }
    
    // 정렬된 인덱스 기록
    [unroll]
    for (uint i = 0; i < 2; ++i)
    {
        uint j = tid + i * SORT_THREADS;
        g_SortedList[j] = gs_idx[j];
    }

}
