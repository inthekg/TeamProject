#include "Trigger.h"
#include "StaticRigidBody.h"
#include "GameInstance.h"

CTrigger::CTrigger(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject{ pDevice, pContext }
{
}

CTrigger::CTrigger(const CTrigger& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CTrigger::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CTrigger::Initialize(void* pArg)
{
	TRIGGER_DESC* pDesc = static_cast<TRIGGER_DESC*>(pArg);
	if (pDesc == nullptr) return E_FAIL;

	if(FAILED(__super::Initialize(pArg)))
		return E_FAIL;	
	
	m_bOnce = pDesc->bOnceTrigger;

	// Trigger가 살아 있는 동안 논리 오너가 유효하도록 참조를 보유한다.
	m_pLogicalOwner = pDesc->pLogicalOwner;
	Safe_AddRef(m_pLogicalOwner);

	m_Func_OnBeginOverlap = pDesc->OnBeginOverlap;
	m_Func_OnEndOverlap = pDesc->OnEndOverlap;

	if (FAILED(Ready_Components(pDesc)))
		return E_FAIL;

	return S_OK;
}

void CTrigger::Begin_Overlap(CGameObject* pOther)
{
	if (pOther == nullptr || Is_InActive()) return;

	if(m_Func_OnBeginOverlap)
		m_Func_OnBeginOverlap(pOther);

	if (m_bOnce)
		Set_InActive();
}

void CTrigger::End_Overlap(CGameObject* pOther)
{
	if (pOther == nullptr || Is_InActive())return;

	if (m_Func_OnEndOverlap)
		m_Func_OnEndOverlap(pOther);

}

HRESULT CTrigger::Ready_Components(TRIGGER_DESC* pDesc)
{
	PHYSIC_DESC PhysicDesc{};
	PhysicDesc.pOwner = this;
	XMStoreFloat3(&PhysicDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&PhysicDesc.vInitQuat, m_pTransformCom->Get_Quat());

	SHAPE_DESC ShapeDesc{};
	ShapeDesc.eType = GEOMETRY_TYPE::BOX;
	ShapeDesc.vHalfExtents = pDesc->vHalfExtents;
	ShapeDesc.bTrigger = true;
	PhysicDesc.Shapes.push_back(ShapeDesc);

	PhysicDesc.iFilterWord0 = ETOUI(FILTER_GROUP::TRIGGER);
	PhysicDesc.iFilterWord1 = pDesc->iTargetFilter;

	//static Rigid
	if (FAILED(__super::Add_Component(0, L"Prototype_Component_StaticRigidBody",
		TEXT("Com_RidigStatic"), reinterpret_cast<CComponent**>(&m_pRigidStaticBody), &PhysicDesc)))
		return E_FAIL;

	return S_OK;
}

CTrigger* CTrigger::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CTrigger* pInstance = new CTrigger(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CTrigger");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CTrigger::Clone(void* pArg)
{
	CTrigger* pInstance = new CTrigger(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone : CTrigger");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CTrigger::Free()
{
	__super::Free();
	Safe_Release(m_pRigidStaticBody);

	// 물리 Trigger를 먼저 정리한 뒤 논리 오너 참조를 반환한다.
	Safe_Release(m_pLogicalOwner);
}
