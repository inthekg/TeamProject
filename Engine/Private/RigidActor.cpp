#include "RigidActor.h"
#include "GameInstance.h"

CRigidActor::CRigidActor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CBasePhysic{ pDevice, pContext }
{
}

CRigidActor::CRigidActor(const CRigidActor& Prototype)
	:CBasePhysic{ Prototype }
{
}

HRESULT CRigidActor::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CRigidActor::Initialize(void* pArg)
{
	return __super::Initialize(pArg);
}

void CRigidActor::Physical_Update(_float fFixedTime)
{
}

HRESULT CRigidActor::Setup_RigidActor(PxRigidActor* pActor)
{
	if (pActor == nullptr) return E_FAIL;

	m_pRigidActor = pActor;
	for(auto& pShape : m_PhysicShapes)
	{
		m_pRigidActor->attachShape(*pShape);
		pShape->release();
	}
	m_PhysicShapes.clear();

	m_pRigidActor->userData = m_pOwner;
#ifdef _DEBUG
	//현재 전역 피직스 랜더 마스크와 동기화 
	_bool bRigidDebug = { (m_pGameInstance->Get_PhysXDebugMask() & ETOUI(PHYSIC_DEBUG_MODE::CHARACTER)) != 0 };
	m_pRigidActor->setActorFlag(PxActorFlag::eVISUALIZATION, bRigidDebug);
#endif


	m_pGameInstance->Add_ActorToScene(m_pRigidActor);

	return S_OK;
}

void CRigidActor::Get_GlobalTransform(OUT _vector& pOutPos, OUT _vector& pOutQuat)
{
	if (m_pRigidActor == nullptr) return;
	PxTransform GlobalTransform = m_pRigidActor->getGlobalPose();

	pOutPos = XMVectorSet(GlobalTransform.p.x, GlobalTransform.p.y, GlobalTransform.p.z, 1.f);
	pOutQuat = XMVectorSet(GlobalTransform.q.x, GlobalTransform.q.y, GlobalTransform.q.z, GlobalTransform.q.w);

}

void CRigidActor::Set_GlobalTransform(_fvector vPos, _fvector vQuat)
{
	if(m_pRigidActor == nullptr) return;
	PxTransform Transform;
	Transform.p = PxVec3(XMVectorGetX(vPos), XMVectorGetY(vPos), XMVectorGetZ(vPos));
	Transform.q = PxQuat(XMVectorGetX(vQuat), XMVectorGetY(vQuat), XMVectorGetZ(vQuat), XMVectorGetW(vQuat));	

	m_pRigidActor->setGlobalPose(Transform);
}

void CRigidActor::Set_ActorFlag(PxActorFlag::Enum eFlag, _bool bEnable)
{
	if (m_pRigidActor == nullptr) return;
	m_pRigidActor->setActorFlag(eFlag, bEnable);
}

void CRigidActor::Free()
{
	__super::Free();
	if (m_pRigidActor)
	{
		if (nullptr != m_pRigidActor->getScene())
			m_pGameInstance->Remove_ActorFromScene(m_pRigidActor);
		m_pRigidActor->release();
		m_pRigidActor = nullptr;
	}
}
