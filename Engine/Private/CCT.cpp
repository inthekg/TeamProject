#include "CCT.h"
#include "GameInstance.h"
#include "Transform.h"
#include "GameObject.h"

static bool Is_FilterCleared(const PxController& controller)
{
	PxRigidDynamic* pActor = controller.getActor();
	if (nullptr == pActor)
		return false;

	PxShape* pShape = nullptr;
	if (0 == pActor->getShapes(&pShape, 1) || nullptr == pShape)
		return false;

	const PxFilterData tData = pShape->getSimulationFilterData();
	return (0 == tData.word0 && 0 == tData.word1);
}

class CCTFilterCallback : public PxControllerFilterCallback
{
public:
	virtual bool filter(const PxController& a, const PxController& b) override
	{
		CGameObject* pOwnerA = static_cast<CGameObject*>(a.getUserData());
		CGameObject* pOwnerB = static_cast<CGameObject*>(b.getUserData());

		if ((nullptr != pOwnerA && pOwnerA->Is_InActive()) ||
			(nullptr != pOwnerB && pOwnerB->Is_InActive()))
			return false;

		if (true == Is_FilterCleared(a) || true == Is_FilterCleared(b))
			return false;

		return true;
	}
};
static CCTFilterCallback g_CCTFilterCallback;




CCT::CCT(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CBasePhysic{ pDevice, pContext }
{
}

CCT::CCT(const CCT& Prototype)
	:CBasePhysic{ Prototype }
{
}

HRESULT CCT::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CCT::Initialize(void* pArg)
{
	if (pArg == nullptr) return E_FAIL;
	CONTROLLER_DESC* pControllerDesc = reinterpret_cast<CONTROLLER_DESC*>(pArg);

	//Shape 생성 
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_eControllerType = pControllerDesc->eControllerType;

	PxMaterial* pMaterial = CGameInstance::Get_Instance()->Get_Default_Material();
	if (pMaterial == nullptr) return E_FAIL;

	if (pControllerDesc->eControllerType == CCT_TYPE::CAPSULE)
	{
		PxCapsuleControllerDesc CapsuleDesc;
		CapsuleDesc.radius = pControllerDesc->fRadius;
		m_fRadius = pControllerDesc->fRadius;
		CapsuleDesc.height = pControllerDesc->fHeight;
		CapsuleDesc.slopeLimit = pControllerDesc->fSlopeLimit;
		CapsuleDesc.stepOffset = pControllerDesc->fStepOffset;
		CapsuleDesc.contactOffset = pControllerDesc->fContactOffset;
		CapsuleDesc.upDirection = PxVec3(0.f, 1.f, 0.f);
		//CapsuleDesc.position =  PxExtendedVec3(pControllerDesc->vInitPos.x, pControllerDesc->vInitPos.y + fHalfHeight, pControllerDesc->vInitPos.z);
		CapsuleDesc.material = pMaterial;
		CapsuleDesc.userData = m_pOwner;
		CapsuleDesc.behaviorCallback = m_pGameInstance->Get_CCTBehaviorCallback();
		m_pController = m_pGameInstance->Create_CapsuleController(CapsuleDesc);

	}
	else if (pControllerDesc->eControllerType == CCT_TYPE::BOX)
	{
		PxBoxControllerDesc BoxDesc;
		BoxDesc.halfHeight = pControllerDesc->vBoxHalf.y;
		BoxDesc.halfSideExtent = pControllerDesc->vBoxHalf.x;
		m_fRadius = BoxDesc.halfSideExtent;
		BoxDesc.halfForwardExtent = pControllerDesc->vBoxHalf.z;
		BoxDesc.slopeLimit = pControllerDesc->fSlopeLimit;
		BoxDesc.stepOffset = pControllerDesc->fStepOffset;
		BoxDesc.upDirection = PxVec3(0.f, 1.f, 0.f);
		BoxDesc.position = PxExtendedVec3(pControllerDesc->vInitPos.x, pControllerDesc->vInitPos.y, pControllerDesc->vInitPos.z);
		BoxDesc.material = pMaterial;
		BoxDesc.contactOffset = pControllerDesc->fContactOffset;
		BoxDesc.userData = m_pOwner;
		BoxDesc.behaviorCallback = m_pGameInstance->Get_CCTBehaviorCallback();

		m_pController = m_pGameInstance->Create_BoxController(BoxDesc);
	}
	else return E_FAIL;

	if (m_pController == nullptr) return E_FAIL;

	PxRigidActor* pActor = m_pController->getActor();
	if (pActor) pActor->userData = m_pOwner;

#ifdef _DEBUG
	//현재 전역 피직스 랜더 마스크와 동기화 
	if (pActor)
	{
		_bool bCCTDebug = { (m_pGameInstance->Get_PhysXDebugMask() & ETOUI(PHYSIC_DEBUG_MODE::CHARACTER)) != 0 };
		pActor->setActorFlag(PxActorFlag::eVISUALIZATION, bCCTDebug);
	}
#endif 
	PxShape* pShape = nullptr;

	//컨트롤러 필터 데이터 (move시 필터링 설정) 
	m_FilterData.word0 = m_iFilterWord0;
	m_FilterData.word1 = m_iFilterWord1;

	m_ControllerFilters.mFilterData = &m_FilterData;
	//rigid 강체들도 모두 검사하겠다
	//내가 설정한 필터링 셰이더를 거치게함 
	auto* pCCTQueryFilter = m_pGameInstance->Get_CCTQueryFilter();
	if (pCCTQueryFilter)
	{
		m_ControllerFilters.mFilterCallback = pCCTQueryFilter;
		m_ControllerFilters.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER | PxQueryFlag::eDISABLE_HARDCODED_FILTER;
	}
	else
	{
		m_ControllerFilters.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;
	}

	// 추가
	m_ControllerFilters.mCCTFilterCallback = &g_CCTFilterCallback;

	//CCT는 하나의 Actor와 Shape를 기본적으로가짐. 디폴트 Shape의 필터링 Set
	//본체 Shape 1개 얻어서 필터링 설정. 
	if (pActor->getShapes(&pShape, 1) > 0)
	{
		pShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
		Set_Filter(pShape, m_iFilterWord0, m_iFilterWord1);
	}

	//Desc로 들어온 추가 Shape있으면 추가
	for (PxShape* pExtraShape : m_PhysicShapes)
	{
		pActor->attachShape(*pExtraShape);
		pExtraShape->release();
	}

	m_PhysicShapes.clear();

	//발밑으로 보정 
	Set_FootPosition(XMLoadFloat3(&pControllerDesc->vInitPos));

	
	return S_OK;
}

void CCT::InvalidateCache()
{
	if (m_pController) m_pController->invalidateCache();
}

void CCT::Set_Active(_bool bActive)
{
	if (nullptr == m_pController)
		return;

	if (m_bPhysicsActive == bActive)
		return;

	PxRigidDynamic* pActor = m_pController->getActor();
	if (nullptr == pActor)
		return;

	const PxU32 iShapeCount = pActor->getNbShapes();
	if (0 == iShapeCount)
		return;

	vector<PxShape*> Shapes(iShapeCount);
	pActor->getShapes(Shapes.data(), iShapeCount);

	const _uint iWord0 = bActive ? m_iFilterWord0 : 0;
	const _uint iWord1 = bActive ? m_iFilterWord1 : 0;

	for (PxU32 i = 0; i < iShapeCount; i++)
		Set_Filter(Shapes[i], iWord0, iWord1);

	if (PxScene* pScene = pActor->getScene())
		pScene->resetFiltering(*pActor);

	m_bPhysicsActive = bActive;
}


CCTResult CCT::Move(_fvector vDeltaPos, _float fFixedTime)
{
	CCTResult result = {};
	if (m_pController == nullptr) return result;
	 
	PxVec3 deltaPos = PxVec3(XMVectorGetX(vDeltaPos), XMVectorGetY(vDeltaPos), XMVectorGetZ(vDeltaPos));

	PxControllerFilters filters = m_ControllerFilters;
	PxControllerCollisionFlags collisionFlags = m_pController->move(deltaPos, 0.001f, fFixedTime, filters);

	result.m_bGrounded = collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);
	result.m_bSideHit = collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_SIDES);
	result.m_bCeilingHit = collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_UP);

	//발 위치로 반환 
	XMStoreFloat4(&result.vSimulatedPosition, Get_FootPosition());
	return result;
}

void CCT::Set_FootPosition(_fvector vPosition)
{
	if (m_pController == nullptr) return;
	m_pController->setFootPosition(PxExtendedVec3(XMVectorGetX(vPosition), XMVectorGetY(vPosition), XMVectorGetZ(vPosition)));
}

_vector CCT::Get_FootPosition() const
{
	if (m_pController == nullptr) return XMVectorZero();

	PxExtendedVec3 footPos = m_pController->getFootPosition();
	return XMVectorSet((_float)footPos.x, (_float)footPos.y, (_float)footPos.z, 1.f);
}

void CCT::Resize(_float fHeight)
{
	if (m_pController == nullptr) return;
	m_pController->resize(fHeight);
}

void CCT::Resize_Radius(_float fRadius)
{
	if (m_pController == nullptr) return;
	if (m_eControllerType == CCT_TYPE::CAPSULE)
	{
		m_fRadius = fRadius;
		static_cast<PxCapsuleController*>(m_pController)->setRadius(fRadius);
	}
}

CCT* CCT::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CCT* pInstance = new CCT(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("CCT : Create Failed");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CCT::Clone(void* pArg)
{
	CCT* pInstance = new CCT(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("CCT : Clone Failed");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CCT::Free()
{
	__super::Free();
	if (m_pController)
	{
		m_pController->release();
		m_pController = nullptr;
	}
}