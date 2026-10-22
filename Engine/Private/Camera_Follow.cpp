#include "Camera_Follow.h"
#include "GameInstance.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CCamera_Follow, CCamera)
ADD_PROPERTY_SETTER(CCamera_Follow, m_fFovy, Float, Setting_Fovy, _float)
ADD_PROPERTY(CCamera_Follow, m_vOffset, Float3)
ADD_PROPERTY(CCamera_Follow, m_fLookAhead, Float)
END_REFLECTION(CCamera_Follow)
#endif // _DEBUG

CCamera_Follow::CCamera_Follow(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CCamera{ pDevice, pContext }
{
}

CCamera_Follow::CCamera_Follow(const CCamera_Follow& Prototype)
    : CCamera{ Prototype }
{
}

HRESULT CCamera_Follow::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CCamera_Follow::Initialize(void* pArg)
{
    if (nullptr != pArg)
    {
        auto pDesc = static_cast<CAMERA_FOLLOW_DESC*>(pArg);
        m_vOffset = pDesc->vOffset;
        m_fLookAhead = pDesc->fLookAhead;
    }

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    return S_OK;
}

void CCamera_Follow::Priority_Update(_float fTimeDelta)
{
}

void CCamera_Follow::Update(_float fTimeDelta)
{

}

void CCamera_Follow::Late_Update(_float fTimeDelta)
{
    if (nullptr != m_pTarget)
        Follow_Target();

    __super::Update_PipeLine();
}

HRESULT CCamera_Follow::Render()
{
    return S_OK;
}

HRESULT CCamera_Follow::Setting_Fovy(const _float& fFovy)
{
    m_fFovy = fFovy;
    XMStoreFloat4x4(&m_ProjMatrix,
        XMMatrixPerspectiveFovLH(m_fFovy, m_fAspect, m_fNear, m_fFar));
    return S_OK;
}

void CCamera_Follow::Follow_Target()
{
    CTransform* pTargetTransform = m_pTarget->Get_Transform();
    if (nullptr == pTargetTransform)
        return;

    _vector vTargetPos = pTargetTransform->Get_State(STATE::POSITION);
    _vector vTargetLook = XMVector3Normalize(pTargetTransform->Get_State(STATE::LOOK));
    _vector vTargetRight = XMVector3Normalize(pTargetTransform->Get_State(STATE::RIGHT));
    _vector vWorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    _vector vCamPos = vTargetPos
        - vTargetLook * m_vOffset.z
        + vTargetRight * m_vOffset.x
        + vWorldUp * m_vOffset.y;

    m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vCamPos, 1.f));

    _vector vLookAt = vTargetPos
        + vTargetLook * m_fLookAhead
        + vWorldUp * m_vOffset.y;

    m_pTransformCom->LookAt(XMVectorSetW(vLookAt, 1.f));
}

CCamera_Follow* CCamera_Follow::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CCamera_Follow* pInstance = new CCamera_Follow(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CCamera_Follow");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CCamera_Follow::Clone(void* pArg)
{
    CCamera_Follow* pInstance = new CCamera_Follow(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CCamera_Follow");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CCamera_Follow::Free()
{
    __super::Free();
}
