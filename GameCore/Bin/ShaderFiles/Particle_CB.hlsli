
/* 파티클 공용 상수버퍼 정의 헤더

- CEffect_Emitter의 RenderCB / CamCB 구조체와 바이트 단위로 일치해야 함.
- Sprite와 Mesh를 공통 관리 하기 위하여 이 헤더를 include해서 사용 */

cbuffer ParticleCameraCB : register(b7)
{
    matrix g_matVP;
    float4 g_CamRight;
    float4 g_CamUp;
    float4 g_CamPos;
}

cbuffer ParticleRenderCB : register(b6)
{
    uint    g_bFlipbook;           uint     g_iFlipCols;        uint    g_iFlipRows;        float g_fFrameRate;
    uint    g_bSeedBased;          uint     g_bSoft;            float   g_fSoftFade;        float g_fCamFar;
    uint    g_iAlignment;          float    g_fStretchScale;    uint    g_iFlipPlayMode;    uint  g_iFlipFixedFrame;
    uint    g_iSpinMode;           float3   g_vSpinAxis;
    float3  g_vMeshScale;          float    g_fBasePower;
    uint    g_bLocalSpace;         float    g_fEmiIntensity;    float   g_fSphereHardness;  float g_fDiffuseSharpness;
    matrix  g_matEmitterWorld;
    
    uint    g_bUVDistort;          float    g_fUVDistortStr;    float   g_fTime;            float g_fBaseDensity;
    float4  g_vUVPan; // (pannelA.x, pannelA.y, pannelB.x, pannelB.y)
    float4  g_vNoiseTile;
    
    uint    g_bMask;               float    g_fMaskPower;       uint    g_bMaskClamp;       float g_fLinearOpacity;
    float4  g_vMaskTileOffset; // (tileX, tileY, offX, offY)
    
    uint    g_bEmissive;           float    g_fEmisStr;         float   g_fEmisStrA;        float g_fEmisPower;
    float4  g_vEmisTileA; // (tileA.x, tileA.y, pannelA.x, pannelA.y)
    float4  g_vEmisTileB; // (tileB.x, tileB.y, pannelB.x, pannelB.y)
    float4  g_vEmisTint;  // rgb + padding
    float4  g_vEmisExtra;
    
    uint    g_bDissolve;           float    g_fDissolveZoom;    float   g_fDissolvePower;   float g_fDissolveEdge;
    float4  g_vDissolveTiling; // (tileX, tileY, edgeIntensity, padding)
    float4  g_vDissolveEdgeColor; // rgb + padding;
    
    uint    g_bFresnel;            float    g_fFresPower;       float   g_fFresOutInt;      float g_fFresInInt;
    float4  g_vFresColor; // rgb + inverseStrength(w)
    
    uint    g_bScale3D;            float3   g_vEmitOrigin;
    uint    g_bDistort;            float    g_fDistortStr;      uint    g_iDistortMode;     uint  g_bDistortDepth;
    float2  g_vScreenSize;         float2   g_vDistortPan;
    float2  g_vDistortTile;        uint     g_bDistortTex;      float   g_distortPad;
    float2  g_vPivot;              float2   g_pivotPad;
}