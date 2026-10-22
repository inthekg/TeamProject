#include "Particle_Header.hlsli"

struct MeshEmitTri
{
    float3 p0;      float cdf;
    float3 p1;      float pad1;
    float3 p2;      float pad2;
    float3 nrm;     float pad3;
};

RWStructuredBuffer<Particle> g_Pool : register(u0);
ConsumeStructuredBuffer<uint> g_DeadList : register(u1);
Buffer<uint> g_DeadCount : register(t0);
StructuredBuffer<float4x4> g_BoneMatrices : register(t1);
StructuredBuffer<MeshEmitTri> g_MeshTris : register(t2);
StructuredBuffer<float> g_MeshCDF : register(t3);
StructuredBuffer<uint> g_MeshIndices : register(t4);
StructuredBuffer<SkinOutVtx> g_SkinVerts : register(t5);
StructuredBuffer<Particle> g_SrcPool : register(t6);
StructuredBuffer<uint> g_SrcAlive : register(t7);
Buffer<uint> g_SrcCount : register(t8);

cbuffer EmitCB : register(b0)
{
    uint        g_spawnCount;   float   g_time;             float   g_lifeMin;      float g_lifeMax;
    float       g_speedMin;     float   g_speedMax;         float   g_sizeMin;      float g_sizeMax;
    float4      g_color;
    float3      g_emitPos;      float   g_shapeRadius;
    float3      g_biasDir;      float   g_biasWeight;
    uint        g_shapeType;    uint    g_edgeEmit;         float   g_startRotMin;  float g_startRotMax;
    float4x4    g_emitRot;
    float3      g_prevEmitPos;  float   g_meshScale;
    float3      g_orbitCenter;  uint    g_meshPinned;
    float3      g_orbitAxis;    uint    g_bConverge;
    uint        g_boneCount;    uint    g_boneInheritRot;   uint    g_frameSeed;       uint  g_meshTriCount;
    uint        g_meshRelease;  uint    g_spawnFromSource;  float   g_dt;              uint  g_arcEnable;
    float3      g_arcAxis;      float   g_arcAngle;
    float3      g_prevEmitPos2; uint    g_splineEnable;
    uint        g_boneAttach;   float3  g_emitPad0;
};

float3 CatmullRomCentripetal(float3 p0, float3 p1, float3 p2, float3 p3, float u)
{
    float t0 = 0.0f;
    float t1 = t0 + sqrt(length(p1 - p0)) + 1e-5f;
    float t2 = t1 + sqrt(length(p2 - p1)) + 1e-5f;
    float t3 = t2 + sqrt(length(p3 - p2)) + 1e-5f;

    float t = t1 + u * (t2 - t1);

    float3 A1 = lerp(p0, p1, (t - t0) / (t1 - t0));
    float3 A2 = lerp(p1, p2, (t - t1) / (t2 - t1));
    float3 A3 = lerp(p2, p3, (t - t2) / (t3 - t2));
    float3 B1 = lerp(A1, A2, (t - t0) / (t2 - t0));
    float3 B2 = lerp(A2, A3, (t - t1) / (t3 - t1));
    return lerp(B1, B2, (t - t1) / (t2 - t1));
}

float3 ScrewPoint(float3 A, float3 B, float3 axis, float angle, float t)
{
    float3 vBA = B - A;
    float d = dot(vBA, axis);
    float3 Bp = B - axis * d;
    float3 vMid = (A + Bp) * 0.5f;
    float3 vChord = Bp - A;
    float fTanHalf = tan(angle * 0.5f);
    float3 vPerp = cross(axis, vChord);
    float3 c = vMid + vPerp * (1.0f / (2.0f * fTanHalf));

    float3 rel = A - c;
    float ang = angle * t;
    float ct = cos(ang);
    float st = sin(ang);
    
    float3 vRot = rel * ct + cross(axis, rel) * st + axis * dot(axis, rel) * (1.0f - ct);
    return c + vRot + axis * (t * d);
}

uint PickTriCDF(float rnd)
{
    uint lo = 0;
    uint hi = g_meshTriCount - 1;
    [loop]
    while(lo < hi)
    {
        uint mid = (lo + hi) >> 1;
        if (g_MeshCDF[mid] < rnd)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

uint PickMeshTriangle(float rnd)
{
    uint lo = 0u;
    uint hi = g_meshTriCount - 1u;
    [loop]
    while (lo < hi)
    {
        uint mid = (lo + hi) >> 1u;
        if (g_MeshTris[mid].cdf < rnd)
            lo = mid + 1u;
        else
            hi = mid;
    }
    return lo;
}

void BuildBasis(float3 n, out float3 t, out float3 b)
{
    float3 up = abs(n.y) < 0.999f ? float3(0.f, 1.f, 0.f) : float3(1.f, 0.f, 0.f);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

[numthreads(64, 1, 1)]
void CS_Main(uint3 id : SV_DispatchThreadID)
{
    uint deadCount = g_DeadCount.Load(0);
    
    if (id.x >= min(g_spawnCount, deadCount))
        return;
    
    if (g_spawnFromSource != 0 && g_SrcCount.Load(0) == 0)
        return;
    
    uint slot = g_DeadList.Consume();

    uint seed = slot * 74779640U + asuint(g_time) + id.x * 2654435761U;
    float3 r = hash31(seed);
    float3 r2 = hash31(seed + 7919U);
    
    if (g_meshPinned != 0 && g_meshTriCount > 0)
    {
        uint ti = PickTriCDF(hash11(seed + 3301U));
        float bu = r2.x;
        float bv = r2.y;
        if (bu + bv > 1.f)
        {
            bu = 1.f - bu;
            bv = 1.f - bv;
        }

        uint base = ti * 3u;
        SkinOutVtx sa = g_SkinVerts[g_MeshIndices[base + 0]];
        SkinOutVtx sb = g_SkinVerts[g_MeshIndices[base + 1]];
        SkinOutVtx sc = g_SkinVerts[g_MeshIndices[base + 2]];
        float bw = 1.f - bu - bv;

        float3 pos = sa.pos * bw + sb.pos * bu + sc.pos * bv;
        float3 nrm = normalize(sa.nrm * bw + sb.nrm * bu + sc.nrm * bv);
        float3 ptang, pbitn;
        BuildBasis(nrm, ptang, pbitn);
  
        float fSize = lerp(g_sizeMin, g_sizeMax, r.x);
        
        if (g_meshRelease == 0)
            pos += nrm * 0.01f;
        
        float3 emitVel = float3(0.f, 0.f, 0.f);
        float emitSpeed = 0.f;
        if (g_meshRelease != 0)
        {
            emitSpeed = lerp(g_speedMin, g_speedMax, hash11(seed + 512));
            emitVel = nrm * emitSpeed;
        }
  

        Particle pp;
        pp.Pos = pos;
        pp.Velocity = emitVel;
        pp.Size = fSize;
        pp.Age = 0.f;
        pp.Life = lerp(g_lifeMin, g_lifeMax, r.z);
        pp.Color = g_color;
        pp.Rotation = radians(lerp(g_startRotMin, g_startRotMax, r2.z));
        pp.Padding = float2(hash11(seed + 99U), pp.Size);
        pp.SpawnT = float4(ptang, 0.f);
        pp.SpawnB = float4(pbitn, 0.f);
        pp.RenderVel = emitVel;
        pp.BaseSpeed = emitSpeed;
        pp.OrbitCetner = float3(0.f, 0.f, 0.f);
        pp.OrbitPad = 0.f;
        pp.OrbitAxis = float3(0.f, 1.f, 0.f);
        pp.OrbitAxisPad = 0.f;
        pp.AnchorTri = ti;
        pp.BaryU = bu;
        pp.BaryV = bv;
        pp.bAnchored = (g_meshRelease != 0) ? 0 : 1;

        g_Pool[slot] = pp;
        return;
    }
    
    float3 axis = normalize(g_biasDir + float3(0.f, 0.001f, 0.f));
    
    float3 tdir, bdir;
    BuildBasis(axis, tdir, bdir);
    float phi = r2.y * 6.2831853f;
    
    float fp = max(1e-3f, 1.f - g_biasWeight);
    
    float3 dir;
    
    float3 meshLocalPos = float3(0.f, 0.f, 0.f);
    bool bMeshShape = (g_shapeType == 5 && g_meshTriCount > 0);
    
    if (g_shapeType == 1)
    {
        float cosT = 2.f * pow(r2.x, fp) - 1.f;
        float sinT = sqrt(saturate(1.f - cosT * cosT));
        dir = normalize(tdir * (sinT * cos(phi)) + bdir * (sinT * sin(phi)) + axis * cosT);
    }
    else if (g_shapeType == 0)
    {
        float cosT = pow(r2.x, fp);
        float sinT = sqrt(saturate(1.f - cosT * cosT));
        dir = normalize(tdir * (sinT * cos(phi)) + bdir * (sinT * sin(phi)) + axis * cosT);
    }
    else if (g_shapeType == 2)
    {
        float cosMax = lerp(-1.f, 0.999f, g_biasWeight);
        float ct = (g_edgeEmit != 0) ? cosMax : lerp(1.f, cosMax, r2.x);
        float st = sqrt(saturate(1.f - ct * ct));
        dir = normalize(axis * ct + (tdir * cos(phi) + bdir * sin(phi)) * st);
    }
    else if (g_shapeType == 4)
    {
        float cphi = r2.y * 6.2831853f;
        float3 tdir, bdir;
        BuildBasis(axis, tdir, bdir);
        dir = normalize(tdir * cos(cphi) + bdir * sin(cphi));
    }
    else if (bMeshShape)
    {
        uint ti = PickMeshTriangle(hash11(seed + 3301U));
        MeshEmitTri tri = g_MeshTris[ti];

        float bu = r2.x;
        float bv = r2.y;
        if (bu + bv > 1.f)
        {
            bu = 1.f - bu;
            bv = 1.f - bv;
        }

        meshLocalPos = (tri.p0 + (tri.p1 - tri.p0) * bu + (tri.p2 - tri.p0) * bv) * g_meshScale;
        dir = normalize(tri.nrm);
    }
    else
    {
        float3 jitter = (r2 * 2.f - 1.f) * 0.05f;
        dir = normalize(axis + jitter);
    }
    
    dir = normalize(mul(float4(dir, 0.f), g_emitRot).xyz);
    
    float3 spawnT = normalize(mul(float4(1.f, 0.f, 0.f, 0.f), g_emitRot).xyz);
    float3 spawnB = normalize(mul(float4(0.f, 1.f, 0.f, 0.f), g_emitRot).xyz);
    
    float fSpeed = lerp(g_speedMin, g_speedMax, r.y);
    float fSize = lerp(g_sizeMin, g_sizeMax, r.x);
    
    float fDist = (g_edgeEmit != 0 || g_bConverge != 0) ? g_shapeRadius : g_shapeRadius * r.z;
    
    if (bMeshShape)
        fDist = 0.f;
    
    uint boneAttachIdx = 0u;
    float3 boneLocalOffset = float3(0.f, 0.f, 0.f);
    bool bBoneAttach = false;
    
    float3 emitBase;
    if (g_spawnFromSource != 0)
    {
        uint srcAlive = g_SrcCount.Load(0);
        uint pick = (id.x + g_frameSeed) % srcAlive;
        uint srcSlot = g_SrcAlive[pick];
        float3 srcPos = g_SrcPool[srcSlot].Pos;
        float3 srcVel = g_SrcPool[srcSlot].RenderVel;
        
        float emitT = saturate((id.x + hash11(seed + 4099U)) / max((float) g_spawnCount, 1.f));
        emitBase = srcPos - srcVel * (g_dt * (1.f - emitT));
    }
    else if (g_boneCount > 0)
    {
        uint boneIdx = (id.x + g_frameSeed) % g_boneCount;
        float4x4 boneM = g_BoneMatrices[boneIdx];

        if (g_boneAttach != 0)
        {
            bBoneAttach = true;
            boneAttachIdx = boneIdx;
            boneLocalOffset = g_emitPos + dir * fDist;
        }
        
        if (g_boneInheritRot != 0)
        {
            dir = normalize(mul(float4(dir, 0.f), boneM).xyz);
            spawnT = normalize(mul(float4(spawnT, 0.f), boneM).xyz);
            spawnB = normalize(mul(float4(spawnB, 0.f), boneM).xyz);
        }
        emitBase = mul(float4(g_emitPos, 1.f), boneM).xyz;
    }
    else if (bMeshShape)
    {
        emitBase = mul(float4(meshLocalPos, 1.f), g_emitRot).xyz + g_emitPos;
    }
    else
    {
        float emitT = saturate((id.x + hash11(seed + 4099U)) / max((float) g_spawnCount, 1.f));
        if (g_arcEnable != 0)
            emitBase = ScrewPoint(g_prevEmitPos, g_emitPos, g_arcAxis, g_arcAngle, emitT);
        else if (g_splineEnable != 0)
            emitBase = CatmullRomCentripetal(g_prevEmitPos2, g_prevEmitPos, g_emitPos, g_emitPos, emitT);
        else
            emitBase = lerp(g_prevEmitPos, g_emitPos, emitT);
    }
    
    float3 velDir = (g_bConverge != 0) ? -dir : dir;
    
    Particle p;
    p.Pos = emitBase + dir * fDist;
    p.Velocity = velDir * fSpeed;
    p.Size = fSize;
    p.Age = 0.f;
    p.Life = lerp(g_lifeMin, g_lifeMax, r.z);
    p.Color = g_color;
    p.Rotation = radians(lerp(g_startRotMin, g_startRotMax, r2.z));
    p.Padding = float2(hash11(seed + 99), fSize);
    p.SpawnT = float4(spawnT, 0.f);
    p.SpawnB = float4(spawnB, 0.f);
     
    p.OrbitCetner = emitBase + (g_orbitCenter - g_emitPos);
    p.OrbitPad = 0.f;
    
    p.OrbitAxis = g_orbitAxis;
    p.OrbitAxisPad = 0.f;
    
    p.RenderVel = p.Velocity;
    p.BaseSpeed = fSpeed;
    
    if (bBoneAttach)
    {
        p.Pos = mul(float4(boneLocalOffset, 1.f), g_BoneMatrices[boneAttachIdx]).xyz;
        p.RenderVel = float3(0.f, 0.f, 0.f);
        p.AnchorTri = boneAttachIdx;
        p.BaryU = boneLocalOffset.x;
        p.BaryV = boneLocalOffset.y;
        p.OrbitPad = boneLocalOffset.z;
        p.bAnchored = 2u;
    }
    else
    {
        p.AnchorTri = 0;
        p.BaryU = 0.f;
        p.BaryV = 0.f;
        p.bAnchored = 0;
    }
    
    g_Pool[slot] = p;
}
