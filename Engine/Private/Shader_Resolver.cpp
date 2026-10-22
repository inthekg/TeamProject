#include "Shader_Resolver.h"
#include "GameInstance.h"
#include "Shader.h"


CShader_Resolver::CShader_Resolver()
    : m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CShader_Resolver::Initialize()
{
    CShader* pVtxMesh = nullptr;
    CShader* pVtxAnim = nullptr;
    CShader* pVtxHair = nullptr;
    CShader* pVtxHairStatic = nullptr;
    CShader* pVtxHairInstance = nullptr;
    CShader* pEyelash = nullptr;
    CShader* pEyelashStatic = nullptr;
    CShader* pEyelashInstance = nullptr;
    CShader* pEye = nullptr;
    CShader* pEyeStatic = nullptr;
    CShader* pEyeInstance = nullptr;
    CShader* pSkin = nullptr;
    CShader* pSkinStatic = nullptr;
    CShader* pSkinInstance = nullptr;
    CShader* pMapInstance = nullptr;
    CShader* pUnlit = nullptr;
    CShader* pUnlitInstance = nullptr;
    CShader* pUnlitAnim = nullptr;
    CShader* pGlass = nullptr;
    CShader* pGlassInstance = nullptr;
    CShader* pFur = nullptr;
    CShader* pWater = nullptr;
    CShader* pWaterInstance = nullptr;    
    CShader* pSpiderWeb = nullptr;
    CShader* pSpiderWebInstance = nullptr;

    if (FAILED(m_pGameInstance->Get_Shader(TEXT("Shader_VtxMesh"), &pVtxMesh)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Shader(TEXT("Shader_VtxAnimMesh"), &pVtxAnim)))
    {
        Safe_Release(pVtxMesh);
        return E_FAIL;
    }

    m_pGameInstance->Get_Shader(TEXT("Shader_Hair"), &pVtxHair);
    m_pGameInstance->Get_Shader(TEXT("Shader_HairStatic"), &pVtxHairStatic);
    m_pGameInstance->Get_Shader(TEXT("Shader_HairInstance"), &pVtxHairInstance);
    m_pGameInstance->Get_Shader(TEXT("Shader_Eyelash"), &pEyelash);
    m_pGameInstance->Get_Shader(TEXT("Shader_EyelashStatic"), &pEyelashStatic);
    m_pGameInstance->Get_Shader(TEXT("Shader_EyelashInstance"), &pEyelashInstance);
    m_pGameInstance->Get_Shader(TEXT("Shader_Skin"), &pSkin);
    m_pGameInstance->Get_Shader(TEXT("Shader_SkinStatic"), &pSkinStatic);
    m_pGameInstance->Get_Shader(TEXT("Shader_SkinInstance"), &pSkinInstance);
    m_pGameInstance->Get_Shader(TEXT("Shader_Eye"), &pEye);
    m_pGameInstance->Get_Shader(TEXT("Shader_EyeStatic"), &pEyeStatic);
    m_pGameInstance->Get_Shader(TEXT("Shader_EyeInstance"), &pEyeInstance);
    m_pGameInstance->Get_Shader(TEXT("Shader_MapInstance"), &pMapInstance);
    m_pGameInstance->Get_Shader(TEXT("Shader_Unlit"), &pUnlit);
    m_pGameInstance->Get_Shader(TEXT("Shader_UnlitInstance"), &pUnlitInstance);
    m_pGameInstance->Get_Shader(TEXT("Shader_UnlitAnim"), &pUnlitAnim);
    m_pGameInstance->Get_Shader(TEXT("Shader_Glass"), &pGlass);
    m_pGameInstance->Get_Shader(TEXT("Shader_GlassInstance"), &pGlassInstance);
    m_pGameInstance->Get_Shader(TEXT("Shader_HairFur"), &pFur);
    m_pGameInstance->Get_Shader(TEXT("Shader_Water"), &pWater);
    m_pGameInstance->Get_Shader(TEXT("Shader_WaterInstance"), &pWaterInstance);
    m_pGameInstance->Get_Shader(TEXT("Shader_SpiderWeb"), &pSpiderWeb);
    m_pGameInstance->Get_Shader(TEXT("Shader_SpiderWebInstance"), &pSpiderWebInstance);

    m_pGameInstance->Get_Shader(TEXT("Shader_ReflectionOpaque"), &m_pReflectionStatic);
    m_pGameInstance->Get_Shader(TEXT("Shader_ReflectionOpaqueInstance"), &m_pReflectionInstance);

    if (pVtxMesh == nullptr || pVtxAnim == nullptr)
        return E_FAIL;

    for (_uint i = ETOUI(BLEND_MODE::BLEND_OPAQUE); i <= ETOUI(BLEND_MODE::BLEND_MASKED); ++i)
    {
        Register(Shader_Key{ SHADING_MODEL::DEFAULT_LIT, static_cast<BLEND_MODE>(i), false }, pVtxMesh);
        Register(Shader_Key{ SHADING_MODEL::DEFAULT_LIT, static_cast<BLEND_MODE>(i), true }, pVtxAnim);
        // Subsurface 머테리얼을 메시 형태에 맞는 Skin 셰이더로 라우팅한다.
        if (pSkinStatic != nullptr)
            Register(Shader_Key{ SHADING_MODEL::SUBSURFACE, static_cast<BLEND_MODE>(i), false, false }, pSkinStatic);
        if (pSkin != nullptr)
            Register(Shader_Key{ SHADING_MODEL::SUBSURFACE, static_cast<BLEND_MODE>(i), true, false }, pSkin);
        if (pSkinInstance != nullptr)
            Register(Shader_Key{ SHADING_MODEL::SUBSURFACE, static_cast<BLEND_MODE>(i), false, true }, pSkinInstance);
        // Eye 머테리얼만 메시 형태에 맞는 전용 셰이더를 사용한다.
        if (pEyeStatic != nullptr)
            Register(Shader_Key{ SHADING_MODEL::EYE, static_cast<BLEND_MODE>(i), false, false }, pEyeStatic);
        if (pEye != nullptr)
            Register(Shader_Key{ SHADING_MODEL::EYE, static_cast<BLEND_MODE>(i), true, false }, pEye);
        if (pEyeInstance != nullptr)
            Register(Shader_Key{ SHADING_MODEL::EYE, static_cast<BLEND_MODE>(i), false, true }, pEyeInstance);

        if (pMapInstance != nullptr)
            Register(Shader_Key{ SHADING_MODEL::DEFAULT_LIT, static_cast<BLEND_MODE>(i), false, true }, pMapInstance);
        if (pUnlit != nullptr)
            Register(Shader_Key{ SHADING_MODEL::UNLIT, static_cast<BLEND_MODE>(i), false, false }, pUnlit);
        if (pUnlitInstance != nullptr)
            Register(Shader_Key{ SHADING_MODEL::UNLIT, static_cast<BLEND_MODE>(i), false, true }, pUnlitInstance);
        if (pUnlitAnim != nullptr)
            Register(Shader_Key{ SHADING_MODEL::UNLIT, (BLEND_MODE)i, true, false }, pUnlitAnim);
    }

    if (pGlass != nullptr)
        Register(Shader_Key{ SHADING_MODEL::DEFAULT_LIT, BLEND_MODE::BLEND_TRANSLUCENT, false, false }, pGlass);

    if (pGlassInstance != nullptr)
        Register(Shader_Key{ SHADING_MODEL::DEFAULT_LIT, BLEND_MODE::BLEND_TRANSLUCENT, false, true }, pGlassInstance);

    if (pWater != nullptr)
        Register(Shader_Key{ SHADING_MODEL::WATER, BLEND_MODE::BLEND_TRANSLUCENT, false, false }, pWater);

    if (pWaterInstance != nullptr)
        Register(Shader_Key{ SHADING_MODEL::WATER, BLEND_MODE::BLEND_TRANSLUCENT, false, true }, pWaterInstance);

    if (pSpiderWeb != nullptr)
        Register(Shader_Key{ SHADING_MODEL::SPIDERWEB, BLEND_MODE::BLEND_TRANSLUCENT, false, false }, pSpiderWeb);

    if (pSpiderWebInstance != nullptr)
        Register(Shader_Key{ SHADING_MODEL::SPIDERWEB, BLEND_MODE::BLEND_TRANSLUCENT, false, true }, pSpiderWebInstance);

    // Hair 머테리얼을 메시 형태에 맞는 전용 셰이더로 라우팅한다.
    if (pVtxHairStatic != nullptr)
        Register(Shader_Key{ SHADING_MODEL::HAIR, BLEND_MODE::BLEND_MASKED, false, false }, pVtxHairStatic);
    if (pVtxHair != nullptr)
        Register(Shader_Key{ SHADING_MODEL::HAIR, BLEND_MODE::BLEND_MASKED, true, false }, pVtxHair);
    if (pVtxHairInstance != nullptr)
        Register(Shader_Key{ SHADING_MODEL::HAIR, BLEND_MODE::BLEND_MASKED, false, true }, pVtxHairInstance);

    // 속눈썹 머테리얼을 메시 형태에 맞는 전용 셰이더로 라우팅한다.
    if (pEyelashStatic != nullptr)
        Register(Shader_Key{ SHADING_MODEL::EYELASH, BLEND_MODE::BLEND_MASKED, false, false }, pEyelashStatic);
    if (pEyelash != nullptr)
        Register(Shader_Key{ SHADING_MODEL::EYELASH, BLEND_MODE::BLEND_MASKED, true, false }, pEyelash);
    if (pEyelashInstance != nullptr)
        Register(Shader_Key{ SHADING_MODEL::EYELASH, BLEND_MODE::BLEND_MASKED, false, true }, pEyelashInstance);
    if (pFur != nullptr)
        Register(Shader_Key{ SHADING_MODEL::FUR, BLEND_MODE::BLEND_MASKED, true }, pFur);

    Safe_Release(pVtxMesh);
    Safe_Release(pVtxAnim);
    Safe_Release(pVtxHair);
    Safe_Release(pVtxHairStatic);
    Safe_Release(pVtxHairInstance);
    Safe_Release(pEyelash);
    Safe_Release(pEyelashStatic);
    Safe_Release(pEyelashInstance);
    Safe_Release(pSkin);
    Safe_Release(pSkinStatic);
    Safe_Release(pSkinInstance);
    Safe_Release(pEye);
    Safe_Release(pEyeStatic);
    Safe_Release(pEyeInstance);
    Safe_Release(pMapInstance);
    Safe_Release(pUnlit);
    Safe_Release(pUnlitInstance);
    Safe_Release(pUnlitAnim);
    Safe_Release(pGlass);
    Safe_Release(pGlassInstance);
    Safe_Release(pFur);
    Safe_Release(pWater);
    Safe_Release(pWaterInstance);
    Safe_Release(pSpiderWeb);
    Safe_Release(pSpiderWebInstance);

    return S_OK;
}

CShader* CShader_Resolver::Resolve(const Material_Meta& tMeta, _bool bSkinned, _bool bInstanced) const
{
    if (CShader* pShader = Find(tMeta.eShadingModel, tMeta.eBlendMode, bSkinned, bInstanced))
        return pShader;
    if (CShader* pShader = Find(SHADING_MODEL::DEFAULT_LIT, tMeta.eBlendMode, bSkinned, bInstanced))
        return pShader;

    return Find(SHADING_MODEL::DEFAULT_LIT, BLEND_MODE::BLEND_OPAQUE, bSkinned, bInstanced);
}

CShader* CShader_Resolver::Get_ReflectionShader(_bool bInstanced) const
{
    return bInstanced ? m_pReflectionInstance : m_pReflectionStatic;
}

RENDERID CShader_Resolver::Resolve_Group(const Material_Meta& tMeta)
{
    if (tMeta.bIsNull)
        return RENDERID::END;

    if (tMeta.eShadingModel == SHADING_MODEL::EYELASH)
        return RENDERID::FORWARD;

    if (tMeta.eBlendMode == BLEND_MODE::BLEND_TRANSLUCENT)
        return RENDERID::BLEND2;

    return RENDERID::NONBLEND;
}



void CShader_Resolver::Register(Shader_Key tKey, CShader* pShader)
{
    if (pShader == nullptr)
        return;

    auto iter = m_ShaderTables.find(tKey);
    if (iter != m_ShaderTables.end())
        Safe_Release(iter->second);

    Safe_AddRef(pShader);
    m_ShaderTables[tKey] = pShader;
}

CShader* CShader_Resolver::Find(SHADING_MODEL eShadingModel, BLEND_MODE eBlendMode, _bool bSkinned, _bool bInstanced) const
{
    auto iter = m_ShaderTables.find(Shader_Key{ eShadingModel, eBlendMode, bSkinned ,bInstanced });
    if (iter == m_ShaderTables.end())
        return nullptr;
    return iter->second;
}

CShader_Resolver* CShader_Resolver::Create()
{
    CShader_Resolver* pInstance = new CShader_Resolver();

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CShader_Resolver");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CShader_Resolver::Free()
{
    __super::Free();

    Safe_Release(m_pReflectionStatic);
    Safe_Release(m_pReflectionInstance);

    for (auto& Pair : m_ShaderTables)
        Safe_Release(Pair.second);
    m_ShaderTables.clear();
}
