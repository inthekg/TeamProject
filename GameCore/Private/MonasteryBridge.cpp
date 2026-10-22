#include "MonasteryBridge.h"

#include "DynamicRigidBody.h"
#include "Transform.h"

CMonasteryBridge::CMonasteryBridge(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CMonasteryBridge::CMonasteryBridge(const CMonasteryBridge& Prototype)
	: CMapGameplayObject{ Prototype }
{
}

HRESULT CMonasteryBridge::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CMonasteryBridge::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	MONASTERY_BRIDGE_DESC Desc = *static_cast<MONASTERY_BRIDGE_DESC*>(pArg);

	Ready_DefaultDesc(&Desc);

	if (false == std::isfinite(Desc.fMoveDuration) || Desc.fMoveDuration <= 0.f)
		return E_FAIL;

	if (false == std::isfinite(Desc.vMoveOffset.x) || false == std::isfinite(Desc.vMoveOffset.y) || false == std::isfinite(Desc.vMoveOffset.z))
		return E_FAIL;

	if (XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&Desc.vMoveOffset))) <= FLT_EPSILON)
		return E_FAIL;

	m_vMoveOffset = Desc.vMoveOffset;
	m_fMoveDuration = Desc.fMoveDuration;

	MONASTERY_BRIDGE_DESC BaseDesc = Desc;
	BaseDesc.bUsePhysics = false;

	if (FAILED(__super::Initialize(&BaseDesc)))
		return E_FAIL;

	Update_MovePositions();

	// Editor Preview에서는 Dynamic Rigidbody를 생성하지 않는다.
	if (true == Desc.bPhysicsStartRegistered && FAILED(Ready_DynamicPhysics(Desc)))
		return E_FAIL;

	m_eState = BRIDGE_STATE::READY;

	return S_OK;
}

void CMonasteryBridge::Ready_DefaultDesc(MONASTERY_BRIDGE_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	pDesc->eVisualType = VISUAL_TYPE::STATIC;
	pDesc->iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	if (true == pDesc->strModelPrototypeTag.empty())
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_Monastery_Bridge");

	pDesc->bUsePhysics = false;
}

HRESULT CMonasteryBridge::Ready_DynamicPhysics(const MONASTERY_BRIDGE_DESC& Desc)
{
	if (nullptr != m_pDynamicRigidBody)
		return S_OK;

	if (nullptr == m_pTransformCom)
		return E_FAIL;

	if (Desc.vPlatformHalfExtents.x <= 0.f || Desc.vPlatformHalfExtents.y <= 0.f || Desc.vPlatformHalfExtents.z <= 0.f)
		return E_FAIL;

	m_pDynamicRigidBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);

	if (nullptr == m_pDynamicRigidBody)
		return E_FAIL;

	PHYSIC_DESC PhysicDesc{};
	PhysicDesc.pOwner = this;

	// Dynamic Actor를 사용하지만 코드로 위치를 제어하기 위해 Kinematic으로 설정한다.
	PhysicDesc.bIsKinematic = true;
	PhysicDesc.iFilterWord0 = ETOUI(FILTER_GROUP::GROUND);
	PhysicDesc.iFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);

	XMStoreFloat3(&PhysicDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&PhysicDesc.vInitQuat, m_pTransformCom->Get_Quat());

	const _float3 vAbsScale = _float3(fabsf(Desc.vScale.x), fabsf(Desc.vScale.y), fabsf(Desc.vScale.z));

	SHAPE_DESC PlatformShape{};
	PlatformShape.eType = GEOMETRY_TYPE::BOX;
	PlatformShape.vHalfExtents = _float3(Desc.vPlatformHalfExtents.x * vAbsScale.x, Desc.vPlatformHalfExtents.y * vAbsScale.y, Desc.vPlatformHalfExtents.z * vAbsScale.z);
	PlatformShape.vLocalOffset = _float3(Desc.vPlatformLocalOffset.x * vAbsScale.x, Desc.vPlatformLocalOffset.y * vAbsScale.y, Desc.vPlatformLocalOffset.z * vAbsScale.z);
	PlatformShape.bTrigger = false;
	PhysicDesc.Shapes.push_back(PlatformShape);

	if (FAILED(m_pDynamicRigidBody->Initialize(&PhysicDesc)))
	{
		Safe_Release(m_pDynamicRigidBody);
		return E_FAIL;
	}

	return S_OK;
}

void CMonasteryBridge::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);

	if (BRIDGE_STATE::RISING != m_eState || nullptr == m_pTransformCom || nullptr == m_pDynamicRigidBody)
		return;

	if (fFixedTime <= 0.f)
		return;

	_vector vCurrentPosition = XMLoadFloat3(&m_vPhysicsPosition);
	_vector vEndPosition = XMLoadFloat3(&m_vEndPosition);
	_vector vToTarget = vEndPosition - vCurrentPosition;

	const _float fRemainDistance = XMVectorGetX(XMVector3Length(vToTarget));

	if (fRemainDistance <= FLT_EPSILON)
	{
		Finish_Rising();
		return;
	}

	const _float fTotalDistance = XMVectorGetX(XMVector3Length(XMLoadFloat3(&m_vMoveOffset)));
	const _float fMoveSpeed = fTotalDistance / m_fMoveDuration;
	const _float fMoveDistance = min(fMoveSpeed * fFixedTime, fRemainDistance);

	_vector vNextPosition = vCurrentPosition + XMVector3Normalize(vToTarget) * fMoveDistance;
	_vector vQuaternion = m_pTransformCom->Get_Quat();

	if (FAILED(m_pDynamicRigidBody->Set_KinematicTarget(vNextPosition, vQuaternion)))
		return;

	XMStoreFloat3(&m_vPhysicsPosition, vNextPosition);
	m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vNextPosition, 1.f));

	if (fMoveDistance >= fRemainDistance)
		Finish_Rising();
}

HRESULT CMonasteryBridge::Start_Rising()
{
	// READY에서만 시작하므로 도착 후 다시 이동하지 않는다.
	if (BRIDGE_STATE::READY != m_eState)
		return S_OK;

	if (nullptr == m_pDynamicRigidBody || nullptr == m_pDynamicRigidBody->Get_RigidActor())
		return E_FAIL;

	if (nullptr == m_pDynamicRigidBody->Get_RigidActor()->getScene())
		return E_FAIL;

	m_eState = BRIDGE_STATE::RISING;

	return S_OK;
}

void CMonasteryBridge::On_LinkedActivate()
{
	Start_Rising();
}

void CMonasteryBridge::Update_MovePositions()
{
	if (nullptr == m_pTransformCom)
		return;

	_vector vStartPosition = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vEndPosition = vStartPosition + XMLoadFloat3(&m_vMoveOffset);

	XMStoreFloat3(&m_vStartPosition, vStartPosition);
	XMStoreFloat3(&m_vEndPosition, vEndPosition);
	XMStoreFloat3(&m_vPhysicsPosition, vStartPosition);
}

void CMonasteryBridge::Finish_Rising()
{
	m_vPhysicsPosition = m_vEndPosition;
	m_eState = BRIDGE_STATE::ARRIVED;
}

void CMonasteryBridge::Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition)
{
	__super::Set_MapTransform(vScale, vRotationDegree, vPosition);

	Update_MovePositions();

	if (nullptr != m_pDynamicRigidBody && nullptr != m_pTransformCom)
		m_pDynamicRigidBody->Set_GlobalTransform(m_pTransformCom->Get_State(STATE::POSITION), m_pTransformCom->Get_Quat());

	m_eState = BRIDGE_STATE::READY;
}

CMonasteryBridge* CMonasteryBridge::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonasteryBridge* pInstance = new CMonasteryBridge(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonasteryBridge");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CMonasteryBridge::Clone(void* pArg)
{
	CMonasteryBridge* pInstance = new CMonasteryBridge(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonasteryBridge");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMonasteryBridge::Free()
{
	Safe_Release(m_pDynamicRigidBody);

	__super::Free();
}