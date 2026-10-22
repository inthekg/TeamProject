#include "Camera_Free.h"
#include "GameInstance.h"

CCamera_Free::CCamera_Free(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CCamera{ pDevice, pContext }
{
}

CCamera_Free::CCamera_Free(const CCamera_Free& Prototype)
    : CCamera{ Prototype }
{
}

HRESULT CCamera_Free::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CCamera_Free::Initialize(void* pArg)
{
    auto        pDesc = static_cast<CAMERA_FREE_DESC*>(pArg);

    m_fMouseSensor = pDesc->fMouseSensor;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    return S_OK;
}

void CCamera_Free::Priority_Update(_float fTimeDelta)
{
     if (m_pGameInstance->Key_Pressing(DIK_W))
    {
        m_pTransformCom->Go_Straight(fTimeDelta/2);
    }
    if (m_pGameInstance->Key_Pressing(DIK_S))
    {
        m_pTransformCom->Go_Backward(fTimeDelta / 2);
    }
    if (m_pGameInstance->Key_Pressing(DIK_A))
    {
        m_pTransformCom->Go_Left(fTimeDelta / 2);
    }
    if (m_pGameInstance->Key_Pressing(DIK_D))
    {
        m_pTransformCom->Go_Right(fTimeDelta / 2);
    }

    _long       MouseMove = {};
    if (m_pGameInstance->Mouse_Pressing(DIMB::RBUTTON))
    {
        if (MouseMove = m_pGameInstance->Get_DIMouseMove(DIMM::X))
        {
            m_pTransformCom->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), MouseMove * m_fMouseSensor * fTimeDelta);
        }

        if (MouseMove = m_pGameInstance->Get_DIMouseMove(DIMM::Y))
        {
            m_pTransformCom->Turn(m_pTransformCom->Get_State(STATE::RIGHT), MouseMove * m_fMouseSensor * fTimeDelta);
        }
    }

    __super::Priority_Update(fTimeDelta);
}

void CCamera_Free::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);
}

void CCamera_Free::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
}

HRESULT CCamera_Free::Render()
{
    return S_OK;
}

CCamera_Free* CCamera_Free::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CCamera_Free* pInstance = new CCamera_Free(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CCamera_Free");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CCamera_Free::Clone(void* pArg)
{
    CCamera_Free* pInstance = new CCamera_Free(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CCamera_Free");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CCamera_Free::Free()
{
    __super::Free();
}
