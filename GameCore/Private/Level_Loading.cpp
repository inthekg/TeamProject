#include "Level_Loading.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_GamePlay.h"
#include "Level_Editor.h"
#include "Level_Shader.h"
#include "UIFactory.h"
#include "UILoadingController.h"

CLevel_Loading::CLevel_Loading(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
	: CLevel{ pDevice, pContext, hWnd }
{
}

HRESULT CLevel_Loading::Initialize(LEVEL eNextLevelID)
{
    m_eNextLevelID = eNextLevelID;

    ///* 로더 스레드 시작 전에 메인 스레드에서 로딩 화면 오브젝트를 clone(동기) */
    //if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
    //    return E_FAIL;

    if (FAILED(CUIFactory::Ready_LoadingUI(&m_hLoadingUI)))
        return E_FAIL;

    CUILoadingController* pLoadingController = static_cast<CUILoadingController*>(m_pGameInstance->Find_UIController(L"LoadingController"));
    if (pLoadingController != nullptr)
        pLoadingController->Begin_LoadingSession();

    m_pLoader = CLoader::Create(m_pDevice, m_pContext, eNextLevelID, m_hWnd);
    if (nullptr == m_pLoader)
    {
        m_pGameInstance->Close_UIObject(m_hLoadingUI);
        m_hLoadingUI.Invalidate();

        return E_FAIL;
    }

    return S_OK;
}

HRESULT CLevel_Loading::Ready_Layer_BackGround(const _wstring& strLayerTag)
{
    //CBackGround::BACKGROUND_DESC     Desc{};
    //Desc.pMaterialTag = TEXT("Loading");

    //if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_BackGround"),
    //    ETOUI(LEVEL::LOADING), strLayerTag, &Desc)))
    //    return E_FAIL;

    return S_OK;
}

void CLevel_Loading::Update(_float fTimeDelta)
{
    CUILoadingController* pLoadingController = static_cast<CUILoadingController*>(m_pGameInstance->Find_UIController(L"LoadingController"));
    if (pLoadingController != nullptr && m_pLoader != nullptr)
        pLoadingController->Set_LoadingProgress(m_pLoader->Get_LoadingProgress());

    if (true == m_pLoader->isFinished())
    {
        if (true == m_pLoader->isFailed())
            return;

        //로딩완료되면 메테리얼 버퍼 갱신 
        m_pGameInstance->Update_MaterialBuffer();

        CLevel* pNextLevel = { nullptr };

        switch (m_eNextLevelID)
        {
        case LEVEL::GAMEPLAY:
            pNextLevel = CLevel_GamePlay::Create(m_pDevice, m_pContext, m_hWnd);
            break;

        case LEVEL::EDIT:
            pNextLevel = CLevel_Editor::Create(m_pDevice, m_pContext, m_hWnd);
            break;

        case LEVEL::SHADER:
            pNextLevel = CLevel_Shader::Create(m_pDevice, m_pContext, m_hWnd);
            break;
        }

        if (nullptr == pNextLevel)
        {
            MSG_BOX("Failed to Changed");
            return;
        }

        if (SUCCEEDED(m_pGameInstance->Request_ChangeLevel(ETOI(m_eNextLevelID), pNextLevel)))
        {
            // 인스턴스를 제거하지 않고 숨겨서 이후 챕터 로딩에서 재사용한다.
            if (m_hLoadingUI.Is_Valid())
            {
                m_pGameInstance->Close_UIObject(m_hLoadingUI);
                m_hLoadingUI.Invalidate();
            }

            return;
        }
    }
}

void CLevel_Loading::Late_Update(_float fTimeDelta)
{
    //비워두기. super호출 X
}

HRESULT CLevel_Loading::Render()
{
//#ifdef _DEBUG
    m_pLoader->Show();
//#endif
    return S_OK;
}

CLevel_Loading* CLevel_Loading::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID, HWND hWnd)
{
    CLevel_Loading* pInstance = new CLevel_Loading(pDevice, pContext, hWnd);

    if (FAILED(pInstance->Initialize(eNextLevelID)))
    {
        MSG_BOX("Failed to Created : CLevel_Loading");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CLevel_Loading::Free()
{
    if (m_hLoadingUI.Is_Valid())
    {
        m_pGameInstance->Close_UIObject(m_hLoadingUI);
        m_hLoadingUI.Invalidate();
    }

    __super::Free();

    Safe_Release(m_pLoader);
}
