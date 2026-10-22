#include "SteppingStone.h"

#include "Core_Struct.h"
#include "DynamicRigidBody.h"
#include "GameInstance.h"
#include "Player.h"
#include "Transform.h"

CSteppingStone::CSteppingStone(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CSteppingStone::CSteppingStone(const CSteppingStone& Prototype)
	: CMapGameplayObject{ Prototype }
{
}

HRESULT CSteppingStone::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CSteppingStone::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	STEPPING_STONE_DESC Desc = *static_cast<STEPPING_STONE_DESC*>(pArg);

	Ready_DefaultDesc(&Desc);

	if (UINT_MAX == Desc.iTargetSpawnID)
		return E_FAIL;

	if (false == std::isfinite(Desc.fPressDuration) || Desc.fPressDuration <= 0.f)
		return E_FAIL;

	if (false == std::isfinite(Desc.vPressOffset.x) || false == std::isfinite(Desc.vPressOffset.y) || false == std::isfinite(Desc.vPressOffset.z))
		return E_FAIL;

	if (XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&Desc.vPressOffset))) <= FLT_EPSILON)
		return E_FAIL;

	m_iTargetSpawnID = Desc.iTargetSpawnID;
	m_vPressOffset = Desc.vPressOffset;
	m_fPressDuration = Desc.fPressDuration;

	// 부모의 Static Rigidbody는 생성하지 않는다.
	STEPPING_STONE_DESC BaseDesc = Desc;
	BaseDesc.bUsePhysics = false;

	if (FAILED(__super::Initialize(&BaseDesc)))
		return E_FAIL;

	Update_MovePositions();

	// Editor Preview에서는 Dynamic Rigidbody를 생성하지 않는다.
	if (true == Desc.bPhysicsStartRegistered && FAILED(Ready_DynamicPhysics(Desc)))
		return E_FAIL;

	m_eState = STEPPING_STONE_STATE::READY;

	return S_OK;
}

void CSteppingStone::Ready_DefaultDesc(STEPPING_STONE_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	pDesc->eVisualType = VISUAL_TYPE::STATIC;
	pDesc->iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	if (true == pDesc->strModelPrototypeTag.empty())
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_SteppingStone");

	pDesc->bUsePhysics = false;
}

HRESULT CSteppingStone::Ready_DynamicPhysics(const STEPPING_STONE_DESC& Desc)
{
	if (nullptr != m_pDynamicRigidBody)
		return S_OK;

	if (nullptr == m_pTransformCom)
		return E_FAIL;

	if (Desc.vPlatformHalfExtents.x <= 0.f || Desc.vPlatformHalfExtents.y <= 0.f || Desc.vPlatformHalfExtents.z <= 0.f)
		return E_FAIL;

	if (Desc.vTriggerHalfExtents.x <= 0.f || Desc.vTriggerHalfExtents.y <= 0.f || Desc.vTriggerHalfExtents.z <= 0.f)
		return E_FAIL;

	m_pDynamicRigidBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);

	if (nullptr == m_pDynamicRigidBody)
		return E_FAIL;

	PHYSIC_DESC PhysicDesc{};
	PhysicDesc.pOwner = this;
	PhysicDesc.bIsKinematic = true;
	PhysicDesc.iFilterWord0 = ETOUI(FILTER_GROUP::GROUND);
	PhysicDesc.iFilterWord1 = ETOUI(FILTER_GROUP::PLAYER);

	XMStoreFloat3(&PhysicDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&PhysicDesc.vInitQuat, m_pTransformCom->Get_Quat());

	const _float3 vAbsScale = _float3(fabsf(Desc.vScale.x), fabsf(Desc.vScale.y), fabsf(Desc.vScale.z));

	SHAPE_DESC PlatformShape{};
	PlatformShape.eType = GEOMETRY_TYPE::BOX;
	PlatformShape.vHalfExtents = _float3(Desc.vPlatformHalfExtents.x * vAbsScale.x, Desc.vPlatformHalfExtents.y * vAbsScale.y, Desc.vPlatformHalfExtents.z * vAbsScale.z);
	PlatformShape.vLocalOffset = _float3(Desc.vPlatformLocalOffset.x * vAbsScale.x, Desc.vPlatformLocalOffset.y * vAbsScale.y, Desc.vPlatformLocalOffset.z * vAbsScale.z);
	PlatformShape.bTrigger = false;
	PhysicDesc.Shapes.push_back(PlatformShape);

	SHAPE_DESC TriggerShape{};
	TriggerShape.eType = GEOMETRY_TYPE::BOX;
	TriggerShape.vHalfExtents = _float3(Desc.vTriggerHalfExtents.x * vAbsScale.x, Desc.vTriggerHalfExtents.y * vAbsScale.y, Desc.vTriggerHalfExtents.z * vAbsScale.z);
	TriggerShape.vLocalOffset = _float3(Desc.vTriggerLocalOffset.x * vAbsScale.x, Desc.vTriggerLocalOffset.y * vAbsScale.y, Desc.vTriggerLocalOffset.z * vAbsScale.z);
	TriggerShape.bTrigger = true;
	PhysicDesc.Shapes.push_back(TriggerShape);

	if (FAILED(m_pDynamicRigidBody->Initialize(&PhysicDesc)))
	{
		Safe_Release(m_pDynamicRigidBody);
		return E_FAIL;
	}

	return S_OK;
}

void CSteppingStone::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);

	if (STEPPING_STONE_STATE::PRESSING != m_eState || nullptr == m_pTransformCom || nullptr == m_pDynamicRigidBody)
		return;

	if (fFixedTime <= 0.f)
		return;

	_vector vCurrentPosition = XMLoadFloat3(&m_vPhysicsPosition);
	_vector vPressedPosition = XMLoadFloat3(&m_vPressedPosition);
	_vector vToTarget = vPressedPosition - vCurrentPosition;

	const _float fRemainDistance = XMVectorGetX(XMVector3Length(vToTarget));

	if (fRemainDistance <= FLT_EPSILON)
	{
		Finish_Pressing();
		return;
	}

	const _float fTotalDistance = XMVectorGetX(XMVector3Length(XMLoadFloat3(&m_vPressOffset)));
	const _float fPressSpeed = fTotalDistance / m_fPressDuration;
	const _float fMoveDistance = min(fPressSpeed * fFixedTime, fRemainDistance);

	_vector vNextPosition = vCurrentPosition + XMVector3Normalize(vToTarget) * fMoveDistance;
	_vector vQuaternion = m_pTransformCom->Get_Quat();

	// PhysX Target 적용에 실패하면 렌더 모델을 먼저 이동시키지 않는다.
	if (FAILED(m_pDynamicRigidBody->Set_KinematicTarget(vNextPosition, vQuaternion)))
		return;

	XMStoreFloat3(&m_vPhysicsPosition, vNextPosition);
	m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vNextPosition, 1.f));

	if (fMoveDistance >= fRemainDistance)
		Finish_Pressing();
}

void CSteppingStone::Begin_Overlap(CGameObject* pOther)
{
	if (STEPPING_STONE_STATE::READY != m_eState)
		return;

	if (nullptr == dynamic_cast<CPlayer*>(pOther))
		return;

	Start_Pressing();
}

HRESULT CSteppingStone::Start_Pressing()
{
	if (STEPPING_STONE_STATE::READY != m_eState)
		return S_OK;

	if (nullptr == m_pDynamicRigidBody || nullptr == m_pDynamicRigidBody->Get_RigidActor())
		return E_FAIL;

	if (nullptr == m_pDynamicRigidBody->Get_RigidActor()->getScene())
		return E_FAIL;

	m_eState = STEPPING_STONE_STATE::PRESSING;

	if (nullptr != m_pGameInstance)
	{
		MAP_GAMEPLAY_ACTIVATE_EVENT_DESC EventDesc{};
		EventDesc.iTargetSpawnID = m_iTargetSpawnID;

		// SteppingStone과 연결된 Bridge를 1회 활성화한다.
  		m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::STEPPING_STONE, &EventDesc);

		// PhysX Trigger Callback 밖에서 컷신을 재생하도록 다음 일반 Update에 요청한다.
		m_bCutSceneRequestPending = true;
	}

	return S_OK;
}

void CSteppingStone::Update_MapGameplayObject(_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);

	if (false == m_bCutSceneRequestPending || nullptr == m_pGameInstance)
		return;

	m_bCutSceneRequestPending = false;

	CUTSCENE_REQUEST CutSceneRequest{};
	CutSceneRequest.iTargetSpawnID = Get_SpawnID();
	CutSceneRequest.strAssetName = "Stone_Bridge_A";
	CutSceneRequest.bPlayOnce = true;
	CutSceneRequest.hResult = E_FAIL;

	// 기존 컷신 Trigger와 동일한 요청 구조로 Bridge 연출 컷신을 재생한다.
	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_BRIDGE, &CutSceneRequest);
}

void CSteppingStone::Update_MovePositions()
{
	if (nullptr == m_pTransformCom)
		return;

	_vector vStartPosition = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vPressedPosition = vStartPosition + XMLoadFloat3(&m_vPressOffset);

	XMStoreFloat3(&m_vStartPosition, vStartPosition);
	XMStoreFloat3(&m_vPressedPosition, vPressedPosition);
	XMStoreFloat3(&m_vPhysicsPosition, vStartPosition);
}

void CSteppingStone::Finish_Pressing()
{
	m_vPhysicsPosition = m_vPressedPosition;
	m_eState = STEPPING_STONE_STATE::PRESSED;
}

void CSteppingStone::Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition)
{
	__super::Set_MapTransform(vScale, vRotationDegree, vPosition);

	Update_MovePositions();

	if (nullptr != m_pDynamicRigidBody && nullptr != m_pTransformCom)
		m_pDynamicRigidBody->Set_GlobalTransform(m_pTransformCom->Get_State(STATE::POSITION), m_pTransformCom->Get_Quat());

	m_bCutSceneRequestPending = false;
	m_eState = STEPPING_STONE_STATE::READY;
}

CSteppingStone* CSteppingStone::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CSteppingStone* pInstance = new CSteppingStone(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CSteppingStone");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CSteppingStone::Clone(void* pArg)
{
	CSteppingStone* pInstance = new CSteppingStone(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CSteppingStone");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CSteppingStone::Free()
{
	Safe_Release(m_pDynamicRigidBody);

	__super::Free();
}
