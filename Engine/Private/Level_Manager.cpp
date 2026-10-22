#include "Level_Manager.h"
#include "GameInstance.h"

#include "Level.h"

CLevel_Manager::CLevel_Manager(HWND hWnd)
    : m_pGameInstance { CGameInstance::Get_Instance() }
    , m_hWnd { hWnd }   
{
}

HRESULT CLevel_Manager::Request_ChangeLevel(_int iNewLevelIndex, CLevel* pNewLevel)
{
    if (nullptr == pNewLevel) return E_FAIL;
    //기존 레벨이 없다면 바로 전환 
    if (nullptr == m_pCurrentLevel)
    {
        m_pCurrentLevel = pNewLevel;
        m_iCurrentLevelIndex = iNewLevelIndex;
        m_pCurrentLevel->Level_Enter();
        return S_OK;
    }

    //이미 NextLevel이 설정되었다면 해제 후 재 바인딩 
    if (nullptr != m_pNextLevel) Safe_Release(m_pNextLevel);

    m_pNextLevel = pNewLevel;
    m_iNextLevelIndex = iNewLevelIndex;

    m_bChangeFlag = true;

    return S_OK;
}

HRESULT CLevel_Manager::Change_Level()
{
    if (nullptr == m_pNextLevel) return E_FAIL;

    if (nullptr != m_pCurrentLevel)
        m_pCurrentLevel->Level_Exit();

    m_pGameInstance->Clear_Resources(m_iCurrentLevelIndex);
    Safe_Release(m_pCurrentLevel);

    m_pCurrentLevel = m_pNextLevel;
    m_iCurrentLevelIndex = m_iNextLevelIndex;

    m_pNextLevel = nullptr;
    m_iNextLevelIndex = -1;

    m_bChangeFlag = false;

    m_pCurrentLevel->Level_Enter();
    return S_OK;
}

void CLevel_Manager::Update(_float fTimeDelta)
{
    if (nullptr == m_pCurrentLevel)
        return;

    m_pCurrentLevel->Update(fTimeDelta);
}

void CLevel_Manager::Late_Update(_float fTimeDelta)
{
    if (nullptr == m_pCurrentLevel)
        return;

    //Material Buffer 재갱신 (if Flag on)
    m_pCurrentLevel->Late_Update(fTimeDelta);

    //m_bChangeFlag가 Set되면 모든 처리 끝나면 전환 
    if (m_bChangeFlag) Change_Level();
}


HRESULT CLevel_Manager::Render()
{
    if (nullptr == m_pCurrentLevel)
        return E_FAIL;

    return m_pCurrentLevel->Render();
}

CLevel_Manager* CLevel_Manager::Create(HWND hWnd)
{
    return new CLevel_Manager(hWnd);
}

void CLevel_Manager::Free()
{
    __super::Free();

    Safe_Release(m_pCurrentLevel);
    Safe_Release(m_pNextLevel);

}
