#include "Particle_Header.hlsli"
#include "Particle_CB.hlsli"
#include "Particle_WBOIT.hlsli"

StructuredBuffer<Particle>  g_Pool          : register(t0);
StructuredBuffer<uint>      g_AliveList     : register(t1);
StructuredBuffer<float3>    g_Scale3DLUT    : register(t3);

Texture2D   g_DiffuseTex        : register(t2);
Texture2D   g_NoiseATex         : register(t4);
Texture2D   g_NoiseBTex         : register(t5);
Texture2D   g_MaskTex           : register(t6);
Texture2D   g_EmisBTex          : register(t7);
Texture2D   g_DissolveTex       : register(t8);
Texture2D   g_CrackTex          : register(t9);
Texture2D   g_SceneTex          : register(t10);
Texture2D   g_DistortNoiseTex   : register(t11);
Texture2D   g_EmisATex           : register(t12);

SamplerState    g_Sampler       : register(s0);
SamplerState    g_ClampSampler  : register(s2);

struct VS_IN
{
    float3 vPos : POSITION;
    float3 vNormal : NORMAL;
    float2 vTex : TEXCOORD;
};

struct VS_OUT
{
    float4 vPos : SV_POSITION;
    float2 vTex : TEXCOORD0;
    float3 vNrm : TEXCOORD1;
    float4 vColor : TEXCOORD2;
    float  normAge : TEXCOORD3;
    float3 worldPos : TEXCOORD4;
    float4 projPos : TEXCOORD5;
};

float4 quat_axis_angle(float3 ax, float a)
{
    float h = a * 0.5f;
    return float4(normalize(ax) * sin(h), cos(h));
}

float4 qmul(float4 a, float4 b)
{
    return float4(a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz),
                    a.w * b.w - dot(a.xyz, b.xyz));
}

float4 quat_look_at(float3 dir)
{
    float3 f = float3(0.f, 0.f, 1.f);
    float len = length(dir);
    if (len < 1e-5f)
        return float4(0.f, 0.f, 0.f, 1.f);
    float3 t = dir / len;
    float d = dot(f, t);
    if (d < -0.9999f)
    {
        float3 o = abs(f.x) < 0.9f ? float3(1.f, 0.f, 0.f) : float3(0.f, 1.f, 0.f);
        return float4(normalize(cross(f, o)), 0.f);
    }
    float3 c = cross(f, t);
    float s = sqrt((1.f + d) * 2.f);
    return float4(c / s, s * 0.5f);
}

float3x3 quat_to_mat3(float4 q)
{
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;
    return float3x3(
        1.f - (yy + zz), xy - wz, xz + wy,
        xy + wz, 1.f - (xx + zz), yz - wx,
        xz - wy, yz + wx, 1.f - (xx + yy));
}
 
VS_OUT VS_Main(VS_IN In, uint iId : SV_InstanceID)
{
    VS_OUT Out;
    
    uint slot = g_AliveList[iId];
    Particle p = g_Pool[slot];
    
    float angle = p.Rotation;
    float seed = p.Padding.x;
    
    float3 axis;
    float4 qBase = float4(0.f, 0.f, 0.f, 1.f);
    if (g_iSpinMode == 2)
    {
        qBase = quat_look_at(p.RenderVel);
        axis = (length(p.RenderVel) > 1e-5f) ? normalize(p.RenderVel) : float3(0.f, 0.f, 1.f);
    }
    else if (g_iSpinMode == 1)
    {
        axis = normalize(hash31(asuint(seed)) * 2.f - 1.f);
    }
    else
    {
        axis = (length(g_vSpinAxis) > 1e-5f) ? normalize(g_vSpinAxis) : float3(0.f, 1.f, 0.f);
    }
    
    float4 q = qmul(qBase, quat_axis_angle(axis, angle));
    float3x3 R = quat_to_mat3(q);
    
    float3 axisScale = float3(1.f, 1.f, 1.f);
    if (g_bScale3D != 0)
    {
        float ft = saturate(p.Age / max(p.Life, 1e-5f)) * 63.f;
        uint i0 = (uint) ft;
        uint i1 = min(i0 + 1, 63);
        axisScale = lerp(g_Scale3DLUT[i0], g_Scale3DLUT[i1], ft - i0);
    }
    
    float3 scl = g_vMeshScale * p.Size * axisScale;
    float3 local = In.vPos * scl;
    
    float3 partLocal = p.Pos + mul(R, local);
    float3 nrm = mul(R, In.vNormal / max(scl, 1e-4f)); // 비균일 스케일 노말 보정
    
    float3 wpos = partLocal;
    float3 wnrm = nrm;
    
    if (g_bLocalSpace != 0)
    {
        wpos = mul(float4(partLocal, 1.f), g_matEmitterWorld).xyz;
        wnrm = mul(float4(nrm, 0.f), g_matEmitterWorld).xyz;
    }
    
    Out.vPos = mul(float4(wpos, 1.f), g_matVP);
    Out.vTex = In.vTex;
    Out.vNrm = wnrm;
    Out.vColor = p.Color;
    Out.normAge = saturate(p.Age / max(p.Life, 1e-5f));
    Out.worldPos = wpos;
    Out.projPos = Out.vPos;
    
    return Out;
}

float4 Shade_MeshParticle(VS_OUT In)
{
    float2 uv = In.vTex;
    
    if (g_bUVDistort != 0)
    {
        float2 nA = g_NoiseATex.Sample(g_Sampler, In.vTex * g_vNoiseTile.xy + g_vUVPan.xy * g_fTime).rg * 2.f - 1.f;
        float2 nB = g_NoiseBTex.Sample(g_Sampler, In.vTex * g_vNoiseTile.zw + g_vUVPan.zw * g_fTime).rg * 2.f - 1.f;
        uv += (nA + nB) * g_fUVDistortStr;
    }
    
    float4 c = In.vColor;
    if (g_CamRight.w > 0.5f)
    {
        float4 texCol = g_DiffuseTex.Sample(g_Sampler, uv);
        texCol.rgb = pow(saturate(texCol.rgb), g_fDiffuseSharpness);
        c *= texCol;
    }
    
    c.a = pow(saturate(c.a * g_fBaseDensity), g_fBasePower);
    c.rgb *= g_fEmiIntensity;
    
    if (g_bEmissive != 0)
    {
        float2 uvEA = In.vTex * g_vEmisTileA.xy + g_fTime * g_vEmisTileA.zw;
        float2 uvEB = In.vTex * g_vEmisTileB.xy + g_fTime * g_vEmisTileB.zw;
        if (g_vEmisExtra.y > 1e-4f)
        {
            float2 en = g_NoiseATex.Sample(g_Sampler, In.vTex + g_vUVPan.xy * g_fTime).rg * 2.f - 1.f;
            uvEA += en * g_vEmisExtra.y;
            uvEB += en * g_vEmisExtra.y;
        }

        float aSample = (g_vEmisExtra.w > 0.5f)
            ? g_EmisATex.Sample(g_Sampler, uvEA).r
            : g_DiffuseTex.Sample(g_Sampler, uvEA).r;
        float eA = aSample * g_fEmisStrA;
        float eB = g_EmisBTex.Sample(g_Sampler, uvEB).r;
        
        float combined = (g_vEmisExtra.z > 0.5f) ? lerp(eA, eB, g_vEmisExtra.x) : (eA + eB);
        float e = pow(saturate(combined), g_fEmisPower) * g_fEmisStr;
        c.rgb += e * g_vEmisTint.rgb;
    }
    
    float3 fresAdd = float3(0.f, 0.f, 0.f);
    if (g_bFresnel != 0)
    {
        float3 N = normalize(In.vNrm);
        float3 V = normalize(g_CamPos.xyz - In.worldPos);
        float ndv = saturate(dot(N, V));
        float fres = pow(1.f - ndv, g_fFresPower);
        float crack = (g_bFresnel == 2) ? g_CrackTex.Sample(g_Sampler, In.vTex).r : 1.f;
        float outline = fres * g_fFresOutInt * crack;
        float inside = (1.f - fres) * g_fFresInInt;
        float invF = pow(ndv, 2.f) * g_vFresColor.w;
        fresAdd = (outline + inside + invF) * g_vFresColor.rgb;
    }
    
    if (g_bMask != 0)
    {
        float2 muv = In.vTex * g_vMaskTileOffset.xy + g_vMaskTileOffset.zw;
        float m = (g_bMaskClamp != 0)
            ? g_MaskTex.Sample(g_ClampSampler, muv).r
            : g_MaskTex.Sample(g_Sampler, muv).r;
        float mp = pow(saturate(m), g_fMaskPower);
        if (g_fLinearOpacity > 0.5f)
            c.a *= mp;
        else
            c *= mp;
    }
    
    float3 burnGlow = float3(0.f, 0.f, 0.f);
    if (g_bDissolve != 0)
    {
        float2 duv = In.vTex * g_vDissolveTiling.xy * g_fDissolveZoom;
        float dn = pow(saturate(g_DissolveTex.Sample(g_Sampler, duv).r), g_fDissolvePower);
        float th = (g_vDissolveEdgeColor.w > 0.5f) ? In.normAge : g_vDissolveTiling.w;
        float edge = max(g_fDissolveEdge, 1e-4f);
        float keep = smoothstep(th - edge, th + edge, dn);
        float band = saturate(1.f - abs(dn - th) / edge);
        burnGlow = band * g_vDissolveTiling.z * g_vDissolveEdgeColor.rgb;
        if (g_fLinearOpacity > 0.5f)
            c.a *= keep;
        else
            c *= keep;
    }
    
    if (g_bDistort != 0)
    {
        float2 screenUV = In.vPos.xy / g_vScreenSize;

        float2 dd = In.vTex * 2.f - 1.f;
        float r = saturate(1.f - dot(dd, dd));

        float2 dir;
        if (g_bDistortTex != 0)
        {
            float2 nUV = screenUV * g_vDistortTile + g_vDistortPan * g_fTime;
            dir = g_DistortNoiseTex.Sample(g_Sampler, nUV).rg * 2.f - 1.f;
        }
        else
            dir = dd;

        float2 offset = dir * (g_fDistortStr * c.a * r) / g_vScreenSize;
        offset = clamp(offset, -0.08f, 0.08f);
        
        float2 sUV = screenUV + offset;
        sUV = 1.f - abs(1.f - abs(sUV));

        float3 refr = g_SceneTex.Sample(g_ClampSampler, sUV).rgb;
        c.rgb = (g_iDistortMode == 1) ? (c.rgb + refr) : refr;
    }
    
    float fresCoverage = c.a;
    c.rgb *= c.a;
    c.rgb += fresAdd * fresCoverage;
    c.rgb += burnGlow;
    
    return c;
}

float4 PS_Main(VS_OUT In) : SV_Target
{
    return Shade_MeshParticle(In);
}

PS_OIT_OUT PS_Main_WBOIT(VS_OUT In)
{
    float4 c = Shade_MeshParticle(In);
    
    float w = OIT_Weight(In.projPos.w);
    
    PS_OIT_OUT o;
    o.vAccum = float4(c.rgb, c.a) * w;
    o.vReveal = c.a;
    return o;
}