#include "HotelDoor.h"
#include "Animator.h"
#include "DynamicRigidBody.h"
#include "GameInstance.h"
#include "Model.h"

CHotelDoor::CHotelDoor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CHotelDoor::CHotelDoor(const CHotelDoor& Prototype)
	: CMapGameplayObject{ Prototype }
{
}

HRESULT CHotelDoor::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CHotelDoor::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	MAP_GAMEPLAY_OBJECT_DESC Desc = *static_cast<MAP_GAMEPLAY_OBJECT_DESC*>(pArg);
	Ready_DefaultDesc(&Desc);

	m_eInteractionType = Resolve_InteractionType(Desc.strAnimFolderTag);

	if (INTERACTION_TYPE::END == m_eInteractionType)
		return E_FAIL;

	// HotelDoor는 문짝 Bone을 따라가는 Dynamic Rigidbody를 직접 생성하므로 공통 Static Rigidbody를 막는다.
	MAP_GAMEPLAY_OBJECT_DESC BaseDesc = Desc;
	BaseDesc.bUsePhysics = false;

	if (FAILED(__super::Initialize(&BaseDesc)))
		return E_FAIL;

	if (FAILED(Enter_CloseIdle()))
		return E_FAIL;

	// Controller Entry Pose를 Bone에 반영한 뒤 문짝별 Dynamic Rigidbody를 부착한다.
	Drive_Animator(0.f);

	if (nullptr != m_pModelCom && nullptr != m_pAnimator)
		m_pModelCom->Update_Bones(&m_pAnimator->Get_CurPose());

	if (true == Desc.bUsePhysics && FAILED(Ready_DynamicPhysics(Desc)))
		return E_FAIL;

	return S_OK;
}

void CHotelDoor::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);

	// Update에서 계산된 최신 Animation Pose를 물리 Simulation 전에 Bone 행렬로 갱신한다.
	if (nullptr != m_pModelCom && nullptr != m_pAnimator)
		m_pModelCom->Update_Bones(&m_pAnimator->Get_CurPose());

	for (_uint i = 0; i < m_iDoorRigidBodyCount; ++i)
	{
		if (nullptr == m_pDoorRigidBodies[i])
			continue;

		PxRigidActor* pActor = m_pDoorRigidBodies[i]->Get_RigidActor();

		// Scene에서 제거된 문 Actor에는 Kinematic Target을 전달하지 않는다.
		if (nullptr == pActor || nullptr == pActor->getScene())
			continue;

		m_pDoorRigidBodies[i]->Physical_Update(fFixedTime);
	}
}

void CHotelDoor::Ready_DefaultDesc(MAP_GAMEPLAY_OBJECT_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	// HotelDoor는 컨트롤러 기반으로 Door_Close_Idle Entry 상태에서 시작한다.
	pDesc->eVisualType = VISUAL_TYPE::SKINNED;
	pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_HotelDoor") : pDesc->strModelPrototypeTag;
	pDesc->strAnimFolderTag = pDesc->strAnimFolderTag.empty() ? TEXT("Hotel_Door") : pDesc->strAnimFolderTag;
	pDesc->strAnimTag = pDesc->strAnimTag.empty() ? TEXT("Hotel_Door") : pDesc->strAnimTag;
	pDesc->bControllerEnabled = true;
	pDesc->bPlayDefault = false;
	pDesc->bApplyRootMotion = false;
}

INTERACTION_TYPE CHotelDoor::Resolve_InteractionType(const _wstring& strAnimFolderTag)
{
	if (TEXT("Hotel_Door") == strAnimFolderTag || TEXT("Hotel_InDoor") == strAnimFolderTag)
		return INTERACTION_TYPE::DOOR_HOTEL;

	if (TEXT("Monastery_Door") == strAnimFolderTag)
		return INTERACTION_TYPE::DOOR_BIG;

	if (TEXT("Train_Door") == strAnimFolderTag)
		return INTERACTION_TYPE::DOOR_SLIDE;

	if (TEXT("Boss_Door") == strAnimFolderTag)
		return INTERACTION_TYPE::DOOR_BOSS;

	return INTERACTION_TYPE::END;
}

HRESULT CHotelDoor::Ready_DynamicPhysics(const MAP_GAMEPLAY_OBJECT_DESC& Desc)
{
	if (nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return E_FAIL;

	// Train_Door는 Door 단일 본, Boss_Door는 전용 좌우 본, 나머지 양문은 Door_R/Door_L 본을 사용한다.
	const _bool bSingleDoorBone = TEXT("Train_Door") == Desc.strAnimFolderTag;
	const _bool bBossDoor = TEXT("Boss_Door") == Desc.strAnimFolderTag;
	m_iDoorRigidBodyCount = true == bSingleDoorBone ? 1 : 2;

	const char* pDoorBoneNames[MAX_DOOR_RIGID_BODY_COUNT] = { "Door_R", "Door_L" };
	const _float3 vRightHalfExtents = _float3(Desc.vPhysicsHalfExtents.x, Desc.vPhysicsHalfExtents.z, Desc.vPhysicsHalfExtents.y);
	const _float3 vLeftHalfExtents = _float3(Desc.vPhysicsHalfExtents.z, Desc.vPhysicsHalfExtents.x, Desc.vPhysicsHalfExtents.y);
	const _float3 vRightLocalOffset = _float3(-Desc.vPhysicsHalfExtents.x, 0.f, Desc.vPhysicsHalfExtents.y);
	const _float3 vLeftLocalOffset = _float3(0.f, Desc.vPhysicsHalfExtents.x, -Desc.vPhysicsHalfExtents.y);
	_float3 vHalfExtents[MAX_DOOR_RIGID_BODY_COUNT] = { vRightHalfExtents, vLeftHalfExtents };
	_float3 vLocalOffsets[MAX_DOOR_RIGID_BODY_COUNT] = { vRightLocalOffset, vLeftLocalOffset };

	if (true == bBossDoor)
	{
		// Boss_Door의 양쪽 문짝은 동일한 크기이며 본 로컬 Z축을 기준으로 중심만 반대 방향이다.
		pDoorBoneNames[0] = "LV_Krat_Gate01K_SM_LSJ_RIGHT";
		pDoorBoneNames[1] = "LV_Krat_Gate01K_SM_LSJ_LEFT";
		vHalfExtents[0] = Desc.vPhysicsHalfExtents;
		vHalfExtents[1] = Desc.vPhysicsHalfExtents;
		vLocalOffsets[0] = Desc.vPhysicsLocalOffset;
		vLocalOffsets[1] = _float3(Desc.vPhysicsLocalOffset.x, Desc.vPhysicsLocalOffset.y, -Desc.vPhysicsLocalOffset.z);
	}
	else if (true == bSingleDoorBone)
	{
		pDoorBoneNames[0] = "Door";
		vHalfExtents[0] = Desc.vPhysicsHalfExtents;
		vLocalOffsets[0] = Desc.vPhysicsLocalOffset;
	}

	for (_uint i = 0; i < m_iDoorRigidBodyCount; ++i)
	{
		const _float4x4* pBoneCombinedMatrix = m_pModelCom->Get_Bone_CombinedMatPtr(pDoorBoneNames[i]);

		if (nullptr == pBoneCombinedMatrix)
			return E_FAIL;

		m_pDoorRigidBodies[i] = CDynamicRigidBody::Create(m_pDevice, m_pContext);

		if (nullptr == m_pDoorRigidBodies[i])
			return E_FAIL;

		PHYSIC_DESC PhysicDesc{};
		PhysicDesc.pOwner = this;
		PhysicDesc.bIsKinematic = true;
		PhysicDesc.iFilterWord0 = Desc.iPhysicsFilterWord0;
		PhysicDesc.iFilterWord1 = Desc.iPhysicsFilterWord1;
		XMStoreFloat3(&PhysicDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
		XMStoreFloat4(&PhysicDesc.vInitQuat, m_pTransformCom->Get_Quat());

		SHAPE_DESC ShapeDesc{};
		ShapeDesc.eType = GEOMETRY_TYPE::BOX;
		ShapeDesc.vHalfExtents = vHalfExtents[i];
		ShapeDesc.vLocalOffset = vLocalOffsets[i];
		ShapeDesc.bTrigger = false;
		PhysicDesc.Shapes.push_back(ShapeDesc);

		if (FAILED(m_pDoorRigidBodies[i]->Initialize(&PhysicDesc)))
			return E_FAIL;

		// 문 본의 최신 Combined Matrix를 Kinematic Target으로 사용한다.
		m_pDoorRigidBodies[i]->Attach_ToBone(pBoneCombinedMatrix, XMMatrixIdentity());
		m_pDoorRigidBodies[i]->Physical_Update(0.f);

		if (false == Desc.bPhysicsStartRegistered && nullptr != m_pDoorRigidBodies[i]->Get_RigidActor()->getScene())
			m_pGameInstance->Remove_ActorFromScene(m_pDoorRigidBodies[i]->Get_RigidActor());
	}

	return S_OK;
}

void CHotelDoor::UnRegister_DynamicPhysics()
{
	if (nullptr == m_pGameInstance)
		return;

	for (_uint i = 0; i < m_iDoorRigidBodyCount; ++i)
	{
		if (nullptr == m_pDoorRigidBodies[i])
			continue;

		PxRigidActor* pActor = m_pDoorRigidBodies[i]->Get_RigidActor();

		if (nullptr == pActor || nullptr == pActor->getScene())
			continue;

		// 문 애니메이션 중 플레이어가 문짝 충돌체에 밀리거나 막히지 않도록 제거한다.
		m_pGameInstance->Remove_ActorFromScene(pActor);
	}
}

HRESULT CHotelDoor::Begin_Open()
{
	if (m_eState != DOOR_STATE::CLOSE_IDLE)
		return S_OK;

	//TODO : 정확한 이벤트 위치 지정하기 

	Activate_EventDesc EventDesc{};
	EventDesc.eInteractionType = m_eInteractionType;
	XMStoreFloat4(&EventDesc.vPosition, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&EventDesc.vQuat, m_pTransformCom->Get_Quat());

	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::OPEN_DOOR, &EventDesc);

	return Enter_Open();
}

void CHotelDoor::Deactivate_ForBossRetry()
{
	// 재도전부터는 기존 애니메이션 문 대신 BossArenaBarrier가 입구를 제어한다.
	m_bPlayerInTrigger = false;

	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, OBJECT_TYPE::DOOR);
		m_bInteractionPromptVisible = false;
	}

	// 부활 시 비활성 Boss_Door의 Trigger 생성 실패가 다음 Boss_Barrier Trigger 생성을 막지 않도록 연결 Trigger도 함께 소비한다.
	Deactivate_LinkedTriggers();

	UnRegister_DynamicPhysics();
	Set_InActive();
}

HRESULT CHotelDoor::Enter_CloseIdle()
{
	m_eState = DOOR_STATE::CLOSE_IDLE;

	// 닫힌 상태만 true로 둔다.
	Set_AnimBool(TEXT("Door_Close_Idle"), true);
	Set_AnimBool(TEXT("Door_Open"), false);
	Set_AnimBool(TEXT("Door_Open_Idle"), false);

	return S_OK;
}

HRESULT CHotelDoor::Enter_Open()
{
	// 문 열림 애니메이션을 요청하기 전에 Door_R, Door_L 충돌체를 먼저 제거한다.
	UnRegister_DynamicPhysics();

	m_eState = DOOR_STATE::OPENING;

	// Door_Open State는 AS_Open_Push Clip을 Until End로 재생한다.
	Set_AnimBool(TEXT("Door_Close_Idle"), false);
	Set_AnimBool(TEXT("Door_Open_Idle"), false);
	Set_AnimBool(TEXT("Door_Open"), true);
	Request_AnimState(TEXT("Door_Open"));

	return S_OK;
}

HRESULT CHotelDoor::Enter_OpenIdle()
{
	m_eState = DOOR_STATE::OPEN_IDLE;

	// 열림 애니메이션 종료 후 Open_Idle 상태로 고정한다.
	Set_AnimBool(TEXT("Door_Open"), false);
	Set_AnimBool(TEXT("Door_Close_Idle"), false);
	Set_AnimBool(TEXT("Door_Open_Idle"), true);

	return S_OK;
}

void CHotelDoor::Update_MapGameplayObject(_float fTimeDelta)
{
	// Update는 문 열림 애니메이션 상태만 진행한다.
	if (DOOR_STATE::OPENING == m_eState && true == Is_AnimationFinished())
		Enter_OpenIdle();
}

_bool CHotelDoor::Can_Interact(CGameObject* pPlayer) const
{
	// 닫힌 문과 Linked Trigger 안의 플레이어만 상호작용할 수 있다.
	if (nullptr == pPlayer || nullptr == m_pGameInstance)
		return false;

	return DOOR_STATE::CLOSE_IDLE == m_eState && true == m_bPlayerInTrigger;
}

_bool CHotelDoor::Interact(CGameObject* pPlayer)
{
	if (false == Can_Interact(pPlayer))
		return false;

	if (FAILED(Begin_Open()))
		return false;

	// 한 번 사용한 문은 현재 트리거 체류 상태와 안내 UI를 즉시 종료한다.
	m_bPlayerInTrigger = false;

	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, OBJECT_TYPE::DOOR);
		m_bInteractionPromptVisible = false;
	}

	return true;
}
void CHotelDoor::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	// 이미 사용한 문은 트리거가 다시 감지돼도 상호작용 상태로 만들지 않는다.
	if (DOOR_STATE::CLOSE_IDLE != m_eState)
		return;

	Set_PlayerInTrigger(true);

	if (false == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(true, OBJECT_TYPE::DOOR);
		m_bInteractionPromptVisible = true;
	}
}

void CHotelDoor::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	Set_PlayerInTrigger(false);

	if (false == m_bInteractionPromptVisible)
		return;

	Broadcast_ObjUI(false, OBJECT_TYPE::DOOR);
	m_bInteractionPromptVisible = false;
}

void CHotelDoor::On_LinkedActivate()
{
	if (DOOR_STATE::CLOSE_IDLE != m_eState)
		return;

	Enter_Open();
}

CHotelDoor* CHotelDoor::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CHotelDoor* pInstance = new CHotelDoor(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CHotelDoor");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CHotelDoor::Clone(void* pArg)
{
	CHotelDoor* pInstance = new CHotelDoor(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CHotelDoor");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CHotelDoor::Free()
{
	// 트리거 종료 콜백 없이 언로드돼도 안내 UI가 남지 않게 한다.
	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, OBJECT_TYPE::DOOR);
		m_bInteractionPromptVisible = false;
	}

	for (_uint i = 0; i < MAX_DOOR_RIGID_BODY_COUNT; ++i)
	{
		if (nullptr != m_pDoorRigidBodies[i])
		{
			PxRigidActor* pActor = m_pDoorRigidBodies[i]->Get_RigidActor();

			// 기존 CRigidActor 해제 경로가 Scene 제거 후 Actor를 해제할 수 있도록 잠시 복귀시킨다.
			if (nullptr != pActor && nullptr == pActor->getScene())
				m_pGameInstance->Add_ActorToScene(pActor);
		}

		Safe_Release(m_pDoorRigidBodies[i]);
	}

	__super::Free();
}
