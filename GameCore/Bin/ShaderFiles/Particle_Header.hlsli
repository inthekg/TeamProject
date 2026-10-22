
struct Particle
{
    float3  Pos;
    float   Size;
    float3  Velocity;
    float   Age;
    float4  Color;
    float   Life;
    float   Rotation;
    float2  Padding;
    float4  SpawnT;
    float4  SpawnB;
    float3  RenderVel;
    float   BaseSpeed;
    float3  OrbitCetner;
    float   OrbitPad;
    float3  OrbitAxis;
    float   OrbitAxisPad;
    uint    AnchorTri;
    float   BaryU;
    float   BaryV;
    uint    bAnchored;
};

struct SkinOutVtx
{
    float3  pos;    float   pad0;
    float3  nrm;    float   pad1;
};

float hash11(uint n)
{
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    
    return float(n & 0x7fffffffU) / float(0x7fffffff);
}

float3 hash31(uint n)
{
    return float3(hash11(n), hash11(n + 1U), hash11(n + 2U));
}