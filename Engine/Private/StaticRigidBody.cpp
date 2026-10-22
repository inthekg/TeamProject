#include "StaticRigidBody.h"
#include "GameInstance.h"

CStaticRigidBody::CStaticRigidBody(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CRigidActor{ pDevice, pContext }   
{
}

CStaticRigidBody::CStaticRigidBody(const CStaticRigidBody& Prototype)
	: CRigidActor{ Prototype }
{
}

HRESULT CStaticRigidBody::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CStaticRigidBody::Initialize(void* pArg)
{
    if (pArg == nullptr) return E_FAIL;
	PHYSIC_DESC* pPhysicDesc = reinterpret_cast<PHYSIC_DESC*>(pArg);
    
    if(FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	_vector vPos = XMLoadFloat3(&pPhysicDesc->vInitPos);
	_vector vRot = XMLoadFloat4(&pPhysicDesc->vInitQuat);
	m_pRigidActor = m_pGameInstance->Create_RigidStatic(vPos, vRot);
	if (m_pRigidActor == nullptr) return E_FAIL;

	if (FAILED(Setup_RigidActor(m_pRigidActor)))
		return E_FAIL;

    return S_OK;
}

CStaticRigidBody* CStaticRigidBody::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CStaticRigidBody* pInstance = new CStaticRigidBody(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CStaticRigidBody");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CStaticRigidBody::Clone(void* pArg)
{
	CStaticRigidBody* pInstance = new CStaticRigidBody(*this);
	if(FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CStaticRigidBody");
		Safe_Release(pInstance);
	}
	return pInstance;	
}

void CStaticRigidBody::Free()
{
	__super::Free();

}
