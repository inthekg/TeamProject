#include "Renderer.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Allocator.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer.h"
#include "Material.h"
#include "VIBuffer_Rect.h"
#include "Shader_Resolver.h"
#include "IBL_Baker.h"
#include "AreaTex.h"
#include "SearchTex.h"
#include "Decal.h"

#include "ComputeShader.h"


ID3D11ShaderResourceView* CRenderer::m_pNullSRVs[128] = { nullptr };

static const XMFLOAT3 s_PointFaceDir[6] = { { 1.f, 0.f, 0.f}, {-1.f, 0.f, 0.f}, { 0.f, 1.f, 0.f}, { 0.f,-1.f, 0.f}, { 0.f, 0.f, 1.f}, { 0.f, 0.f,-1.f} };
static const XMFLOAT3 s_PointFaceUp[6] = { { 0.f, 1.f, 0.f}, { 0.f, 1.f, 0.f}, { 0.f, 0.f,-1.f}, { 0.f, 0.f, 1.f}, { 0.f, 1.f, 0.f}, { 0.f, 1.f, 0.f} };

static constexpr _uint DECAL_RECEIVER_STENCIL = 0x40;
static constexpr _float BLOOM_SCATTER = 0.7f;

static _wstring Get_TuningPath()
{
    return _wstring(SOLUTION_DIR) + L"GameCore\\Bin\\Resources\\Data\\Shader\\TuningData.json";
}
static void Ensure_TuningDir()
{
    const _wstring strBase = _wstring(SOLUTION_DIR) + L"GameCore\\Bin\\Resources";
    CreateDirectoryW((strBase + L"\\Data").c_str(), nullptr);
    CreateDirectoryW((strBase + L"\\Data\\Shader").c_str(), nullptr);
}

CRenderer::CRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CRenderer::Initialize()
{
    /* Create Allocator */
    m_pAllocator = CAllocator::Create(m_iAllocMemory); // 2MB
    m_pLastCommand = new RenderCommand;

    // SpriteBatch
    m_pUISpriteBatch = new SpriteBatch(m_pContext);
    if (m_pUISpriteBatch == nullptr) return E_FAIL;

    /* Create UI Proj*/
    _uint               iNumViewport = { 1 };
    D3D11_VIEWPORT      ViewportDesc = {};
    m_pContext->RSGetViewports(&iNumViewport, &ViewportDesc);
    XMStoreFloat4x4(&m_OrthoProjMatrix, XMMatrixOrthographicLH(ViewportDesc.Width, ViewportDesc.Height, 0.f, 1.f));

    //Create Constant Buffer
    m_pPerObjectBuffer = CConstantBuffer::Create(m_pDevice, m_pContext, nullptr, sizeof(PerObjectData));
    if (m_pPerObjectBuffer == nullptr) return E_FAIL;
    //고정 상수 버퍼는 한번만 바인딩
    //다른 셰이더(CS 등) 에 바인딩할거면 BindXX 추가해야함
    m_pPerObjectBuffer->BindVS(OBJECT_SLOT);
    m_pPerObjectBuffer->BindPS(OBJECT_SLOT);

    m_pPerCameraBuffer = CConstantBuffer::Create(m_pDevice, m_pContext, nullptr, sizeof(PerCameraData));
    if (m_pPerCameraBuffer == nullptr) return E_FAIL;
    m_pPerCameraBuffer->BindVS(CAMERA_SLOT);
    m_pPerCameraBuffer->BindPS(CAMERA_SLOT);

    //m_pPerMaterialBuffer = CConstantBuffer::Create(m_pDevice, m_pContext, nullptr, m_iMaterialBufferSize);
    //if (m_pPerMaterialBuffer == nullptr) return E_FAIL;
    //m_pPerMaterialBuffer->BindPS(MATERIAL_SLOT);

    m_pPerLightBuffer = CConstantBuffer::Create(m_pDevice, m_pContext, nullptr, sizeof(PerLightData));
    if (m_pPerLightBuffer == nullptr) return E_FAIL;
    m_pPerLightBuffer->BindPS(LIGHT_SLOT);

    //Materials 구조체 버퍼
    CEditableStructuredBuffer::SB_DESC MaterialBufferDesc{};
    MaterialBufferDesc.iElementCount = g_iNumMaxMaterals;
    MaterialBufferDesc.iElementStride = sizeof(MaterialData);
    MaterialBufferDesc.pInitialData = nullptr;

    m_pMaterialsBuffer = CEditableStructuredBuffer::Create(m_pDevice, m_pContext, MaterialBufferDesc);
    if (m_pMaterialsBuffer == nullptr) return E_FAIL;
    m_pMaterialsBuffer->BindPS(SRV_MATERIAL_BUFFER_SLOT);

    m_pUIBuffer = CConstantBuffer::Create(m_pDevice, m_pContext, nullptr, sizeof(PerUIData));
    if (m_pUIBuffer == nullptr) return E_FAIL;
    m_pUIBuffer->BindPS(UI_SLOT);

    CEditableStructuredBuffer::SB_DESC IBLBufferDesc{};
    IBLBufferDesc.iElementCount = 1;
    IBLBufferDesc.iElementStride = sizeof(PerIBLData);
    IBLBufferDesc.pInitialData = nullptr;
    m_pIBLBuffer = CEditableStructuredBuffer::Create(m_pDevice, m_pContext, IBLBufferDesc);
    if (m_pIBLBuffer == nullptr) return E_FAIL;
    m_pIBLBuffer->BindPS(SRV_IBL_BUFFER_SLOT);

    m_pGodRayBlurBuffer = CConstantBuffer::Create(m_pDevice, m_pContext, nullptr, sizeof(GodRayBlurData));
    if (m_pGodRayBlurBuffer == nullptr) return E_FAIL;
    m_pGodRayBlurBuffer->BindPS(GODRAY_BLUR_SLOT);

    CEditableStructuredBuffer::SB_DESC svDesc{};
    svDesc.iElementCount = SHADOW_VIEW_COUNT;
    svDesc.iElementStride = sizeof(ShadowViewGPU);
    svDesc.pInitialData = nullptr;
    m_pShadowViewsBuffer = CEditableStructuredBuffer::Create(m_pDevice, m_pContext, svDesc);
    if (m_pShadowViewsBuffer == nullptr) return E_FAIL;
    m_pShadowViewsBuffer->BindPS(SRV_SHADOW_VIEW_BUFFER_SLOT);

    m_pShadowGlobalsBuffer = CConstantBuffer::Create(m_pDevice, m_pContext, nullptr, sizeof(ShadowGlobalsData));
    if (m_pShadowGlobalsBuffer == nullptr) return E_FAIL;
    m_pShadowGlobalsBuffer->BindVS(SHADOW_GLOBALS_SLOT);
    m_pShadowGlobalsBuffer->BindPS(SHADOW_GLOBALS_SLOT);

    CEditableStructuredBuffer::SB_DESC rtDesc{};
    rtDesc.iElementCount = 1;
    rtDesc.iElementStride = sizeof(RenderTuning);
    rtDesc.pInitialData = &m_tRenderTuning;
    m_pRenderTuningBuffer = CEditableStructuredBuffer::Create(m_pDevice, m_pContext, rtDesc);
    if (m_pRenderTuningBuffer == nullptr) return E_FAIL;
    m_pRenderTuningBuffer->BindPS(SRV_TUNING_BUFFER_SLOT);

    Load_RenderTuning_FromFile();
    m_pRenderTuningBuffer->Update_Buffer(&m_tRenderTuning, 1);

    CEditableStructuredBuffer::SB_DESC fvDesc{};
    fvDesc.iElementCount = g_iMaxFogVolumes;
    fvDesc.iElementStride = sizeof(FogVolume);
    fvDesc.pInitialData = nullptr;
    m_pFogVolumeBuffer = CEditableStructuredBuffer::Create(m_pDevice, m_pContext, fvDesc);
    if (m_pFogVolumeBuffer == nullptr) return E_FAIL;
    m_pFogVolumeBuffer->BindPS(SRV_FOGVOLUME_BUFFER_SLOT);

    m_pPerMaterialBuffer = CConstantBuffer::Create(m_pDevice, m_pContext, nullptr, m_iMaterialCBufferMax);
    if (m_pPerMaterialBuffer == nullptr) return E_FAIL;
    m_pPerMaterialBuffer->BindPS(MATERIAL_SLOT);

    m_pPostGlobalsBuffer = CConstantBuffer::Create(m_pDevice, m_pContext, &m_tPostGlobals, sizeof(PostGlobals));
    if (nullptr == m_pPostGlobalsBuffer) return E_FAIL;
    m_pPostGlobalsBuffer->BindVS(POST_GLOBALS_SLOT);
    m_pPostGlobalsBuffer->BindPS(POST_GLOBALS_SLOT);

    // 0 = none
    m_RimProfiles[1] = { XMFLOAT3(1.f, 0.04f, 0.00f), 0.05f, 1.0f, {} };          // Fury Attack
    m_RimProfiles[2] = { XMFLOAT3(0.3f, 1.f, 0.5f), 2.5f, 2.f, {} };              // PlayerHeal
    m_RimProfiles[3] = { XMFLOAT3(0.3f, 0.7f, 1.f), 3.f, 2.f, {} };               // Stragazer
    m_RimProfiles[4] = { XMFLOAT3(1.f, 0.45f, 0.08f), 1.2f, 2.5f, {} };

    CEditableStructuredBuffer::SB_DESC rpDesc{};
    rpDesc.iElementStride = sizeof(RimProfile);
    rpDesc.iElementCount = 8;
    rpDesc.pInitialData = m_RimProfiles;
    m_pRimProfileBuffer = CEditableStructuredBuffer::Create(m_pDevice, m_pContext, rpDesc);
    if (m_pRimProfileBuffer == nullptr) return E_FAIL;
    m_pRimProfileBuffer->BindPS(SRV_RIMPROFILE_BUFFER_SLOT);
    m_pRimProfileBuffer->Update_Buffer(m_RimProfiles, 8);

    /* Create SamplerState */
    {
        //Linear Sampler (Default)
        CD3D11_SAMPLER_DESC SampleDesc(D3D11_DEFAULT);
        SampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        SampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        SampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;

        if (FAILED(m_pDevice->CreateSamplerState(&SampleDesc, &m_pLinearSampler))) return E_FAIL;
        m_pContext->PSSetSamplers(LINEAR_WRAP_SAMPLER_SLOT, 1, &m_pLinearSampler);

        //Point Sampler
        CD3D11_SAMPLER_DESC PointSampleDesc(D3D11_DEFAULT);
        PointSampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        PointSampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        PointSampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;

        if (FAILED(m_pDevice->CreateSamplerState(&PointSampleDesc, &m_pPointSampler))) return E_FAIL;
        m_pContext->PSSetSamplers(POINT_WRAP_SAMPLER_SLOT, 1, &m_pPointSampler);

        //Clamp Sampler (Linear) — 블룸 업/다운스케일 오프셋 샘플링 시 가장자리 wrap 번짐 방지
        CD3D11_SAMPLER_DESC ClampSampleDesc(D3D11_DEFAULT);
        ClampSampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        ClampSampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        ClampSampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        ClampSampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

        if (FAILED(m_pDevice->CreateSamplerState(&ClampSampleDesc, &m_pLinearClampSampler))) return E_FAIL;
        m_pContext->PSSetSamplers(LINEAR_CLAMP_SAMPLER_SLOT, 1, &m_pLinearClampSampler);
    }

    /* Create Rasterizer State */
    {
        //Default ResterizerState
        CD3D11_RASTERIZER_DESC  RS_Desc(D3D11_DEFAULT);
        if (FAILED(m_pDevice->CreateRasterizerState(&RS_Desc, &m_pDefaultRS))) return E_FAIL;

        //None Cull ResterizerState
        CD3D11_RASTERIZER_DESC  RS_CullCWDesc(D3D11_DEFAULT);
        RS_CullCWDesc.FillMode = D3D11_FILL_SOLID;
        RS_CullCWDesc.CullMode = D3D11_CULL_NONE;
        RS_CullCWDesc.FrontCounterClockwise = false;

        if (FAILED(m_pDevice->CreateRasterizerState(&RS_CullCWDesc, &m_pNoneCullRS))) return E_FAIL;

        CD3D11_RASTERIZER_DESC RS_Scissor(D3D11_DEFAULT);
        RS_Scissor.FillMode = D3D11_FILL_SOLID;
        RS_Scissor.CullMode = D3D11_CULL_BACK;
        RS_Scissor.FrontCounterClockwise = FALSE;
        RS_Scissor.DepthClipEnable = TRUE;
        RS_Scissor.ScissorEnable = TRUE;
        if (FAILED(m_pDevice->CreateRasterizerState(&RS_Scissor, &m_pShadowScissorRS))) return E_FAIL;
    }

    /* Depth Stencil State Create  */
    {
        //Depth Enable State (Default)
        CD3D11_DEPTH_STENCIL_DESC dsEnableDesc(D3D11_DEFAULT);
        dsEnableDesc.DepthEnable = TRUE;
        dsEnableDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsEnableDesc.DepthFunc = D3D11_COMPARISON_LESS;
        dsEnableDesc.StencilEnable = FALSE;
        m_pDevice->CreateDepthStencilState(&dsEnableDesc, &m_pDepth_EnableState);

        //Depth Disable State
        CD3D11_DEPTH_STENCIL_DESC dsDisableDesc(D3D11_DEFAULT);
        dsDisableDesc.DepthEnable = FALSE;
        dsDisableDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDisableDesc.StencilEnable = FALSE;
        m_pDevice->CreateDepthStencilState(&dsDisableDesc, &m_pDepth_DisableState);

        //Depth Read Only State
        CD3D11_DEPTH_STENCIL_DESC dsReadOnlyDesc(D3D11_DEFAULT);
        dsReadOnlyDesc.DepthEnable = TRUE;
        dsReadOnlyDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsReadOnlyDesc.StencilEnable = FALSE;
        m_pDevice->CreateDepthStencilState(&dsReadOnlyDesc, &m_pDepth_OnlyReadState);

        //Depth Sky
        CD3D11_DEPTH_STENCIL_DESC dsSky(D3D11_DEFAULT);
        dsSky.DepthEnable = TRUE;
        dsSky.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsSky.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        dsSky.StencilEnable = FALSE;
        m_pDevice->CreateDepthStencilState(&dsSky, &m_pDepth_Sky);

        //Depth Fwd State
        CD3D11_DEPTH_STENCIL_DESC dsFwd(D3D11_DEFAULT);
        dsFwd.DepthEnable = TRUE;
        dsFwd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsFwd.DepthFunc = D3D11_COMPARISON_LESS;
        dsFwd.StencilEnable = TRUE;
        dsFwd.StencilReadMask = 0xFF;
        dsFwd.StencilWriteMask = 0xFF;
        dsFwd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        dsFwd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        dsFwd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsFwd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsFwd.BackFace = dsFwd.FrontFace;   // 포워드 NoneCull(양면) → 동일
        if (FAILED(m_pDevice->CreateDepthStencilState(&dsFwd, &m_pDepth_Fwd_StencilWrite))) return E_FAIL;

        // Depth Combine
        CD3D11_DEPTH_STENCIL_DESC dsRefl(D3D11_DEFAULT);
        dsRefl.DepthEnable = FALSE;
        dsRefl.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsRefl.StencilEnable = TRUE;
        dsRefl.StencilReadMask = 0xFF;
        dsRefl.StencilWriteMask = 0x00;
        dsRefl.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
        dsRefl.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsRefl.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsRefl.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsRefl.BackFace = dsRefl.FrontFace;
        if (FAILED(m_pDevice->CreateDepthStencilState(&dsRefl, &m_pDepth_Refl_StencilTest))) return E_FAIL;

        CD3D11_DEPTH_STENCIL_DESC dsDecalWrite(D3D11_DEFAULT);
        dsDecalWrite.DepthEnable = TRUE;
        dsDecalWrite.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDecalWrite.DepthFunc = D3D11_COMPARISON_LESS;
        dsDecalWrite.StencilEnable = TRUE;
        dsDecalWrite.StencilReadMask = DECAL_RECEIVER_STENCIL;
        dsDecalWrite.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        dsDecalWrite.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        dsDecalWrite.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsDecalWrite.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsDecalWrite.BackFace = dsDecalWrite.FrontFace;
        if (FAILED(m_pDevice->CreateDepthStencilState(&dsDecalWrite, &m_pDepth_GBuffer_ReceiverWrite))) return E_FAIL;

        CD3D11_DEPTH_STENCIL_DESC dsDecalTest(D3D11_DEFAULT);
        dsDecalTest.DepthEnable = FALSE;
        dsDecalTest.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDecalTest.StencilEnable = TRUE;
        dsDecalTest.StencilReadMask = DECAL_RECEIVER_STENCIL;
        dsDecalTest.StencilWriteMask = 0x00;
        dsDecalTest.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        dsDecalTest.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsDecalTest.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsDecalTest.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsDecalTest.BackFace = dsDecalTest.FrontFace;
        if (FAILED(m_pDevice->CreateDepthStencilState(&dsDecalTest, &m_pDepth_Decal_ReceiverTest))) return E_FAIL;
    }

    /* Blend State Create*/
    {
        //Alpha Blend State
        CD3D11_BLEND_DESC AlphaBlendDesc(D3D11_DEFAULT);
        AlphaBlendDesc.AlphaToCoverageEnable = FALSE;
        AlphaBlendDesc.IndependentBlendEnable = FALSE;

        AlphaBlendDesc.RenderTarget[0].BlendEnable = TRUE;
        //색 연산
        AlphaBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        AlphaBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        AlphaBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        //알파연산
        AlphaBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        AlphaBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        AlphaBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

        AlphaBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(m_pDevice->CreateBlendState(&AlphaBlendDesc, &m_pAlphaBlendState))) return E_FAIL;

        CD3D11_BLEND_DESC EffectAlphaBlendDesc(D3D11_DEFAULT);
        EffectAlphaBlendDesc.AlphaToCoverageEnable = FALSE;
        EffectAlphaBlendDesc.IndependentBlendEnable = FALSE;

        EffectAlphaBlendDesc.RenderTarget[0].BlendEnable = TRUE;
        EffectAlphaBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        EffectAlphaBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        EffectAlphaBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

        EffectAlphaBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        EffectAlphaBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        EffectAlphaBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        EffectAlphaBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(m_pDevice->CreateBlendState(&EffectAlphaBlendDesc, &m_pEffectAlphaBlendState))) return E_FAIL;

        CD3D11_BLEND_DESC AddBlendDesc(D3D11_DEFAULT);

        //Additive Blend State
        AddBlendDesc.AlphaToCoverageEnable = FALSE;
        AddBlendDesc.IndependentBlendEnable = FALSE;

        AddBlendDesc.RenderTarget[0].BlendEnable = TRUE;
        AddBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        AddBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        AddBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

        AddBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        AddBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE; //D3D11_BLEND_ZERO
        AddBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        AddBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(m_pDevice->CreateBlendState(&AddBlendDesc, &m_pAdditiveBlendState))) return E_FAIL;

        // Bloom Upsample Blend State
        CD3D11_BLEND_DESC BloomUpBlendDesc(D3D11_DEFAULT);
        BloomUpBlendDesc.AlphaToCoverageEnable = FALSE;
        BloomUpBlendDesc.IndependentBlendEnable = FALSE;

        BloomUpBlendDesc.RenderTarget[0].BlendEnable = TRUE;
        BloomUpBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
        BloomUpBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
        BloomUpBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

        BloomUpBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_BLEND_FACTOR;
        BloomUpBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_BLEND_FACTOR;
        BloomUpBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        BloomUpBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(m_pDevice->CreateBlendState(&BloomUpBlendDesc, &m_pBloomUpsampleBlendState))) return E_FAIL;

        CD3D11_BLEND_DESC MulBlendDesc(D3D11_DEFAULT);

        // Multiply Blend State
        MulBlendDesc.AlphaToCoverageEnable = FALSE;
        MulBlendDesc.IndependentBlendEnable = FALSE;

        MulBlendDesc.RenderTarget[0].BlendEnable = TRUE;
        MulBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
        MulBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
        MulBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

        MulBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        MulBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        MulBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        MulBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(m_pDevice->CreateBlendState(&MulBlendDesc, &m_pMultiplyBlendState))) return E_FAIL;

        // Weight Blend Order Indpendent Trasparency State
        CD3D11_BLEND_DESC WBOITBlendDesc(D3D11_DEFAULT);
        WBOITBlendDesc.AlphaToCoverageEnable = FALSE;
        WBOITBlendDesc.IndependentBlendEnable = TRUE;

        WBOITBlendDesc.RenderTarget[0].BlendEnable = TRUE;
        WBOITBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        WBOITBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        WBOITBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        WBOITBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        WBOITBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        WBOITBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        WBOITBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        WBOITBlendDesc.RenderTarget[1].BlendEnable = TRUE;
        WBOITBlendDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_ZERO;
        WBOITBlendDesc.RenderTarget[1].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
        WBOITBlendDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
        WBOITBlendDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ZERO;
        WBOITBlendDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        WBOITBlendDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        WBOITBlendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;

        if (FAILED(m_pDevice->CreateBlendState(&WBOITBlendDesc, &m_pWBOITBlendState))) return E_FAIL;
    }

    /*Depth Stencil View Create*/
    {
        // SPOTLIGHT DSV
        D3D11_TEXTURE2D_DESC SpotDesc{};
        SpotDesc.Width = g_iSpotAtlasSize;
        SpotDesc.Height = g_iSpotAtlasSize;
        SpotDesc.MipLevels = 1;
        SpotDesc.ArraySize = 1;
        SpotDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        SpotDesc.SampleDesc.Count = 1;
        SpotDesc.Usage = D3D11_USAGE_DEFAULT;
        SpotDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D* pSpotTex = { nullptr };
        if (FAILED(m_pDevice->CreateTexture2D(&SpotDesc, nullptr, &pSpotTex))) return E_FAIL;

        HRESULT SpotHr = m_pDevice->CreateDepthStencilView(pSpotTex, nullptr, &m_pSpotDSV);
        Safe_Release(pSpotTex);
        if (FAILED(SpotHr)) return E_FAIL;

        // CSM DSV ( Directional Light )
        D3D11_TEXTURE2D_DESC CSMDesc{};
        CSMDesc.Width = g_iCSMAtlasSize;
        CSMDesc.Height = g_iCSMAtlasSize;
        CSMDesc.MipLevels = 1;
        CSMDesc.ArraySize = 1;
        CSMDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        CSMDesc.SampleDesc.Count = 1;
        CSMDesc.Usage = D3D11_USAGE_DEFAULT;
        CSMDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D* pCSMTex = { nullptr };
        if (FAILED(m_pDevice->CreateTexture2D(&CSMDesc, nullptr, &pCSMTex))) return E_FAIL;

        HRESULT CSMHr = m_pDevice->CreateDepthStencilView(pCSMTex, nullptr, &m_pCSMDSV);
        Safe_Release(pCSMTex);
        if (FAILED(CSMHr)) return E_FAIL;

        D3D11_TEXTURE2D_DESC PointDesc{};
        PointDesc.Width = g_iPointAtlasSize;
        PointDesc.Height = g_iPointAtlasSize;
        PointDesc.MipLevels = 1;
        PointDesc.ArraySize = 1;
        PointDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        PointDesc.SampleDesc.Count = 1;
        PointDesc.Usage = D3D11_USAGE_DEFAULT;
        PointDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D* pPointTex = { nullptr };
        if (FAILED(m_pDevice->CreateTexture2D(&PointDesc, nullptr, &pPointTex))) return E_FAIL;

        HRESULT hr = m_pDevice->CreateDepthStencilView(pPointTex, nullptr, &m_pPointDSV);
        Safe_Release(pPointTex);
        if (FAILED(hr)) return E_FAIL;

        // Planar Reflection DSV ( 화면 해상도와 동일 )
        D3D11_TEXTURE2D_DESC ReflectDesc{};
        ReflectDesc.Width = ETOUI(ViewportDesc.Width);
        ReflectDesc.Height = ETOUI(ViewportDesc.Height);
        ReflectDesc.MipLevels = 1;
        ReflectDesc.ArraySize = 1;
        ReflectDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        ReflectDesc.SampleDesc.Count = 1;
        ReflectDesc.Usage = D3D11_USAGE_DEFAULT;
        ReflectDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D* pReflectTex = { nullptr };
        if (FAILED(m_pDevice->CreateTexture2D(&ReflectDesc, nullptr, &pReflectTex))) return E_FAIL;

        HRESULT ReflectHr = m_pDevice->CreateDepthStencilView(pReflectTex, nullptr, &m_pReflectionDSV);
        Safe_Release(pReflectTex);
        if (FAILED(ReflectHr)) return E_FAIL;
    }

    if (FAILED(Create_Shaders())) return E_FAIL;

    if (FAILED(Create_MRTs())) return E_FAIL;

    if (FAILED(Ready_HiZ())) return E_FAIL;

    if (FAILED(Ready_DefaultTexture()))
        return E_FAIL;

#pragma region VIBuffer_Copy
    /* Copy VIBuffer_Rect */
    m_pVIBuffer_Rect = CVIBuffer_Rect::Create(m_pDevice, m_pContext);
    if (m_pVIBuffer_Rect == nullptr) return E_FAIL;
#pragma endregion

#pragma region Ready_RT_DEBUG
#ifdef _DEBUG
    //if (FAILED(m_pGameInstance->Ready_RT_Debug(TEXT("Target_PlanarReflection"), 150.f, 150.f, 300.f, 300.f))) return E_FAIL;
#endif
#pragma endregion

    m_pIBLBaker = CIBL_Baker::Create(m_pDevice, m_pContext);
    if (m_pIBLBaker == nullptr) return E_FAIL;

    return S_OK;
}

void CRenderer::Add_RenderGroup(RENDERID eGroupID, CGameObject* pGameObject)
{
    m_RenderObjects[ETOUI(eGroupID)].push_back(pGameObject);
    Safe_AddRef(pGameObject);
}

void CRenderer::Add_Decal(CDecal* pDecal)
{
    if (pDecal == nullptr)
        return;

    m_Decals.push_back(pDecal);
    Safe_AddRef(pDecal);
}

HRESULT CRenderer::Draw()
{
    if (FAILED(Bind_LightBuffer(LIGHT::DIRECTIONAL, 0)))
        return E_FAIL;

    if (FAILED(Bind_CameraBuffer()))
        return E_FAIL;

    if (FAILED(Update_PostGlobals()))
        return E_FAIL;

    if (FAILED(Render_Priority()))
        return E_FAIL;

    if (FAILED(Render_Shadow()))
        return E_FAIL;

    if (FAILED(Render_NonBlend()))
        return E_FAIL;

    if (FAILED(Render_Decals()))
        return E_FAIL;

    if (FAILED(Build_HiZ()))
        return E_FAIL;

    if (FAILED(Render_SSAO()))
        return E_FAIL;

    if (FAILED(Render_Lights()))
        return E_FAIL;

    if (FAILED(Render_Combined()))
        return E_FAIL;

    if (FAILED(Render_Cubemap()))
        return E_FAIL;

    if (FAILED(Render_Fog()))
        return E_FAIL;

    if (FAILED(Render_Forward()))
        return E_FAIL;

    if (FAILED(Render_HalfRes_Effects()))
        return E_FAIL;

    if (FAILED(Render_PostComposite()))
        return E_FAIL;

    if (FAILED(Render_PlanarReflection()))
        return E_FAIL;

    if (FAILED(Render_Blend2()))
        return E_FAIL;

    if (FAILED(Render_Blend()))
        return E_FAIL;

    if (FAILED(Render_Bloom()))
        return E_FAIL;

    if (FAILED(Render_Blend_WBOIT()))
        return E_FAIL;

    if (FAILED(Render_UIPreviewBackground()))
        return E_FAIL;

    if (FAILED(Render_Final()))
        return E_FAIL;

    if (FAILED(Render_SMAA()))
        return E_FAIL;

#ifdef _DEBUG
    if (FAILED(Render_Debug())) return E_FAIL;
#endif // _DEBUG

    if (FAILED(Render_UI()))
        return E_FAIL;


    return S_OK;
}

HRESULT CRenderer::Command_Execute()
{
    if (m_pGameInstance != nullptr)
    {
        if (FAILED(m_pGameInstance->Flush_SkinningJobs()))
            return E_FAIL;
    }

    Clear_LastState();

    // Commands List 정렬  
    if (m_bNeedDepthSort)
        sort(m_pCommands.begin(), m_pCommands.end(), RenderHeader::RenderSort_Depth);
    else
        sort(m_pCommands.begin(), m_pCommands.end(), RenderHeader::RenderSort);

    //Shader Binding
    for (RenderCommand* pCurrentPtr : m_pCommands)
    {
        // UI_TEXT
        if (pCurrentPtr->header.etype == RENDER_TYPE::UI_TEXT)
        {
            UITextCommand* pTextCommand = static_cast<UITextCommand*>(pCurrentPtr);

            if (m_pUISpriteBatch == nullptr || pTextCommand->pFont == nullptr || pTextCommand->pText == nullptr)
                continue;

           /*
            * SpriteBatch가 자신에게 필요한 Shader와 Buffer를 바인딩, 한 Text Command씩 Begin/End하므로 
            * 기존 UI와 SortOrder를 섞어서 그릴 수 있음.
            */

            m_pUISpriteBatch->Begin(
                SpriteSortMode_Deferred,
                m_pAlphaBlendState,
                m_pLinearClampSampler,
                m_pDepth_DisableState,
                m_pNoneCullRS,
                nullptr,
                XMLoadFloat4x4(&pTextCommand->worldMatrix));

            pTextCommand->pFont->DrawString(
                m_pUISpriteBatch,
                pTextCommand->pText,
                pTextCommand->vPosition,
                XMLoadFloat4(&pTextCommand->vColor),
                pTextCommand->fRotation,
                pTextCommand->vOrigin,
                pTextCommand->vScale,
                SpriteEffects_None,
                pTextCommand->fLayerDepth);

            m_pUISpriteBatch->End();
            
            // SpriteBatch는 전달받은 Sampler를 PS의 s0에 바인딩하므로
            // 이후 일반 UI를 위해 엔진 기본 Sampler를 다시 연결한다.
            if (m_pLinearSampler != nullptr)
            {
                m_pContext->PSSetSamplers(LINEAR_WRAP_SAMPLER_SLOT, 1, &m_pLinearSampler);
            }

            /*
             * SpriteBatch는 VS의 CAMERA_SLOT에 자신의 64바이트
             * 상수 버퍼를 연결한다.
             *
             * 이후 기존 UI Shader는 CAMERA_SLOT에서 128바이트인
             * PerCameraData를 기대하므로 엔진 Camera Buffer를 다시 연결한다.
             */
            if (m_pPerCameraBuffer != nullptr)
            {
                m_pPerCameraBuffer->BindVS(CAMERA_SLOT);
                m_pPerCameraBuffer->BindPS(CAMERA_SLOT);
            }

            /*
             * SpriteBatch가 Shader/InputLayout 등 D3D 상태를 변경
             * 다음 일반 RenderCommand가 모든 상태를 다시 바인딩
             */

            Clear_LastState();

            continue;
        }

        /* Bind VS*/
        if (m_pLastCommand->pVIBuffer != pCurrentPtr->pVIBuffer)
        {
            pCurrentPtr->pVIBuffer->Bind_Resources();
            m_pLastCommand->pVIBuffer = pCurrentPtr->pVIBuffer;
        }

        /* Bind VS */
        if (pCurrentPtr->pVS == nullptr) return E_FAIL;
        if (m_pLastCommand->pVS != pCurrentPtr->pVS)
        {
            m_pContext->VSSetShader(pCurrentPtr->pVS, nullptr, 0);
            m_pLastCommand->pVS = pCurrentPtr->pVS;
        }

        /* Bind PS */
        if (pCurrentPtr->pPS == nullptr) return E_FAIL;
        if (m_pLastCommand->pPS != pCurrentPtr->pPS)
        {
            m_pContext->PSSetShader(pCurrentPtr->pPS, nullptr, 0);
            m_pLastCommand->pPS = pCurrentPtr->pPS;
        }

        /* Bind InputLayout */
        if (pCurrentPtr->pLayout == nullptr && pCurrentPtr->header.etype != RENDER_TYPE::EFFECT)
            return E_FAIL;

        if (m_pLastCommand->pLayout != pCurrentPtr->pLayout)
        {
            m_pContext->IASetInputLayout(pCurrentPtr->pLayout);
            m_pLastCommand->pLayout = pCurrentPtr->pLayout;
        }

        //Effect/Debug면서 Material없으면 패스 
        _bool bFlagMaterialPass = (pCurrentPtr->pMaterial == nullptr) &&
            (pCurrentPtr->header.etype == RENDER_TYPE::EFFECT || pCurrentPtr->header.etype == RENDER_TYPE::DEBUG);

        /* Bind MaterialBuffer */
        if (!bFlagMaterialPass && m_pLastCommand->pMaterial != pCurrentPtr->pMaterial)
        {
            // Texture만 바인딩 
            if (FAILED(Bind_Material_Textures(pCurrentPtr->pMaterial))) return E_FAIL;
            Bind_MaterialBuffer(pCurrentPtr->pMaterial);
            m_pLastCommand->pMaterial = pCurrentPtr->pMaterial;

            if (m_eCurRenderPass == RENDERID::NONBLEND)
            {
                if (pCurrentPtr->pMaterial->Get_Meta_Desc().bTwoSide)
                    m_pContext->RSSetState(m_pNoneCullRS);
                else
                    m_pContext->RSSetState(m_pDefaultRS);
            }
        }

        //Bind ObjectBuffer
        if (FAILED(Bind_ObjectBuffer(pCurrentPtr))) return E_FAIL;


        /* Render Type별 추가 바인딩 */
        if (pCurrentPtr->header.etype == RENDER_TYPE::STATIC)
        {
            //
            if (m_eCurRenderPass == RENDERID::NONBLEND)
                m_pContext->OMSetDepthStencilState(m_pDepth_GBuffer_ReceiverWrite, DECAL_RECEIVER_STENCIL);
        }
        else if (pCurrentPtr->header.etype == RENDER_TYPE::SKINNED)
        {
            if (m_eCurRenderPass == RENDERID::NONBLEND)
                m_pContext->OMSetDepthStencilState(m_pDepth_GBuffer_ReceiverWrite, 0);

            SkinnedCommand* pSkinnedPtr = static_cast<SkinnedCommand*>(pCurrentPtr);

            if (pSkinnedPtr->pBoneMatrixSRV == nullptr)
                return E_FAIL;

            m_pContext->VSSetShaderResources(SRV_BONE_MATRIX_SLOT, 1, &pSkinnedPtr->pBoneMatrixSRV);
        }
        else if (pCurrentPtr->header.etype == RENDER_TYPE::EFFECT)
        {
            EffectCommand* pEffectPtr = static_cast<EffectCommand*>(pCurrentPtr);

            if (m_eCurRenderPass == RENDERID::BLEND_WBOIT)
            {
                m_pContext->OMSetBlendState(m_pWBOITBlendState, nullptr, 0xffffffff);
                m_pContext->OMSetDepthStencilState(pEffectPtr->bZTest ? m_pDepth_OnlyReadState : m_pDepth_DisableState, 0);
            }
            else
            {
                ID3D11BlendState* pBlend = m_pEffectAlphaBlendState;
                if (pEffectPtr->iBlendMode == 1) pBlend = m_pAdditiveBlendState;
                else if (pEffectPtr->iBlendMode == 2) pBlend = m_pMultiplyBlendState;
                m_pContext->OMSetBlendState(pBlend, nullptr, 0xffffffff);

                ID3D11DepthStencilState* pDepth = m_pDepth_DisableState;
                if (pEffectPtr->bZTest)
                    pDepth = pEffectPtr->bZWrite ? m_pDepth_EnableState : m_pDepth_OnlyReadState;
                m_pContext->OMSetDepthStencilState(pDepth, 0);
            }

            if (pEffectPtr->iAlignment == 2 || pEffectPtr->iAlignment == 3)
                m_pContext->RSSetState(m_pNoneCullRS);
        }
        else if (pCurrentPtr->header.etype == RENDER_TYPE::UI)
        {
            UICommand* pUICommand = static_cast<UICommand*>(pCurrentPtr);

            ID3D11BlendState* pBlend = m_pAlphaBlendState;

            if (pUICommand->iBlendMode == 1)
                pBlend = m_pAdditiveBlendState;
            else if (pUICommand->iBlendMode == 2)
                pBlend = m_pMultiplyBlendState;

            m_pContext->OMSetBlendState(pBlend, nullptr, 0xffffffff);
            ID3D11DepthStencilState* pDepthState = m_eCurRenderPass == RENDERID::UI_PREVIEW_BACKGROUND ? m_pDepth_Sky : m_pDepth_DisableState;
            m_pContext->OMSetDepthStencilState(pDepthState, 0);

            if (FAILED(Bind_UIBuffer(pUICommand))) return E_FAIL;
        }

        //Draw 
        if (FAILED(pCurrentPtr->pVIBuffer->Render())) return E_FAIL;

    }

    m_pCommands.clear();
    m_pAllocator->Clear();

    m_bNeedDepthSort = false;
    return S_OK;
}

void CRenderer::Submit_UI(UICommand& command)
{
    command.header.etype = RENDER_TYPE::UI;
    command.header.iCommandSize = sizeof(UICommand);

    RenderCommand* pAllocatedPtr = static_cast<RenderCommand*>(m_pAllocator->Allocate(command.header.iCommandSize));

    if (pAllocatedPtr == nullptr)
        return;

    memcpy(pAllocatedPtr, &command, command.header.iCommandSize);

    m_pCommands.push_back(pAllocatedPtr);
}

void CRenderer::Submit_UIText(UITextCommand& Command)
{
    Command.header.etype = RENDER_TYPE::UI_TEXT;
    Command.header.iCommandSize = sizeof(UITextCommand);

    RenderCommand* pAllocatedCommand = static_cast<RenderCommand*>(m_pAllocator->Allocate(Command.header.iCommandSize));

    if (pAllocatedCommand == nullptr)
        return;

    memcpy(pAllocatedCommand, &Command, Command.header.iCommandSize);

    m_pCommands.push_back(pAllocatedCommand);
}

void CRenderer::Clear_LastState()
{
    *m_pLastCommand = {};

    for (_uint i = 0; i < g_iMaxTextureStack; i++)
    {
        m_pAlbedoSRVs[i] = nullptr;
        m_pNormalSRVs[i] = nullptr;
        m_pMaskSRVs[i] = nullptr;
    }
    m_pMask2SRV = nullptr;
    m_pDetaillNormalSRV = nullptr;
    m_pDefaultMaskSRV = nullptr;
    m_pEmissiveSRV = nullptr;
}

void CRenderer::Clear_BindingSRV(_uint iTextureSlot, _uint iCount)
{
    m_pContext->PSSetShaderResources(iTextureSlot, iCount, m_pNullSRVs);
}

HRESULT CRenderer::Bind_LightBuffer(LIGHT eLightType, _uint iIndex, _int iShadowIndex)
{
    PerLightData tLightData{};

    if (FAILED(m_pGameInstance->Fill_LightData(eLightType, iIndex, tLightData)))
    {
        tLightData.vDirection = _float4(0.f, -1.f, 0.f, 0.f);
        tLightData.iShadowIndex = -1;
        return m_pPerLightBuffer->Update_Data(&tLightData, sizeof(PerLightData));
    }

    tLightData.iShadowIndex = iShadowIndex;
    return m_pPerLightBuffer->Update_Data(&tLightData, sizeof(PerLightData));
}

HRESULT CRenderer::Bind_CameraBuffer()
{
    PerCameraData tCameraData; 
    m_pGameInstance->Fill_CameraData(tCameraData);

    //행렬은 전치해서 바인딩하기
    XMStoreFloat4x4(&tCameraData.ViewMatrix, XMMatrixTranspose(XMLoadFloat4x4(&tCameraData.ViewMatrix)));

    XMStoreFloat4x4(&tCameraData.ProjMatrix, XMMatrixTranspose(XMLoadFloat4x4(&tCameraData.ProjMatrix)));

    XMStoreFloat4x4(&tCameraData.ViewInvMatrix, XMMatrixTranspose(XMLoadFloat4x4(&tCameraData.ViewInvMatrix)));

    XMStoreFloat4x4(&tCameraData.ProjInvMatrix, XMMatrixTranspose(XMLoadFloat4x4(&tCameraData.ProjInvMatrix)));

    return m_pPerCameraBuffer->Update_Data(&tCameraData, sizeof(PerCameraData));
}

HRESULT CRenderer::Bind_CameraBuffer(PerCameraData& tCameraData)
{
    //행렬은 전치해서 바인딩하기
    XMStoreFloat4x4(&tCameraData.ViewMatrix, XMMatrixTranspose(XMLoadFloat4x4(&tCameraData.ViewMatrix)));

    XMStoreFloat4x4(&tCameraData.ProjMatrix, XMMatrixTranspose(XMLoadFloat4x4(&tCameraData.ProjMatrix)));

    XMStoreFloat4x4(&tCameraData.ViewInvMatrix, XMMatrixTranspose(XMLoadFloat4x4(&tCameraData.ViewInvMatrix)));

    XMStoreFloat4x4(&tCameraData.ProjInvMatrix, XMMatrixTranspose(XMLoadFloat4x4(&tCameraData.ProjInvMatrix)));

    return m_pPerCameraBuffer->Update_Data(&tCameraData, sizeof(PerCameraData));
}

#pragma region Backup
//HRESULT CRenderer::Bind_Material_Textures(CMaterial* pMaterial)
//{
//    if (pMaterial == nullptr) return S_OK;
//   
//    /* Detail Normal */
//    ID3D11ShaderResourceView* pSRV = pMaterial->Get_SRV(TEXTURE_TYPE::DETAIL_NORMAL, 0);
//    if (pSRV != nullptr && pSRV != m_pDetaillNormalSRV)
//    {
//        m_pDetaillNormalSRV = pSRV;
//        m_pContext->PSSetShaderResources(SRV_DETAIL_NORMAL_SLOT, 1, &m_pDetaillNormalSRV);
//    }
//
//    //여러장 
//    /* Albedo */
//    _bool isEqualLastSRV = true;
//    _uint iAlbedoCount = (_uint)pMaterial->Get_Textures(TEXTURE_TYPE::BASE_COLOR)->size();
//    for (_uint i = 0; i < iAlbedoCount; i++)
//    {
//        pSRV = pMaterial->Get_SRV(TEXTURE_TYPE::BASE_COLOR, i);
//        if (m_pAlbedoSRVs[i] != pSRV)
//        {
//            m_pAlbedoSRVs[i] = pSRV;
//            isEqualLastSRV = false;
//        }
//    }
//    //지난 SRV 입력과 달라졌다면 Bind PS
//    if(!isEqualLastSRV)
//        m_pContext->PSSetShaderResources(SRV_ALBEDO_SLOT, iAlbedoCount, m_pAlbedoSRVs);
//
//    /* Normal */
//    isEqualLastSRV = true;
//    _uint iNormalCount = (_uint)pMaterial->Get_Textures(TEXTURE_TYPE::NORMAL)->size();
//    for (_uint i = 0; i < iNormalCount; i++)
//    {
//        pSRV = pMaterial->Get_SRV(TEXTURE_TYPE::NORMAL, i);
//        if (m_pNormalSRVs[i] != pSRV)
//        {
//            m_pNormalSRVs[i] = pSRV;
//            isEqualLastSRV = false;
//        }
//    }
//    if (!isEqualLastSRV)
//        m_pContext->PSSetShaderResources(SRV_NORMAL_SLOT, iNormalCount, m_pNormalSRVs);
//
//    /* Mask1 */
//    isEqualLastSRV = true;
//    _uint iMaskCount = (_uint)pMaterial->Get_Textures(TEXTURE_TYPE::MASK1)->size();
//    for (_uint i = 0; i < iMaskCount; i++)
//    {
//        pSRV = pMaterial->Get_SRV(TEXTURE_TYPE::MASK1, i);
//        if (m_pMaskSRVs[i] != pSRV)
//        {
//            m_pMaskSRVs[i] = pSRV;
//            isEqualLastSRV = false;
//        }
//    }
//
//    if (!isEqualLastSRV)
//        m_pContext->PSSetShaderResources(SRV_MASK1_SLOT, iMaskCount, m_pMaskSRVs);
//
//
//
//    /* Mask2 */
//    isEqualLastSRV = true;
//    _uint iMask2Count = (_uint)pMaterial->Get_Textures(TEXTURE_TYPE::MASK2)->size();
//    for (_uint i = 0; i < iMask2Count; i++)
//    {
//        pSRV = pMaterial->Get_SRV(TEXTURE_TYPE::MASK2, i);
//        if (m_pMaskSRVs[i] != pSRV)
//        {
//            m_pMaskSRVs[i] = pSRV;
//            isEqualLastSRV = false;
//        }
//    }
//
//    if (!isEqualLastSRV)
//        m_pContext->PSSetShaderResources(SRV_MASK2_SLOT, iMask2Count, m_pMaskSRVs);
//
//    /* Emissive */
//    ID3D11ShaderResourceView* pEmissiveSRV = pMaterial->Get_SRV(TEXTURE_TYPE::EMISSIVE, 0);    
//    if (pEmissiveSRV != m_pEmissiveSRV)
//    {
//        m_pEmissiveSRV = pEmissiveSRV;
//        m_pContext->PSSetShaderResources(SRV_EMSSIVE_SLOT, 1, &m_pEmissiveSRV);
//    }
//
//    return S_OK;
//}
#pragma endregion

// 변경 사유 : 텍스쳐가 0장 인 애들이 들어올경우 SRV 잔류 
HRESULT CRenderer::Bind_Material_Textures(CMaterial* pMaterial)
{
    if (pMaterial == nullptr) return S_OK;

    ID3D11ShaderResourceView* pDetailSRV = pMaterial->Get_SRV(TEXTURE_TYPE::DETAIL_NORMAL, 0);
    if (pDetailSRV != m_pDetaillNormalSRV)
    {
        m_pDetaillNormalSRV = pDetailSRV;
        m_pContext->PSSetShaderResources(SRV_DETAIL_NORMAL_SLOT, 1, &m_pDetaillNormalSRV);
    }

    Bind_TextureStack(pMaterial, TEXTURE_TYPE::BASE_COLOR, SRV_ALBEDO_SLOT, m_pAlbedoSRVs, m_pDefaultAlbedoSRV);
    Bind_TextureStack(pMaterial, TEXTURE_TYPE::NORMAL, SRV_NORMAL_SLOT, m_pNormalSRVs, nullptr);
    Bind_TextureStack(pMaterial, TEXTURE_TYPE::MASK1, SRV_MASK1_SLOT, m_pMaskSRVs, nullptr);

    ID3D11ShaderResourceView* pMask2SRV = pMaterial->Get_SRV(TEXTURE_TYPE::MASK2, 0);
    if (pMask2SRV != m_pMask2SRV)
    {
        m_pMask2SRV = pMask2SRV;
        m_pContext->PSSetShaderResources(SRV_MASK2_SLOT, 1, &m_pMask2SRV);
    }

    ID3D11ShaderResourceView* pEmissiveSRV = pMaterial->Get_SRV(TEXTURE_TYPE::EMISSIVE, 0);
    if (pEmissiveSRV != m_pEmissiveSRV)
    {
        m_pEmissiveSRV = pEmissiveSRV;
        m_pContext->PSSetShaderResources(SRV_EMSSIVE_SLOT, 1, &m_pEmissiveSRV);
    }

    return S_OK;
}

HRESULT CRenderer::Bind_ObjectBuffer(RenderCommand* pCommand)
{
    if (m_pPerObjectBuffer == nullptr || pCommand == nullptr) return E_FAIL;

    PerObjectData objectData{};
    XMStoreFloat4x4(&objectData.WorldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&pCommand->worldMatrix)));
    if (pCommand->pMaterial)
        objectData.iMaterialIndex = pCommand->pMaterial->Get_ID();
    else objectData.iMaterialIndex = -1;

    objectData.iAlphaClip = pCommand->iAlphaClip;
    objectData.fEmissiveAmount = pCommand->fEmissiveAmount;
    objectData.iRimProfile = pCommand->iRimProfile;
    objectData.fRimAmount = pCommand->fRimAmount;
    objectData.fDissolveAmount = pCommand->fDissolveAmount;
    objectData.vDissolveColor = pCommand->vDissolveColor;

    if (pCommand->header.etype == RENDER_TYPE::SKINNED)
    {
        SkinnedCommand* pSkinnedPtr = static_cast<SkinnedCommand*>(pCommand);
        objectData.iBoneOffset = pSkinnedPtr->iBoneOffset;
        objectData.iBoneCount = pSkinnedPtr->iBoneCount;
    }

    if (FAILED(m_pPerObjectBuffer->Update_Data(&objectData, sizeof(PerObjectData)))) return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Bind_MaterialBuffer(CMaterial* pMaterial)
{
    if (m_pPerMaterialBuffer == nullptr || pMaterial == nullptr)
        return S_OK;

    if (!pMaterial->Has_MaterialBuffer())
        return S_OK;

    m_pPerMaterialBuffer->Update_Data_CallBack([pMaterial](void* pBuffer)
        {
            pMaterial->Write_MaterialParams(pBuffer);
        });

    return S_OK;
}

HRESULT CRenderer::Update_MaterialBuffer(vector<CMaterial*>* pMaterialVector)
{
    if (m_pMaterialsBuffer == nullptr || pMaterialVector == nullptr) return E_FAIL;

    const _uint iVecCount = static_cast<_uint>(pMaterialVector->size());

    _uint iMaxID = 0;
    for (_uint i = 0; i < iVecCount; ++i)
    {
        CMaterial* pMat = (*pMaterialVector)[i];
        if (pMat != nullptr)
            iMaxID = max(iMaxID, pMat->Get_ID());
    }

    const _uint iCount = min(iMaxID + 1, g_iNumMaxMaterals);
    vector<MaterialData> MaterialDatas(iCount);

    for (_uint i = 0; i < iVecCount; ++i)
    {
        CMaterial* pMat = (*pMaterialVector)[i];
        if (pMat == nullptr) continue;

        const _uint iID = pMat->Get_ID();
        if (iID < iCount)
            MaterialDatas[iID] = pMat->Get_MaterialData();
    }

    m_pMaterialsBuffer->Update_Buffer(MaterialDatas.data(), static_cast<_uint>(MaterialDatas.size()));
    return S_OK;
}

HRESULT CRenderer::Bind_UIBuffer(const UICommand* pCommand)
{
    if (m_pUIBuffer == nullptr || pCommand == nullptr)
        return E_FAIL;

    PerUIData tUIData = {};

    tUIData.vColor = pCommand->vColor;

    tUIData.vColorLT = pCommand->vColorLT;
    tUIData.vColorRT = pCommand->vColorRT;
    tUIData.vColorRB = pCommand->vColorRB;
    tUIData.vColorLB = pCommand->vColorLB;

    tUIData.vUVRect = pCommand->vUVRect;
    tUIData.fFillAmount = pCommand->fFillAmount;
    tUIData.iUseTexture = static_cast<_int>(pCommand->iUseTexture);
    tUIData.iBlendMode = static_cast<_int>(pCommand->iBlendMode);

    tUIData.fRevealProgress = pCommand->fRevealProgress;
    tUIData.fRevealSoftness = pCommand->fRevealSoftness;
    tUIData.vRevealCenter = pCommand->vRevealCenter;
    tUIData.iRevealType = pCommand->iRevealType;

    return m_pUIBuffer->Update_Data(&tUIData, sizeof(PerUIData));
}

HRESULT CRenderer::Ready_DefaultTexture()
{
    const _wstring strPath   = _wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/Textures/Default_Texture.dds";

    if (FAILED(CreateDDSTextureFromFile(m_pDevice, strPath.c_str(), nullptr, &m_pDefaultAlbedoSRV)))
    {
        m_pDefaultAlbedoSRV = nullptr;
        MSG_BOX("[Renderer] Default_Texture.dds not found -> albedo fallback = null");
        return E_FAIL;
    }
    
    D3D11_TEXTURE2D_DESC AreaDesc = {};
    AreaDesc.Width = AREATEX_WIDTH;
    AreaDesc.Height = AREATEX_HEIGHT;
    AreaDesc.MipLevels = 1;
    AreaDesc.ArraySize = 1;
    AreaDesc.Format = DXGI_FORMAT_R8G8_UNORM;
    AreaDesc.SampleDesc.Count = 1;
    AreaDesc.Usage = D3D11_USAGE_IMMUTABLE;
    AreaDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA AreaData = {};
    AreaData.pSysMem = areaTexBytes;
    AreaData.SysMemPitch = AREATEX_PITCH;

    ID3D11Texture2D* pAreaTex = nullptr;
    if (FAILED(m_pDevice->CreateTexture2D(&AreaDesc, &AreaData, &pAreaTex)))
        return E_FAIL;
    if (FAILED(m_pDevice->CreateShaderResourceView(pAreaTex, nullptr, &m_pAreaTexSRV)))
    {
        Safe_Release(pAreaTex);
        return E_FAIL;
    }
    Safe_Release(pAreaTex);

    D3D11_TEXTURE2D_DESC SearchDesc = {};
    SearchDesc.Width = SEARCHTEX_WIDTH;
    SearchDesc.Height = SEARCHTEX_HEIGHT;
    SearchDesc.MipLevels = 1;
    SearchDesc.ArraySize = 1;
    SearchDesc.Format = DXGI_FORMAT_R8_UNORM;
    SearchDesc.SampleDesc.Count = 1;
    SearchDesc.Usage = D3D11_USAGE_IMMUTABLE;
    SearchDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA SearchData = {};
    SearchData.pSysMem = searchTexBytes;
    SearchData.SysMemPitch = SEARCHTEX_PITCH;

    ID3D11Texture2D* pSearchTex = nullptr;
    if (FAILED(m_pDevice->CreateTexture2D(&SearchDesc, &SearchData, &pSearchTex)))
        return E_FAIL;
    if (FAILED(m_pDevice->CreateShaderResourceView(pSearchTex, nullptr, &m_pSearchTexSRV)))
    {
        Safe_Release(pSearchTex);
        return E_FAIL;
    }
    Safe_Release(pSearchTex);

    return S_OK;
    return S_OK;
}

void CRenderer::Bind_TextureStack(CMaterial* pMaterial, TEXTURE_TYPE eType, _uint iSlot, ID3D11ShaderResourceView** pCache, ID3D11ShaderResourceView* pDefaultSRV)
{
    const _uint iCount = (_uint)pMaterial->Get_Textures(eType)->size();

    _bool bChanged = false;
    for (_uint i = 0; i < g_iMaxTextureStack; ++i)
    {
        ID3D11ShaderResourceView* pSRV = nullptr;
        if (i < iCount)
            pSRV = pMaterial->Get_SRV(eType, i);
        else if (0 == iCount && 0 == i)
            pSRV = pDefaultSRV;

        if (pCache[i] != pSRV)
        {
            pCache[i] = pSRV;
            bChanged = true;
        }
    }

    if (true == bChanged)
        m_pContext->PSSetShaderResources(iSlot, g_iMaxTextureStack, pCache);
}

void CRenderer::Submit_Static(StaticCommand& command)
{
    RenderCommand* pAllocatedPtr = (RenderCommand*)m_pAllocator->Allocate(command.header.iCommandSize);
    if (pAllocatedPtr == nullptr)
    {
        MSG_BOX("StaticCommand Allocate Failed!");
        return;
    }
    memcpy(pAllocatedPtr, &command, command.header.iCommandSize);
    m_pCommands.push_back(pAllocatedPtr);

}

void CRenderer::Submit_Skinned(SkinnedCommand& Command)
{
    Command.header.etype = RENDER_TYPE::SKINNED;
    Command.header.iCommandSize = sizeof(SkinnedCommand);

    RenderCommand* pAllocatedPtr = static_cast<RenderCommand*>(m_pAllocator->Allocate(Command.header.iCommandSize));

    if (pAllocatedPtr == nullptr)
    {
        MSG_BOX("SkinnedCommand Allocate Failed!");
        return;
    }

    memcpy(pAllocatedPtr, &Command, sizeof(SkinnedCommand));

    m_pCommands.push_back(pAllocatedPtr);
}


//void CRenderer::Submit_Skinned(SkinnedCommand& Command)
//{
//    RenderCommand* pAllocatedPtr = (RenderCommand*)m_pAllocator->Allocate(Command.header.iCommandSize);
//    if (pAllocatedPtr == nullptr)
//    {
//        MSG_BOX("SkinnedCommand Allocate Failed!");
//        return;
//    }
//
//    memcpy(pAllocatedPtr, &Command, sizeof(SkinnedCommand));
//    //SkinnedCommand의 뒤에 뼈대 행렬 배열을 붙여 복사함
//    uint8_t* arrayPtr = reinterpret_cast<uint8_t*>(pAllocatedPtr) + sizeof(SkinnedCommand);
//    memcpy(arrayPtr, Command.pBones, sizeof(XMFLOAT4X4) * Command.iBoneCount);
//    m_pCommands.push_back(pAllocatedPtr);
//
//}


void CRenderer::Submit_Effect(EffectCommand& Command)
{
    RenderCommand* pAllocatedPtr = (RenderCommand*)m_pAllocator->Allocate(Command.header.iCommandSize);
    if (pAllocatedPtr == nullptr)
    {
        MSG_BOX("EffectCommand Allocate Failed!");
        return;
    }

    memcpy(pAllocatedPtr, &Command, sizeof(EffectCommand));
    m_pCommands.push_back(pAllocatedPtr);
}

void CRenderer::Submit_Static_Resolved(StaticCommand& command, _bool bInstanced)
{
    Resolve_Command(command, false, bInstanced);

    if (command.pMaterial != nullptr && command.pMaterial->Get_RenderGroup() == RENDERID::END)
        return;

    Submit_Static(command);
}

void CRenderer::Submit_Skinned_Resolved(SkinnedCommand& command)
{
    Resolve_Command(command, true, false);

    if (command.pMaterial != nullptr && command.pMaterial->Get_RenderGroup() == RENDERID::END)
        return;

    Submit_Skinned(command);
}

void CRenderer::Resolve_Command(RenderCommand& tCommand, _bool bSkinned, _bool bInstanced)
{
    if (nullptr == tCommand.pMaterial)
        return;

    CShader_Resolver* pResolver = m_pGameInstance->Get_ShaderResolver();
    if (nullptr == pResolver)
        return;

    CShader* pShader = nullptr;

    if (RENDERID::REFLECTION == m_eCurRenderPass)
        pShader = pResolver->Get_ReflectionShader(bInstanced);

    if (nullptr == pShader)
        pShader = pResolver->Resolve(tCommand.pMaterial->Get_Meta_Desc(), bSkinned, bInstanced);

    if (nullptr == pShader)
        return;

    tCommand.pVS = pShader->Get_VertexShader();
    tCommand.pPS = pShader->Get_PixelShader();
    tCommand.pLayout = pShader->Get_InputLayout();

    tCommand.pMaterial->Set_ReflectionInfo(pShader->Get_ReflectionMap(), pShader->Get_MaterialBufferSize());
    tCommand.header.iSortKey = RenderHeader::MakeSortKey(pShader->Get_ID(), tCommand.pMaterial->Get_ID());
}

//VIBUFFER_RECT 대신 SV_VertexID 0,1,2 로 큰 삼각형으로 화면을 채우는 방법 
void CRenderer::Render_FullScreenRect()
{
    ID3D11Buffer* nullBuffer = nullptr;
    UINT stride = 0;
    UINT offset = 0;

    //Vertex Buffer 해제 
    m_pContext->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
    m_pContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    m_pContext->IASetInputLayout(nullptr);

    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->Draw(3, 0);
}

HRESULT CRenderer::Create_Shaders()
{
    /* Register_Shader_Light_Direction*/
    {
        // 디렉셔널 라이트 → MRT_LightAcc (가산 블렌드)
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Light_Direction";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Lighting.hlsl";
        desc.strPShaderFuncName = "PS_LIGHT_DIRECTIONAL";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Light_Point*/
    {
        // 포인트 라이트 → MRT_LightAcc (가산 블렌드)
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Light_Point";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Lighting.hlsl";
        desc.strPShaderFuncName = "PS_LIGHT_POINT";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Light_Spot*/
    {
        // 포인트 라이트 → MRT_LightAcc (가산 블렌드)
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Light_Spot";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Lighting.hlsl";
        desc.strPShaderFuncName = "PS_LIGHT_SPOT";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Combined*/
    {
        // 합성 (Shade+Specular → 톤매핑 → 백버퍼)
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Combined";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Combine.hlsl";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Debug*/
    {
        // 디버그 (RT 단일 출력)
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Debug";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_VtxTex.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";        
        desc.strPShaderFuncName = "PS_MAIN_DEBUG";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Final*/
    {
        // 디버그 (RT 단일 출력)
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Final";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFuncName = "PS_FINAL";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_IBL*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_IBL";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Lighting.hlsl";
        desc.strPShaderFuncName = "PS_IBL_AMBIENT";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_SSAO*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_SSAO";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SSAO.hlsl";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_SSAO_Blur*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_SSAO_Blur";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SSAO.hlsl";
        desc.strPShaderFuncName = "PS_BLUR";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_SSR*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_SSR";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SSR.hlsl";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_ReflComposite*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_ReflComposite";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_ReflectionComposite.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";        
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Weight_Blend_Order_Independent_Transparency*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_WBOITComposite";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_WBOIT_Composite.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Bloom_Prefilter */
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Bloom_Prefilter";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Bloom.hlsl";
        desc.strPShaderFuncName = "PS_PREFILTER";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc))))
            return E_FAIL;
    }
    /* Register_Shader_Bloom_DownSample */
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Bloom_Downsample";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Bloom.hlsl";
        desc.strPShaderFuncName = "PS_DOWNSAMPLE";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc))))
            return E_FAIL;
    }
    /* Register_Shader_Bloom_Upsample */
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Bloom_Upsample";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Bloom.hlsl";
        desc.strPShaderFuncName = "PS_UPSAMPLE";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc))))
            return E_FAIL;
    }
    /* Register_Shader_Bloom_Composite */
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Bloom_Composite";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Bloom.hlsl";
        desc.strPShaderFuncName = "PS_COMPOSITE";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc))))
            return E_FAIL;
    }
    /* Register_Shader_GodRay*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_GodRay";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_GodRay.hlsl";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_GodRayBlur*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_GodRayBlur";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_GodRayBlur.hlsl";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Spot_Static*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Spot_Static";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SpotDepth.hlsl";
        desc.strVShaderFuncName = "VS_MAIN_STATIC";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Spot_Skinned*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Spot_Skinned";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SpotDepth.hlsl";
        desc.strVShaderFuncName = "VS_MAIN_SKINNED";        
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_CSM_Static*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_CSM_Static";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_CSMDepth.hlsl";
        desc.strVShaderFuncName = "VS_MAIN_STATIC";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_CSM_Skinned*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_CSM_Skinned";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_CSMDepth.hlsl";
        desc.strVShaderFuncName = "VS_MAIN_SKINNED";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Point_Static*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Point_Static";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_PointDepth.hlsl";
        desc.strVShaderFuncName = "VS_MAIN_STATIC";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Point_Skinned*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Point_Skinned";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_PointDepth.hlsl";
        desc.strVShaderFuncName = "VS_MAIN_SKINNED";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_Fog*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_Fog";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Fog.hlsl";
        desc.strPShaderFuncName = "PS_MAIN";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_RimLight*/
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_RimLight";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Final.hlsl";
        desc.strVShaderFuncName = "VS_MAIN";
        desc.strPShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Lighting.hlsl";
        desc.strPShaderFuncName = "PS_RIMLIGHT";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    /* Register_Shader_UI */
    {
        CShader::SHADER_DESC UI_ShaderDesc;
        UI_ShaderDesc.strShaderName = L"Shader_UI";
        UI_ShaderDesc.strVShaderFilePath = _wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_UI.hlsl";
        UI_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        UI_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, UI_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_SMAA */
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_SMAA_Edge";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SMAA.hlsl";
        desc.strVShaderFuncName = "VS_EDGE";
        desc.strPShaderFuncName = "PS_EDGE";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_SMAA_Weight";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SMAA.hlsl";
        desc.strVShaderFuncName = "VS_WEIGHT";
        desc.strPShaderFuncName = "PS_WEIGHT";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }
    {
        CShader::SHADER_DESC desc;
        desc.strShaderName = L"Shader_SMAA_Blend";
        desc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SMAA.hlsl";
        desc.strVShaderFuncName = "VS_BLEND";
        desc.strPShaderFuncName = "PS_BLEND";
        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, desc)))) return E_FAIL;
    }

#pragma region Get_Shader

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Light_Direction", &m_pLightDirectShaderCom))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Light_Point", &m_pLightPointShaderCom))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Light_Spot", &m_pLightSpotShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Combined", &m_pCombineShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Final", &m_pFinalShaderCom))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_IBL", &m_pIBLShaderCom))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_SSAO", &m_pSSAOShaderCom))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_SSAO_Blur", &m_pSSAOBlurShaderCom))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_SSR", &m_pSSRShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_WBOITComposite", &m_pWBOITShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_ReflComposite", &m_pReflCompositeShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_GodRay", &m_pGodRayShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_GodRayBlur", &m_pGodRayBlurShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Bloom_Prefilter", &m_pBloomPrefilterShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Bloom_Downsample", &m_pBloomDownShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Bloom_Upsample", &m_pBloomUpShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Bloom_Composite", &m_pBloomCompositeShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Spot_Static", &m_pSpotShader_Static)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Spot_Skinned", &m_pSpotShader_SKinned))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_CSM_Static", &m_pCSMShader_Static))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_CSM_Skinned", &m_pCSMShader_Skinned))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Point_Static", &m_pPointShader_Static))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Point_Skinned", &m_pPointShader_Skinned))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Fog", &m_pFogShaderCom))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_RimLight", &m_pRimShaderCom))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_SMAA_Edge", &m_pSMAAEdgeShaderCom)))   
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_SMAA_Weight", &m_pSMAAWeightShaderCom))) 
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_SMAA_Blend", &m_pSMAABlendShaderCom)))  
        return E_FAIL;

#ifdef _DEBUG
    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Debug", &m_pDebugShaderCom))) return E_FAIL;        
#endif // _DEBUG
#pragma endregion
    return S_OK;
}

HRESULT CRenderer::Create_MRTs()
{
    _float2     vViewportDesc = m_pGameInstance->Get_ViewportDesc();

    /*----------------------------------------- RT-----------------------------------------------------*/

    /* GameObjects*/
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Diffuse"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R8G8B8A8_UNORM, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Normal"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R16G16B16A16_UNORM, _float4(0.f, 0.f, 0.f, 1.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_ORMS"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R8G8B8A8_UNORM, _float4(1.f, 1.f, 0.f, 1.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Depth"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R32G32B32A32_FLOAT, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Emissive"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    /* Light */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Light"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    /* Final */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Final"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_SceneColor"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f))))
        return E_FAIL;

    /* Planar Reflection */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_PlanarReflection"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.35f, 0.45f, 0.55f, 1.f))))
        return E_FAIL;

    /* SSAO_RAW( BLUR 처리 전 )*/
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_SSAO_Raw"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R8_UNORM, _float4(1.f, 1.f, 1.f, 1.f)))) return E_FAIL;

    /* SSAO */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_SSAO"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R8_UNORM, _float4(1.f, 1.f, 1.f, 1.f)))) return E_FAIL;

    /*SSR*/
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Reflection"), ETOUI(vViewportDesc.x * 0.5f), ETOUI(vViewportDesc.y * 0.5f), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_GodRay"), ETOUI(vViewportDesc.x * 0.5f), ETOUI(vViewportDesc.y * 0.5f), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_GodRayBlurH"), ETOUI(vViewportDesc.x * 0.5f), ETOUI(vViewportDesc.y * 0.5f), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_GodRayBlurV"), ETOUI(vViewportDesc.x * 0.5f), ETOUI(vViewportDesc.y * 0.5f), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_SpotShadow"), g_iSpotAtlasSize, g_iSpotAtlasSize, DXGI_FORMAT_R32_FLOAT, _float4(1.f, 1.f, 1.f, 1.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_CSMShadow"), g_iCSMAtlasSize, g_iCSMAtlasSize, DXGI_FORMAT_R32_FLOAT, _float4(1.f, 1.f, 1.f, 1.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_PointShadow"), g_iPointAtlasSize, g_iPointAtlasSize, DXGI_FORMAT_R32_FLOAT, _float4(1.f, 1.f, 1.f, 1.f)))) return E_FAIL;

    /* Bloom : 다운 / 업 샘플 mip 피라미드 */
    {
        const _uint iFullW = ETOUI(vViewportDesc.x);
        const _uint iFullH = ETOUI(vViewportDesc.y);
        for (_uint i = 0; i < BLOOM_MIP_COUNT; ++i)
        {
            const _uint iDiv = 1u << (i + 1);
            _uint iW = iFullW / iDiv; if (iW < 1u) iW = 1u;
            _uint iH = iFullH / iDiv; if (iH < 1u) iH = 1u;
            const _wstring strTarget = _wstring(L"Target_BloomMip") + to_wstring(i);
            if (FAILED(m_pGameInstance->Add_RenderTarget(strTarget, iW, iH, DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f))))
                return E_FAIL;
        }
    }

    /* SMAA */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_LDR"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R8G8B8A8_UNORM, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_SMAA_Edges"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R8G8B8A8_UNORM, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_SMAA_Blend"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R8G8B8A8_UNORM, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_WBOIT_Accum"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f)))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_WBOIT_Reveal"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R16_FLOAT, _float4(1.f, 1.f, 1.f, 1.f)))) return E_FAIL;

    //if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_BloomBlur"), ETOUI(vViewportDesc.x), ETOUI(vViewportDesc.y), DXGI_FORMAT_R16G16B16A16_FLOAT, _float4(0.f, 0.f, 0.f, 0.f))))
    //    return E_FAIL;

    /*-----------------------------------------MRT-----------------------------------------------------*/

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObjects"), TEXT("Target_Diffuse")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObjects"), TEXT("Target_Normal")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObjects"), TEXT("Target_ORMS")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObjects"), TEXT("Target_Depth")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObjects"), TEXT("Target_Emissive")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_LightAcc"), TEXT("Target_Light")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Final"), TEXT("Target_Final")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_PlanarReflection"), TEXT("Target_PlanarReflection")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_WBOIT"), TEXT("Target_WBOIT_Accum")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_WBOIT"), TEXT("Target_WBOIT_Reveal")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_SSAO_Raw"), TEXT("Target_SSAO_Raw")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_SSAO"), TEXT("Target_SSAO")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Reflection"), TEXT("Target_Reflection")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GodRay"), TEXT("Target_GodRay")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Spot"), TEXT("Target_SpotShadow")))) return E_FAIL;

    for (_uint i = 0; i < BLOOM_MIP_COUNT; ++i)
    {
        const _wstring strMRT = _wstring(L"MRT_BloomMip") + to_wstring(i);
        const _wstring strTarget = _wstring(L"Target_BloomMip") + to_wstring(i);
        if (FAILED(m_pGameInstance->Add_MRT(strMRT, strTarget)))
            return E_FAIL;
    }

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_CSM"), TEXT("Target_CSMShadow")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Point"), TEXT("Target_PointShadow")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GodRayBlurH"), TEXT("Target_GodRayBlurH")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GodRayBlurV"), TEXT("Target_GodRayBlurV")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_LDR"), TEXT("Target_LDR"))))        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_SMAA_Edges"), TEXT("Target_SMAA_Edges")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_SMAA_Blend"), TEXT("Target_SMAA_Blend")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Decal"), TEXT("Target_Diffuse")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Decal"), TEXT("Target_Normal")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Decal"), TEXT("Target_ORMS")))) return E_FAIL;

    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Decal"), TEXT("Target_Emissive")))) return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Bake_IBL()
{
    if (m_bIBLBaked || m_pIBLBaker->Is_Baked())
    {
        m_bIBLBaked = true;
        return S_OK;
    }

    CTexture* pSky = nullptr;

    if (FAILED(m_pGameInstance->Get_Texture(TEXT("SkyBox"), &pSky)))
    {
        MSG_BOX("Failed Get SkyBox Texture");
        return E_FAIL;
    }

    if (nullptr == pSky)
        return E_FAIL;

    HRESULT hr = m_pIBLBaker->Bake(pSky->Get_SRV());
    Safe_Release(pSky);
    if (SUCCEEDED(hr))
        m_bIBLBaked = true;
    
    m_tIBLData.fMaxMip = m_pIBLBaker->Get_MaxMip();
    m_tIBLData.fDiffuseIntensity = 1.f;
    m_tIBLData.fSpecularIntensity = 0.25f;
    m_pIBLBuffer->Update_Buffer(&m_tIBLData, 1);
    return hr;
}

HRESULT CRenderer::Command_Execute_Depth(CShader* pStatic, CShader* pSkinned)
{
    if (m_pGameInstance != nullptr)
    {
        if (FAILED(m_pGameInstance->Flush_SkinningJobs()))
            return E_FAIL;
    }

    Clear_LastState();
    ID3D11VertexShader* pVS_Static = pStatic->Get_VertexShader();
    ID3D11VertexShader* pVS_Skinned = pSkinned->Get_VertexShader();
    ID3D11PixelShader* pPS = pStatic->Get_PixelShader();

    for (RenderCommand* pCur : m_pCommands)
    {
        if (pCur->pVIBuffer == nullptr) continue;

        pCur->pVIBuffer->Bind_Resources();
        if (pCur->pLayout) m_pContext->IASetInputLayout(pCur->pLayout);

        const bool bSkinned = (pCur->header.etype == RENDER_TYPE::SKINNED);
        m_pContext->VSSetShader(bSkinned ? pVS_Skinned : pVS_Static, nullptr, 0);
        m_pContext->PSSetShader(pPS, nullptr, 0);

        Bind_ObjectBuffer(pCur);

        if (pCur->iAlphaClip > 0 && pCur->pMaterial)
        {
            ID3D11ShaderResourceView* pMask2 = pCur->pMaterial->Get_SRV(TEXTURE_TYPE::MASK2, 0);
            m_pContext->PSSetShaderResources(SRV_MASK2_SLOT, 1, &pMask2);
        }

        if (bSkinned)
        {
            SkinnedCommand* pSk = static_cast<SkinnedCommand*>(pCur);

            if (pSk->pBoneMatrixSRV == nullptr)
                return E_FAIL;

            m_pContext->VSSetShaderResources(SRV_BONE_MATRIX_SLOT, 1, &pSk->pBoneMatrixSRV);
        }

        if (FAILED(pCur->pVIBuffer->Render())) return E_FAIL;
    }

    m_pCommands.clear();
    m_pAllocator->Clear();
    m_bNeedDepthSort = false;
    return S_OK;
}

HRESULT CRenderer::Render_RenderGroup(RENDERID eGroup)
{
    m_eCurRenderPass = eGroup;

    for (auto& pObject : m_RenderObjects[ETOUI(eGroup)])
    {
        if (nullptr != pObject)
        {
            if (FAILED(pObject->Render()))
            {
                MSG_BOX("Object Render Failed : CRenderer");
            }
            
            Safe_Release(pObject);
        }        
    }
    return S_OK;
}

void CRenderer::Set_Viewport(_float fWidth, _float fHeight)
{
    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.f;
    vp.TopLeftY = 0.f;
    vp.Width = fWidth;
    vp.Height = fHeight;
    vp.MinDepth = 0.f;
    vp.MaxDepth = 1.f;
    m_pContext->RSSetViewports(1, &vp);
}

void CRenderer::Set_ShadowPassState()
{
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);
    m_pContext->RSSetState(m_pShadowScissorRS);
    m_eCurRenderPass = RENDERID::SHADOW;
}

void CRenderer::Set_AtlasTile(_int iTileIndex, _uint iTileSize, _uint iAtlasCols)
{
    const _uint iCol = ETOUI(iTileIndex) % iAtlasCols;
    const _uint iRow = ETOUI(iTileIndex) / iAtlasCols;

    D3D11_VIEWPORT vp{};
    vp.TopLeftX = static_cast<_float>(iCol * iTileSize);
    vp.TopLeftY = static_cast<_float>(iRow * iTileSize);
    vp.Width = vp.Height = static_cast<_float>(iTileSize);
    vp.MaxDepth = 1.f;
    m_pContext->RSSetViewports(1, &vp);

    D3D11_RECT sc{};
    sc.left = static_cast<_long>(iCol * iTileSize);
    sc.top = static_cast<_long>(iRow * iTileSize);
    sc.right = sc.left + static_cast<_long>(iTileSize);
    sc.bottom = sc.top + static_cast<_long>(iTileSize);
    m_pContext->RSSetScissorRects(1, &sc);
}

void CRenderer::Add_FogVolume(const FogVolume& tVolume)
{    
    m_FogVolumes.push_back(tVolume);
}

void CRenderer::Set_ExposureScale(_float fTarget, _float fBlendTime)
{
    _float fClamped = fTarget;
    if (fClamped < 0.05f) fClamped = 0.05f;
    if (fClamped > 8.f)   fClamped = 8.f;

    m_fExposureScaleTarget = fClamped;

    if (fBlendTime <= 0.f)
    {
        m_fExposureScale = m_fExposureScaleTarget;
        m_fExposureBlendRemain = 0.f;
        return;
    }

    m_fExposureBlendRemain = fBlendTime;
}

void CRenderer::Update_ExposureScale(_float fTimeDelta)
{
    if (m_fExposureBlendRemain <= 0.f)
    {
        m_fExposureScale = m_fExposureScaleTarget;
        return;
    }

    if (fTimeDelta >= m_fExposureBlendRemain)
    {
        m_fExposureScale = m_fExposureScaleTarget;
        m_fExposureBlendRemain = 0.f;
        return;
    }

    const _float fRatio = fTimeDelta / m_fExposureBlendRemain;
    m_fExposureScale += (m_fExposureScaleTarget - m_fExposureScale) * fRatio;
    m_fExposureBlendRemain -= fTimeDelta;
}

void CRenderer::Update_SceneTime(_float fTimeDelta)
{
    m_fSceneTime += fTimeDelta;

    if (m_fSceneTime > 3600.f)
        m_fSceneTime -= 3600.f;
}

void CRenderer::Set_ReflectionPlane(_float fHeight, _bool bEnable)
{
    m_fReflectionHeight = fHeight;
    m_bReflectionEnable = bEnable;
}

HRESULT CRenderer::Load_RenderTuning_FromFile()
{
    const _wstring strPath = Get_TuningPath();

    ifstream fin(strPath);
    if (!fin.is_open())
    {
        Save_RenderTuning_ToFile();
        return S_OK;
    }

    json Root = {};
    try { fin >> Root; }
    catch (...) { fin.close(); return S_OK; }
    fin.close();

    if (Root.contains("RenderTuning") && Root["RenderTuning"].is_object())
    {
        const json& j = Root["RenderTuning"];
#define RT_L(n) if (j.contains(#n)) m_tRenderTuning.n = j[#n].get<float>()
        RT_L(fCSMBiasBase);   RT_L(fCSMFilterTexels); RT_L(fSpotBias);      RT_L(fSpotFilterTexels);
        RT_L(fPointBias);     RT_L(fPointFilterTexels); RT_L(fShadowStrength); RT_L(fSSAODiffuseMin);
        RT_L(fAmbientDesat);  RT_L(fSSAORadius);      RT_L(fSSAOBias);      RT_L(fSSAOPower);
        RT_L(fSSAOStrength);  RT_L(fSSRMaxDistance);  RT_L(fSSRThickness);  RT_L(fSSRRoughCutoff);
        RT_L(fSSRBias);       RT_L(fGodRayG);         RT_L(fGodRayDensity); RT_L(fGodRayIntensity);
        RT_L(fGodRayMaxDist); RT_L(fExposure);
        RT_L(fGamma);         RT_L(fBloomThreshold);  RT_L(fBloomKnee);     RT_L(fBloomIntensity);
        RT_L(fEmissiveScale); RT_L(fLampEmissiveScale);
        RT_L(fGlassBaseAlpha); RT_L(fGlassTint); RT_L(fGlassFresnel);
        RT_L(fGlassReflect);   RT_L(fGlassSpec); RT_L(fGlassDirtAlpha);
        RT_L(fFogDensity);    RT_L(fFogStart);        RT_L(fFogHeight);
        RT_L(fFogHeightFalloff); RT_L(fFogFeather);
#undef RT_L
        if (j.contains("vAmbientFloor") && j["vAmbientFloor"].is_array() && j["vAmbientFloor"].size() >= 3)
        {
            m_tRenderTuning.vAmbientFloor.x = j["vAmbientFloor"][0].get<float>();
            m_tRenderTuning.vAmbientFloor.y = j["vAmbientFloor"][1].get<float>();
            m_tRenderTuning.vAmbientFloor.z = j["vAmbientFloor"][2].get<float>();
        }
        if (j.contains("vFogColor") && j["vFogColor"].is_array() && j["vFogColor"].size() >= 3)
        {
            m_tRenderTuning.vFogColor.x = j["vFogColor"][0].get<float>();
            m_tRenderTuning.vFogColor.y = j["vFogColor"][1].get<float>();
            m_tRenderTuning.vFogColor.z = j["vFogColor"][2].get<float>();
        }
        if (j.contains("iGlobalFogEnable"))
            m_tRenderTuning.iGlobalFogEnable = j["iGlobalFogEnable"].get<int>();
    }
    return S_OK;
}

HRESULT CRenderer::Save_RenderTuning_ToFile()
{
    Ensure_TuningDir();
    const _wstring strPath = Get_TuningPath();

    json Root = {};
    {
        ifstream fin(strPath);
        if (fin.is_open()) { try { fin >> Root; } catch (...) { Root = json{}; } fin.close(); }
    }
    Root["Version"] = 1;

    json RT = {};
#define RT_F(n) RT[#n] = m_tRenderTuning.n
    RT_F(fCSMBiasBase);   RT_F(fCSMFilterTexels); RT_F(fSpotBias);      RT_F(fSpotFilterTexels);
    RT_F(fPointBias);     RT_F(fPointFilterTexels); RT_F(fShadowStrength); RT_F(fSSAODiffuseMin);
    RT_F(fAmbientDesat);  RT_F(fSSAORadius);      RT_F(fSSAOBias);      RT_F(fSSAOPower);
    RT_F(fSSAOStrength);  RT_F(fSSRMaxDistance);  RT_F(fSSRThickness);  RT_F(fSSRRoughCutoff);
    RT_F(fSSRBias);       RT_F(fGodRayG);         RT_F(fGodRayDensity); RT_F(fGodRayIntensity);
    RT_F(fGodRayMaxDist); RT_F(fExposure);
    RT_F(fGamma);         RT_F(fBloomThreshold);  RT_F(fBloomKnee);     RT_F(fBloomIntensity);
    RT_F(fEmissiveScale); RT_F(fLampEmissiveScale);
    RT_F(fGlassBaseAlpha); RT_F(fGlassTint); RT_F(fGlassFresnel);
    RT_F(fGlassReflect);   RT_F(fGlassSpec); RT_F(fGlassDirtAlpha);
    RT_F(fFogDensity);    RT_F(fFogStart);        RT_F(fFogHeight);
    RT_F(fFogHeightFalloff); RT_F(fFogFeather);
#undef RT_F
    RT["vAmbientFloor"] = { m_tRenderTuning.vAmbientFloor.x, m_tRenderTuning.vAmbientFloor.y, m_tRenderTuning.vAmbientFloor.z };
    RT["vFogColor"] = { m_tRenderTuning.vFogColor.x, m_tRenderTuning.vFogColor.y, m_tRenderTuning.vFogColor.z };
    RT["iGlobalFogEnable"] = m_tRenderTuning.iGlobalFogEnable;
    Root["RenderTuning"] = RT;

    ofstream fout(strPath, ios::out | ios::trunc);
    if (!fout.is_open())
        return E_FAIL;
    fout << Root.dump(4);
    fout.close();
    return S_OK;
}

HRESULT CRenderer::Build_ShadowViews()
{
    ShadowViewGPU tViews[SHADOW_VIEW_COUNT] = {};

    // SPOT [0..3]
    for (_int i = 0; i < ETOI(g_iMaxSpots); ++i)
    {
        tViews[SHADOW_SPOT_BASE + i].ViewProj = m_tSpotData.LightViewProj[i];
        tViews[SHADOW_SPOT_BASE + i].AtlasRect = m_tSpotData.AtlasRect[i];
    }

    // CSM [4..7]
    for (_int i = 0; i < ETOI(g_iMaxCSM); ++i)
    {
        tViews[SHADOW_CSM_BASE + i].ViewProj = m_tCSMData.CascadeViewProj[i];
        tViews[SHADOW_CSM_BASE + i].AtlasRect = m_tCSMData.CascadeAtlasRect[i];
    }

    // Point [8..19]  (2광 × 6면 = 12)
    for (_int i = 0; i < 6 * ETOI(g_iMaxPoints); ++i)
    {
        tViews[SHADOW_POINT_BASE + i].ViewProj = m_tPointData.FaceViewProj[i];
        tViews[SHADOW_POINT_BASE + i].AtlasRect = m_tPointData.FaceAtlasRect[i];
    }

    m_pShadowViewsBuffer->Update_Buffer(tViews, SHADOW_VIEW_COUNT);

    ShadowGlobalsData& g = m_tShadowGlobals;
    g.iSpotBase = SHADOW_SPOT_BASE;   g.iSpotCount = (int)m_tSpotData.iShadowCount;
    g.iSpotTile = ETOI(g_iSpotTileSize);   g.iSpotEnabled = (m_tSpotData.iShadowCount > 0) ? 1 : 0;

    g.iCSMBase = SHADOW_CSM_BASE;     g.iCSMCount = m_tCSMData.iCascadeCount;
    g.iCSMTile = ETOI(g_iCSMTileSize);     g.iCSMEnabled = m_tCSMData.iCSMEnabled;

    g.iPointBase = SHADOW_POINT_BASE; g.iPointCount = m_tPointData.iPointShadowCount;
    g.iPointTile = ETOI(g_iPointTileSize); g.iPointEnabled = (m_tPointData.iPointShadowCount > 0) ? 1 : 0;

    g.iCurView = 0;
    g.CSMSplitViewZ = m_tCSMData.CascadeSplitViewZ;
    g.PointLightPosRange[0] = m_tPointData.LightPosRange[0];
    g.PointLightPosRange[1] = m_tPointData.LightPosRange[1];

    m_pShadowGlobalsBuffer->Update_Data(&g, sizeof(ShadowGlobalsData));

    return S_OK;
}

HRESULT CRenderer::Build_SpotData()
{
    constexpr float SHADOW_HOLD_BONUS = 0.75f;
    constexpr float SADOW_OFFSCREEN_PENALTY = 1.0e9f;

    ZeroMemory(&m_tSpotData, sizeof(SpotData));
    m_tSpotData.iTitleSizePixels = ETOI(g_iSpotTileSize);

    const _uint iSpotCount = m_pGameInstance->Get_LightCount(LIGHT::SPOT);
    if (0 == iSpotCount)
        return S_OK;

    // 직전 프레임 소유자 보관 후 현재 소유자 리셋
    memcpy(m_PrevSlotOwnerSpot, m_SlotOwnerSpot, sizeof(m_SlotOwnerSpot));
    for (_int i = 0; i < ETOI(g_iMaxSpots); i++)
        m_SlotOwnerSpot[i] = -1;

    // 직전 프레임 슬롯 보유 여부 
    auto WasHeld = [this](_uint iSpot) ->bool
    {
        for (_int i = 0; i < ETOI(g_iMaxSpots); i++)            
            if (m_PrevSlotOwnerSpot[i] == ETOI(iSpot)) return true;
        return false;
    };

    // 카메라 월드 프러스텀 + 카메라 위치
    BoundingFrustum frustum{};
    {
        _matrix matProj = XMLoadFloat4x4(m_pGameInstance->Get_Transform(D3DTS::PROJ));
        _matrix matViewInv = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Inverse(D3DTS::VIEW));
        BoundingFrustum::CreateFromMatrix(frustum, matProj);
        frustum.Transform(frustum, matViewInv);
    }
    _vector vCamPos = XMLoadFloat4(m_pGameInstance->Get_CamPosition());

    // top-Max 선택을 고정 배열 삽입정렬로  
    struct CAND { int iSpot; bool bVisible; float fSort; };
    CAND best[g_iMaxSpots] = {};
    int  iBest = 0;
    
    for (_uint i = 0; i < iSpotCount; i++)
    {
        //TODO : 0625 라이트 끄기 KSB 
        if (false == m_pGameInstance->Is_LightEnabled(LIGHT::SPOT, i))
            continue;

        const LIGHT_DESC* pDesc = m_pGameInstance->Get_LightDesc(LIGHT::SPOT, i);
        if (nullptr == pDesc || false == pDesc->bShadow)
            continue;

        _vector vPos = XMLoadFloat4(&pDesc->vPosition);

        BoundingSphere sphere;
        XMStoreFloat3(&sphere.Center, vPos);
        sphere.Radius = max(pDesc->fRange, 1.f);

        const _bool bVisible = (frustum.Intersects(sphere) == true);
        _float       fDist = XMVectorGetX(XMVector3Length(vPos - vCamPos));

        if (WasHeld(i))
            fDist *= SHADOW_HOLD_BONUS;
        _float fSort = bVisible ? fDist : (fDist + SADOW_OFFSCREEN_PENALTY);

        if (iBest < ETOI(g_iMaxSpots) || fSort < best[g_iMaxSpots - 1].fSort)
        {
            int p = (iBest < ETOI(g_iMaxSpots)) ? iBest++ : ETOI(g_iMaxSpots - 1);
            for (;p > 0 && best[p - 1].fSort > fSort; --p)
                best[p] = best[p - 1];
            best[p] = CAND{ ETOI(i), bVisible, fSort };
        }
    }

    const _float fInvCols = 1.f / static_cast<_float>(g_iSpotAtlasCols);
    _int iSlot = 0;
    for (_int b = 0; b < iBest; b++)
    {
        const CAND& c = best[b];
        if (!c.bVisible)
            break;

        const LIGHT_DESC* pDesc = m_pGameInstance->Get_LightDesc(LIGHT::SPOT, ETOUI(c.iSpot));

        _vector vEye = XMLoadFloat4(&pDesc->vPosition);
        _vector vDir = XMVector3Normalize(XMLoadFloat4(&pDesc->vDirection));
        _vector vUp = (fabsf(XMVectorGetY(vDir)) > 0.99f)
            ? XMVectorSet(0.f, 0.f, 1.f, 0.f) : XMVectorSet(0.f, 1.f, 0.f, 0.f);
        _matrix matView = XMMatrixLookToLH(vEye, vDir, vUp);

        _float fOuter = acosf(min(max(pDesc->fOuterCos, -1.f), 1.f));
        _float fFovY = min(fOuter * 2.f, XM_PI * 0.95f);
        _matrix matProj = XMMatrixPerspectiveFovLH(fFovY, 1.f, 1.f, max(pDesc->fRange, 2.f));

        XMStoreFloat4x4(&m_tSpotData.LightViewProj[iSlot],Build_LightViewProj(vEye, vDir, vUp, matProj));

        const _uint col = ETOUI(iSlot) % g_iSpotAtlasCols;
        const _uint row = ETOUI(iSlot) / g_iSpotAtlasCols;
        m_tSpotData.AtlasRect[iSlot] = XMFLOAT4(col * fInvCols, row * fInvCols, fInvCols, fInvCols);

        m_SlotOwnerSpot[iSlot] = c.iSpot;
        ++iSlot;
    }

    m_tSpotData.iShadowCount = iSlot;

    return S_OK;
}

HRESULT CRenderer::Build_CSMData()
{
    constexpr _float CSM_SPLIT_LAMBDA = 0.92f;    // log/uniform 블렌드
    constexpr _float CSM_SHADOW_DIST = 120.f;     // CSM 최대 거리 캡
    constexpr _float CSM_Z_EXTRUDE = 10.f;        // 라이트 near 끌어당김

    ZeroMemory(&m_tCSMData, sizeof(CSMData));
    m_tCSMData.iCSMTileSize = ETOI(g_iCSMTileSize);
    m_tCSMData.iCascadeCount = ETOI(g_iMaxCSM);

    // 디렉셔널 라이트 존재 여부 + bShadow 켜져있는지
    const LIGHT_DESC* pDir = m_pGameInstance->Get_LightDesc(LIGHT::DIRECTIONAL, 0);
    m_bCSMEnabled = (pDir != nullptr && pDir->bShadow);
    m_tCSMData.iCSMEnabled = m_bCSMEnabled ? 1 : 0;
    if (!m_bCSMEnabled)
        return S_OK;

    // Proj 에서 near far fov aspect 추출    
    const _float4x4* pProj = m_pGameInstance->Get_Transform(D3DTS::PROJ);
    const _float fM33 = pProj->_33, fM43 = pProj->_43;
    const _float fM22 = pProj->_22, fM11 = pProj->_11;

    _float fNear = -fM43 / fM33;
    _float fFar = -fM43 / (fM33 - 1.f);
    fFar = min(fFar, CSM_SHADOW_DIST);

    const _float fTanHalfFovY = 1.f / fM22;
    const _float fAspect = fM22 / fM11;

    _matrix matViewInv = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Inverse(D3DTS::VIEW));
    _vector vDir = XMVector3Normalize(XMLoadFloat4(&pDir->vDirection));
    _vector vUp = (fabsf(XMVectorGetY(vDir)) > 0.99f)
        ? XMVectorSet(0.f, 0.f, 1.f, 0.f) : XMVectorSet(0.f, 1.f, 0.f, 0.f);

    // 실용 분할 거리
    const _int iCascadeCount = ETOI(g_iMaxCSM);
    _float fSplit[8];
    fSplit[0] = fNear;
    fSplit[iCascadeCount] = fFar;
    for (_int i = 1; i < iCascadeCount; ++i)
    {
        _float fRatio = static_cast<_float>(i) / static_cast<_float>(iCascadeCount);
        _float fLogSplit = fNear * powf(fFar / fNear, fRatio);
        _float fUniformSplit = fNear + (fFar - fNear) * fRatio;
        fSplit[i] = CSM_SPLIT_LAMBDA * fLogSplit + (1.f - CSM_SPLIT_LAMBDA) * fUniformSplit;
    }

    const _float fInvCols = 1.f / static_cast<_float>(g_iCSMAtlasCols);
    _float* pSplitOut = reinterpret_cast<_float*>(&m_tCSMData.CascadeSplitViewZ);

    for (_int iCascade = 0; iCascade < iCascadeCount; ++iCascade)
    {
        const _float fNearZ = fSplit[iCascade];
        const _float fFarZ = fSplit[iCascade + 1];

        // 서브 - 프러스텀 8 코너 ( 뷰공간 -> 월드 )
        _vector vCorners[8];
        _int    iCorner = 0;
        const _float fSplitZ[2] = { fNearZ, fFarZ };
        for (_int iZ = 0; iZ < 2; ++iZ)
        {
            _float fZ = fSplitZ[iZ];
            _float fExtentX = fZ * fTanHalfFovY * fAspect;
            _float fExtentY = fZ * fTanHalfFovY;
            vCorners[iCorner++] = XMVector3Transform(XMVectorSet(-fExtentX, -fExtentY, fZ, 1.f), matViewInv);
            vCorners[iCorner++] = XMVector3Transform(XMVectorSet(fExtentX, -fExtentY, fZ, 1.f), matViewInv);
            vCorners[iCorner++] = XMVector3Transform(XMVectorSet(-fExtentX, fExtentY, fZ, 1.f), matViewInv);
            vCorners[iCorner++] = XMVector3Transform(XMVectorSet(fExtentX, fExtentY, fZ, 1.f), matViewInv);
        }

        // 바운딩 스피어 ( 회전 불변 -> Ortho 크기 고정 ) 
        _vector vCenter = XMVectorZero();
        for (_int i = 0; i < 8; ++i)
            vCenter += vCorners[i];
        vCenter /= 8.f;
        _float fRadius = 0.f;
        for (_int i = 0; i < 8; ++i)
            fRadius = max(fRadius, XMVectorGetX(XMVector3Length(vCorners[i] - vCenter)));
        fRadius = ceilf(fRadius * 16.f) / 16.f;     // 거친 그리드로 반올림 -> 크기 떨림 제거

        // 라이트 뷰 ( 광원 방향 뒤로 끌어당겨 캐스터 포함 ) 
        _vector vEye = vCenter - vDir * (fRadius + CSM_Z_EXTRUDE);
        _matrix matLightView = XMMatrixLookToLH(vEye, vDir, vUp);

        // 텍셀 스냅 ( 라이트 공간에서 center 를 텍셀 그리드에 정렬 ) 
        _vector vOrigin = XMVector3Transform(vCenter, matLightView);
        const _float fTexelWorld = (2.f * fRadius) / static_cast<_float>(g_iCSMTileSize);
        _float fSnapX = floorf(XMVectorGetX(vOrigin) / fTexelWorld) * fTexelWorld - XMVectorGetX(vOrigin);
        _float fSnapY = floorf(XMVectorGetY(vOrigin) / fTexelWorld) * fTexelWorld - XMVectorGetY(vOrigin);        

        // Ortho
        _matrix matOrtho = XMMatrixOrthographicOffCenterLH(
            -fRadius - fSnapX, fRadius - fSnapX,
            -fRadius - fSnapY, fRadius - fSnapY,
            0.f, 2.f * fRadius + 2.f * CSM_Z_EXTRUDE);

        // 결합 전치
        XMStoreFloat4x4(&m_tCSMData.CascadeViewProj[iCascade],Build_LightViewProj(vEye, vDir, vUp, matOrtho));

        // 아틀라스
        const _uint iCol = ETOUI(iCascade) % g_iCSMAtlasCols;
        const _uint iRow = ETOUI(iCascade) / g_iCSMAtlasCols;
        m_tCSMData.CascadeAtlasRect[iCascade] = XMFLOAT4(iCol * fInvCols, iRow * fInvCols, fInvCols, fInvCols);
        
        pSplitOut[iCascade] = fFarZ;
    }

    return S_OK;
}

HRESULT CRenderer::Build_PointData()
{
    ZeroMemory(&m_tPointData, sizeof(PointData));
    m_tPointData.iPointTileSize = ETOI(g_iPointTileSize);

    const _uint iPointCount = m_pGameInstance->Get_LightCount(LIGHT::POINT);
    if (0 == iPointCount)
        return S_OK;
    
    memcpy(m_PrevSlotOwnerPoint, m_SlotOwnerPoint, sizeof(m_SlotOwnerPoint));
    for (_int i = 0; i < ETOI(g_iMaxPoints); ++i)
        m_SlotOwnerPoint[i] = -1;
    
    BoundingFrustum tFrustum{};
    {
        _matrix matProjC = XMLoadFloat4x4(m_pGameInstance->Get_Transform(D3DTS::PROJ));
        _matrix matViewInv = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Inverse(D3DTS::VIEW));
        BoundingFrustum::CreateFromMatrix(tFrustum, matProjC);
        tFrustum.Transform(tFrustum, matViewInv);
    }
    _vector vCamPos = XMLoadFloat4(m_pGameInstance->Get_CamPosition());
    
    struct CAND { _int iPoint; _bool bVisible; _float fSort; };
    CAND tBest[g_iMaxPoints] = {};
    _int iBestCount = 0;
    for (_uint i = 0; i < iPointCount; ++i)
    {
        // 0625 조명 끄기 추가 KSB
        if (false == m_pGameInstance->Is_LightEnabled(LIGHT::POINT, i))
            continue;

        const LIGHT_DESC* pDesc = m_pGameInstance->Get_LightDesc(LIGHT::POINT, i);
        if (nullptr == pDesc || false == pDesc->bShadow)
            continue;

        _vector vPos = XMLoadFloat4(&pDesc->vPosition);
        BoundingSphere tSphere; XMStoreFloat3(&tSphere.Center, vPos);
        tSphere.Radius = max(pDesc->fRange, 1.f);

        const _bool bVisible = (tFrustum.Intersects(tSphere) == true);
        _float fSort = XMVectorGetX(XMVector3Length(vPos - vCamPos));
        if (!bVisible) fSort += 1.0e9f;

        if (iBestCount < ETOI(g_iMaxPoints) || fSort < tBest[g_iMaxPoints - 1].fSort)
        {
            _int iIns = (iBestCount < ETOI(g_iMaxPoints)) ? iBestCount++ : ETOI(g_iMaxPoints - 1);
            for (; iIns > 0 && tBest[iIns - 1].fSort > fSort; --iIns) tBest[iIns] = tBest[iIns - 1];
            tBest[iIns] = CAND{ ETOI(i), bVisible, fSort };
        }
    }

    const _float fTileUV = static_cast<_float>(g_iPointTileSize) / static_cast<_float>(g_iPointAtlasSize);
    _int iSlot = 0;
    for (_int b = 0; b < iBestCount; ++b)
    {
        if (!tBest[b].bVisible) break;
        const LIGHT_DESC* pDesc = m_pGameInstance->Get_LightDesc(LIGHT::POINT, ETOUI(tBest[b].iPoint));

        _vector vEye = XMLoadFloat4(&pDesc->vPosition);
        _matrix matProj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.05f, max(pDesc->fRange, 2.f));

        for (_int f = 0; f < 6; ++f)
        {
            _vector vDir = XMLoadFloat3(&s_PointFaceDir[f]);
            _vector vUp = XMLoadFloat3(&s_PointFaceUp[f]);
            const _int t = iSlot * 6 + f;            
            XMStoreFloat4x4(&m_tPointData.FaceViewProj[t], Build_LightViewProj(vEye, vDir, vUp, matProj));

            const _uint col = ETOUI(t) % g_iPointAtlasCols;
            const _uint row = ETOUI(t) / g_iPointAtlasCols;
            m_tPointData.FaceAtlasRect[t] = XMFLOAT4(col * fTileUV, row * fTileUV, fTileUV, fTileUV);
        }
        m_tPointData.LightPosRange[iSlot] = pDesc->vPosition;
        m_tPointData.LightPosRange[iSlot].w = pDesc->fRange;
        m_SlotOwnerPoint[iSlot] = tBest[b].iPoint;
        ++iSlot;
    }
    m_tPointData.iPointShadowCount = iSlot;

    return S_OK;
}

_int CRenderer::Find_ShadowSlot_OfSpot(_uint iSpotIndex) const
{
    for (_int i = 0; i < ETOI(m_tSpotData.iShadowCount); ++i)
        if (m_SlotOwnerSpot[i] == ETOI(iSpotIndex))
            return i;
    return -1;
}

_int CRenderer::Find_ShadowSlot_OfPoint(_uint iPointIndex) const
{
    for (_int i = 0; i < m_tPointData.iPointShadowCount; ++i)
        if (m_SlotOwnerPoint[i] == ETOI(iPointIndex))
            return i;
    return -1;
}

HRESULT CRenderer::Render_Test()
{
    if (!m_pAllocator) return E_FAIL;

    if (FAILED(Render_RenderGroup(RENDERID::TEST)))
        return E_FAIL;

    //모인 Command 실행 
    HRESULT hr = Command_Execute();
    m_RenderObjects[ETOUI(RENDERID::TEST)].clear();
    return hr;
}

HRESULT CRenderer::Render_Priority()
{
    if (!m_pAllocator) return E_FAIL;

    m_eCurRenderPass = RENDERID::PRIORITY;

    m_pContext->RSSetState(m_pNoneCullRS);                         
    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0); 
   
    if (FAILED(Render_RenderGroup(RENDERID::PRIORITY)))
        return E_FAIL;

    //모인 Command 실행 
    HRESULT hr = Command_Execute();

    m_RenderObjects[ETOUI(RENDERID::PRIORITY)].clear();
    m_pContext->RSSetState(m_pDefaultRS);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);
    return hr;
}

HRESULT CRenderer::Render_Shadow()
{
    if(FAILED(Build_SpotData())) return E_FAIL;        
    if (FAILED(Build_CSMData())) return E_FAIL;        
    if (FAILED(Build_PointData())) return E_FAIL;        
    if (FAILED(Build_ShadowViews())) return E_FAIL;        

    ID3D11ShaderResourceView* pViewsSRV = m_pShadowViewsBuffer->Get_SRV();
    m_pContext->VSSetShaderResources(SRV_SHADOW_VIEW_BUFFER_SLOT, 1, &pViewsSRV);    

    auto& shadowObjs = m_RenderObjects[ETOUI(RENDERID::SHADOW)];
    const _bool bSpot = (m_tSpotData.iShadowCount > 0);
    const _bool bPoint = (m_tPointData.iPointShadowCount > 0);

    if (bSpot || m_bCSMEnabled || bPoint)
        Set_ShadowPassState();

    if (bSpot)
        if (FAILED(Render_SPOT(shadowObjs)))
            return E_FAIL;

    if (m_bCSMEnabled)
        if (FAILED(Render_CSM(shadowObjs)))
            return E_FAIL;

    if (bPoint)
        if (FAILED(Render_POINT(shadowObjs)))
            return E_FAIL;

    for (auto& pObject : shadowObjs) Safe_Release(pObject);
    shadowObjs.clear();

    m_pContext->RSSetState(m_pDefaultRS);

    const _float2 vFull = m_pGameInstance->Get_ViewportDesc();
    Set_Viewport(vFull.x, vFull.y);

    return S_OK;
}

HRESULT CRenderer::Render_NonBlend()
{
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_GameObjects"))))
        return E_FAIL;

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);
    m_pContext->RSSetState(m_pDefaultRS);

    if (FAILED(Render_RenderGroup(RENDERID::NONBLEND)))
        return E_FAIL;
    
    HRESULT hr = Command_Execute();
    m_RenderObjects[ETOUI(RENDERID::NONBLEND)].clear();

    m_pContext->RSSetState(m_pDefaultRS);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return hr;
}

HRESULT CRenderer::Render_Decals()
{
    if (m_Decals.empty())
        return S_OK;

    m_Decals.sort([](CDecal* pA, CDecal* pB)
        {
            return pA->Get_SortPriority() < pB->Get_SortPriority();
        });

    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Decal"))))
        return E_FAIL;

    m_pContext->OMSetDepthStencilState(m_pDepth_Decal_ReceiverTest, DECAL_RECEIVER_STENCIL);
    m_pContext->OMSetBlendState(m_pAlphaBlendState, nullptr, 0xffffffff);
    m_pContext->RSSetState(m_pDefaultRS);

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))
        return E_FAIL;

    for (auto& pDecal : m_Decals)
    {
        if (pDecal != nullptr)
        {
            pDecal->Render();
            Safe_Release(pDecal);
        }
    }
    m_Decals.clear();

    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    ID3D11ShaderResourceView* pNullSRVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    m_pContext->PSSetShaderResources(0, 5, pNullSRVs);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_Lights()
{
    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);

    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_LightAcc"))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Diffuse"), SRV_RTV_DIFFUSE_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Normal"), SRV_RTV_NORMAL_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_ORMS"), SRV_RTV_ORMS_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_SSAO"), SRV_SSAO_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_SpotShadow"), SRV_SPOT_SHADOW_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_CSMShadow"), SRV_CSM_SHADOW_SLOT)))
        return E_FAIL;

    if(FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_PointShadow"), SRV_POINT_SHADOW_SLOT)))
        return E_FAIL;

    // 가산 블렌드 
    m_pContext->OMSetBlendState(m_pAdditiveBlendState, nullptr, 0xffffffff);

    CShader* pLightShaders[ETOUI(LIGHT::END)] =
    {
        m_pLightDirectShaderCom,
        m_pLightPointShaderCom,
        m_pLightSpotShaderCom
    };

    for (_uint eType = 0; eType < ETOUI(LIGHT::END); eType++)
    {
        CShader* pShader = pLightShaders[eType];
        if (nullptr == pShader)
            continue;
        const _uint iCount = m_pGameInstance->Get_LightCount(static_cast<LIGHT>(eType));

        for (_uint iIndex = 0; iIndex < iCount; ++iIndex)
        {
            if (nullptr == m_pGameInstance->Get_LightDesc(static_cast<LIGHT>(eType), iIndex))
                continue;

            //TODO : 0625 라이트 끄기 KSB 
            if (false == m_pGameInstance->Is_LightEnabled(static_cast<LIGHT>(eType), iIndex))
                continue;

            _int iSI = -1;
            if (static_cast<LIGHT>(eType) == LIGHT::SPOT)
                iSI = Find_ShadowSlot_OfSpot(iIndex);
            else if ((LIGHT)eType == LIGHT::POINT)
                iSI = Find_ShadowSlot_OfPoint(iIndex);

            if (FAILED(Bind_LightBuffer((LIGHT)eType, iIndex, iSI)))
                break;

            pShader->Render();
            Render_FullScreenRect();
        }       
    }
          
    if (m_bIBLBaked)
    {
        ID3D11ShaderResourceView* pIrr = m_pIBLBaker->Get_Irradiance_SRV();
        ID3D11ShaderResourceView* pPre = m_pIBLBaker->Get_Prefiltered_SRV();
        ID3D11ShaderResourceView* pLut = m_pIBLBaker->Get_BRDFLUT_SRV();

        m_pContext->PSSetShaderResources(SRV_IRRADIANCE_SLOT, 1, &pIrr);
        m_pContext->PSSetShaderResources(SRV_PREFILTERED_SLOT, 1, &pPre);
        m_pContext->PSSetShaderResources(SRV_BRDFLUT_SLOT, 1, &pLut);

        m_pIBLShaderCom->Render();
        Render_FullScreenRect();

        Clear_BindingSRV(SRV_IRRADIANCE_SLOT, 3);
        Clear_BindingSRV(SRV_IBL_BUFFER_SLOT, 3);

    }

    if (m_pRimShaderCom != nullptr)
    {
        m_pRimShaderCom->Render();
        Render_FullScreenRect();
    }

    // 가산 블렌드 
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    //SRV로 바인딩되어있는데 MRT로 사용하면 경고가 뜨므로, 슬롯 초기화  
    Clear_BindingSRV(SRV_ALBEDO_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_DIFFUSE_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_NORMAL_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_ORMS_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);
    Clear_BindingSRV(SRV_SSAO_SLOT, 1);
    Clear_BindingSRV(SRV_SPOT_SHADOW_SLOT, 1);
    Clear_BindingSRV(SRV_CSM_SHADOW_SLOT, 1);
    Clear_BindingSRV(SRV_POINT_SHADOW_SLOT, 1);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    Bind_LightBuffer(LIGHT::DIRECTIONAL, 0);

    return S_OK;
}

HRESULT CRenderer::Render_SSR()
{
    if (!m_bIBLBaked)
        return S_OK;

    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_Reflection"), nullptr, false)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Diffuse"), SRV_RTV_DIFFUSE_SLOT)))  return E_FAIL;
    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Normal"), SRV_RTV_NORMAL_SLOT)))   return E_FAIL;
    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_ORMS"), SRV_RTV_ORMS_SLOT)))     return E_FAIL;
    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))    return E_FAIL;
    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Final"), SRV_RTV_LIGHT_SLOT)))    return E_FAIL;

    ID3D11ShaderResourceView* pPre = m_pIBLBaker->Get_Prefiltered_SRV();
    ID3D11ShaderResourceView* pLut = m_pIBLBaker->Get_BRDFLUT_SRV();
    m_pContext->PSSetShaderResources(SRV_PREFILTERED_SLOT, 1, &pPre);
    m_pContext->PSSetShaderResources(SRV_BRDFLUT_SLOT, 1, &pLut);

    m_pSSRShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(SRV_RTV_DIFFUSE_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_NORMAL_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_ORMS_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_LIGHT_SLOT, 1);
    Clear_BindingSRV(SRV_PREFILTERED_SLOT, 2);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_GodRay()
{
    m_bGodRayActiveThisFrame = false;


    const _uint iCount = m_pGameInstance->Get_LightCount(LIGHT::SPOT);
    if (iCount == 0)
        return S_OK;

    _bool bAny = false;
    for (_uint i = 0; i < iCount; i++)
    {
        //TODO : 0625 라이트 끄기 KSB 
        if (false == m_pGameInstance->Is_LightEnabled(LIGHT::SPOT, i))
            continue;

        const LIGHT_DESC* pDesc = m_pGameInstance->Get_LightDesc(LIGHT::SPOT, i);
        if (pDesc == nullptr)
            continue;
        if (true == pDesc->bGodRay)
        {
            bAny = true;
            break;
        }
    }

    if (false == bAny)
        return S_OK;

    m_bGodRayActiveThisFrame = true;

    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);


    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_GodRay"), nullptr, false)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_SpotShadow"), SRV_SPOT_SHADOW_SLOT)))
        return E_FAIL;
    

    m_pContext->OMSetBlendState(m_pAdditiveBlendState, nullptr, 0xffffffff);

    for (_uint i = 0; i < iCount; i++)
    {
        //TODO : 0625 라이트 끄기 KSB 
        if (false == m_pGameInstance->Is_LightEnabled(LIGHT::SPOT, i))
            continue;
        const LIGHT_DESC* pDesc = m_pGameInstance->Get_LightDesc(LIGHT::SPOT, i);
        if (nullptr == pDesc || !pDesc->bGodRay)
            continue;

        _int iSI = Find_ShadowSlot_OfSpot(i);
        if (FAILED(Bind_LightBuffer(LIGHT::SPOT, i, iSI)))
            continue;

        m_pGodRayShaderCom->Render();
        Render_FullScreenRect();
    }

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);
    Clear_BindingSRV(SRV_SPOT_SHADOW_SLOT, 1);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    
    Bind_LightBuffer(LIGHT::DIRECTIONAL, 0);

    return S_OK;
}

HRESULT CRenderer::Render_GodRay_Blur()
{
    if (false == m_bGodRayActiveThisFrame)
        return S_OK;

    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    GodRayBlurData tData{};
    tData.fDepthFallOff = 4.0f;

    // ---- H: Target_GodRay(t29) -> Target_GodRayBlurH ----
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_GodRayBlurH"), nullptr, false)))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_GodRay"), SRV_RTV_REFLECTION_SLOT)))  // t29
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))
        return E_FAIL;

    tData.vBlurDir = _float2(1.f, 0.f);
    if (FAILED(m_pGodRayBlurBuffer->Update_Data(&tData, sizeof(GodRayBlurData))))
        return E_FAIL;

    m_pGodRayBlurShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(SRV_RTV_REFLECTION_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    // ---- V: Target_GodRayBlurH(t29) -> Target_GodRayBlurV ----
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_GodRayBlurV"), nullptr, false)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_GodRayBlurH"), SRV_RTV_REFLECTION_SLOT))) // t29
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))
        return E_FAIL;

    tData.vBlurDir = _float2(0.f, 1.f);

    if (FAILED(m_pGodRayBlurBuffer->Update_Data(&tData, sizeof(GodRayBlurData))))
        return E_FAIL;

    m_pGodRayBlurShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(SRV_RTV_REFLECTION_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_Cubemap()
{
    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    m_pContext->RSSetState(m_pNoneCullRS);
    m_pContext->OMSetDepthStencilState(m_pDepth_Sky, 0);

    if (FAILED(Render_RenderGroup(RENDERID::CUBEMAP)))
        return E_FAIL;
   
    HRESULT hr = Command_Execute();
    m_RenderObjects[ETOUI(RENDERID::CUBEMAP)].clear();

    m_pContext->RSSetState(m_pDefaultRS);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return hr;
}

HRESULT CRenderer::Render_UIPreviewBackground()
{
    auto& PreviewBackgroundObjects = m_RenderObjects[ETOUI(RENDERID::UI_PREVIEW_BACKGROUND)];

    if (PreviewBackgroundObjects.empty())
        return S_OK;

    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    m_pContext->RSSetState(m_pNoneCullRS);
    m_pContext->OMSetDepthStencilState(m_pDepth_Sky, 0);
    m_pContext->OMSetBlendState(m_pAlphaBlendState, nullptr, 0xffffffff);

    if (FAILED(Render_RenderGroup(RENDERID::UI_PREVIEW_BACKGROUND)))
        return E_FAIL;

    const HRESULT hr = Command_Execute();

    PreviewBackgroundObjects.clear();

    // 다음 렌더 패스가 영향을 받지 않도록 기본 상태로 복구
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);
    m_pContext->RSSetState(m_pDefaultRS);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return hr;
}

HRESULT CRenderer::Render_Combined()
{
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Diffuse"), SRV_RTV_DIFFUSE_SLOT)))
        return E_FAIL;  

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Light"), SRV_RTV_LIGHT_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Emissive"), SRV_RTV_EMSSIVE_SLOT)))
        return E_FAIL;

    m_pCombineShaderCom->Render();

    Render_FullScreenRect();
    
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);

    //슬롯 초기화
    Clear_BindingSRV(SRV_ALBEDO_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_DIFFUSE_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_LIGHT_SLOT, 1);   
    Clear_BindingSRV(SRV_RTV_EMSSIVE_SLOT, 1);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

//FinalObjects가 RTV에 바인딩 유지됨 
HRESULT CRenderer::Render_NonLight()
{
    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    if (FAILED(Render_RenderGroup(RENDERID::NONLIGHT)))
        return E_FAIL;

    HRESULT hr = Command_Execute();

    m_RenderObjects[ETOUI(RENDERID::NONLIGHT)].clear();

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return hr;
}

HRESULT CRenderer::Render_Blend()
{
    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;
    {
        ID3D11ShaderResourceView* pSceneSRV = m_pGameInstance->Get_RenderTargetSRV(TEXT("Target_SceneColor"));
        if (pSceneSRV != nullptr)
        {
            Clear_BindingSRV(3, 1);

            ID3D11RenderTargetView* pCurRTV = nullptr;
            ID3D11DepthStencilView* pCurDSV = nullptr;

            m_pContext->OMGetRenderTargets(1, &pCurRTV, &pCurDSV);

            if (pCurRTV != nullptr)
            {
                ID3D11RenderTargetView* pNullRTV = nullptr;
                m_pContext->OMSetRenderTargets(1, &pNullRTV, nullptr);

                ID3D11Resource* pBackRes = nullptr;
                ID3D11Resource* pSceneRes = nullptr;
                pCurRTV->GetResource(&pBackRes);
                pSceneSRV->GetResource(&pSceneRes);

                if (pBackRes != nullptr && pSceneRes != nullptr)
                {
                    ID3D11Texture2D* pBackTex = nullptr;
                    ID3D11Texture2D* pSceneTex = nullptr;
                    pBackRes->QueryInterface(IID_PPV_ARGS(&pBackTex));
                    pSceneRes->QueryInterface(IID_PPV_ARGS(&pSceneTex));
                    if (pBackTex != nullptr && pSceneTex != nullptr)
                    {
                        D3D11_TEXTURE2D_DESC bd{}, sd{};
                        pBackTex->GetDesc(&bd);
                        pSceneTex->GetDesc(&sd);
                        if (bd.Format == sd.Format && bd.Width == sd.Width &&
                            bd.Height == sd.Height && bd.SampleDesc.Count == sd.SampleDesc.Count)
                            m_pContext->CopyResource(pSceneRes, pBackRes);
                    }
                    Safe_Release(pBackTex);
                    Safe_Release(pSceneTex);
                }
                Safe_Release(pBackRes);
                Safe_Release(pSceneRes);
            
                m_pContext->OMSetRenderTargets(1, &pCurRTV, pCurDSV);
            }
            Safe_Release(pCurRTV);
            Safe_Release(pCurDSV);
        }
    }


    m_pContext->OMSetBlendState(m_pAdditiveBlendState, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_OnlyReadState, 0);
    
    if (FAILED(Render_RenderGroup(RENDERID::BLEND)))
        return E_FAIL;

    HRESULT hr = Command_Execute();

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);
    m_pContext->RSSetState(m_pDefaultRS);
    m_RenderObjects[ETOUI(RENDERID::BLEND)].clear();

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

// Weight Blend OIT = 알파 파티클 전용 패스
// 1. MRT_WBOIT(Accum/Reveal)에 알파 파티클을 순서 무관하게 누적한다.
// 2. Accum/Reveal을 읽은 뒤 Target_Final 위에 합성한다.
HRESULT CRenderer::Render_Blend_WBOIT()
{
    // 알파 파티클이 하나도 없으면 스킵 (불필요한 클리어 및 합성을 방지함)
    if (m_RenderObjects[ETOUI(RENDERID::BLEND_WBOIT)].empty())
        return S_OK;

    // 1. 누적 패스 : MRT_WBOIT
    // 깊이 클리어를 하지 않은 상태로 현재 씬 깊이 DSV를 그대로 바인딩함
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_WBOIT"))))
        return E_FAIL;

    // 블랜드 & 깊이 상태는 Command_Execute의 EFFECT 분기 에서 BLEND_WBOIT 패스 상태값을 세팅
    // Read-Only Depth로 강제 세팅함. (ZWrite를 무시하기 위함)
    m_pContext->OMSetBlendState(m_pWBOITBlendState, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_OnlyReadState, 0);
    m_pContext->RSSetState(m_pDefaultRS);

    if (FAILED(Render_RenderGroup(RENDERID::BLEND_WBOIT)))
        return E_FAIL;

    HRESULT hr = Command_Execute();

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->RSSetState(m_pDefaultRS);
    m_RenderObjects[ETOUI(RENDERID::BLEND_WBOIT)].clear();

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    // 2. 합성 패스 : Accum / Reveal -> Target_Final에 합성
    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_WBOIT_Accum"), 0)))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_WBOIT_Reveal"), 1)))
        return E_FAIL;

    m_pContext->OMSetBlendState(m_pAlphaBlendState, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);

    m_pWBOITShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(0, 1);
    Clear_BindingSRV(1, 1);

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_Forward()
{
    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    //if (FAILED(Bind_LightBuffer(LIGHT::DIRECTIONAL, 0)))
    //    return E_FAIL;

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);   // 불투명
    m_pContext->OMSetDepthStencilState(m_pDepth_Fwd_StencilWrite, 1); // 깊이 쓰기 ON
    m_pContext->RSSetState(m_pNoneCullRS);                       // 양면

    if (FAILED(Render_RenderGroup(RENDERID::FORWARD)))
        return E_FAIL;

    HRESULT hr = Command_Execute();

    m_RenderObjects[ETOUI(RENDERID::FORWARD)].clear();

    // 상태 원복
    m_pContext->RSSetState(m_pDefaultRS);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);


    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    Clear_BindingSRV(SRV_FOGVOLUME_BUFFER_SLOT, 1);
    m_FogVolumes.clear();

    return S_OK;
}

HRESULT CRenderer::Render_SSAO()
{
    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_SSAO_Raw"))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Normal"), SRV_RTV_NORMAL_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))
        return E_FAIL;
    

    m_pSSAOShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(SRV_RTV_NORMAL_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_SSAO"))))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_SSAO_Raw"), SRV_SSAO_SLOT)))
        return E_FAIL;

    m_pSSAOBlurShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(SRV_SSAO_SLOT, 1);
    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_Final()
{
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_LDR"))))
        return E_FAIL;

    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Final"), SRV_ALBEDO_SLOT)))   // t0
        return E_FAIL;

    m_pFinalShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(SRV_ALBEDO_SLOT, 1);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_UI()
{
    //Camera 버퍼 값 저장하고 ortho 용으로 바인딩하기 
    //m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);

    PerCameraData tUICameraData = {};

    XMStoreFloat4x4(&tUICameraData.ViewMatrix, XMMatrixIdentity());
    tUICameraData.ProjMatrix = m_OrthoProjMatrix;
    XMStoreFloat4x4(&tUICameraData.ViewInvMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&tUICameraData.ProjInvMatrix, XMMatrixIdentity());

    Bind_CameraBuffer(tUICameraData);

    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);
    m_pContext->OMSetBlendState(m_pAlphaBlendState, nullptr, 0xffffffff);
    m_pContext->RSSetState(m_pNoneCullRS);

    // UI 매니저를 통해 command 제출

    m_pGameInstance->Submit_UIRenderCommand();
    HRESULT hr = Command_Execute();

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);
    m_pContext->RSSetState(m_pDefaultRS);

    if (FAILED(Bind_CameraBuffer()))
        return E_FAIL;

    return hr;
}

HRESULT CRenderer::Render_HalfRes_Effects()
{
    const _float2 vFull = m_pGameInstance->Get_ViewportDesc();

    Set_Viewport(vFull.x * 0.5f, vFull.y * 0.5f);

    if (FAILED(Render_SSR()))
        return E_FAIL;

    if (FAILED(Render_GodRay()))
        return E_FAIL;

    if (FAILED(Render_GodRay_Blur()))
        return E_FAIL;

    Set_Viewport(vFull.x, vFull.y);

    return S_OK;
}

HRESULT CRenderer::Render_PostComposite()
{
    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    m_pContext->OMSetBlendState(m_pAdditiveBlendState, nullptr, 0xffffffff);

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))  // t23
        return E_FAIL;

    m_pContext->OMSetDepthStencilState(m_pDepth_Refl_StencilTest, 1);

    /* Reflection */
    if (m_bIBLBaked)
    {
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Reflection"), SRV_RTV_REFLECTION_SLOT))) // t29
            return E_FAIL;
        m_pReflCompositeShaderCom->Render();
        Render_FullScreenRect();

    }

    /* GodRay */
    {
        if (m_bGodRayActiveThisFrame)
        {
            if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_GodRayBlurV"), SRV_RTV_REFLECTION_SLOT)))     // 재사용
                return E_FAIL;
            m_pReflCompositeShaderCom->Render();
            Render_FullScreenRect();
        }         
    }

    Clear_BindingSRV(SRV_RTV_REFLECTION_SLOT, 1);
    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;
    return S_OK;
}

HRESULT CRenderer::Render_CSM(list<class CGameObject*>& shadowObjs)
{
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_CSM"), m_pCSMDSV)))
        return E_FAIL;

    for (int c = 0; c < m_tCSMData.iCascadeCount; ++c)
    {
        m_tShadowGlobals.iCurView = SHADOW_CSM_BASE + c;
        m_pShadowGlobalsBuffer->Update_Data(&m_tShadowGlobals, sizeof(ShadowGlobalsData));

        Set_AtlasTile(c, g_iCSMTileSize, g_iCSMAtlasCols);

        for (auto& pObject : shadowObjs)
            if (nullptr != pObject) pObject->Render_Shadow();
        Command_Execute_Depth(m_pCSMShader_Static, m_pCSMShader_Skinned);
    }

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;
    return S_OK;
}

HRESULT CRenderer::Render_Bloom()
{
    const _float2 vFull = m_pGameInstance->Get_ViewportDesc();

    const _uint iFullW = ETOUI(vFull.x);
    const _uint iFullH = ETOUI(vFull.y);
    _uint iMipW[BLOOM_MIP_COUNT];
    _uint iMipH[BLOOM_MIP_COUNT];
    for (_uint i = 0; i < BLOOM_MIP_COUNT; ++i)
    {
        const _uint iDiv = 1u << (i + 1);
        iMipW[i] = iFullW / iDiv; if (iMipW[i] < 1u) iMipW[i] = 1u;
        iMipH[i] = iFullH / iDiv; if (iMipH[i] < 1u) iMipH[i] = 1u;
    }

    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    Set_Viewport(static_cast<_float>(iMipW[0]), static_cast<_float>(iMipH[0]));

    if (FAILED(m_pGameInstance->Begin_MRT(_wstring(L"MRT_BloomMip0"), nullptr, false)))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Final"), SRV_ALBEDO_SLOT)))
        return E_FAIL;
    m_pBloomPrefilterShaderCom->Render();
    Render_FullScreenRect();
    Clear_BindingSRV(SRV_ALBEDO_SLOT, 1);
    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    for (_uint i = 1; i < BLOOM_MIP_COUNT; ++i)
    {
        Set_Viewport(static_cast<_float>(iMipW[i]), static_cast<_float>(iMipH[i]));
        if (FAILED(m_pGameInstance->Begin_MRT(_wstring(L"MRT_BloomMip") + to_wstring(i), nullptr, false)))
            return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(_wstring(L"Target_BloomMip") + to_wstring(i - 1), SRV_ALBEDO_SLOT)))
            return E_FAIL;
        m_pBloomDownShaderCom->Render();
        Render_FullScreenRect();
        Clear_BindingSRV(SRV_ALBEDO_SLOT, 1);
        if (FAILED(m_pGameInstance->End_MRT()))
            return E_FAIL;
    }

        // Vertical : Target_BloomBlur -> Target_BloomBright
    const _float fBlendFactor[4] = { BLOOM_SCATTER, BLOOM_SCATTER, BLOOM_SCATTER, BLOOM_SCATTER };
    m_pContext->OMSetBlendState(m_pBloomUpsampleBlendState, fBlendFactor, 0xffffffff);
    for (_int i = static_cast<_int>(BLOOM_MIP_COUNT) - 1; i >= 1; --i)
    {
        Set_Viewport(static_cast<_float>(iMipW[i - 1]), static_cast<_float>(iMipH[i - 1]));

        if (FAILED(m_pGameInstance->Begin_MRT(_wstring(L"MRT_BloomMip") + to_wstring(i - 1), nullptr, false, false)))
            return E_FAIL;
        if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(_wstring(L"Target_BloomMip") + to_wstring(i), SRV_ALBEDO_SLOT)))
            return E_FAIL;
        m_pBloomUpShaderCom->Render();
        Render_FullScreenRect();
        Clear_BindingSRV(SRV_ALBEDO_SLOT, 1);
        if (FAILED(m_pGameInstance->End_MRT()))
            return E_FAIL;
    }

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    Set_Viewport(vFull.x, vFull.y);

    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    m_pContext->OMSetBlendState(m_pAdditiveBlendState, nullptr, 0xffffffff);

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_BloomMip0"), SRV_ALBEDO_SLOT)))
        return E_FAIL;
    m_pBloomCompositeShaderCom->Render();
    Render_FullScreenRect();
    Clear_BindingSRV(SRV_ALBEDO_SLOT, 1);

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);

    return S_OK;
}

HRESULT CRenderer::Render_Fog()
{    
    if (m_tPostGlobals.iFogVolumeCount <= 0 && 0 == m_tRenderTuning.iGlobalFogEnable)
        return S_OK;

    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);
    m_pContext->OMSetBlendState(m_pAlphaBlendState, nullptr, 0xffffffff);

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))
        return E_FAIL;

    m_pFogShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_Blend2()
{

    if (FAILED(m_pGameInstance->Bind_MRT(TEXT("MRT_Final"))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), SRV_RTV_DEPTH_SLOT)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_PlanarReflection"), SRV_PLANAR_REFLECTION_SLOT)))
        return E_FAIL;

    m_pContext->OMSetBlendState(m_pAlphaBlendState, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_OnlyReadState, 0);
    m_pContext->RSSetState(m_pNoneCullRS);
    m_pContext->RSSetState(m_pDefaultRS);

    if (FAILED(Render_RenderGroup(RENDERID::BLEND2)))
        return E_FAIL;

    HRESULT hr = Command_Execute();

    Clear_BindingSRV(SRV_RTV_DEPTH_SLOT, 1);
    Clear_BindingSRV(SRV_PLANAR_REFLECTION_SLOT, 1);

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);
    m_pContext->RSSetState(m_pDefaultRS);
    m_RenderObjects[ETOUI(RENDERID::BLEND2)].clear();

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_SMAA()
{
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);
    m_pContext->RSSetState(m_pDefaultRS);

    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_SMAA_Edges"))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_LDR"), 0)))
        return E_FAIL;

    m_pSMAAEdgeShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(0, 1);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_SMAA_Blend"))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_SMAA_Edges"), 1)))
        return E_FAIL;

    m_pContext->PSSetShaderResources(3, 1, &m_pAreaTexSRV);
    m_pContext->PSSetShaderResources(4, 1, &m_pSearchTexSRV);

    m_pSMAAWeightShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(1, 1);
    Clear_BindingSRV(3, 1);
    Clear_BindingSRV(4, 1);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_LDR"), 0)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_SMAA_Blend"), 2)))
        return E_FAIL;

    m_pSMAABlendShaderCom->Render();
    Render_FullScreenRect();

    Clear_BindingSRV(0, 1);
    Clear_BindingSRV(2, 1);

    return S_OK;
}

HRESULT CRenderer::Render_PlanarReflection()
{
    auto& ReflectObjects = m_RenderObjects[ETOUI(RENDERID::REFLECTION)];

    if (nullptr == m_pReflectionDSV)
    {
        for (auto& pObject : ReflectObjects)
            Safe_Release(pObject);
        ReflectObjects.clear();
        return S_OK;
    }

    if (false == m_bReflectionEnable)
    {
        for (auto& pObject : ReflectObjects)
            Safe_Release(pObject);
        ReflectObjects.clear();

        m_pContext->ClearDepthStencilView(m_pReflectionDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

        if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_PlanarReflection"), m_pReflectionDSV, true, true)))
            return E_FAIL;

        if (FAILED(m_pGameInstance->End_MRT()))
            return E_FAIL;

        return S_OK;
    }

    PerCameraData tReflectCamera{};
    m_pGameInstance->Fill_CameraData(tReflectCamera);

    _matrix matView = XMLoadFloat4x4(&tReflectCamera.ViewMatrix);

    _matrix matMirror = XMMatrixIdentity();
    matMirror.r[1] = XMVectorSet(0.f, -1.f, 0.f, 0.f);
    matMirror.r[3] = XMVectorSet(0.f, 2.f * m_fReflectionHeight, 0.f, 1.f);

    _matrix matReflectView = XMMatrixMultiply(matMirror, matView);

    XMStoreFloat4x4(&tReflectCamera.ViewMatrix, matReflectView);
    XMStoreFloat4x4(&tReflectCamera.ViewInvMatrix, XMMatrixInverse(nullptr, matReflectView));

    tReflectCamera.vCamPos.y = 2.f * m_fReflectionHeight - tReflectCamera.vCamPos.y;

    if (FAILED(Bind_CameraBuffer(tReflectCamera)))
        return E_FAIL;

    m_pContext->ClearDepthStencilView(m_pReflectionDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_PlanarReflection"), m_pReflectionDSV, true, true)))
        return E_FAIL;

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);
    m_pContext->RSSetState(m_pNoneCullRS);

    if (FAILED(Render_RenderGroup(RENDERID::REFLECTION)))
        return E_FAIL;

    HRESULT hr = Command_Execute();

    ReflectObjects.clear();

    m_pContext->RSSetState(m_pDefaultRS);
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    if (FAILED(Bind_CameraBuffer()))
        return E_FAIL;

    return hr;
}


HRESULT CRenderer::Ready_HiZ()
{
    const _float2 vViewportSize = m_pGameInstance->Get_ViewportDesc();
    const _uint iViewportWidth = ETOUI(vViewportSize.x);
    const _uint iViewportHeight = ETOUI(vViewportSize.y);

    if (0 == iViewportWidth || 0 == iViewportHeight)
        return E_FAIL;

    // 원본 Depth를 2x2 축소한 크기부터 Hi-Z를 생성한다.
    m_iHiZWidth = max(1u, (iViewportWidth + 1u) / 2u);
    m_iHiZHeight = max(1u, (iViewportHeight + 1u) / 2u);
    m_iHiZMipCount = 1;

    _uint iMipWidth = m_iHiZWidth;
    _uint iMipHeight = m_iHiZHeight;

    while (1 < iMipWidth || 1 < iMipHeight)
    {
        iMipWidth = max(1u, iMipWidth / 2u);
        iMipHeight = max(1u, iMipHeight / 2u);
        ++m_iHiZMipCount;
    }

    D3D11_TEXTURE2D_DESC TextureDesc{};
    TextureDesc.Width = m_iHiZWidth;
    TextureDesc.Height = m_iHiZHeight;
    TextureDesc.MipLevels = m_iHiZMipCount;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

    if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, nullptr, &m_pHiZTexture)))
        return E_FAIL;

    D3D11_SHADER_RESOURCE_VIEW_DESC FullSRVDesc{};
    FullSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
    FullSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    FullSRVDesc.Texture2D.MostDetailedMip = 0;
    FullSRVDesc.Texture2D.MipLevels = m_iHiZMipCount;

    if (FAILED(m_pDevice->CreateShaderResourceView(m_pHiZTexture, &FullSRVDesc, &m_pHiZSRV)))
    {
        Release_HiZ();
        return E_FAIL;
    }

    m_HiZMipSRVs.resize(m_iHiZMipCount, nullptr);
    m_HiZMipUAVs.resize(m_iHiZMipCount, nullptr);

    for (_uint iMip = 0; iMip < m_iHiZMipCount; ++iMip)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC MipSRVDesc{};
        MipSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
        MipSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        MipSRVDesc.Texture2D.MostDetailedMip = iMip;
        MipSRVDesc.Texture2D.MipLevels = 1;

        if (FAILED(m_pDevice->CreateShaderResourceView(m_pHiZTexture, &MipSRVDesc, &m_HiZMipSRVs[iMip])))
        {
            Release_HiZ();
            return E_FAIL;
        }

        D3D11_UNORDERED_ACCESS_VIEW_DESC MipUAVDesc{};
        MipUAVDesc.Format = DXGI_FORMAT_R32_FLOAT;
        MipUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        MipUAVDesc.Texture2D.MipSlice = iMip;

        if (FAILED(m_pDevice->CreateUnorderedAccessView(m_pHiZTexture, &MipUAVDesc, &m_HiZMipUAVs[iMip])))
        {
            Release_HiZ();
            return E_FAIL;
        }
    }

    const _wstring strShaderPath = _wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_HiZBuildCS.hlsl";
    CComputeShader::COMPUTE_SHADER_DESC ShaderDesc{};

    ShaderDesc.strShaderName = L"Shader_HiZCopyCS";
    ShaderDesc.strCSFilePath = strShaderPath;
    ShaderDesc.strCSFunName = "CS_CopyDepth";
    m_pHiZCopyCs = CComputeShader::Create(m_pDevice, m_pContext, ShaderDesc);

    if (nullptr == m_pHiZCopyCs)
    {
        Release_HiZ();
        return E_FAIL;
    }

    ShaderDesc.strShaderName = L"Shader_HiZDownsampleCS";
    ShaderDesc.strCSFunName = "CS_Downsample";
    m_pHiZDownsampleCS = CComputeShader::Create(m_pDevice, m_pContext, ShaderDesc);

    if (nullptr == m_pHiZDownsampleCS)
    {
        Release_HiZ();
        return E_FAIL;
    }

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = sizeof(HIZ_BUILD_DESC);
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pHiZBuildConstantBuffer)))
    {
        Release_HiZ();
        return E_FAIL;
    }

    m_HiZHistory = {};

    return S_OK;
}

HRESULT CRenderer::Build_HiZ()
{
    ID3D11ShaderResourceView* pDepthSRV = m_pGameInstance->Get_RenderTargetSRV(TEXT("Target_Depth"));

    if (nullptr == pDepthSRV || nullptr == m_pHiZCopyCs || nullptr == m_pHiZDownsampleCS || nullptr == m_pHiZBuildConstantBuffer)
        return E_FAIL;

    if (m_HiZMipSRVs.size() != m_iHiZMipCount || m_HiZMipUAVs.size() != m_iHiZMipCount)
        return E_FAIL;

    const _float2 vViewportSize = m_pGameInstance->Get_ViewportDesc();

    // Target_Depth를 Hi-Z 0번 mip으로 2x2 max 축소한다.
    HIZ_BUILD_DESC BuildDesc{};
    BuildDesc.iSourceWidth = static_cast<int>(vViewportSize.x);
    BuildDesc.iSourceHeight = static_cast<int>(vViewportSize.y);
    BuildDesc.iDestinationWidth = static_cast<int>(m_iHiZWidth);
    BuildDesc.iDestinationHeight = static_cast<int>(m_iHiZHeight);

    m_pContext->UpdateSubresource(m_pHiZBuildConstantBuffer, 0, nullptr, &BuildDesc, 0, 0);

    m_pHiZCopyCs->Bind_ConstantBuffer(0, m_pHiZBuildConstantBuffer);
    m_pHiZCopyCs->Bind_SRV(0, pDepthSRV);
    m_pHiZCopyCs->Bind_UAV(0, m_HiZMipUAVs[0], UINT_MAX);
    m_pHiZCopyCs->Dispatch((m_iHiZWidth + 7u) / 8u, (m_iHiZHeight + 7u) / 8u, 1);
    m_pHiZCopyCs->Unbind_UAV(0);
    m_pHiZCopyCs->Unbind_SRV(0);

    // 이전 mip을 2x2 max 축소하여 다음 mip을 생성한다.
    for (_uint iMip = 1; iMip < m_iHiZMipCount; ++iMip)
    {
        const _uint iSourceWidth = max(1u, m_iHiZWidth >> (iMip - 1u));
        const _uint iSourceHeight = max(1u, m_iHiZHeight >> (iMip - 1u));
        const _uint iDestinationWidth = max(1u, m_iHiZWidth >> iMip);
        const _uint iDestinationHeight = max(1u, m_iHiZHeight >> iMip);

        BuildDesc.iSourceWidth = static_cast<int>(iSourceWidth);
        BuildDesc.iSourceHeight = static_cast<int>(iSourceHeight);
        BuildDesc.iDestinationWidth = static_cast<int>(iDestinationWidth);
        BuildDesc.iDestinationHeight = static_cast<int>(iDestinationHeight);

        m_pContext->UpdateSubresource(m_pHiZBuildConstantBuffer, 0, nullptr, &BuildDesc, 0, 0);

        m_pHiZDownsampleCS->Bind_ConstantBuffer(0, m_pHiZBuildConstantBuffer);
        m_pHiZDownsampleCS->Bind_SRV(1, m_HiZMipSRVs[iMip - 1u]);
        m_pHiZDownsampleCS->Bind_UAV(0, m_HiZMipUAVs[iMip], UINT_MAX);
        m_pHiZDownsampleCS->Dispatch((iDestinationWidth + 7u) / 8u, (iDestinationHeight + 7u) / 8u, 1);
        m_pHiZDownsampleCS->Unbind_UAV(0);
        m_pHiZDownsampleCS->Unbind_SRV(1);
    }

    const _float4x4* pView = m_pGameInstance->Get_Transform(D3DTS::VIEW);
    const _float4x4* pProjection = m_pGameInstance->Get_Transform(D3DTS::PROJ);

    if (nullptr == pView || nullptr == pProjection)
        return E_FAIL;

    // 현재 Depth를 만든 행렬을 다음 프레임의 occlusion 검사에서 사용한다.
    const _matrix PreviousViewProjection = XMMatrixMultiply(XMLoadFloat4x4(pView), XMLoadFloat4x4(pProjection));
    XMStoreFloat4x4(&m_HiZHistory.PreviousViewProjection, XMMatrixTranspose(PreviousViewProjection));

    m_HiZHistory.vTextureSize = _float2(static_cast<_float>(m_iHiZWidth), static_cast<_float>(m_iHiZHeight));
    m_HiZHistory.iMipCount = static_cast<int>(m_iHiZMipCount);
    m_HiZHistory.iValid = 1;
    m_vHiZPreviousCameraPosition = *m_pGameInstance->Get_CamPosition();
    m_vHiZPreviousCameraLook = *m_pGameInstance->Get_CamLook();
    m_vHiZPreviousProjectionScale = _float2(pProjection->_11, pProjection->_22);
    m_iHiZCameraDiscontinuityRevision = m_pGameInstance->Get_CameraDiscontinuityRevision();

    return S_OK;
}

void CRenderer::Release_HiZ()
{
    for (auto& pMipSRV : m_HiZMipSRVs)
        Safe_Release(pMipSRV);

    for (auto& pMipUAV : m_HiZMipUAVs)
        Safe_Release(pMipUAV);

    m_HiZMipSRVs.clear();
    m_HiZMipUAVs.clear();

    Safe_Release(m_pHiZSRV);
    Safe_Release(m_pHiZTexture);
    Safe_Release(m_pHiZBuildConstantBuffer);
    Safe_Release(m_pHiZDownsampleCS);
    Safe_Release(m_pHiZCopyCs);

    m_HiZHistory = {};
    m_vHiZPreviousCameraPosition = {};
    m_vHiZPreviousCameraLook = {};
    m_vHiZPreviousProjectionScale = {};
    m_iHiZCameraDiscontinuityRevision = UINT_MAX;
    m_iHiZWidth = 0;
    m_iHiZHeight = 0;
    m_iHiZMipCount = 0;
}

HRESULT CRenderer::Update_PostGlobals()
{
    if (nullptr == m_pPostGlobalsBuffer || nullptr == m_pFogVolumeBuffer)
        return E_FAIL;

    if (m_FogVolumes.size() > g_iMaxFogVolumes)
    {
        const _float4* pCam = m_pGameInstance->Get_CamPosition();
        _float3 vCam = { pCam->x, pCam->y, pCam->z };

        auto fSurfDist = [&vCam](const FogVolume& v) -> float
            {
                float dx = v.vCenter.x - vCam.x;
                float dy = v.vCenter.y - vCam.y;
                float dz = v.vCenter.z - vCam.z;
                return sqrtf(dx * dx + dy * dy + dz * dz) - v.fRadius;
            };

        std::nth_element(m_FogVolumes.begin(),
            m_FogVolumes.begin() + g_iMaxFogVolumes,
            m_FogVolumes.end(),
            [&](const FogVolume& a, const FogVolume& b) { return fSurfDist(a) < fSurfDist(b); });
    }

    _uint iCount = static_cast<_uint>(m_FogVolumes.size());
    if (iCount > g_iMaxFogVolumes)
        iCount = g_iMaxFogVolumes;

    if (iCount > 0)
        m_pFogVolumeBuffer->Update_Buffer(m_FogVolumes.data(), iCount);

    m_pFogVolumeBuffer->BindPS(SRV_FOGVOLUME_BUFFER_SLOT);

    const _float2 vViewport = m_pGameInstance->Get_ViewportDesc();

    m_tPostGlobals.vSMAA_RT_METRICS = _float4(1.f / vViewport.x, 1.f / vViewport.y, vViewport.x, vViewport.y);
    m_tPostGlobals.iFogVolumeCount = ETOI(iCount);
    m_tPostGlobals.fExposureScale = m_fExposureScale;
    m_tPostGlobals.fTime = m_fSceneTime;
    m_tPostGlobals.fReflectClipHeight = m_fReflectionHeight;

    if (FAILED(m_pPostGlobalsBuffer->Update_Data(&m_tPostGlobals, sizeof(PostGlobals))))
        return E_FAIL;

    return S_OK;
}


HRESULT CRenderer::Render_SPOT(list<class CGameObject*>& shadowObjs)
{
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_Spot"), m_pSpotDSV)))
        return E_FAIL;

    for (int slot = 0; slot < ETOI(m_tSpotData.iShadowCount); ++slot)
    {
        m_tShadowGlobals.iCurView = SHADOW_SPOT_BASE + slot;
        m_pShadowGlobalsBuffer->Update_Data(&m_tShadowGlobals, sizeof(ShadowGlobalsData));

        Set_AtlasTile(slot, g_iSpotTileSize, g_iSpotAtlasCols);

        for (auto& pObject : shadowObjs)
            if (nullptr != pObject) pObject->Render_Shadow();
        Command_Execute_Depth(m_pSpotShader_Static, m_pSpotShader_SKinned);
    }

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_POINT(list<class CGameObject*>& shadowObjs)
{
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_Point"), m_pPointDSV)))
        return E_FAIL;

    for (_int s = 0; s < m_tPointData.iPointShadowCount; ++s)
        for (_int f = 0; f < 6; ++f)
        {
            m_tShadowGlobals.iCurView = SHADOW_POINT_BASE + (s * 6 + f);
            m_pShadowGlobalsBuffer->Update_Data(&m_tShadowGlobals, sizeof(ShadowGlobalsData));

            Set_AtlasTile(s * 6 + f, g_iPointTileSize, g_iPointAtlasCols);

            for (auto& pObject : shadowObjs)
                if (nullptr != pObject) pObject->Render_Shadow();

            Command_Execute_Depth(m_pPointShader_Static, m_pPointShader_Skinned);
        }

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}


_bool CRenderer::Get_HiZHistory(HIZ_HISTORY_DESC* pOutHistory)  const
{
    if (nullptr == pOutHistory || 0 == m_HiZHistory.iValid)
        return false;

    // 하드컷이나 직접 Seek 이후에는 직전 카메라의 Depth History를 현재 카메라 컬링에 사용하지 않는다.
    if (m_iHiZCameraDiscontinuityRevision != m_pGameInstance->Get_CameraDiscontinuityRevision())
        return false;

    const _float4x4* pProjection = m_pGameInstance->Get_Transform(D3DTS::PROJ);
    const _float4* pCameraPosition = m_pGameInstance->Get_CamPosition();
    const _float4* pCameraLook = m_pGameInstance->Get_CamLook();

    if (nullptr == pProjection || nullptr == pCameraPosition || nullptr == pCameraLook)
        return false;

    const _vector vCurrentCameraPosition = XMLoadFloat4(pCameraPosition);
    const _vector vPreviousCameraPosition = XMLoadFloat4(&m_vHiZPreviousCameraPosition);

    if (XMVectorGetX(XMVector3LengthSq(vCurrentCameraPosition - vPreviousCameraPosition)) > 0.0625f)
        return false;

    const _vector vCurrentCameraLook = XMVector3Normalize(XMLoadFloat4(pCameraLook));
    const _vector vPreviousCameraLook = XMVector3Normalize(XMLoadFloat4(&m_vHiZPreviousCameraLook));

    if (XMVectorGetX(XMVector3Dot(vCurrentCameraLook, vPreviousCameraLook)) < cosf(XMConvertToRadians(5.f)))
        return false;

    const _float fProjectionEpsilon = 0.0001f;
    const _float fProjectionChangeLimit = 0.01f;
    const _float fProjectionXChange = fabsf(pProjection->_11 - m_vHiZPreviousProjectionScale.x) / max(fabsf(m_vHiZPreviousProjectionScale.x), fProjectionEpsilon);
    const _float fProjectionYChange = fabsf(pProjection->_22 - m_vHiZPreviousProjectionScale.y) / max(fabsf(m_vHiZPreviousProjectionScale.y), fProjectionEpsilon);

    // FOV가 크게 달라진 이전 Projection도 화면 영역이 일치하지 않으므로 한 프레임 폐기한다.
    if (fProjectionXChange > fProjectionChangeLimit || fProjectionYChange > fProjectionChangeLimit)
        return false;

    *pOutHistory = m_HiZHistory;
    return true;

}

HRESULT CRenderer::Resize_HiZ(_uint iViewportWidth, _uint iViewportHeight)
{
    iViewportWidth = max<_uint>(iViewportWidth, 1);
    iViewportHeight = max<_uint>(iViewportHeight, 1);

    const _uint iRequiredHiZWidth = max<_uint>((iViewportWidth + 1u) / 2u, 1);
    const _uint iRequiredHiZHeight = max<_uint>((iViewportHeight + 1u) / 2u, 1);

    if (m_iHiZWidth == iRequiredHiZWidth && m_iHiZHeight == iRequiredHiZHeight && nullptr != m_pHiZSRV)
        return S_OK;

    Release_HiZ();

    return Ready_HiZ();
}


#ifdef _DEBUG
HRESULT CRenderer::Render_Debug()
{
    m_pContext->OMSetDepthStencilState(m_pDepth_DisableState, 0);

    Bind_CameraBuffer();

    //Px용 디버거 
    if (FAILED(Render_RenderGroup(RENDERID::DEBUG)))
        return E_FAIL;

    HRESULT hr = Command_Execute();
    m_RenderObjects[ETOUI(RENDERID::DEBUG)].clear();
    //Px용 디버거 End 

    PerCameraData debugCam{};
    XMStoreFloat4x4(&debugCam.ViewMatrix, XMMatrixIdentity());
    debugCam.ProjMatrix = m_OrthoProjMatrix;    
    XMStoreFloat4x4(&debugCam.ViewInvMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&debugCam.ProjInvMatrix, XMMatrixIdentity());


    Bind_CameraBuffer(debugCam);

    //m_pGameInstance->Render_RT_Debug(TEXT("MRT_PlanarReflection"), m_pDebugShaderCom, m_pVIBuffer_Rect, m_pPerObjectBuffer);    
    //m_pGameInstance->Render_RT_Debug(TEXT("MRT_Final"), m_pDebugShaderCom, m_pVIBuffer_Rect, m_pPerObjectBuffer);
    //m_pGameInstance->Render_RT_Debug(TEXT("MRT_SSAO"), m_pDebugShaderCom, m_pVIBuffer_Rect, m_pPerObjectBuffer);
    //m_pGameInstance->Render_RT_Debug(TEXT("MRT_Reflection"), m_pDebugShaderCom, m_pVIBuffer_Rect, m_pPerObjectBuffer);
    
    m_pContext->OMSetDepthStencilState(m_pDepth_EnableState, 0);
    Clear_BindingSRV(SRV_ALBEDO_SLOT, 1);
    return S_OK;
}

#endif // _DEBUG




CRenderer* CRenderer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CRenderer* pInstance = new CRenderer(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CRenderer");
        Safe_Release(pInstance);
    }

    return pInstance;
}


//==============================================================================
// Engine/Private/Renderer.cpp   -   Free 통째 교체
// DSV 해제 구간에 m_pReflectionDSV 한 줄이 추가됐다.
//==============================================================================

void CRenderer::Free()
{
    Release_HiZ();

    __super::Free();
    m_pCommands.clear();

    for (auto& RenderObjects : m_RenderObjects)
    {
        for (auto& pRenderObject : RenderObjects)
            Safe_Release(pRenderObject);
        RenderObjects.clear();
    }
    m_RenderObjects->clear();

    for (auto& pDecal : m_Decals)
        Safe_Release(pDecal);
    m_Decals.clear();

    // 추가분 해제
    Safe_Release(m_pIBLBaker);

    Safe_Release(m_pSMAAEdgeShaderCom);
    Safe_Release(m_pSMAAWeightShaderCom);
    Safe_Release(m_pSMAABlendShaderCom);
    Safe_Release(m_pRimShaderCom);
    Safe_Release(m_pFogShaderCom);
    Safe_Release(m_pPointShader_Static);
    Safe_Release(m_pPointShader_Skinned);
    Safe_Release(m_pCSMShader_Static);
    Safe_Release(m_pCSMShader_Skinned);
    Safe_Release(m_pSpotShader_Static);
    Safe_Release(m_pSpotShader_SKinned);
    Safe_Release(m_pGodRayBlurShaderCom);
    Safe_Release(m_pGodRayShaderCom);
    Safe_Release(m_pBloomPrefilterShaderCom);
    Safe_Release(m_pBloomDownShaderCom);
    Safe_Release(m_pBloomUpShaderCom);
    Safe_Release(m_pBloomCompositeShaderCom);
    Safe_Release(m_pReflCompositeShaderCom);
    Safe_Release(m_pWBOITShaderCom);
    Safe_Release(m_pWBOITBlendState);
    Safe_Release(m_pSSRShaderCom);
    Safe_Release(m_pSSAOBlurShaderCom);
    Safe_Release(m_pSSAOShaderCom);
    Safe_Release(m_pLightDirectShaderCom);
    Safe_Release(m_pLightPointShaderCom);
    Safe_Release(m_pLightSpotShaderCom);
    Safe_Release(m_pIBLShaderCom);
    Safe_Release(m_pCombineShaderCom);
    Safe_Release(m_pFinalShaderCom);
    Safe_Release(m_pVIBuffer_Rect);

#ifdef _DEBUG
    Safe_Release(m_pDebugShaderCom);
#endif // _DEBUG


    Safe_Delete(m_pLastCommand);
    Safe_Release(m_pLinearSampler);
    Safe_Release(m_pPointSampler);
    Safe_Release(m_pLinearClampSampler);

    Safe_Release(m_pDefaultRS);
    Safe_Release(m_pNoneCullRS);
    Safe_Release(m_pShadowScissorRS);

    Safe_Release(m_pDepth_Refl_StencilTest);
    Safe_Release(m_pDepth_Fwd_StencilWrite);
    Safe_Release(m_pDepth_DisableState);
    Safe_Release(m_pDepth_OnlyReadState);
    Safe_Release(m_pDepth_EnableState);
    Safe_Release(m_pDepth_Sky);
    Safe_Release(m_pDepth_GBuffer_ReceiverWrite);
    Safe_Release(m_pDepth_Decal_ReceiverTest);
    Safe_Release(m_pAlphaBlendState);
    Safe_Release(m_pAdditiveBlendState);
    Safe_Release(m_pMultiplyBlendState);
    Safe_Release(m_pEffectAlphaBlendState);
    Safe_Release(m_pBloomUpsampleBlendState);

    Safe_Release(m_pPostGlobalsBuffer);
    Safe_Release(m_pUIBuffer);
    Safe_Release(m_pPerMaterialBuffer);
    Safe_Release(m_pEffectBuffer);
    Safe_Release(m_pPerObjectBuffer);
    Safe_Release(m_pPerCameraBuffer);
    Safe_Release(m_pMaterialsBuffer);
    Safe_Release(m_pPerLightBuffer);
    Safe_Release(m_pIBLBuffer);
    Safe_Release(m_pGodRayBlurBuffer);
    Safe_Release(m_pShadowViewsBuffer);
    Safe_Release(m_pShadowGlobalsBuffer);
    Safe_Release(m_pRenderTuningBuffer);
    Safe_Release(m_pFogVolumeBuffer);
    Safe_Release(m_pRimProfileBuffer);

    Safe_Release(m_pCSMDSV);
    Safe_Release(m_pSpotDSV);
    Safe_Release(m_pPointDSV);
    Safe_Release(m_pReflectionDSV);

    Safe_Release(m_pAllocator);

    Safe_Release(m_pAreaTexSRV);
    Safe_Release(m_pSearchTexSRV);
    Safe_Release(m_pDefaultAlbedoSRV);
    Safe_Release(m_pDefaultMaskSRV);

    Safe_Delete(m_pUISpriteBatch);

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}
