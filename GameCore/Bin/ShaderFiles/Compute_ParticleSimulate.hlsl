#include "Particle_Header.hlsli"

cbuffer SimCB : register(b0)
{
    float   g_dt;               float3  g_gravity;
    uint    g_maxCount;         uint    g_bColorOverLife;   uint    g_bSizeOverLife;    uint    g_lutSize;
    float   g_drag;             float   g_time;             uint    g_iNoise;           uint    g_noiseType;
    float   g_noiseStrength;    float   g_noiseFreq;        float   g_noiseDamping;     float   g_noiseScroll;
    uint    g_bNoiseTex;        uint    g_bRotOverLife;     float2  g_simPad;
    float3  g_vLinearVel;       float   g_fOrbit;
    float3  g_vOrbitCenter;     uint    g_bVelocity;
    float3  g_vOrbitAxis;       uint    g_bSpiral;
    float   g_gravStart;        float   g_gravEnd;          float   g_velSpeedMin;      float   g_velSpeedMax;
    float3  g_vEmitOrigin;      float   g_fSpeedByAngle;
    float   g_noiseOffset;      uint    g_bSizeMultiply;    uint    g_bSpeedOverLife;   float   g_simPad2;
    uint    g_bUseVF;           float   g_vfIntensity;      float   g_vfTightness;      float   g_vfPad0;
    float3  g_vfTileSize;       float   g_vfPad1;
    uint    g_bConverge;        float   g_fArrivalRadius;   uint    g_infinite;         uint    g_colorMode;
    uint    g_sizeMode;         uint    g_rotMode;          uint    g_speedMode;        uint    g_simpad4;
};

float RemapPhase(float raw, uint mode)
{
    if (mode == 1)
        return frac(raw);
    if (mode == 2)
        return 1.f - abs(2.f * frac(raw * 0.5f) - 1.f);
    return raw; 
}

float3 ABCFlow(float3 q)
{
    return float3(sin(q.z) + cos(q.y),
                  sin(q.x) + cos(q.z),
                  sin(q.y) + cos(q.x));
}

float3 fbmABC(float3 q)
{
    float3 f = ABCFlow(q);
    f += 0.50f * ABCFlow(q * 2.03f + 4.7f);
    f += 0.25f * ABCFlow(q * 4.01f + 9.1f);
    f += 0.125f * ABCFlow(q * 8.05f + 1.3f);
    return f;
}

float3 TurbuleceForce(float3 pos, float3 phase)
{
    float3 q = pos * g_noiseFreq + g_time * g_noiseScroll + phase;
    
    if (g_noiseType == 1)
        return ABCFlow(q);
    
    return fbmABC(q);
}

RWStructuredBuffer<Particle> g_Pool : register(u0);
AppendStructuredBuffer<uint> g_DeadList : register(u1);
AppendStructuredBuffer<uint> g_AliveList : register(u2);

StructuredBuffer<float4> g_ColorLUT : register(t0);
StructuredBuffer<float> g_SizeLUT : register(t1);
StructuredBuffer<float> g_RotLUT : register(t2);
StructuredBuffer<float> g_SpeedLUT : register(t4);
StructuredBuffer<SkinOutVtx> g_SkinVerts : register(t5);
StructuredBuffer<uint> g_MeshIndices : register(t6);
StructuredBuffer<float4x4> g_BoneMatrices : register(t7);
Texture3D<float3> g_VectorField : register(t3);
SamplerState g_VFSampler : register(s0);


float4 SampleColorLUT(float t)
{
    float f = saturate(t) * (g_lutSize - 1);
    uint i0 = (uint) f;
    uint i1 = min(i0 + 1, g_lutSize - 1);
    return lerp(g_ColorLUT[i0], g_ColorLUT[i1], f - i0);
}

float SampleSizeLUT(float t)
{
    float f = saturate(t) * (g_lutSize - 1);
    uint i0 = (uint) f;
    uint i1 = min(i0 + 1, g_lutSize - 1);
    return lerp(g_SizeLUT[i0], g_SizeLUT[i1], f - i0);
}

float SampleRotLUT(float t)
{
    float f = saturate(t) * (g_lutSize - 1);
    uint i0 = (uint) f;
    uint i1 = min(i0 + 1, g_lutSize - 1);
    return lerp(g_RotLUT[i0], g_RotLUT[i1], f - i0);
}

float SampleSpeedLUT(float t)
{
    float f = saturate(t) * (g_lutSize - 1);
    uint i0 = (uint) f;
    uint i1 = min(i0 + 1, g_lutSize - 1);
    return lerp(g_SpeedLUT[i0], g_SpeedLUT[i1], f - i0);
}

[numthreads(128, 1, 1)]
void CS_Main(uint3 id : SV_DispatchThreadID)
{
    uint i = id.x;
    if (i >= g_maxCount)
        return;
    
    Particle p = g_Pool[i];
    
    if (p.Life <= 0.f)
        return;
    
    p.Age += g_dt;
    if (g_infinite == 0 && p.Age >= p.Life)
    {
        p.Life = 0.f;
        g_Pool[i] = p;
        g_DeadList.Append(i);
        return;
    }
    
    if (p.bAnchored == 2)
    {
        float rawA = p.Age / max(p.Life, 1e-5f);
        float lifeTaC = (g_infinite != 0) ? RemapPhase(rawA, g_colorMode) : rawA;
        float lifeTaS = (g_infinite != 0) ? RemapPhase(rawA, g_sizeMode) : rawA;
        float lifeTaR = (g_infinite != 0) ? RemapPhase(rawA, g_rotMode) : rawA;

        float3 localOff = float3(p.BaryU, p.BaryV, p.OrbitPad);
        p.Pos = mul(float4(localOff, 1.f), g_BoneMatrices[p.AnchorTri]).xyz;
        p.RenderVel = float3(0.f, 0.f, 0.f);

        if (g_bColorOverLife)
            p.Color = SampleColorLUT(lifeTaC);
        if (g_bSizeOverLife)
        {
            float s = SampleSizeLUT(lifeTaS);
            p.Size = (g_bSizeMultiply != 0) ? (p.Padding.y * s) : s;
        }
        if (g_bRotOverLife)
            p.Rotation = radians(SampleRotLUT(lifeTaR));

        g_Pool[i] = p;
        g_AliveList.Append(i);
        return;
    }

    if (p.bAnchored == 1)
    {
        float rawA = p.Age / max(p.Life, 1e-5f);
        float lifeTaC = (g_infinite != 0) ? RemapPhase(rawA, g_colorMode) : rawA;
        float lifeTaS = (g_infinite != 0) ? RemapPhase(rawA, g_sizeMode) : rawA;
        float lifeTaR = (g_infinite != 0) ? RemapPhase(rawA, g_rotMode) : rawA;
        uint base = p.AnchorTri * 3u;
        SkinOutVtx va = g_SkinVerts[g_MeshIndices[base + 0]];
        SkinOutVtx vb = g_SkinVerts[g_MeshIndices[base + 1]];
        SkinOutVtx vc = g_SkinVerts[g_MeshIndices[base + 2]];
        float w = 1.f - p.BaryU - p.BaryV;

        p.Pos = va.pos * w + vb.pos * p.BaryU + vc.pos * p.BaryV;

        float3 nrm = normalize(va.nrm * w + vb.nrm * p.BaryU + vc.nrm * p.BaryV);
        p.Pos += nrm * 0.01f;
        float3 up = abs(nrm.y) < 0.999f ? float3(0.f, 1.f, 0.f) : float3(1.f, 0.f, 0.f);
        float3 tang = normalize(cross(up, nrm));
        float3 bitn = cross(nrm, tang);
        p.SpawnT = float4(tang, 0.f);
        p.SpawnB = float4(bitn, 0.f);
        p.RenderVel = float3(0.f, 0.f, 0.f);

        if (g_bColorOverLife)
            p.Color = SampleColorLUT(lifeTaC);
        if (g_bSizeOverLife)
        {
            float s = SampleSizeLUT(lifeTaS);
            p.Size = (g_bSizeMultiply != 0) ? (p.Padding.y * s) : s;
        }
        if (g_bRotOverLife)
            p.Rotation = radians(SampleRotLUT(lifeTaR));

        g_Pool[i] = p;
        g_AliveList.Append(i);
        return;
    }
    
    float lifeT = p.Age / max(p.Life, 1e-5f);
    float tColor = (g_infinite != 0) ? RemapPhase(lifeT, g_colorMode) : lifeT;
    float tSize = (g_infinite != 0) ? RemapPhase(lifeT, g_sizeMode) : lifeT;
    float tRot = (g_infinite != 0) ? RemapPhase(lifeT, g_rotMode) : lifeT;
    float tSpeed = (g_infinite != 0) ? RemapPhase(lifeT, g_speedMode) : lifeT;
    float3 oldPos = p.Pos;
    
    float3 vOrbitC = p.OrbitCetner;
    float3 vOrbitA = p.OrbitAxis;
    
    if (g_bSpeedOverLife)
    {
        float curSpd = length(p.Velocity);
        if (curSpd > 1e-4f)
        {
            float target = p.BaseSpeed * SampleSpeedLUT(tSpeed);
            p.Velocity = (p.Velocity / curSpd) * target;
        }
    }
    
    p.Velocity += g_gravity * (lerp(g_gravStart, g_gravEnd, saturate(lifeT)) * g_dt);
    
    if (g_iNoise)
    {
        float damp = 1.f / (1.f + g_noiseDamping * length(p.Velocity));
        float3 phase = (hash31(i) - 0.5f) * (2.f + g_noiseOffset);
        p.Velocity += TurbuleceForce(p.Pos - g_vEmitOrigin, phase) * (g_noiseStrength * damp * g_dt);
    }
    
    if (g_bUseVF)
    {
        float3 vfUVW = (p.Pos - g_vEmitOrigin) / max(g_vfTileSize, 1e-3f);
        float3 vfVec = g_VectorField.SampleLevel(g_VFSampler, vfUVW, 0) * g_vfIntensity;
        p.Velocity += vfVec * g_dt;
        p.Velocity = lerp(p.Velocity, vfVec, saturate(g_vfTightness));
    }
    
    if (g_bVelocity)
    {
        float spd = length(p.Velocity);
        if (spd > 1e-4f)
        {
            float addV = lerp(g_velSpeedMin, g_velSpeedMax, hash11(i + 17U));
            p.Velocity += (p.Velocity / spd) * (addV * g_dt);
        }
        float3 radial = p.Pos - g_vEmitOrigin;
        float rlen = length(radial);
        if (rlen > 1e-4f)
            p.Velocity += (radial / rlen) * (g_fSpeedByAngle * g_dt);
    }
    
    p.Velocity *= saturate(1.f - g_drag * g_dt);
    
    p.Pos += p.Velocity * g_dt;
    
    if (g_bVelocity)
    {
        p.Pos += g_vLinearVel * g_dt;
        
        if (abs(g_fOrbit) > 1e-6f)
        {
            float3 k = normalize(vOrbitA);
            float3 rel = p.Pos - vOrbitC;
            float ang = g_fOrbit * g_dt;
            float s = sin(ang), c = cos(ang);
            rel = rel * c + cross(k, rel) * s + k * dot(k, rel) * (1.f - c);
            p.Pos = vOrbitC + rel;
        }
    }
    
    if (g_bSpiral)
    {
        float3 sAxis = normalize(vOrbitA);
        float3 sUp = abs(sAxis.y) < 0.999f ? float3(0.f, 1.f, 0.f) : float3(1.f, 0.f, 0.f);
        float3 sT = normalize(cross(sUp, sAxis));
        float3 sB = cross(sAxis, sT);
        
        float3 sRnd = hash31(asuint(p.Padding.x) + 101u);
        float sVary = g_iNoise ? g_noiseStrength : 0.f;
        float sRadSpeed = g_fSpeedByAngle * (1.f + (sRnd.y - 0.5f) * sVary * 0.5f);

        float sRad = p.Age * sRadSpeed;
        float sTight = g_fOrbit * (1.f + (sRnd.x - 0.5f) * sVary * 0.5f);
        float sAng = p.Padding.x * 6.2831853f + sRad * sTight;
        
        float3 sp = vOrbitC + (cos(sAng) * sT + sin(sAng) * sB) * sRad;

        if (g_iNoise)
        {
            float3 sPh = (sRnd - 0.5f) * (2.f + g_noiseOffset);
            sp += TurbuleceForce(sp - g_vEmitOrigin, sPh) * (g_noiseStrength * 0.15f * sRad);
        }

        p.Pos = sp;
    }
    
    p.RenderVel = (p.Pos - oldPos) / max(g_dt, 1e-5f);
    
    if (g_bConverge != 0)
    {
        float3 toCenter = p.Pos - g_vEmitOrigin;
        if (dot(toCenter, toCenter) <= g_fArrivalRadius * g_fArrivalRadius)
        {
            p.Life = 0.f;
            g_Pool[i] = p;
            g_DeadList.Append(i);
            return;

        }
    }
    
    if (g_bColorOverLife)
        p.Color = SampleColorLUT(tColor);
    if (g_bSizeOverLife)
    {
        float s = SampleSizeLUT(tSize);
        p.Size = (g_bSizeMultiply != 0) ? (p.Padding.y * s) : s;
    }
    if (g_bRotOverLife)
        p.Rotation = radians(SampleRotLUT(tRot));
    
    g_Pool[i] = p;
    g_AliveList.Append(i);
}
