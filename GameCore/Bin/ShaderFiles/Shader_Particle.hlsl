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
Texture2D   g_DepthTex          : register(t23);
Texture2D   g_SceneTex          : register(t10);
Texture2D   g_DistortNoiseTex   : register(t11);
Texture2D   g_EmisATex          : register(t12);

SamplerState    g_LinearSampler : register(s0);
SamplerState    g_ClampSampler  : register(s2);

struct VS_OUT
{
    float4  pos : SV_POSITION;
    float4  color : COLOR;
    float4  projPos : TEXCOORD1;
    float   normAge : TEXCOORD2;
    float2 localUV : TEXCOORD0;
    float2 cellOrg : TEXCOORD3;
};

float2 ResolveCellUV(float2 vLocal, float2 vCellOrg, float2 vCellScale, bool bWrap)
{
    if (g_bFlipbook == 0)
        return vLocal;
    vLocal = bWrap ? frac(vLocal) : saturate(vLocal);
    return (vCellOrg + vLocal) * vCellScale;
}

static const float2 QUAD[4] =
{
    float2(-1, -1), float2(-1, 1), float2(1, -1), float2(1, 1)
};

static const float2 UVS[4] =
{
    float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
};

VS_OUT VS_Main(uint vid : SV_VertexID, uint iId : SV_InstanceID)
{
#ifdef USE_ALIVE_LIST
    uint slot = g_AliveList[iId];
#else
    uint slot = iId;
#endif
    
    Particle p = g_Pool[slot];
    
    float2 c = QUAD[vid];
    c.x -= (2.f * g_vPivot.x - 1.f);
    c.y -= (1.f - 2.f * g_vPivot.y);
    
    float3 wp;
    
    float3 pos = p.Pos;
    float3 vel = p.RenderVel;
    
    if (g_bLocalSpace != 0)
    {
        pos = mul(float4(p.Pos, 1.f), g_matEmitterWorld).xyz;
        vel = mul(float4(p.RenderVel, 0.f), g_matEmitterWorld).xyz;
    }
    
    float speed = length(vel);
    
    float fNormAge = saturate(p.Age / max(p.Life, 1e-5f));
    
    float2 mscale = g_vMeshScale.xy;
    if (g_bScale3D != 0)
    {
        float ft = fNormAge * 63.f;
        uint i0 = (uint) ft;
        uint i1 = min(i0 + 1, 63);
        mscale *= lerp(g_Scale3DLUT[i0], g_Scale3DLUT[i1], ft - i0).xy;
    }
    
    if (g_iAlignment == 1 && speed > 1e-4f)
    {
        float3 lenAxis = vel / speed;
        float3 viewDir = normalize(cross(g_CamRight.xyz, g_CamUp.xyz));
        float3 widAxis = cross(lenAxis, viewDir);
        float wlen = length(widAxis);
        widAxis = (wlen > 1e-4f) ? widAxis / wlen : g_CamRight.xyz;
        
        float rs, rco;
        sincos(p.Rotation, rs, rco);
        float3 rotWid = widAxis * rco - lenAxis * rs;
        float3 rotLen = widAxis * rs + lenAxis * rco;
        widAxis = rotWid;
        lenAxis = rotLen;
        
        float halfLen = (p.Size + speed * g_fStretchScale) * 0.5f * mscale.y;
        float halfWid = p.Size * 0.5f * mscale.x;
        wp = pos + widAxis * (c.x * halfWid) + lenAxis * (c.y * halfLen);
    }
    else if (g_iAlignment == 3)
    {
        float2 sc = c * mscale;
        float s, co;
        sincos(p.Rotation, s, co);
        float2 rc = float2(sc.x * co - sc.y * s, sc.x * s + sc.y * co) * p.Size;
        
        if (g_bLocalSpace != 0)
        {
            float3 localCorner = p.Pos + float3(rc, 0.f);
            wp = mul(float4(localCorner, 1.f), g_matEmitterWorld).xyz;
        }
        else
        {
            float3 T = p.SpawnT.xyz;
            float3 B = p.SpawnB.xyz;
            wp = pos + rc.x * T + rc.y * B;
        }
    }
    else if (g_iAlignment == 4)
    {
        float3 radial = pos - g_vEmitOrigin;
        float rlen = length(radial);
        float3 lenAxis = (rlen > 1e-4f) ? radial / rlen : g_CamRight.xyz;
        float3 viewDir = normalize(cross(g_CamRight.xyz, g_CamUp.xyz));
        float3 widAxis = cross(lenAxis, viewDir);
        float wlen = length(widAxis);
        widAxis = (wlen > 1e-4f) ? widAxis / wlen : g_CamRight.xyz;
        float halfLen = (p.Size + rlen * g_fStretchScale) * 0.5f * mscale.y;
        float halfWid = p.Size * 0.5f * mscale.x;
        wp = pos + widAxis * (c.x * halfWid) + lenAxis * (c.y * halfLen);
    }
    else if (g_iAlignment == 5)
    {
        float3 up = float3(0.f, 1.f, 0.f);
        float3 toCam = g_CamPos.xyz - pos;
        toCam.y = 0.f; 
        float l = length(toCam);
        float3 right = (l > 1e-4f) ? normalize(cross(toCam, up)) : g_CamRight.xyz;
        float2 sc = c * mscale * p.Size;
        wp = pos + right * sc.x + up * sc.y;
    }
    else
    {
        float2 sc = c * mscale;
        float s, co;
        sincos(p.Rotation, s, co);
        float2 rc = float2(sc.x * co - sc.y * s, sc.x * s + sc.y * co);
        wp = pos + (rc.x * g_CamRight.xyz + rc.y * g_CamUp.xyz) * p.Size;
    }
    
    float2 localUV = UVS[vid];
    float2 cellOrg = float2(0.f, 0.f);
    
    if (g_bFlipbook != 0)
    {
        uint total = max(g_iFlipCols * g_iFlipRows, 1);
        uint frame;

        if (g_iFlipPlayMode == 2)
        {
            if (g_bSeedBased != 0)
                frame = (uint) (hash11(slot) * total);
            else
                frame = g_iFlipFixedFrame;
            frame = frame % total;
        }
        else
        {
            frame = (uint) (p.Age * g_fFrameRate);
            if (g_bSeedBased != 0)
                frame += (uint) (hash11(slot) * total);

            if (g_iFlipPlayMode == 1)
                frame = min(frame, total - 1);
            else
                frame = frame % total;
        }
        
        uint col = frame % g_iFlipCols;
        uint row = frame / g_iFlipCols;
        cellOrg = float2(col, row);
    }
    
    VS_OUT o;
    o.pos = mul(float4(wp, 1.f), g_matVP);
    o.color = p.Color;
    o.projPos = o.pos;
    o.normAge = fNormAge;
    o.localUV = localUV;
    o.cellOrg = cellOrg;
    
    return o;
}

float4 Shade_Particle(VS_OUT In)
{
    float2 cellScale = (g_bFlipbook != 0) ? 1.f / float2(g_iFlipCols, g_iFlipRows) : float2(1.f, 1.f);
    
    float2 luv = In.localUV;
    
    if (g_bUVDistort != 0)
    {
        float2 nA = g_NoiseATex.Sample(g_LinearSampler, In.localUV * g_vNoiseTile.xy + g_vUVPan.xy * g_fTime).rg * 2.f - 1.f;
        float2 nB = g_NoiseBTex.Sample(g_LinearSampler, In.localUV * g_vNoiseTile.zw + g_vUVPan.zw * g_fTime).rg * 2.f - 1.f;
        luv += (nA + nB) * 0.5f * g_fUVDistortStr;
    }
    
    float2 uv = ResolveCellUV(luv, In.cellOrg, cellScale, false);
    
    float4 c;
    
    if (g_CamRight.w > 0.5f)
    {
        float4 texCol = g_DiffuseTex.Sample(g_LinearSampler, uv);
        texCol.rgb = pow(saturate(texCol.rgb), 2.2f);
        texCol.rgb = pow(saturate(texCol.rgb), g_fDiffuseSharpness);
        c = texCol * In.color;
    }
    else
    {
        float2 d = In.localUV * 2.f - 1.f;
        float r = saturate(1.f - dot(d, d));
        float fEdge = lerp(0.9f, 0.02f, saturate(g_fSphereHardness));
        r = smoothstep(0.f, fEdge, r);
        c = float4(In.color.rgb, In.color.a * r);
    }
    
    c.a = pow(saturate(c.a * g_fBaseDensity), g_fBasePower);
    c.rgb *= g_fEmiIntensity;
    
    if (g_bEmissive != 0)
    {
        float2 luvEA = In.localUV * g_vEmisTileA.xy + g_fTime * g_vEmisTileA.zw;
        float2 luvEB = In.localUV * g_vEmisTileB.xy + g_fTime * g_vEmisTileB.zw;
        if (g_vEmisExtra.y > 1e-4f)
        {
            float2 en = g_NoiseATex.Sample(g_LinearSampler, In.localUV + g_vUVPan.xy * g_fTime).rg * 2.f - 1.f;
            luvEA += en * g_vEmisExtra.y;
            luvEB += en * g_vEmisExtra.y;
        }

        float2 uvEAResolved = ResolveCellUV(luvEA, In.cellOrg, cellScale, true);
        float aSample = (g_vEmisExtra.w > 0.5f)
            ? g_EmisATex.Sample(g_LinearSampler, uvEAResolved).r
            : g_DiffuseTex.Sample(g_LinearSampler, uvEAResolved).r;
        float eA = aSample * g_fEmisStrA;
        float eB = g_EmisBTex.Sample(g_LinearSampler, luvEB).r;
        
        float combined = (g_vEmisExtra.z > 0.5f) ? lerp(eA, eB, g_vEmisExtra.x) : (eA + eB);
        float e = pow(saturate(combined), g_fEmisPower) * g_fEmisStr;
        c.rgb += e * g_vEmisTint.rgb;
    }
    
    if (g_bMask != 0)
    {
        float2 muv = In.localUV * g_vMaskTileOffset.xy + g_vMaskTileOffset.zw;
        float m = (g_bMaskClamp != 0)
            ? g_MaskTex.Sample(g_ClampSampler, muv).r
            : g_MaskTex.Sample(g_LinearSampler, muv).r;
        float mp = pow(saturate(m), g_fMaskPower);
       if (g_fLinearOpacity > 0.5f)
            c.a *= mp;
        else
            c *= mp;
    }
    
    float3 burnGlow = float3(0.f, 0.f, 0.f);
    if (g_bDissolve != 0)
    {
        float2 duv = In.localUV * g_vDissolveTiling.xy * g_fDissolveZoom;
        float dn = pow(saturate(g_DissolveTex.Sample(g_LinearSampler, duv).r), g_fDissolvePower);
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
    
    if (g_bSoft != 0)
    {
        float2 suv = In.projPos.xy / In.projPos.w * float2(0.5f, -0.5f) + 0.5f;
        float sceneG = g_DepthTex.SampleLevel(g_LinearSampler, suv, 0).g;
        float sceneViewZ = (sceneG > 0.f) ? sceneG * g_fCamFar : g_fCamFar;
        float partViewZ = In.projPos.w;
        float fade = saturate((sceneViewZ - partViewZ) / max(g_fSoftFade, 1e-4f));
        c.a *= fade;
    }
    
    if (g_bDistort != 0)
    {
        float2 screenUV = In.pos.xy / g_vScreenSize;
        float2 dd = In.localUV * 2.f - 1.f;
        float r = saturate(1.f - dot(dd, dd));
        r = r * r;
        
        float2 dir;
        if (g_bDistortTex != 0)
        {
            float2 nUV = screenUV * g_vDistortTile + g_vDistortPan * g_fTime;
            dir = g_DistortNoiseTex.Sample(g_LinearSampler, nUV).rg * 2.f - 1.f;
        }
        else
            dir = dd;
        
        float2 offset = dir * (g_fDistortStr * c.a * r) / g_vScreenSize;
        offset = clamp(offset, -0.3f, 0.3f);
        
        if (g_bDistortDepth != 0)
        {
            float sceneG = g_DepthTex.SampleLevel(g_LinearSampler, screenUV + offset, 0).g;
            float sceneViewZ = (sceneG > 0.f) ? sceneG * g_fCamFar : g_fCamFar;
            offset *= smoothstep(0.f, 0.5f, sceneViewZ - In.projPos.w);
        }
        
        float aspect = g_vScreenSize.x / g_vScreenSize.y;
        float2 fadeW = float2(0.1f, 0.1f * aspect);
        float2 edgeFade = smoothstep(0.f, fadeW, screenUV) * smoothstep(0.f, fadeW, 1.f - screenUV);
        offset *= edgeFade.x * edgeFade.y;
        
        float2 sUV = screenUV + offset;
        sUV = 1.f - abs(1.f - abs(sUV));

        float3 refr = g_SceneTex.Sample(g_ClampSampler, sUV).rgb;
        c.rgb = (g_iDistortMode == 1) ? (c.rgb + refr) : refr;
    }
    
    c.rgb *= c.a;
    c.rgb += burnGlow;
    
    return c;
}

float4 PS_Main(VS_OUT In) : SV_Target
{
    return Shade_Particle(In);
}

PS_OIT_OUT PS_Main_WBOIT(VS_OUT In)
{
    float4 c = Shade_Particle(In);
    
    float w = OIT_Weight(In.projPos.w);
    
    PS_OIT_OUT o;
    o.vAccum = float4(c.rgb, c.a) * w;
    o.vReveal = c.a;
    return o;
}