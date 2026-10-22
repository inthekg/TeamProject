// 메쉬 표현 emit용 정점 스킨 캐시

#include "Particle_Header.hlsli"

#define MAX_SKIN_GROUPS 16

struct SkinInVtx
{
    float3  pos;    uint    meshBoneBase;
    float3  nrm;    uint    groupIndex;
    uint4   bidx;
    float4  bw;
};

StructuredBuffer<SkinInVtx>     g_InVerts   : register(t0);
StructuredBuffer<float4x4>      g_Palette   : register(t1);
RWStructuredBuffer<SkinOutVtx>  g_OutVerts  : register(u0);

cbuffer MeshSkinCB : register(b0)
{
    uint        g_vertexCount;
    uint        g_groupCount;
    uint        g_paletteCapacity;
    uint        g_skinPad;
    float4x4    g_worldMatrix;
    uint4       g_groupBase[MAX_SKIN_GROUPS / 4];
}

[numthreads(64, 1, 1)]
void CS_Main(uint3 id : SV_DispatchThreadID)
{
    uint vi = id.x;
    if (vi >= g_vertexCount)
        return;
    
    SkinInVtx v = g_InVerts[vi];
    
    float w3 = 1.f - (v.bw.x + v.bw.y + v.bw.z);
    
    uint cap = max(g_paletteCapacity, 1);
    uint gi = (g_groupCount > 0) ? min(v.groupIndex, g_groupCount - 1) : 0;
    uint gbase = g_groupBase[gi >> 2][gi & 3];
    uint bo = gbase + v.meshBoneBase;
    uint i0 = min(bo + v.bidx.x, cap - 1);
    uint i1 = min(bo + v.bidx.y, cap - 1);
    uint i2 = min(bo + v.bidx.z, cap - 1);
    uint i3 = min(bo + v.bidx.w, cap - 1);
    
    float4x4 M = g_Palette[i0] * v.bw.x
                + g_Palette[i1] * v.bw.y
                + g_Palette[i2] * v.bw.z
                + g_Palette[i3] * w3;
    
    float3 localPos = mul(float4(v.pos, 1.f), M).xyz;
    float3 localNrm = mul(float4(v.nrm, 0.f), M).xyz;
    
    SkinOutVtx o;
    o.pos = mul(float4(localPos, 1.f), g_worldMatrix).xyz;
    o.nrm = normalize(mul(float4(localNrm, 0.f), g_worldMatrix).xyz);
    o.pad0 = 0.f;
    o.pad1 = 0.f;
    
    g_OutVerts[vi] = o;
}