#include "Level.h"
#include "GameInstance.h"

CLevel::CLevel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
    : m_pDevice { pDevice }
    , m_pContext { pContext }
    , m_pGameInstance { CGameInstance::Get_Instance() }
    , m_hWnd { hWnd }   
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CLevel::Initialize()
{
    return S_OK;
}

void CLevel::Update(_float fTimeDelta)
{
    //TODD : 구현 완료 후 디버그전용으로 전환하기 

    ++m_iFrameCount;
    m_fTimeAcc += fTimeDelta;
    if (m_fTimeAcc >= 1.f)
    {
        m_iFPS = m_iFrameCount;
        m_iFrameCount = 0;
        m_fTimeAcc = 0.f;
    }

}

void CLevel::Late_Update(_float fTimeDelta)
{
    if(m_pGameInstance->Is_MaterialDirty())
        m_pGameInstance->Update_MaterialBuffer();
}

HRESULT CLevel::Render()
{
    
    return S_OK;
}

void CLevel::Free()
{
    __super::Free();

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}
