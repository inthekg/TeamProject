#include "Transform.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CTransform, CComponent)
ADD_PROPERTY(CTransform, m_fSpeedPerSec, Float)
ADD_PROPERTY_SETTER(CTransform, m_vScale, Float3, Setting_Scale, _float3)
END_REFLECTION(CTransform)
#endif

CTransform::CTransform(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent{ pDevice, pContext }
{
}

CTransform::CTransform(const CTransform& Prototype)
    : CComponent{ Prototype }
    , m_WorldMatrix{ Prototype.m_WorldMatrix }
    , m_vQuat{Prototype.m_vQuat}
    , m_vScale{Prototype.m_vScale}
{
}


HRESULT CTransform::Initialize_Prototype()
{
    XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());
    XMStoreFloat4(&m_vQuat, XMQuaternionIdentity());

    return S_OK;
}

HRESULT CTransform::Initialize(void* pArg)
{
    if (nullptr == pArg)
        return S_OK;

    auto    pDesc = static_cast<TRANSFORM_DESC*>(pArg);

    m_fRotationPerSec = pDesc->fRotationPerSec;
    m_fSpeedPerSec = pDesc->fSpeedPerSec;

    Translate(XMLoadFloat4(&pDesc->vTranslate));
    Set_Scale(pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z);
    Rotation(pDesc->vRotationDegree.x, pDesc->vRotationDegree.y, pDesc->vRotationDegree.z);

    return S_OK;
}

void CTransform::Set_Quat(_fvector vQuat)
{
	XMStoreFloat4(&m_vQuat, vQuat);
    //해당 Quat으로 회전하는 행렬
    _matrix RotationMatrix = XMMatrixRotationQuaternion(vQuat);

    Set_State(STATE::RIGHT, RotationMatrix.r[0] * m_vScale.x);
    Set_State(STATE::UP,    RotationMatrix.r[1] * m_vScale.y);
    Set_State(STATE::LOOK,  RotationMatrix.r[2] * m_vScale.z);

}

void CTransform::Set_Scale(_float fScaleX, _float fScaleY, _float fScaleZ)
{
	m_vScale = { fScaleX, fScaleY, fScaleZ };
	
    _vector vQuat = XMLoadFloat4(&m_vQuat);
	_matrix RotationMatrix = XMMatrixRotationQuaternion(vQuat);

    Set_State(STATE::RIGHT, RotationMatrix.r[0] * m_vScale.x);
    Set_State(STATE::UP,    RotationMatrix.r[1] * m_vScale.y);
    Set_State(STATE::LOOK,  RotationMatrix.r[2] * m_vScale.z);
}

void CTransform::Set_Scale(const _float3& vScale)
{
    Set_Scale(vScale.x, vScale.y, vScale.z);
}

#ifdef _DEBUG
HRESULT CTransform::Setting_Scale(const _float3& vScale)
{
    Set_Scale(vScale.x, vScale.y, vScale.z);
    return S_OK;
}
#endif // _DEBUG

void CTransform::Translate(_fvector vPosition)
{
    Set_State(STATE::POSITION, vPosition);
}

void CTransform::Scaling(_float fScaleX, _float fScaleY, _float fScaleZ)
{
    m_vScale.x += fScaleX;
	m_vScale.y += fScaleY;
	m_vScale.z += fScaleZ;

    _vector vQuat = XMLoadFloat4(&m_vQuat);

    _matrix RotationMatrix = XMMatrixRotationQuaternion(vQuat);

    Set_State(STATE::RIGHT, RotationMatrix.r[0] * m_vScale.x);
    Set_State(STATE::UP,    RotationMatrix.r[1] * m_vScale.y);
    Set_State(STATE::LOOK,  RotationMatrix.r[2] * m_vScale.z);
}

void CTransform::Go_Straight(_float fTimeDelta)
{
    _vector     vPosition = Get_State(STATE::POSITION);
    _vector     vLook = Get_State(STATE::LOOK);

    vPosition += XMVector3Normalize(vLook) * m_fSpeedPerSec * fTimeDelta;

    Set_State(STATE::POSITION, vPosition);
}

void CTransform::Go_Backward(_float fTimeDelta)
{
    _vector     vPosition = Get_State(STATE::POSITION);
    _vector     vLook = Get_State(STATE::LOOK);

    vPosition -= XMVector3Normalize(vLook) * m_fSpeedPerSec * fTimeDelta;

    Set_State(STATE::POSITION, vPosition);
}

void CTransform::Go_Left(_float fTimeDelta)
{
    _vector     vPosition = Get_State(STATE::POSITION);
    _vector     vRight = Get_State(STATE::RIGHT);

    vPosition -= XMVector3Normalize(vRight) * m_fSpeedPerSec * fTimeDelta;

    Set_State(STATE::POSITION, vPosition);
}

void CTransform::Go_Right(_float fTimeDelta)
{
    _vector     vPosition = Get_State(STATE::POSITION);
    _vector     vRight = Get_State(STATE::RIGHT);

    vPosition += XMVector3Normalize(vRight) * m_fSpeedPerSec * fTimeDelta;

    Set_State(STATE::POSITION, vPosition);
}

void CTransform::Rotation(_fvector vAxis, _float fRadian)
{
	_vector vQuat = XMQuaternionRotationAxis(vAxis, fRadian);
    _matrix RotationMatrix = XMMatrixRotationQuaternion(vQuat);
	XMStoreFloat4(&m_vQuat, vQuat);

    Set_State(STATE::RIGHT, RotationMatrix.r[0] * m_vScale.x);
    Set_State(STATE::UP,    RotationMatrix.r[1] * m_vScale.y);
    Set_State(STATE::LOOK,  RotationMatrix.r[2] * m_vScale.z);
}

void CTransform::Rotation(_float fPitch, _float fYaw, _float fRoll)
{
    _vector vQuat = XMQuaternionRotationRollPitchYaw(fPitch, fYaw, fRoll);
    _matrix RotationMatrix = XMMatrixRotationQuaternion(vQuat);
    XMStoreFloat4(&m_vQuat, vQuat);

    Set_State(STATE::RIGHT, RotationMatrix.r[0] * m_vScale.x);
    Set_State(STATE::UP,    RotationMatrix.r[1] * m_vScale.y);
    Set_State(STATE::LOOK,  RotationMatrix.r[2] * m_vScale.z);
}


void CTransform::Turn(_fvector vAxis, _float fTimeDelta)
{
    _vector vRotationQuat = XMQuaternionRotationAxis(vAxis, m_fRotationPerSec * fTimeDelta);
	_vector vQuat = XMLoadFloat4(&m_vQuat);

    vQuat = XMQuaternionMultiply(vQuat, vRotationQuat);
	XMStoreFloat4(&m_vQuat, vQuat);

    _matrix RotationMatrix = XMMatrixRotationQuaternion(vQuat);

    Set_State(STATE::RIGHT, RotationMatrix.r[0] * m_vScale.x);
    Set_State(STATE::UP,    RotationMatrix.r[1] * m_vScale.y);
    Set_State(STATE::LOOK,  RotationMatrix.r[2] * m_vScale.z);

}

void CTransform::Turn(_vector vNextQuat)
{
    _vector vQuat = XMLoadFloat4(&m_vQuat);
    vQuat = XMQuaternionMultiply(vQuat, vNextQuat);
    XMStoreFloat4(&m_vQuat, vQuat);

    _matrix RotationMatrix = XMMatrixRotationQuaternion(vQuat);

    Set_State(STATE::RIGHT, RotationMatrix.r[0] * m_vScale.x);
    Set_State(STATE::UP,    RotationMatrix.r[1] * m_vScale.y);
    Set_State(STATE::LOOK,  RotationMatrix.r[2] * m_vScale.z);
}


void CTransform::Smooth_Turn(_vector vNextQuat, _float fRatio)
{
    _vector vQuat = XMLoadFloat4(&m_vQuat);

    vQuat = XMQuaternionSlerp(vQuat, vNextQuat, fRatio);

    XMStoreFloat4(&m_vQuat, vQuat);

    _matrix RotationMatrix = XMMatrixRotationQuaternion(vQuat);

    Set_State(STATE::RIGHT, RotationMatrix.r[0] * m_vScale.x);
    Set_State(STATE::UP, RotationMatrix.r[1] * m_vScale.y);
    Set_State(STATE::LOOK, RotationMatrix.r[2] * m_vScale.z);
}

void CTransform::LookAt(_fvector vAt)
{
    _vector vPos = Get_State(STATE::POSITION);
    _vector vLook = vAt - vPos;
    vLook = XMVector3Normalize(vLook);

    _vector vRight = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook);
    vRight = XMVector3Normalize(vRight);

    _vector vUp = XMVector3Cross(vLook, vRight);
    vUp = XMVector3Normalize(vUp);

    _matrix matRot = XMMatrixIdentity();
    matRot.r[0] = vRight;
    matRot.r[1] = vUp;
    matRot.r[2] = vLook;

    _vector vQuat = XMQuaternionRotationMatrix(matRot);

    XMStoreFloat4(&m_vQuat, vQuat);

    _matrix RotationMatrix = XMMatrixRotationQuaternion(vQuat);

    Set_State(STATE::RIGHT, RotationMatrix.r[0] * m_vScale.x);
    Set_State(STATE::UP,    RotationMatrix.r[1] * m_vScale.y);
    Set_State(STATE::LOOK,  RotationMatrix.r[2] * m_vScale.z);
}


void CTransform::LookAt(_fvector vAt, _float fTimeDelta)
{
    _vector vPos = Get_State(STATE::POSITION);
    _vector vLook = XMVectorSubtract(vAt, vPos);

    if (XMVectorGetX(XMVector3LengthSq(vLook)) < 1e-6f) 
        return;

    vLook = XMVector3Normalize(vLook);

    _vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook));
    _vector vUp = XMVector3Cross(vLook, vRight);

    _matrix matRot = XMMatrixIdentity();
    matRot.r[0] = vRight;
    matRot.r[1] = vUp;
    matRot.r[2] = vLook;

    _vector vTargetQuat = XMQuaternionRotationMatrix(matRot);
    _vector vCurQuat = XMLoadFloat4(&m_vQuat);
    _float fDot = XMVectorGetX(XMQuaternionDot(vCurQuat, vTargetQuat));
    if (fDot < 0.f) fDot = -fDot;
    if (fDot > 1.f) fDot = 1.f;
    _float fAngle = 2.f * XMScalarACos(fDot);

    _float fRatio = (fAngle > 1e-4f) ? (m_fRotationPerSec * fTimeDelta) / fAngle : 1.f;
    if (fRatio > 1.f) fRatio = 1.f;
    Smooth_Turn(vTargetQuat, fRatio);
}


CTransform* CTransform::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTransform* pInstance = new CTransform(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CTransform");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CTransform::Clone(void* pArg)
{
    CTransform* pInstance = new CTransform(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CTransform");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTransform::Free()
{
    __super::Free();

}
