#include "Level_Logo.h"
#include "Level_Loading.h"
#include "GameInstance.h"
#include "ChapterMapManager.h"
#include "MapGameplayObject.h"
#include "Model.h"
#include "Sky.h"
#include "Texture.h"
#include "UIFactory.h"

CLevel_Logo::CLevel_Logo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
    : CLevel{ pDevice, pContext, hWnd }
{
}

HRESULT CLevel_Logo::Initialize(LEVEL eNextLevelID)
{
    m_eNextLevelID = eNextLevelID;

    if (FAILED(Ready_Camera()))
        return E_FAIL;

    if (FAILED(Ready_LogoPrototypes()))
        return E_FAIL;

    if (FAILED(Ready_Layer_Sky(TEXT("Layer_Sky"))))
        return E_FAIL;

    if (FAILED(Ready_LogoChapter()))
        return E_FAIL;

    if (FAILED(Ready_UI()))
        return E_FAIL;

    // 로고의 월드와 스카이박스만 어둡게 표시하며, 후처리 이후 그려지는 UI 밝기는 유지한다.
    m_pGameInstance->Set_ExposureScale(0.2f);

    m_pGameInstance->Subscribe(EVENT_TYPE::START_LEVEL, this);

    m_pGameInstance->Play_Sound("SE_UI_TitleScreen_Steelheart_01", SOUND_SFX, 0.9f, true, true);
    m_pGameInstance->Play_BGM("MU_MS_Title_Piano_03", 0.35f);

    return S_OK;
}

void CLevel_Logo::Update(_float fTimeDelta)
{
    if (nullptr != m_pChapterMapManager)
    {
        const _float4* pCameraPosition = m_pGameInstance->Get_CamPosition();

        if (nullptr != pCameraPosition)
        {
            const _vector vCameraPosition = XMLoadFloat4(pCameraPosition);
            m_pChapterMapManager->Update(vCameraPosition, vCameraPosition);
        }
    }
}

void CLevel_Logo::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);

    if (nullptr != m_pChapterMapManager)
        m_pChapterMapManager->Late_Update(fTimeDelta);
}

HRESULT CLevel_Logo::Render()
{
    return S_OK;
}

void CLevel_Logo::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{
    if (eType != EVENT_TYPE::START_LEVEL)
        return;

    Request_StartGame();
}

//HRESULT CLevel_Logo::Ready_Camera()
//{
//    m_pGameInstance->Begin_UIPreviewCamera(
//        XMVectorSet(0.f, 1.5f, -4.f, 1.f),      // Eye
//        XMVectorSet(0.f, 1.f, 0.f, 1.f),        // vAt
//        XMConvertToRadians(40.f));              // FOV
//
//    return S_OK;
//}

HRESULT CLevel_Logo::Ready_UI()
{
    return CUIFactory::Ready_TitleUI(&m_hTitleUI);
}

void CLevel_Logo::Request_StartGame()
{
    if (m_bTransitionRequested)
        return;

    CLevel_Loading* pLoadingLevel = CLevel_Loading::Create(m_pDevice, m_pContext, m_eNextLevelID, m_hWnd);
    if (!pLoadingLevel)
    {
        MSG_BOX("Failed to Create : CLevel_Loading");
        return;
    }

    if (FAILED(m_pGameInstance->Request_ChangeLevel(ETOUI(LEVEL::LOADING), pLoadingLevel)))
    {
        Safe_Release(pLoadingLevel);
        return;
    }

    // 게임 시작이 실제로 승인된 시점에 한 번만 재생한다.
    m_pGameInstance->Play_Sound("SE_UI_StartGame_03", SOUND_SFX, 0.85f, false, false);

    // 로딩 화면으로 전환되는 동안 타이틀 BGM을 서서히 줄인다.
    m_pGameInstance->FadeOut_BGM(1.5f);

    m_bTransitionRequested = true;
}

HRESULT CLevel_Logo::Ready_Camera()
{
    // 기준 화면처럼 Steelheart 전체와 붉은 천이 보이도록 거리를 확보하고 주시점을 위로 보정한다.
    m_pGameInstance->Begin_UIPreviewCamera(
        XMVectorSet(-9.108748f, 1.559234f, 0.948765f, 1.f),              // Eye: 약 2m로 후퇴
        XMVectorSet(-7.109700f, 1.140000f, 1.011583f, 1.f),              // At: 화면 우측 하단 배치
                    0.377316f);                                          // Vertical FOV, 약 21.6186도

    return S_OK;
}

HRESULT CLevel_Logo::Ready_Layer_Sky(const _wstring& strLayerTag)
{
    CTexture* pSkyTexture = { nullptr };

    // 로고는 Gameplay 로더보다 먼저 실행되므로 동일한 SkyBox 텍스처를 선행 등록한다.
    if (FAILED(m_pGameInstance->Get_Texture(TEXT("SkyBox"), &pSkyTexture)))
    {
        _wstring strSkyTextureName = TEXT("SkyBox");
        _wstring strSkyTexturePath = m_pGameInstance->Get_ResourcePath() + TEXT("\\Map\\SkyBox\\SkyBox.dds");

        pSkyTexture = CTexture::Create(m_pDevice, m_pContext, &strSkyTextureName, &strSkyTexturePath);
        if (nullptr == pSkyTexture)
            return E_FAIL;

        if (FAILED(m_pGameInstance->Register_Texture(pSkyTexture)))
        {
            Safe_Release(pSkyTexture);
            return E_FAIL;
        }
    }
    else
        Safe_Release(pSkyTexture);

    // Logo 전용 Prototype과 Layer를 사용해 레벨 전환 시 함께 정리되도록 한다.
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::LOGO), TEXT("Prototype_GameObject_Sky"), CSky::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::LOGO), TEXT("Prototype_GameObject_Sky"), ETOUI(LEVEL::LOGO), strLayerTag)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bake_IBL()))
        return E_FAIL;

    return S_OK;
}

HRESULT CLevel_Logo::Ready_LogoPrototypes()
{
    // Steelheart 원본 단위만 보정하고 위치/회전/추가 배율은 Chapter 데이터에서 받는다.
    const _matrix PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::LOGO), TEXT("Prototype_Component_Model_Steelheart"), CModel::Create(m_pDevice, m_pContext, L"Steelheart", PreTransformMatrix))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::LOGO), TEXT("Prototype_GameObject_MapGameplayObject"), CMapGameplayObject::Create(m_pDevice, m_pContext))))
        return E_FAIL;


    return S_OK;
}

HRESULT CLevel_Logo::Ready_LogoChapter()
{
    const _wstring strChapterPath = m_pGameInstance->Get_ResourcePath() + L"\\Map\\Map_Data\\Logo.chapter.json";

    m_pChapterMapManager = CChapterMapManager::Create(m_pDevice, m_pContext);

    if (nullptr == m_pChapterMapManager)
        return E_FAIL;

    if (FAILED(m_pChapterMapManager->Load_LogoChapter(strChapterPath)))
        return E_FAIL;

    if (FAILED(m_pChapterMapManager->Spawn_MapGameplayObjects()))
        return E_FAIL;

    return S_OK;
}


void CLevel_Logo::Level_Exit()  
{
    m_pGameInstance->StopSound_ByKey("SE_UI_TitleScreen_Steelheart_01");

    m_pGameInstance->UnSubscribe_AllEvent(this);

    if (m_hTitleUI.Is_Valid())
    {
        m_pGameInstance->Request_ReleaseUIObject(m_hTitleUI);
        m_hTitleUI.Invalidate();
    }

    if (nullptr != m_pChapterMapManager && true == m_pChapterMapManager->Is_ChapterLoaded())
        m_pChapterMapManager->Unload_Chapter();

    m_pGameInstance->End_UIPreviewCamera();
}

CLevel_Logo* CLevel_Logo::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID, HWND hWnd)
{
    CLevel_Logo* pInstance = new CLevel_Logo(pDevice, pContext, hWnd);
    if (FAILED(pInstance->Initialize(eNextLevelID)))
    {
        Safe_Release(pInstance);
        return nullptr;
    }
    return pInstance;
}

void CLevel_Logo::Free()
{
    // Initialize 중간 실패 경로에서도 생성된 Logo Layer를 남기지 않는다.
    if (nullptr != m_pChapterMapManager && true == m_pChapterMapManager->Is_ChapterLoaded())
        m_pChapterMapManager->Unload_Chapter();

    Safe_Release(m_pChapterMapManager);
    __super::Free();
}


