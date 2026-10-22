#include "Player.h"
#include "MasterModel.h"
#include "BodyPart.h"
#include "GameInstance.h"
#include "Animator.h"
#include "Engine_Utility.h"
#include "CCT.h"
#include "SpringArm.h"
#include "Status.h"
#include "Movement.h"
#include "StatComponent.h"
#include "Equipment.h"
#include "Weapon.h"
#include "Bone.h"
#include "Effect.h"
#include "AnimDriver.h"
#include "TargetComponent.h"
#include "AIObject.h"
#include "SoundManager.h"
#include "LegionArm_Manager.h"
#include "DynamicRigidBody.h"
#include "Ladder.h"
#include "MapGameplayObject.h"
#include "Monster_Include.h"
#include "Dissolve.h"
#include "PinoLamp.h"
#include "ChargeGun.h"
#include "Partset.h"
#include "ItemTable.h"
#include "Inventory.h"
#include "Item.h"
#include "GrappleGun.h"
#include "NPC.h"
#include "CutScene_Data.h"
#include "AimComponent.h"

#pragma region PLAYER_DATA
namespace
{
	/*Stat*/
	static constexpr _float			m_fMaxHP					= { 10000.f };
	static constexpr _float			m_fMaxStamina				= { 100.f };

	/*Guard*/
	static constexpr _float			GUARD_DAMAGE_REDUCTION		= { 0.5f };
	static constexpr _float			GUARD_STATMINA_COST			= { 25.f };
	static constexpr _float			GUARD_KNOCKBACK_MAX			= { 2.f };
	static constexpr _float			GUARD_KNOCKBACK_DURATION	= { 0.2f };
	const _float					GUARD_FRONT_DOT				= { cosf(XMConvertToRadians(80.f)) };

	/*Shake*/
	static constexpr const char*	CAM_SHAKE_PARRY				= { "BP_Shake_PerfectGuard" };
	static constexpr const char*	CAM_SHAKE_PARRY_FURY		= { "BP_Shake_PerfectGuard_Fury" };
	static constexpr const char*	STR_GRAB_BONE				= { "Grab_Root" };
	
	/*Effect*/ 
	static constexpr const char*	STR_HEAL_BONE_BODY			= { "Bip001-Neck" }; //Bip001-Spine2_Back
	static constexpr const char*	STR_HEAL_BONE				= { "BN_Weapon_L" };
	static constexpr const char*	STR_ELECT_EFFECT			= { "Effect_Player_Electric" };

	/*CCT*/
	static constexpr _float			CCT_HEIGHT					= { 1.7f };
	static constexpr _float			CCT_RADIUS					= { 0.5f };

	/* Grab */
	static constexpr _float			THROW_BODY_RADIUS			= { 0.15f };
	static constexpr _float			THROW_SPEED					= { 80.f };		// 수평 초속
	static constexpr _float			THROW_UP_SPEED				= { -9.f };		// 수직 초속
	static constexpr _float			GRAB_HOLD_TIMEOUT			= { 8.f };
	static constexpr _float			THROW_MIN_AIR_TIME			= { 0.1f };
	static constexpr _float			THROW_MAX_AIR_TIME			= { 4.f };
	static constexpr _float			THROW_SETTLE_SPEED			= { 0.5f };
	static constexpr _float			GROUND_NORMAL_Y				= { 0.5f };
	
	/* Ladder */
	static constexpr _float			LADDER_STEP_HEIGHT			= { 0.5f };		// 한 스탭 상승량 0.475f
	static constexpr _float			LADDER_ATTACH_OFFSET		= { 0.625f };	// 사다리 면 ~ 플레이어 루트 거리
	static constexpr _float			LADDER_MARKER_SURFACE_OFFSET = { 0.116f };

	static constexpr _float			LADDER_END_UP_TRIGGER		= { 2.5f };
	static constexpr _float			LADDER_END_DOWN_TRIGGER		= { 0.4f };

	static constexpr _float			LADDER_END_UP_FORWARD		= { 1.49f };	// 목표 End_Up 루트모션 XZ 총량 
	static constexpr _float			LADDER_END_UP_DELTA_Y		= { 2.11f };	// 목표 End_Up 루트모션 Y 총량
	static constexpr _float			LADDER_END_DOWN_BACK		= { 0.30f };	// 목표 End_Down 루트모션 XZ 총량 
	static constexpr _float			LADDER_END_DOWN_DELTA_Y		= { 0.40f };	// 목표 End_Down 루트모션 Y 총량

	static constexpr _float			LADDER_START_DOWN_BACK		= { 1.14f };


	/* Sound */
	static constexpr const char*	SOUND_PERFECT_GUARD_CUE				= { "SE_PC_SK_GetHit_Sword_PerfectGuard_Cue" };
	static constexpr const char*	SOUND_PERFECT_GUARD_METAL_CUE		= { "SE_PC_SK_GetHit_Sword_PerfectGuard_Metal_Cue" };
	static constexpr const char*	SOUND_PERFECT_GUARD_METAL_FURY_CUE	= { "SE_PC_SK_GetHit_Sword_PerfectGuard_Metal_Fury_Cue" };
	static constexpr const char*	SOUND_PARRY_CUE						= { "SE_PC_SK_GetHit_M_Parry_Cue" };
	static constexpr const char*	SOUND_GUARD_METAL_CUE				= { "SE_PC_SK_GetHit_Guard_Metal_L_Cue" };
	static constexpr const char*	SOUND_GUARD_METAL2_CUE				= { "SE_PC_SK_GetHit_M_Guard_Metal_02_Cue" };
	static constexpr const char*	SOUND_GUARD_BREAK_CUE				= { "SE_PC_SK_GetHit_M_Sword_GuardBreak_Cue" };
	static constexpr const char*	SOUND_HEAL							= { "SE_PC_FX_Item_Heal_Cue" };


	/* EX */
	static constexpr _float			PARRY_GROGGY = { 50.f };
	static constexpr _float			SPRINT_STAMINA_COST = { 10.f };
	static constexpr _float			GRINDER_AMOUNT_PER_SEC = { 20.f };
	static constexpr _float			DEAD_DELAY = { 2.f };
	static constexpr _float			TELEPORT_DELAY = { 4.f };

	static constexpr _float			SPRING_ARM_LENGTH = { 3.5f };
	//시작할때 소유하고있을 장비
	static constexpr _uint64 StartingItemIDs[] =
	{
		ItemID::WEAPON_GREATSWORD,
		ItemID::WEAPON_SCISSOR,

		ItemID::BODY_BASE,
		ItemID::FACE_BASE, 
		ItemID::HAIR_BASE,

		ItemID::LEGION_BASE,
		//디버깅 
		ItemID::LEGION_GRAPPLE,
		ItemID::LEGION_CHARGEGUN,
		//
	};

}

#pragma endregion


// HIT_TYPE별 넉백 거리 
_float CPlayer::m_KnockBackDists[ETOUI(HIT_TYPE::End)] = { 1.25f, 2.f ,0.f, 0.f, 0.f, 1.25f };

CPlayer::CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CCharacter{ pDevice, pContext }
{
	m_eActorType = ACTOR_TYPE::PLAYER;
	m_fGravityWeight = 0.4f;
}

CPlayer::CPlayer(const CPlayer& Prototype)
	:CCharacter{ Prototype }
{
}

HRESULT CPlayer::Initialize_Prototype()
{
	m_strRightHand_BoneName = "BN_Weapon_R";
	m_strLeftHand_BoneName = "BN_Weapon_L";

	return S_OK;
}

HRESULT CPlayer::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components()))
		return E_FAIL;

	if (FAILED(Ready_Animator()))
		return E_FAIL;

	if (FAILED(Ready_PartObjects()))
		return E_FAIL;

	//PartObject 뒤에 와야함
	if (FAILED(Ready_Inventory()))
		return E_FAIL;

	Ready_Event_Subscription();

	if(m_pAnimDriver)
		m_pAnimDriver->Set_LegionArmManager(m_pLegionArm_Manager);

	m_pGrabSocketBone = Get_Bone_CombinedMatPtr(STR_GRAB_BONE);
	if (m_pGrabSocketBone == nullptr)
	{
		MSG_BOX("CPlayer::Initialize : Failed to Find Grab Bone");
		return E_FAIL;
	}

	return S_OK;
}

void CPlayer::Priority_Update(_float fTimeDelta)
{
	Update_UILock();

	m_fGlobalTimeScale_Offset = m_fGlobalTimeScale_Offset_Pending;

	fTimeDelta *= m_fGlobalTimeScale_Offset;

	m_pInputCom->Update(fTimeDelta, !m_pStatusCom->CanInput());
	m_pEquipmentCom->Priority_Update(fTimeDelta);
}

void CPlayer::Update(_float fTimeDelta)
{
	/*수동이벤트*/
	if(m_pGameInstance->Key_Down(DIK_NUMPAD1))
	{
		vector <GET_ITEM_EVENT_DESC> Items;
		GET_ITEM_EVENT_DESC WeaponDesc;
		WeaponDesc.eItemType = ITEM_TYPE::WEAPON;
		WeaponDesc.strItemName = L"Item_Weapon_GreatSword";
		WeaponDesc.iItemID = Hash_String(WeaponDesc.strItemName.c_str());
		WeaponDesc.iItemCount = 1;

		Items.push_back(WeaponDesc);

		GET_ITEM_EVENT_DESC LegionDesc;
		LegionDesc.eItemType = ITEM_TYPE::LEGION_ARM;
		LegionDesc.strItemName = L"Item_Legion_Grapple";
		LegionDesc.iItemID = Hash_String(LegionDesc.strItemName.c_str());
		LegionDesc.iItemCount = 1;

		Items.push_back(LegionDesc);
		CItem::Event_GetItems(Items);
	}

	if (m_pGameInstance->Key_Down(DIK_NUMPAD2))
	{
		vector <GET_ITEM_EVENT_DESC> Items;
		GET_ITEM_EVENT_DESC WeaponDesc;
		WeaponDesc.eItemType = ITEM_TYPE::WEAPON;
		WeaponDesc.strItemName = L"Item_Weapon_Scissor";
		WeaponDesc.iItemID = Hash_String(WeaponDesc.strItemName.c_str());
		WeaponDesc.iItemCount = 1;

		Items.push_back(WeaponDesc);

		GET_ITEM_EVENT_DESC LegionDesc;
		LegionDesc.eItemType = ITEM_TYPE::LEGION_ARM;
		LegionDesc.strItemName = L"Item_Legion_ChargeGun";
		LegionDesc.iItemID = Hash_String(LegionDesc.strItemName.c_str());
		LegionDesc.iItemCount = 1;

		Items.push_back(LegionDesc);
		CItem::Event_GetItems(Items);
	}

	if (m_pGameInstance->Key_Down(DIK_NUMPAD3))
	{
		HitInfo hitInfo;
		hitInfo.eHitType = HIT_TYPE::KnockBack;
		hitInfo.fDamage =  m_pStatCom->Get_CurrentHP() * 0.9f;
		
		Take_Damage(hitInfo);
	}

	if (m_pGameInstance->Key_Down(DIK_NUMPAD4))
	{
		if (m_pInventory)
			m_pInventory->Unlock_Grapple_Mobility();
	}


	// 컷신 매니저가 Transform과 Animator를 직접 평가하므로 게임 이동과 CCT가 결과를 덮지 않게 함
	if (nullptr != m_pAnimDriver && m_pAnimDriver->Is_CutSceneControlled())
	{
		m_pAnimDriver->Update(fTimeDelta);
		return;
	}

	fTimeDelta *= m_fGlobalTimeScale_Offset;


	if (m_pStatCom->Is_Dead())
	{
		Update_Dead(fTimeDelta);
		return;
	}

	if (m_bTeleportPending)
	{
		Update_Teleport(fTimeDelta);
	}

	if (Is_LadderActive())
	{
		Update_Ladder(fTimeDelta);
		return;
	}

	if (m_bMaterializing)
	{
		Update_Materialize(fTimeDelta);
		return;
	}

	Update_LockOn();

	Drive_ItemSlot();

	if (m_pAnimDriver)
		m_pAnimDriver->Update(fTimeDelta);

	//UILock일때 AnimDriver에서 갱신해놓은 상태를 덮어씌워야함 
	if (m_bUILock)
		Set_Player_UILock(true);

	if (m_eGrabState != GRAB_STATE::END)
		Update_Grab(fTimeDelta);

	if (m_pInputCom->Consume(PlayerActionID::LegionArm))
		m_pLegionArm_Manager->Use_CurrentArm();

	else if (m_pInputCom->Consume(PlayerActionID::LegionArm_Ex))
		m_pLegionArm_Manager->Use_Upgrade_CurrentArm();

	if (m_pLegionArm_Manager)
		m_pLegionArm_Manager->Update(fTimeDelta);


#pragma endregion

	if (m_pSpringArm && m_pStatusCom && m_pStatusCom->CanMoveCam())
	{
		_long lVerticalMoveDelta = m_pGameInstance->Get_DIMouseMove(DIMM::Y);
		_long lHorizontalMoveDelta = m_pGameInstance->Get_DIMouseMove(DIMM::X);

		m_pSpringArm->Update(lVerticalMoveDelta, lHorizontalMoveDelta, fTimeDelta);
	}

	/* 이동 처리 */
	if (m_pMovementCom && m_pCCT && m_eGrabState == GRAB_STATE::END)
	{
		if (m_pAnimator)
		{
			const ANIM_CLIP_SETTING* pCurClip = m_pAnimator->Get_CurClip();
			const _bool bHasRootMotion = (pCurClip != nullptr) && pCurClip->bUseRootMotion;

			const ANIM_ROOT_MOTION_DELTA& RootDelta = m_pAnimator->Get_RootMotionDelta();

			TransformData tRootDelta = {};
			tRootDelta.vScale = _float4(1.f, 1.f, 1.f, 0.f);
			tRootDelta.vRot = RootDelta.vRotation;
			tRootDelta.vTranslation = _float4(RootDelta.vTranslation.x, RootDelta.vTranslation.y, RootDelta.vTranslation.z, 0.f);

			m_pMovementCom->Update_RootMotion(bHasRootMotion, tRootDelta);
		}

		//입력 + 루트모션에 의한 이동/회전 계산
		CMovement::MOVEMENT_RESULT moveResult = m_pMovementCom->Compute_Movement(fTimeDelta);

		if (moveResult.bChangedQuat)
			m_pTransformCom->Set_Quat(XMLoadFloat4(&moveResult.vMoveQuat));

		_vector vFinalMove = XMLoadFloat3(&moveResult.vMoveDelta);

		//CCT. 물리연산 시뮬레이션. 최종 위치 값 결정 
		CCTResult CCTResult = m_pCCT->Move(vFinalMove, fTimeDelta);

		m_pStatusCom->Set_Grounded(CCTResult.m_bGrounded);
		m_pMovementCom->Set_SideBlocked(CCTResult.m_bSideHit);
		m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat4(&CCTResult.vSimulatedPosition));


	}


	/* Interact */
	if (m_pInputCom->Consume(PlayerActionID::Interact))
	{
		Interact_Triggers();
	}

	if (m_pEquipmentCom)
		m_pEquipmentCom->Update(fTimeDelta);

	if (m_pAnimDriver)
	{
		/* Effect */
		for (const PLAYER_FX_EVENT& fx : m_pAnimDriver->Get_FxEvents())
		{
			Drive_Effect(fx);
		}
		/*Legion Arm Event*/
		if (m_pLegionArm_Manager)
		{
			for (LegionArmEvent eEvent : m_pAnimDriver->Get_LegionArmEvents())
			{
				m_pLegionArm_Manager->On_AnimEvent(eEvent);
			}
		}
		/*Anim Event*/
		for (const PlayerAnimEvent& animEvent : m_pAnimDriver->Get_AnimEvents())
		{
			switch (animEvent)
			{
				case PlayerAnimEvent::Heal:
				{
					if (m_pStatCom->Consume_Potion())
					{
						m_pStatCom->Heal();

						_float3 vPos = {};
						XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));
						m_pGameInstance->Play_Cue(SOUND_HEAL,CHANNEL_ID::SOUND_PLAYER, vPos);
						
						const _float4x4* pMatPos = m_pTransformCom->Get_WorldMatrixPtr();
						m_pGameInstance->Play_Effect("Effect_Player_Heal", m_pMasterModelCom->Get_Bone_CombinedMatPtr(STR_HEAL_BONE_BODY), pMatPos);
						m_pGameInstance->Play_Effect("Effect_Player_Heal_Hand", m_pMasterModelCom->Get_Bone_CombinedMatPtr(STR_HEAL_BONE), pMatPos);
					}
					break;
				}
				case PlayerAnimEvent::Turn_On_Lamp:
				{
					if (m_pLamp)
					{
						m_pLamp->Set_LightEnable(true);
					}
					break;
				}
				case PlayerAnimEvent::Turn_Off_Lamp:
				{
					if (m_pLamp)
					{
						m_pLamp->Set_LightEnable(false);
					}
					break;
				}
				default:
					break;
			}
		}
	}

	if (m_bGrinding)
		m_pEquipmentCom->Regen_Durability(GRINDER_AMOUNT_PER_SEC, fTimeDelta);

	if (m_pMovementCom->Is_Sprinting() && !m_pStatCom->Is_StaminaEmpty())
		m_pStatCom->Drain_Stamina(SPRINT_STAMINA_COST, fTimeDelta);

	if (m_pStatCom) m_pStatCom->Update(fTimeDelta);
}

void CPlayer::Physical_Update(_float fTimeDelta)
{
	fTimeDelta *= m_fGlobalTimeScale_Offset;
	if (m_pLamp) m_pLamp->Physical_Update(fTimeDelta);
}

void CPlayer::Late_Update(_float fTimeDelta)
{
	fTimeDelta *= m_fGlobalTimeScale_Offset;

	if (m_pAnimator)
	{
		const POSE* pFinalPose = &m_pAnimator->Get_CurPose();
		_bool bAimPose = m_pAimCom->Update_AimPose(fTimeDelta);

		// 컷신 중에는 조준 자세가 컷신 애니메이션 Pose를 변형하지 않게함
		_bool bCutSceneControlled = m_pAnimDriver->Is_CutSceneControlled();
		if (bAimPose && bCutSceneControlled == false)
		{
			m_AimPose = *pFinalPose;
			m_pAimCom->Apply_AimPose(m_AimPose);
			pFinalPose = &m_AimPose;
		}
		m_pModelCom->Update_Bones(pFinalPose);
	}
	else
	{
		m_pModelCom->Update_Bones();
	}

	m_pLegionArm_Manager->Post_BoneUpdate(fTimeDelta);

	if (m_eGrabState != GRAB_STATE::END)
		LateUpdate_Grab();

	//입력된 Action 플래그는 InputCom::Consume_InputBuffer 으로 받기 
	m_pTargetCom->Update(fTimeDelta);
	m_pSpringArm->Late_Update(fTimeDelta);
	m_pEquipmentCom->Late_Update(fTimeDelta);

	m_pGameInstance->Add_RenderGroup(RENDERID::SHADOW, this);
	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
	m_pGameInstance->Add_RenderGroup(RENDERID::FORWARD, this);

	if (m_pLamp) m_pLamp->Late_Update(fTimeDelta);

}

HRESULT CPlayer::Render()
{
	if (m_pPartSet)
		m_pPartSet->Render();

	if (m_pLegionArm_Manager)
		m_pLegionArm_Manager->Render();

	return S_OK;
}

HRESULT CPlayer::Render_Shadow()
{
	if (m_pPartSet)
		m_pPartSet->Render_Shadow();

	if (m_pLegionArm_Manager)
		m_pLegionArm_Manager->Render_Shadow();

	return S_OK;
}


void CPlayer::Respawn(const Respawn_Point& Point)
{
	Reset_ForChangeMap(Point);
	m_pStatCom->Clear();

	m_bRespawnRequested = false;
	m_bDeadFlag = false;
}

void CPlayer::Reset_ForChangeMap(const Respawn_Point& Point)
{
	Cancel_Grab();
	
	Set_GlobalTimeScale_Offset(1.f);

	if (m_pLegionArm_Manager)
		m_pLegionArm_Manager->Cancel_CurrentArm();

	//남은 조준 블렌드까지 정리 
	m_pAimCom->Clear();

	if (Is_LadderActive())
		Release_LadderMode();

	Teleport(Point);
	m_TriggerObjects.clear();
	m_pStatusCom->Clear();
	m_pMovementCom->Clear();
	m_pInputCom->Clear();
	m_pSpringArm->Clear();
	m_pGameInstance->Clear_Camera();

	m_pTargetCom->Clear_Target();
	m_pAnimDriver->Clear();

	m_bTeleportPending = false;
	m_bTeleportAnimStarted = false;
	m_fGlobalTimeScale_Offset = 1.f;

	//죽은뒤 맵 전환이 되었다면 다시 살림 
	if (m_bDeadFlag)
	{
		m_bDeadFlag = false;
		m_bRespawnRequested = false;
		m_pStatCom->Clear();

		if (m_pDissolveCom)
		{
			m_pDissolveCom->Set_Timing(0.3f, 1.2f, 0.f);
			m_pDissolveCom->Begin(CDissolve::DISSOLVE_MODE::DISSOLVE_IN);
			Apply_Dissolve(1.f);
			m_bMaterializing = true;
			m_bDissolveFx = false;
			
			m_pStatusCom->Set_InputLock(true);
			m_pStatusCom->Set_MovementLock(true);
			m_pStatusCom->Set_RotationLock(true);
			m_pStatusCom->Set_Invincible(true);
		}
	}
	else if (m_pDissolveCom)
	{
		m_pDissolveCom->Set_Timing(0.3f, 1.2f, 0.f);
		m_pDissolveCom->Begin(CDissolve::DISSOLVE_MODE::DISSOLVE_IN);
		Apply_Dissolve(1.f);
		m_bMaterializing = true;
		m_bDissolveFx = false;
	}
}

void CPlayer::Teleport(const Respawn_Point& Point)
{
	_vector vSpawnPosition = XMLoadFloat4(&Point.vPosition);
	m_pCCT->Set_FootPosition(vSpawnPosition);
	m_pTransformCom->Set_State(STATE::POSITION, vSpawnPosition);

	_vector vQuat = XMQuaternionRotationRollPitchYaw(0.f, Point.fYaw, 0.f);
	m_pTransformCom->Set_Quat(vQuat);
}

void CPlayer::Apply_CutSceneControl(const CUTSCENE_TRANSFORM_VALUE& Transform, const CUTSCENE_ANIMATION_SECTION* pAnimationSection, _float fCutSceneTime)
{
	if (nullptr == m_pAnimDriver || nullptr == m_pMovementCom)
		return;

	const _bool bBeginControl = false == m_pAnimDriver->Is_CutSceneControlled();

	if (bBeginControl)
	{
		// 컷신 진입 전 플레이어가 수행하던 특수 상태와 예약 입력을 정리한다.
		Cancel_Grab();

		if (nullptr != m_pLegionArm_Manager)
			m_pLegionArm_Manager->Cancel_CurrentArm();

		if (Is_LadderActive())
			Release_LadderMode();

		if (nullptr != m_pTargetCom)
			m_pTargetCom->Clear_Target();

		if (nullptr != m_pStatusCom)
		{
			m_pStatusCom->Set_LockedTarget(nullptr);
		}

		if (nullptr != m_pInputCom)
			m_pInputCom->Clear();

		m_bWantLockOn = false;
		m_bTeleportPending = false;
		m_bTeleportAnimStarted = false;
		m_pMovementCom->Clear();
	}

	Respawn_Point CutScenePoint = {};
	CutScenePoint.vPosition = _float4(Transform.vPosition.x, Transform.vPosition.y, Transform.vPosition.z, 1.f);
	CutScenePoint.fYaw = XMConvertToRadians(Transform.vRotationDegree.y);

	// Transform과 CCT 발 위치를 같은 프레임에 이동한다.
	Teleport(CutScenePoint);
	m_pAnimDriver->Apply_CutSceneAnimation(pAnimationSection, fCutSceneTime);
}

void CPlayer::Clear_CutSceneControl()
{
	if (nullptr == m_pAnimDriver || false == m_pAnimDriver->Is_CutSceneControlled())
		return;

	if (nullptr != m_pMovementCom)
		m_pMovementCom->Clear();

	if (nullptr != m_pInputCom)
		m_pInputCom->Clear();

	m_pAnimDriver->Clear_CutSceneAnimation();
}


void CPlayer::Apply_ExternalDisplacement(_fvector vDisplacement)
{
	if (nullptr == m_pCCT || nullptr == m_pTransformCom)
		return;


	_vector vExternalDisplacement = XMVectorSetW(vDisplacement, 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vExternalDisplacement)) <= FLT_EPSILON)
		return;

	_vector vCurrentFootPosition = m_pCCT->Get_FootPosition();
	_vector vCurrentTransformPosition = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vNextFootPosition = XMVectorSetW(vCurrentFootPosition + vExternalDisplacement, 1.f);

	m_pCCT->Set_FootPosition(vNextFootPosition);
	m_pTransformCom->Set_State(STATE::POSITION, vNextFootPosition);
}

void CPlayer::Begin_Overlap(CGameObject* pTrigger)
{
	if (pTrigger == nullptr) return;

	//여기는 Overlap 1프레임만이고 대상이 CMapGameplayObject이 아닐수있으므로 dynamic 유지하기 
	CMapGameplayObject* pMapObject = dynamic_cast<CMapGameplayObject*>(pTrigger);
	if (pMapObject == nullptr) return;

	m_TriggerObjects.push_back(pMapObject);

}

void CPlayer::End_Overlap(CGameObject* pTrigger)
{
	if (pTrigger == nullptr) return;

	CMapGameplayObject* pMapObject = dynamic_cast<CMapGameplayObject*>(pTrigger);
	if (pMapObject == nullptr) return;

	auto iter = find(m_TriggerObjects.begin(), m_TriggerObjects.end(), pMapObject);
	if (iter == m_TriggerObjects.end()) return;

	m_TriggerObjects.erase(iter);
}


void CPlayer::On_Attack(_float fDamage)
{
	if (fDamage <= 0.f) return;
	m_pStatCom->Charge_Potion(fDamage);
}

_bool CPlayer::Is_DonkeyMask() const
{
	return m_pInventory->Get_EquippedPartID(PLAYER_PART::HELMET) == ItemID::HELMET_MADDONKEY;
}

_bool CPlayer::Take_Damage(const HitInfo& hitInfo)
{
	// 컷신 배우로 제어되는 동안 게임 피격이 컷신 Pose를 변경하지 않게 한다.
	if (nullptr != m_pAnimDriver && m_pAnimDriver->Is_CutSceneControlled())
		return false;

	if (m_eGrabState != GRAB_STATE::END) return false;
	if (Is_LadderActive()) return false;

	//SuporArmor를 Invincible로 전환했으나 혹시 남아았는 윈도우가 있을경우를 대비 
	if (m_pStatusCom->Is_Invincible() || m_pStatusCom->Is_SuperArmor()) return false;
	if (m_pStatCom->Is_Dead()) return false;


	_float  fForwardDot = 0.f;
	HIT_DIR hitDir = Compute_HitDir(hitInfo, fForwardDot);
	_bool bFrontHit = fForwardDot > GUARD_FRONT_DOT;

	if (Is_Defenseless(bFrontHit))
	{
		React_NoGuard(hitInfo, hitDir);
		return true;
	}

	if (!Can_Guard(hitInfo))
	{
		React_Unguarded(hitInfo, hitDir);
		return true;
	}

	if (m_pStatCom->Is_StaminaEmpty())
	{
		React_NoStaminaGuard(hitInfo, hitDir);
		return true;
	}

	React_Guard(hitInfo, hitDir);
	return true;
}

void CPlayer::Begin_Contact(CGameObject* pOther, _fvector vNormal)
{
	//if (m_fGrabPhaseTime < 0.05f) return;
	if (m_eGrabState != GRAB_STATE::THROW) return;

	//바닥과 유사한 노말이 들어온경우에만 착지 인정 
	if (XMVectorGetY(vNormal) > GROUND_NORMAL_Y)
	{
		m_bThrowImpact = true;
	}
}

_bool CPlayer::Is_Dead() const
{
	if (m_pStatCom)
		return m_pStatCom->Is_Dead();
	return false;
}

_bool CPlayer::Can_Interact()
{
	if (m_pStatCom->Is_Dead()) return false;
	if (!m_pAnimDriver->Is_Locomotion() || !m_pStatusCom->CanMove() || Is_LadderActive() || !m_pStatusCom->CanInput())
		return false;

	return true;
}

void CPlayer::Play_Stargazer_Active()
{
	m_pAnimDriver->Play_Action(AnimAction::Stargazer_Active);
}

void CPlayer::Play_Stargazer_InActive()
{
	m_pAnimDriver->Play_Action(AnimAction::Stargazer_InActive);
}

// Effect : 메쉬 출력 관련 테스트 함수
CEffect* CPlayer::Play_BodySurfaceEffect(const string& strEffectName)
{
	if (!m_pInventory) return nullptr;

	vector<CModel*> Parts;
	Parts.reserve(ETOUI(PLAYER_PART::END));
	
	for (_uint i = 0; i < ETOUI(PLAYER_PART::END); ++i)
	{
		CBodyPart* pPart = m_pInventory->Get_EquippedPart(static_cast<PLAYER_PART>(i));
		if (pPart == nullptr)
			continue;
		CModel* pModel = pPart->Get_Model();
		if (pModel != nullptr)
			Parts.push_back(pModel);
	}

	if (Parts.empty())
		return nullptr;

	const _float4x4* pOwnerWorld = m_pTransformCom->Get_WorldMatrixPtr();

	return m_pGameInstance->Play_Effect_Parts(strEffectName, Parts, pOwnerWorld);
}


void CPlayer::Start_Grab(CAIObject* pGrabber)
{
	if (pGrabber == nullptr || m_pGrabSocketBone == nullptr || m_pThrowBody == nullptr) return;
	if (m_eGrabState != GRAB_STATE::END || m_pStatCom->Is_Dead()) return;

	m_eGrabState = GRAB_STATE::GRAB;
	m_fGrabPhaseTime = 0.f;
	m_pGrabber = pGrabber;

	_vector vToGrabber = XMVectorSetY(pGrabber->Get_Transform()->Get_State(STATE::POSITION) - m_pTransformCom->Get_State(STATE::POSITION), 0.f);
	_float fDot = XMVectorGetX(XMVector3Dot(m_pTransformCom->Get_State(STATE::LOOK), vToGrabber));

	_bool bFrontGrab = fDot >= 0.f;

	m_pCCT->Set_Active(false);
	m_pMovementCom->Clear();

	m_pAnimDriver->Set_Param(PARAM_GRABFRONT, bFrontGrab);
	m_pAnimDriver->Set_Param(PARAM_GRABHOLD, true);
	m_pAnimDriver->Play_Action(AnimAction::Raxasia_Grab);
}

void CPlayer::Cancel_Grab()
{
	if (m_eGrabState == GRAB_STATE::END) return;

	RemoveActor_FromScene();

	m_pCCT->Set_FootPosition(m_pTransformCom->Get_State(STATE::POSITION));
	m_pCCT->Set_Active(true);
	m_pCCT->InvalidateCache();

	_vector vLook = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vLook)), 0.f))
		vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);

	vLook = XMVector3Normalize(vLook);
	_float fYaw = atan2f(XMVectorGetX(vLook), XMVectorGetZ(vLook));
	m_pTransformCom->Set_Quat(XMQuaternionRotationRollPitchYaw(0.f, fYaw, 0.f));

	m_pMovementCom->Clear();
	m_pAnimDriver->Set_Param(PARAM_GRABHOLD, false);
	m_pAnimDriver->Set_Param(PARAM_THROW, false);

	m_eGrabState = GRAB_STATE::END;
	m_fGrabPhaseTime = 0.f;
	m_bThrowImpact = false;
	m_pGrabber = nullptr;
}

//던지면 RigidBody 활성화 
void CPlayer::End_Grab()
{
	if (m_eGrabState != GRAB_STATE::GRAB) return;
	//ZoomIn Scene 종료
	m_pGameInstance->Stop_Scene();

	_vector vThrowDir = {};
	_matrix SocketWorld;
	if (m_pGrabber && m_pGrabber->Get_GrabSocketWorld(SocketWorld))
		vThrowDir = SocketWorld.r[1];
	else
		vThrowDir = XMVectorNegate(m_pTransformCom->Get_State(STATE::LOOK));

	vThrowDir = XMVectorSetY(vThrowDir, 0.f);

	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vThrowDir)), 0.f))
		vThrowDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);

	vThrowDir = XMVector3Normalize(vThrowDir);

	//발사 원점 
	_matrix BoneWorld = XMLoadFloat4x4(m_pGrabSocketBone) * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
	_vector vCenter = XMVectorSetW(BoneWorld.r[3], 1.f);

	//최저 높이 제한 
	if (m_pGrabber)
	{
		const _float fMinY = XMVectorGetY(m_pGrabber->Get_Transform()->Get_State(STATE::POSITION)) + THROW_BODY_RADIUS + 0.1f;
		if (XMVectorGetY(vCenter) < fMinY)
			vCenter = XMVectorSetY(vCenter, fMinY);
	}

	if (FAILED(Throw(vCenter, vThrowDir, THROW_SPEED, THROW_UP_SPEED)))
	{
		Cancel_Grab();
		return;
	}

	m_pGrabber = nullptr;
	m_pAnimDriver->Set_Param(PARAM_GRABHOLD, false);
	m_pAnimDriver->Set_Param(PARAM_THROW, true);
}

_float CPlayer::Get_CCTRadius() const
{
	if (!m_pCCT)	return 0.f;
	return m_pCCT->Get_Radius();
}

void CPlayer::Launch_Explosion(_float fDamage, _fvector vExplosionPos, _float fPower, _float fUpPower)
{
	if (m_eGrabState != GRAB_STATE::END) return;
	if (m_pStatCom->Is_Dead() || Is_LadderActive() || m_bTeleportPending) return;
	if (m_pThrowBody == nullptr) return;

	if (m_pLegionArm_Manager)
		m_pLegionArm_Manager->Cancel_CurrentArm();

	_vector vDir = XMVectorSetY(m_pTransformCom->Get_State(STATE::POSITION) - vExplosionPos, 0.f);
	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vDir)), 0.f))
		vDir = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);

	vDir = XMVector3Normalize(vDir);

	_vector vCenter = m_pTransformCom->Get_State(STATE::POSITION) + XMVectorSet(0.f, THROW_BODY_RADIUS + 0.2f, 0.f, 0.f);

	if (FAILED(Throw(vCenter, vDir, fPower, fUpPower)))
	{
		return;
	}

	m_pAnimDriver->Set_Param(PARAM_THROW, true);
	m_pAnimDriver->Play_Action(AnimAction::Blast_Launch);
}

void CPlayer::Update_Dead(_float fTimeDelta)
{
	Cancel_Grab();

	if (m_pLegionArm_Manager)
		m_pLegionArm_Manager->Cancel_CurrentArm();

	if (Is_LadderActive())
		Release_LadderMode();

	if (m_bDeadFlag == false)
	{
		m_bDeadFlag = true;
		m_bRespawnRequested = false;

		m_pStatusCom->Set_Invincible(true);
		m_pStatusCom->Set_InputLock(true);
		m_pStatusCom->Set_LockedTarget(nullptr);
		m_pStatusCom->Set_MovementLock(true);
		m_pStatusCom->Set_RotationLock(true);

		m_bTeleportPending = false;
		m_bTeleportAnimStarted = false;

		m_pAnimDriver->Play_Action(AnimAction::Dead);
		m_pAnimDriver->Update(0.f);
		if (m_pDissolveCom)
		{
			m_pDissolveCom->Set_Timing(DEAD_DELAY, 1.5f, 0.3f);
			m_pDissolveCom->Begin(CDissolve::DISSOLVE_MODE::DISSOLVE_OUT);
			m_bDissolveFx = false;
		}

		return;
	}

	if (m_pAnimator)
		m_pAnimator->Update(fTimeDelta);

	if (m_bRespawnRequested) return;

	if (m_pDissolveCom)
	{
		m_pDissolveCom->Update(fTimeDelta);
		Apply_Dissolve(m_pDissolveCom->Get_Amount());
		Play_DissolveStartFx("Effect_Player_Dead");

		if (m_pDissolveCom->Is_Finished())
		{
			m_pGameInstance->Broadcast(EVENT_TYPE_UI::PLAYER_DEAD_OPEN_REQUEST, nullptr);
			m_bRespawnRequested = true;
		}
	}
}

void CPlayer::Update_Materialize(_float fTimeDelta)
{
	if (m_pAnimator)
		m_pAnimator->Update(fTimeDelta);

	if (m_pDissolveCom == nullptr)
	{
		m_bMaterializing = false;
		return;
	}

	m_pDissolveCom->Update(fTimeDelta);
	Apply_Dissolve(m_pDissolveCom->Get_Amount());
	Play_DissolveStartFx("Effect_Player_Respawn");

	if (m_pDissolveCom->Is_Finished())
	{
		m_bMaterializing = false;
		Apply_Dissolve(0.f);
		m_pDissolveCom->Reset();

		m_pStatusCom->Set_InputLock(false);
		m_pStatusCom->Set_MovementLock(false);
		m_pStatusCom->Set_RotationLock(false);
		m_pStatusCom->Set_Invincible(false);
	}
}

void CPlayer::Play_DissolveStartFx(const string& strFxName)
{
	if (m_bDissolveFx || m_pDissolveCom == nullptr)
		return;
	if (m_pDissolveCom->Get_Phase() != CDissolve::DISSOLVE_PHASE::RUN)
		return;

	m_pGameInstance->Play_Effect_Model(strFxName, m_pMasterModelCom, m_pTransformCom->Get_WorldMatrixPtr());

	m_bDissolveFx = true;
}

void CPlayer::Apply_Dissolve(_float fAmount)
{
	if (m_pDissolveCom == nullptr)
		return;

	// 플레이어는 여러 파츠(CBodyPart)로 렌더되므로 각 파츠에 진행도/색을 반영
	const _float4& vColor = m_pDissolveCom->Get_Color();

	if (m_pPartSet)
		m_pPartSet->Set_Dissolve(fAmount, vColor);
	if (m_pEquipmentCom)
		m_pEquipmentCom->Set_Dissolve(fAmount, vColor);
	if (m_pLegionArm_Manager)
		m_pLegionArm_Manager->Set_Dissolve(fAmount, vColor);
	if (m_pLamp)
		m_pLamp->Set_Dissolve(fAmount, vColor);
}

void CPlayer::Begin_LadderClimb(const _float3& vBottomLanding, const _float3& vTopLanding, const _float3& vTopPosition, const _float4x4& TopWorldMatrix)
{
	if (m_pStatCom->Is_Dead() || Is_LadderActive() || m_eGrabState != GRAB_STATE::END)
		return;

	if (!m_pStatusCom->CanMove() || !m_pStatusCom->Is_Grounded())
		return;

	m_pAnimator->Set_RootYAnchored(true);

	m_vLadder_BottomLandingPos = vBottomLanding;
	m_vLadder_TopLandingPos = vTopLanding;

	//플레이어 정렬 방향
	_matrix LadderWorld = XMLoadFloat4x4(&TopWorldMatrix);
	_vector vFace = XMVector3Normalize(XMVectorSetY(LadderWorld.r[2], 0.f));

	_float fYaw = atan2f(XMVectorGetX(vFace), XMVectorGetZ(vFace));
	_vector vFaceQuat = XMQuaternionRotationRollPitchYaw(0.f, fYaw, 0.f);
	XMStoreFloat4(&m_vLadder_FaceQuat, vFaceQuat);

	//등반 축 
	_vector vAttach = XMLoadFloat3(&vTopPosition) - vFace * LADDER_ATTACH_OFFSET;
	XMStoreFloat3(&m_vLadder_AttachPos, vAttach);

	//Bottom / Top 판단 
	_float fPlayerY = XMVectorGetY(m_pTransformCom->Get_State(STATE::POSITION));
	m_bLadder_EnterFromBottom = fabsf(fPlayerY - m_vLadder_BottomLandingPos.y) <= fabsf(fPlayerY - m_vLadder_TopLandingPos.y);

	//시작 보간 시작점 
	XMStoreFloat3(&m_vLadder_EnterStart, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&m_vLadder_EnterStartQuat, m_pTransformCom->Get_Quat());

	//등반축 - 루트모션 XZ값 고려하여 이동시킴 
	if (m_bLadder_EnterFromBottom)
	{
		_vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
		vPos = XMVectorSetY(vPos, XMVectorGetY(vPos) - LADDER_MARKER_SURFACE_OFFSET * 0.5f);
		m_pTransformCom->Set_State(STATE::POSITION, vPos);
		m_fLadder_GroundY = fPlayerY;
	}
	else // Top 시작 애니메이션이 180도회전되어 반대로나와서 회전시켜야함. 
	{
		m_fLadder_GroundY = m_vLadder_BottomLandingPos.y + LADDER_MARKER_SURFACE_OFFSET;

		_vector vFaceFilpped = XMQuaternionMultiply(vFaceQuat, XMQuaternionRotationRollPitchYaw(0.f, XM_PI, 0.f));
		_vector vStart = vAttach + vFace * LADDER_START_DOWN_BACK;
		vStart = XMVectorSetY(vStart, fPlayerY);

		m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vStart, 1.f));
		m_pTransformCom->Set_Quat(vFaceFilpped);

		XMStoreFloat3(&m_vLadder_EnterStart, vStart);
	}

	m_pStatusCom->Set_GravityDisabled(true);
	m_pStatusCom->Set_JumpLock(true);

	m_pStatusCom->Set_LockedTarget(nullptr);
	m_bWantLockOn = false;

	m_pAnimDriver->Set_LadderMode(true);
	m_pCCT->Set_Active(false);
	m_bLadder_FootLeft = true;
	m_pEquipmentCom->Set_Visibility(false);

	//Ladder_StartUp
	Play_LadderAction(m_bLadder_EnterFromBottom ? AnimAction::Ladder_StartDown : AnimAction::Ladder_StartUp);
	m_eLadderState = LADDER_CLIMB_STATE::ENTER;
}

void CPlayer::Update_Ladder(_float fTimeDelta)
{
	if (m_pSpringArm && m_pStatusCom && m_pStatusCom->CanMoveCam())
	{
		_long lVerticalMoveDelta = m_pGameInstance->Get_DIMouseMove(DIMM::Y);
		_long lHorizontalMoveDelta = m_pGameInstance->Get_DIMouseMove(DIMM::X);

		m_pSpringArm->Update(lVerticalMoveDelta, lHorizontalMoveDelta, fTimeDelta);
	}

	if (m_pAnimDriver)
		m_pAnimDriver->Update(fTimeDelta);

	if (m_pEquipmentCom)
		m_pEquipmentCom->Update(fTimeDelta);

	if (m_pStatCom)
		m_pStatCom->Update(fTimeDelta);

	//요청을 넣었고 아직 수락 판단 전이면 패스 
	if (m_bLadder_ActionPending)
	{
		if (false == m_pAnimator->Was_ActionAcceptedThisFrame())
			return;

		m_bLadder_ActionPending = false;
		//Top진입 끝나면 회전 돌리기 
		if (m_bLadder_QuatFixPending)
		{
			m_bLadder_QuatFixPending = false;
			m_pTransformCom->Set_Quat(XMLoadFloat4(&m_vLadder_FaceQuat));
		}
	}

	const ANIM_ROOT_MOTION_DELTA& Root = m_pAnimator->Get_RootMotionDelta();

	_vector vQuat = XMLoadFloat4(&m_vLadder_FaceQuat);
	_vector vLocalRootDelta = XMVectorSet(Root.vTranslation.x, Root.vTranslation.y, Root.vTranslation.z, 0.f);
	_vector vWorldRootDelta = XMVector3Rotate(vLocalRootDelta, vQuat);
	_vector vWorldRootDeltaXZ = XMVectorSetY(vWorldRootDelta, 0.f);

	_vector vCurrentPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vNextPos = vCurrentPos;

	switch (m_eLadderState)
	{
	case LADDER_CLIMB_STATE::ENTER:
	{
		_float fDuration = m_pAnimator->Get_CurClipDuration();
		_float fProgress = (fDuration > 0.f) ? min(m_pAnimator->Get_LocalTime() / fDuration, 1.f) : 1.f;

		_float fEaseXZ = Ease(EASE_TYPE::OUT_QUINT, fProgress);
		_float fEaseY = Ease(EASE_TYPE::OUT_QUINT, fProgress);
		_vector vEnterStart = XMLoadFloat3(&m_vLadder_EnterStart);
		_vector vAttachPos = XMLoadFloat3(&m_vLadder_AttachPos);

		// Y 축
		_float fY = XMVectorGetY(vCurrentPos) + XMVectorGetY(vWorldRootDelta);
		_float fX = XMVectorGetX(XMVectorLerp(vEnterStart, vAttachPos, fEaseXZ));
		_float fZ = XMVectorGetZ(XMVectorLerp(vEnterStart, vAttachPos, fEaseXZ));

		vNextPos = XMVectorSet(fX, fY, fZ, 1.f);
		//vNextPos = vCurrentPos + vWorldRootDelta;

		if (m_bLadder_EnterFromBottom)
		{
			_vector vStartQuat = XMLoadFloat4(&m_vLadder_EnterStartQuat);
			_vector vFaceQuat = XMLoadFloat4(&m_vLadder_FaceQuat);

			m_pTransformCom->Set_Quat(XMQuaternionSlerp(vStartQuat, vFaceQuat, fEaseY));
		}
		else // 반대로 회전 
		{
			_vector vFaceFlipped = XMQuaternionMultiply(XMLoadFloat4(&m_vLadder_FaceQuat), XMQuaternionRotationRollPitchYaw(0.f, XM_PI, 0.f));
			m_pTransformCom->Set_Quat(vFaceFlipped);

		}

		if (m_pAnimator->Is_Ended())
		{
			m_fLadder_LastY = fY;
			vNextPos = XMVectorSetW(XMVectorSetY(vAttachPos, fY), 1.f);
			if (m_bLadder_EnterFromBottom)
				m_pTransformCom->Set_Quat(XMLoadFloat4(&m_vLadder_FaceQuat));
			else
				m_bLadder_QuatFixPending = true;
			Enter_LadderIdle();
		}
		break;
	}
	case LADDER_CLIMB_STATE::IDLE:
	{
		_int iInputDirY = m_pInputCom->Get_RawAxisY();
		if (iInputDirY != 0)
		{
			Try_LadderStep(iInputDirY);
		}
		break;
	}
	case LADDER_CLIMB_STATE::STEP_UP:
	case LADDER_CLIMB_STATE::STEP_DOWN:
	{
		const _float fMoveDir = (LADDER_CLIMB_STATE::STEP_UP == m_eLadderState) ? 1.f : -1.f;

		_float fDuration = m_pAnimator->Get_CurClipDuration();
		_float fProgress = (fDuration > 0.f) ? min(m_pAnimator->Get_LocalTime() / fDuration, 1.f) : 1.f;

		_float fNextY = m_fLadder_StepStartY + fMoveDir * LADDER_STEP_HEIGHT * fProgress;
		vNextPos = XMVectorSetW(XMVectorSetY(XMLoadFloat3(&m_vLadder_AttachPos), fNextY), 1.f);

		if (m_pAnimator->Is_Ended())
		{
			//애니메이션 끝나면 오차 보정
			m_fLadder_LastY = m_fLadder_StepStartY + fMoveDir * LADDER_STEP_HEIGHT;
			vNextPos = XMVectorSetY(vNextPos, m_fLadder_LastY);

			m_bLadder_FootLeft = !m_bLadder_FootLeft;

			_int iInputDirY = m_pInputCom->Get_RawAxisY();
			if (iInputDirY != 0)
			{
				Try_LadderStep(iInputDirY);
			}
			else Enter_LadderIdle();

		}
		break;
	}
	case LADDER_CLIMB_STATE::EXIT_TOP:
	case LADDER_CLIMB_STATE::EXIT_BOTTOM:
	{
		_vector vFaceDir = XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.f, 0.f), vQuat);

		//루트모션 XZ / Y 이동량 누적
		m_fLadder_ExitAccumXZ += fabsf(XMVectorGetX(XMVector3Dot(vWorldRootDeltaXZ, vFaceDir)));
		m_fLadder_ExitAccumY += fabsf(XMVectorGetY(vWorldRootDelta));

		_bool bUp = (LADDER_CLIMB_STATE::EXIT_TOP == m_eLadderState);

		//실제 목표이동량 총량 
		_float fTottalFwd = bUp ? LADDER_END_UP_FORWARD : LADDER_END_DOWN_BACK;
		_float fTotalY = bUp ? LADDER_END_UP_DELTA_Y : LADDER_END_DOWN_DELTA_Y;

		_float fXZRatio = (fTottalFwd > 0.f) ? min(m_fLadder_ExitAccumXZ / fTottalFwd, 1.f) : 1.f;
		_float fYRatio = (fTotalY > 0.f) ? min(m_fLadder_ExitAccumY / fTotalY, 1.f) : 1.f;

		_vector vStartPos = XMLoadFloat3(&m_vLadder_ExitStart);
		_vector vEndPos = XMLoadFloat3(&m_vLadder_ExitTarget);

		//축별로 따로 보간. 그래야 루트모션 커브에 맞게 보간됨 
		_float fX = XMVectorGetX(XMVectorLerp(vStartPos, vEndPos, fXZRatio));
		_float fZ = XMVectorGetZ(XMVectorLerp(vStartPos, vEndPos, fXZRatio));
		_float fY = XMVectorGetY(XMVectorLerp(vStartPos, vEndPos, fYRatio));

		//vNextPos = XMVectorSetW(XMVectorLerp(vStartPos, vEndPos, fProgress), 1.f);
		vNextPos = XMVectorSet(fX, fY, fZ, 1.f);

		if (m_pAnimator->Is_Ended())
		{
			m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vEndPos, 1.f));
			End_LadderClimb();
			return;
		}
		break;
	}
	default:
		return;
	}

	m_pTransformCom->Set_State(STATE::POSITION, vNextPos);
}

void CPlayer::Enter_LadderIdle()
{
	Play_LadderAction(Get_LadderIdleAction());
	m_eLadderState = LADDER_CLIMB_STATE::IDLE;
}

void CPlayer::Release_LadderMode()
{
	m_pAnimator->Set_RootYAnchored(false);

	_vector vPos = m_pTransformCom->Get_State(STATE::POSITION);

	m_pCCT->Set_Active(true);
	m_pCCT->Set_FootPosition(vPos);
	m_pCCT->InvalidateCache();

	m_pStatusCom->Set_GravityDisabled(false);
	m_pStatusCom->Set_JumpLock(false);
	m_pEquipmentCom->Set_Visibility(true);

	if (m_pSpringArm)
		m_pSpringArm->Set_PivotExtraY(0.f);

	if (m_pAnimDriver)
		m_pAnimDriver->Set_LadderMode(false);

	m_bLadder_ActionPending = false;
	m_bLadder_QuatFixPending = false;
	m_eLadderState = LADDER_CLIMB_STATE::END;
}

void CPlayer::Try_LadderStep(_int iDir)
{
	_vector vCur = m_pTransformCom->Get_State(STATE::POSITION);

	//UP
	if (iDir > 0)
	{
		_float fDeck = m_vLadder_TopLandingPos.y + LADDER_MARKER_SURFACE_OFFSET;

		//Top 마지막 영역 이내라면 전환
		if ((fDeck - m_fLadder_LastY <= LADDER_END_UP_TRIGGER))
		{
			m_fLadder_DeckY = fDeck;
			XMStoreFloat3(&m_vLadder_ExitStart, vCur);
			m_vLadder_ExitTarget = { m_vLadder_TopLandingPos.x, fDeck, m_vLadder_TopLandingPos.z };

			m_fLadder_ExitAccumXZ = 0.f;
			m_fLadder_ExitAccumY = 0.f;

			Play_LadderAction(Get_LadderEndAction(iDir));
			m_eLadderState = LADDER_CLIMB_STATE::EXIT_TOP;
			return;
		}

		Play_LadderAction(Get_LadderStepAction(iDir));
		m_fLadder_StepStartY = m_fLadder_LastY;
		m_eLadderState = LADDER_CLIMB_STATE::STEP_UP;
		return;
	}
	//DOWN
	else
	{
		if (m_fLadder_LastY - m_fLadder_GroundY <= LADDER_END_DOWN_TRIGGER)
		{
			m_fLadder_DeckY = m_fLadder_GroundY;
			XMStoreFloat3(&m_vLadder_ExitStart, vCur);
			m_vLadder_ExitTarget = { m_vLadder_BottomLandingPos.x, m_fLadder_GroundY, m_vLadder_BottomLandingPos.z };

			m_fLadder_ExitAccumXZ = 0.f;
			m_fLadder_ExitAccumY = 0.f;

			Play_LadderAction(Get_LadderEndAction(iDir));
			m_eLadderState = LADDER_CLIMB_STATE::EXIT_BOTTOM;
			return;
		}

		Play_LadderAction(Get_LadderStepAction(iDir));
		m_fLadder_StepStartY = m_fLadder_LastY;
		m_eLadderState = LADDER_CLIMB_STATE::STEP_DOWN;
	}
}

void CPlayer::End_LadderClimb()
{
	Release_LadderMode();
	m_pAnimDriver->Set_CurrentAction(AnimAction::Move);
}

void CPlayer::Play_LadderAction(AnimAction eAction)
{
	m_pAnimDriver->Play_LadderAction(eAction);
	m_bLadder_ActionPending = true;
}


AnimAction CPlayer::Get_LadderStepAction(_int iDir) const
{
	if (iDir > 0)
	{
		if (m_bLadder_FootLeft)
			return AnimAction::Ladder_UpLeft;
		else
			return AnimAction::Ladder_UpRight;
	}
	else
	{
		if (m_bLadder_FootLeft)
			return AnimAction::Ladder_DownLeft;
		else
			return AnimAction::Ladder_DownRight;
	}
}

AnimAction CPlayer::Get_LadderEndAction(_int iDir) const
{
	if (iDir > 0)
	{
		if (m_bLadder_FootLeft)
			return AnimAction::Ladder_EndUpLeft;
		else
			return AnimAction::Ladder_EndUpRight;
	}
	else
	{
		if (m_bLadder_FootLeft)
			return AnimAction::Ladder_EndDownLeft;
		else
			return AnimAction::Ladder_EndDownRight;
	}
}

AnimAction CPlayer::Get_LadderIdleAction() const
{
	///m_bLadder_FootLeft  : 다음에 내딛을 발임 그래서 IDLE은 반대
	if (m_bLadder_FootLeft)
		return AnimAction::Ladder_IdleRight;
	else
		return AnimAction::Ladder_IdleLeft;
}


void CPlayer::Update_UILock()
{
	_bool bUIing = m_pGameInstance->Has_UIInputOwner();
	if (bUIing != m_bUILock)
	{
		m_bUILock = bUIing;

		m_pInputCom->Clear();
		//UI종료
		if (!bUIing)
		{
			Set_Player_UILock(false);
			m_pAnimDriver->End_InteractLoop();
		}

	}

	if (m_bUILock)
		Set_Player_UILock(true);
}

void CPlayer::Set_Player_UILock(_bool bLock)
{
	m_pStatusCom->Set_MovementLock(bLock);
	m_pStatusCom->Set_RotationLock(bLock);
	m_pStatusCom->Set_Invincible(bLock);
	m_pStatusCom->Set_InputLock(bLock);
	m_pStatusCom->Set_CamMoveLock(bLock);
}

void CPlayer::Translation_Chapater()
{
	m_bTeleportPending = false;
	m_bTeleportAnimStarted = false;

	if (m_bRespawnRequested) return;

	ChangeChapter_EventDesc Desc = m_TeleportEventDesc;
	//텔레포트 전환 시작 
	m_pGameInstance->Broadcast(EVENT_TYPE::TELEPORT_START, &Desc);
}

HRESULT CPlayer::Ready_Components()
{
	/* Model */
	_matrix    PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(-90.f));
	m_pMasterModelCom = CMasterModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_AllParts", PreTransformMatrix);
	if (m_pMasterModelCom == nullptr) return E_FAIL;

	m_pModelCom = m_pMasterModelCom;

	/* Physics */
	CCT::CONTROLLER_DESC CCTDesc;
	CCTDesc.eControllerType = CCT_TYPE::CAPSULE; // 캡슐 CCT
	CCTDesc.fHeight = CCT_HEIGHT;
	CCTDesc.fRadius = CCT_RADIUS;
	CCTDesc.fSlopeLimit = cosf(XMConvertToRadians(30.f));
	CCTDesc.fStepOffset = 0.3f;
	CCTDesc.pOwner = this;

	XMStoreFloat3(&CCTDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&CCTDesc.vInitQuat, m_pTransformCom->Get_Quat());

	CCTDesc.iFilterWord0 = ETOUI(FILTER_GROUP::PLAYER);
	CCTDesc.iFilterWord1 = ETOUI(FILTER_GROUP::MONSTER) | ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT); // TODO : 0618 _ksb OBJECT 추가

	// CCT는 외부에서 Find Component AddComponent로 추가함 
	if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_CCT"),
		TEXT("Com_Physics"), reinterpret_cast<CComponent**>(&m_pCCT), &CCTDesc)))
		return E_FAIL;

	/* Throw Body */
	m_pThrowBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);
	if (m_pThrowBody == nullptr)
		return E_FAIL;

	PHYSIC_DESC ThrowDesc{};
	ThrowDesc.pOwner = this;
	XMStoreFloat3(&ThrowDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&ThrowDesc.vInitQuat, m_pTransformCom->Get_Quat());
	ThrowDesc.bIsKinematic = false;
	ThrowDesc.fMass = 60.f;
	ThrowDesc.bEnableCCD = true; // CCD 활성화

	ThrowDesc.iFilterWord0 = 0; // Player 넣으면 지금 필터셰이더가 OR이라 몬스터랑 충돌됨. 
	ThrowDesc.iFilterWord1 = ETOUI(FILTER_GROUP::GROUND);

	SHAPE_DESC ThrowShape{};
	ThrowShape.eType = GEOMETRY_TYPE::SPHERE;
	ThrowShape.fRadius = THROW_BODY_RADIUS;
	ThrowShape.bTrigger = false;

	ThrowDesc.Shapes.push_back(ThrowShape);
	if (FAILED(m_pThrowBody->Initialize(&ThrowDesc)))
	{
		Safe_Release(m_pThrowBody);
		return E_FAIL;
	}

	m_pRigidActor = static_cast<PxRigidDynamic*>(m_pThrowBody->Get_RigidActor());
	RemoveActor_FromScene();

	/* Spring Arm */
	CSpringArm::SPRING_DESC SpringArmDesc;
	SpringArmDesc.pOwner = this;
	SpringArmDesc.fArmLength = SPRING_ARM_LENGTH;
	SpringArmDesc.fMouseSensor = 0.0022f;
	SpringArmDesc.fTargetOffset = 1.7f;
	SpringArmDesc.vArmRotLimits = { -XM_PIDIV4 , XM_PIDIV4 };

	m_pSpringArm = m_pGameInstance->Clone_Prototype<CSpringArm>(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_SpringArm"), &SpringArmDesc);
	if (nullptr == m_pSpringArm) return E_FAIL;

	m_pGameInstance->Set_FollowTarget(m_pSpringArm);

	/* Aim Component */
	CAimComponent::AIM_COMPONENT_DESC AimDesc{};
	AimDesc.pModel = m_pModelCom;
	AimDesc.pOwnerTransform = m_pTransformCom;
	AimDesc.pSpringArm = m_pSpringArm;
	m_pAimCom = CAimComponent::Create(&AimDesc);

	if (m_pAimCom == nullptr) 
		return E_FAIL;

	/*Input Component*/
	m_pInputCom = CInputComponent::Create();
	if (m_pInputCom == nullptr)
		return E_FAIL;

	/* m_pStatusCom*/
	m_pStatusCom = CStatusComponent::Create(m_pDevice, m_pContext);
	if (m_pStatusCom == nullptr)
		return E_FAIL;

	/* m_pStatCom*/
	m_pStatCom = CStatComponent::Create(m_pDevice, m_pContext, m_fMaxHP, m_fMaxStamina);
	if (m_pStatCom == nullptr)
		return E_FAIL;

	/* Equipment Com*/
	m_pEquipmentCom = CEquipment::Create(m_pDevice, m_pContext, this);
	if (m_pEquipmentCom == nullptr)
		return E_FAIL;

	/*Movement Component*/
	CMovement::MOVEMENT_DESC movementDesc{};
	movementDesc.pOwner = this;
	movementDesc.fAcceleration = 10.f;
	movementDesc.fDeceleration = 10.f;
	movementDesc.fRunSpeed = 4.f;
	movementDesc.fSprintSpeed = 6.f;
	movementDesc.fTurnRate = 14.f;

	m_pMovementCom = m_pGameInstance->Clone_Prototype<CMovement>(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Movement"), &movementDesc);
	if (nullptr == m_pMovementCom) return E_FAIL;

	/* Target Component */
	CTargetComponent::TARGET_DESC TargetDesc{};
	TargetDesc.pOwner = this;
	TargetDesc.fSearchRange = 20.f;
	TargetDesc.iMyFilter = FILTER_GROUP::PLAYER;
	TargetDesc.iTargetFilter = FILTER_GROUP::MONSTER;
	TargetDesc.fFOV = XMConvertToRadians(160.f);

	m_pTargetCom = m_pGameInstance->Clone_Prototype<CTargetComponent>(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Target"), &TargetDesc);
	if (nullptr == m_pTargetCom) return E_FAIL;

	CDissolve::DISSOLVE_DESC DissolveDesc{};
	DissolveDesc.fStartDelay = 2.f;
	DissolveDesc.fDuration = 1.5f;
	DissolveDesc.fEndDelay = 2.f;
	DissolveDesc.vEdgeColor = { 0.05f, 0.45f, 1.f, 6.f };
	m_pDissolveCom = CDissolve::Create(&DissolveDesc);
	if (nullptr == m_pDissolveCom) return E_FAIL;

	return S_OK;
}

HRESULT CPlayer::Ready_PartObjects()
{
	CPartSet::PARTSET_DESC PartSetDesc{};
	PartSetDesc.pMasterModel = m_pMasterModelCom;
	PartSetDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();

	m_pPartSet = CPartSet::Create(m_pDevice, m_pContext, PartSetDesc);
	if (m_pPartSet == nullptr)
	{
		MSG_BOX("Pino : Failed to Create CPartSet");
		return E_FAIL;
	}

	/* Create Lamp ------------------------------------------------------------------------*/

	CPinoLamp::PINOLAMP_DESC LampDesc{};
	LampDesc.pOwner = this;
	LampDesc.pSocketBoneMatrix = m_pModelCom->Get_Bone_CombinedMatPtr(m_strLampBoneName.c_str());
	LampDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();

	m_pLamp = CPinoLamp::Create(m_pDevice, m_pContext, &LampDesc);
	if (m_pLamp == nullptr)
	{
		MSG_BOX("Pino : Failed to Create CPinoLamp");
		return E_FAIL;
	}

	if (m_pEquipmentCom)
	{
		if (FAILED(m_pEquipmentCom->Ready_Weapon(Get_RightHandBoneName(), Get_LeftHandBoneName())))
		{
			MSG_BOX("Player : Failed to Ready_Weapon");
			return E_FAIL;
		}
	}

	/* Create Region Arm -----------------------------------------------------------------------*/

	CLegionArm_Manager::LEGIONARM_MGR_DESC LegionArmDesc{};
	LegionArmDesc.iLevelIndex = ETOUI(LEVEL::GAMEPLAY);
	LegionArmDesc.pMasterModel = m_pMasterModelCom;
	LegionArmDesc.pOwner = this;
	LegionArmDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();

	m_pLegionArm_Manager = CLegionArm_Manager::Create(m_pDevice, m_pContext, LegionArmDesc);
	if (m_pLegionArm_Manager == nullptr)
	{
		MSG_BOX("Failed to Create CLegionArm_Manager");
		return E_FAIL;
	}

	//디버깅 코드
	m_pLegionArm_Manager->Change_Arm(LegionArmType::Grapple);
	m_pLegionArm_Manager->Upgrade_Arm(LegionArmType::Grapple);


	return S_OK;
}

HRESULT CPlayer::Ready_Animator()
{
	CAnimator::ANIMATOR_DESC Desc{};
	Desc.BasePose = *m_pMasterModelCom->Get_BindPosePtr();
	Desc.RawAnimationLoadDesc.strFolderName = L"Pino";
	Desc.RawAnimationLoadDesc.strAnimationBaseName = L"Pino_AllParts";
	Desc.RawAnimationLoadDesc.eTimeUnit = ANIMATION_TIME_UNIT::TICKS;
	Desc.RawAnimationLoadDesc.bStaticAnimation = true;

	const vector<CBone*>& Bones = m_pMasterModelCom->Get_Bones();

	Desc.Bones.clear();
	Desc.Bones.reserve(Bones.size());

	for (_uint i = 0; i < Bones.size(); ++i)
	{
		CAnimator::ANIMATOR_BONE_DESC BoneDesc{};

		if (Bones[i])
		{
			BoneDesc.strName = Bones[i]->Get_Name();

			_wstring strBoneName;
			strBoneName.assign(BoneDesc.strName.begin(), BoneDesc.strName.end());

			BoneDesc.iNameHash = Hash_String(strBoneName.c_str());
			BoneDesc.iParentIndex = Bones[i]->Get_ParentIndex();
		}

		Desc.Bones.push_back(BoneDesc);
	}

	Desc.PreTransformMatrix = m_pMasterModelCom->Get_PreTransformMatrix();

	m_pAnimator = CAnimator::Create(Desc);
	if (m_pAnimator == nullptr)
		return E_FAIL;


	/* Create Anim Driver */
	CAnimDriver::ANIMDRIVER_DESC animDriverDesc;
	animDriverDesc.pAnimator = m_pAnimator;
	animDriverDesc.pInput = m_pInputCom;
	animDriverDesc.pStatus = m_pStatusCom;
	animDriverDesc.pStat = m_pStatCom;
	animDriverDesc.pMovement = m_pMovementCom;
	animDriverDesc.pEquipment = m_pEquipmentCom;
	animDriverDesc.pTarget = m_pTargetCom;
	animDriverDesc.pTransform = m_pTransformCom;
	animDriverDesc.m_pModel = m_pMasterModelCom;

	m_pAnimDriver = CAnimDriver::Create(m_pDevice, m_pContext, animDriverDesc);
	if (nullptr == m_pAnimDriver) return E_FAIL;

	const _int iPelvis = m_pMasterModelCom->Get_BoneIndex("Bip001-Pelvis");
	m_pAnimator->Set_RootBoneIndex(iPelvis);

	if (FAILED(m_pAnimator->Play_RawAnimation(L"MT_Bayonet_SA1")))
		return E_FAIL;

	m_pEquipmentCom->Set_Animator(m_pAnimator);

	return S_OK;
}

HRESULT CPlayer::Ready_Inventory()
{
	/* Inventory */
	m_pInventory = CInventory::Create(m_pDevice, m_pContext, this);
	if (m_pInventory == nullptr)
	{
		MSG_BOX("Failed to Create CInventory");
		return E_FAIL;
	}

	//스타팅 장비들 획득 
	for (_uint64 itemID : StartingItemIDs)
	{
		m_pInventory->Acquire(itemID);
	}

	//장착 
	m_pInventory->Equip(ItemID::BODY_BASE);
	m_pInventory->Equip(ItemID::FACE_BASE);
	m_pInventory->Equip(ItemID::HAIR_BASE);
	m_pInventory->Equip(ItemID::HELMET_SANTA);
	m_pInventory->Equip(ItemID::ACC_RUDOLF_NOSE);

	m_pInventory->Equip(ItemID::WEAPON_GREATSWORD, 0);
	m_pInventory->Equip(ItemID::WEAPON_SCISSOR, 1);

	m_pInventory->Equip(ItemID::LEGION_BASE);
	m_pInventory->Equip(ItemID::LEGION_GRAPPLE);
	m_pInventory->Equip(ItemID::LEGION_CHARGEGUN);

	//디버깅용 
	//m_pInventory->Upgrade_LegionArm(ItemID::LEGION_CHARGEGUN, 1000);
	//m_pInventory->Upgrade_LegionArm(ItemID::LEGION_GRAPPLE, 1000);
	//m_pInventory->Unlock_Grapple_Mobility();

	return S_OK;
}

void CPlayer::Ready_Event_Subscription()
{
	m_pGameInstance->Subscribe(EVENT_TYPE::CAMERA_ACTION_BEGIN, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::CAMERA_ACTION_END, this);

	//텔레포트 연출 시작 
	m_pGameInstance->Subscribe(EVENT_TYPE::TELEPORT_BEGIN, this);
	//전환 후 시작 
	m_pGameInstance->Subscribe(EVENT_TYPE::START_CHAPTER, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::UI_END, this);
	
	/* MAP EVENT*/
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::LADDER, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::LADDER_OPEN, this);

	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::STARGAZER_ACTIVATED, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::STARGAZER_REPAIR, this);

	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::ELEVATOR_OPEN, this);

	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::GET_ITEM, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::OPEN_DOOR, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::TRAP_OPEN, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::OPEN_CHEST, this);

	//Test0731
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::NPC_TALK_START, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::NPC_TALK_END, this);

	m_pGameInstance->Subscribe(EVENT_TYPE_UI::GET_ITEM, this);

}

void CPlayer::Update_Grab(_float fTimeDelta)
{
	m_fGrabPhaseTime += fTimeDelta;

	if (m_eGrabState == GRAB_STATE::GRAB)
	{
		if (m_fGrabPhaseTime >= GRAB_HOLD_TIMEOUT)
			Cancel_Grab();
		return;
	}
	else if (m_eGrabState == GRAB_STATE::THROW)
	{
		if (m_pRigidActor == nullptr)
		{
			Cancel_Grab();
			return;
		}

		_float fSpeed = m_pRigidActor->getLinearVelocity().magnitude();
		_bool bMinAir = (m_fGrabPhaseTime > THROW_MIN_AIR_TIME);
		_bool bFinish = (m_bThrowImpact || fSpeed < THROW_SETTLE_SPEED || m_fGrabPhaseTime > THROW_MAX_AIR_TIME);

		//최소시간 + 종료 조건 만족하면 끝냄d		
		if (bMinAir && bFinish)
		{
			Finish_Throw();
		}

		return;
	}

}

void CPlayer::LateUpdate_Grab()
{
	//그랩 소켓에 붙이기 
	if (m_eGrabState == GRAB_STATE::GRAB)
	{
		_matrix GrabberSocketWorld = {};
		if (m_pGrabber == nullptr || false == m_pGrabber->Get_GrabSocketWorld(GrabberSocketWorld))
		{
			Cancel_Grab();
			return;
		}

		//스케일 제외 
		_vector vScale, vQuat, vTranslation;
		if (false == XMMatrixDecompose(&vScale, &vQuat, &vTranslation, GrabberSocketWorld))
		{
			Cancel_Grab();
			return;
		}

		//잡은 객체의 Grab Socket 본 월드행렬
		_matrix GrabberBoneWorld = XMMatrixAffineTransformation(XMVectorSplatOne(), _vector{ 0.f, 0.f, 0.f, 1.f }, vQuat, vTranslation);

		if (false == XMMatrixDecompose(&vScale, &vQuat, &vTranslation, XMLoadFloat4x4(m_pGrabSocketBone)))
		{
			Cancel_Grab();
			return;
		}

		//플레이어 Grab Root 본 월드행렬
		_matrix MyGrabBoneWorld = XMMatrixAffineTransformation(XMVectorSplatOne(), _vector{ 0.f, 0.f, 0.f, 1.f }, vQuat, vTranslation);

		//최종 월드 :  내 Grab Root 본 월드행렬 역행렬 * 잡은 객체 Grab Socket 본 월드행렬
		_matrix FinalWorld = XMMatrixInverse(nullptr, MyGrabBoneWorld) * GrabberBoneWorld;
		m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(FinalWorld.r[3], 1.f));
		m_pTransformCom->Set_Quat(XMQuaternionRotationMatrix(FinalWorld));

	}
	else if (m_eGrabState == GRAB_STATE::THROW)
	{
		if (m_pThrowBody == nullptr) return;

		_vector vPos = XMVectorZero();
		_vector vQuat = XMVectorZero();

		m_pThrowBody->Get_GlobalTransform(vPos, vQuat);
		_vector vFootPos = XMVectorSetW(XMVectorSubtract(vPos, XMVectorSet(0.f, THROW_BODY_RADIUS, 0.f, 0.f)), 1.f);
		m_pTransformCom->Set_State(STATE::POSITION, vFootPos);
	}
}

void CPlayer::Update_LockOn()
{
	//차지건 사용중에는 락온 해제
	CChargeGun* pChargeGun = m_pLegionArm_Manager ? m_pLegionArm_Manager->Get_ChargeGun() : nullptr;
	if (pChargeGun && pChargeGun->Is_Used())
	{
		if (m_bWantLockOn)
		{
			m_bWantLockOn = false;
			m_pTargetCom->Clear_Target();
		}

		if (m_pStatusCom->Is_LockedOn())
		{
			m_pGameInstance->Try_Carmera_LockOn(false);
			m_pStatusCom->Set_LockedTarget(nullptr);
		}
		return;
	}

	//Test0731
	if (true == m_bDialogueCameraLocked)
	{
		m_pGameInstance->Try_Carmera_LockOn(true);
		m_pGameInstance->Set_Camera_LockPos(XMLoadFloat3(&m_vDialogueLookPos));
		return;
	}

	if (m_eGrabState == GRAB_STATE::GRAB && m_pGrabber != nullptr)
	{
		_vector vWatchPos;
		m_pGrabber->Get_LockOnWorldPosition(vWatchPos);

		m_pGameInstance->Try_Carmera_LockOn(true);
		m_pGameInstance->Set_Camera_LockPos(vWatchPos);
		return;
	}

	if (m_pGameInstance->Mouse_Down(DIMB::WHEEL))
	{
		m_bWantLockOn = !m_bWantLockOn;
		if (m_bWantLockOn)
			m_pTargetCom->Find_LockOnTarget();
		else
			m_pTargetCom->Clear_Target();
	}

	CAIObject* pTarget = m_pTargetCom->Get_CurrentTarget();
	_bool bWantLockedOn = m_bWantLockOn && (pTarget != nullptr);

	//액션 모드를 대비하여 매프레임 LockOn체크 
	_bool bLockedOn = m_pGameInstance->Try_Carmera_LockOn(bWantLockedOn);
	if (bLockedOn)
	{
		_vector vTargetLockOnPos;

		pTarget->Get_LockOnWorldPosition(vTargetLockOnPos);
		m_pGameInstance->Set_Camera_LockPos(vTargetLockOnPos);
		m_pStatusCom->Set_LockedTarget(pTarget);
	}
	else
	{
		if (pTarget == nullptr && m_bWantLockOn)
		{
			m_bWantLockOn = false;
			m_pTargetCom->Clear_Target();
		}
		m_pStatusCom->Set_LockedTarget(nullptr);
	}
}

void CPlayer::Update_Teleport(_float fTimeDelta)
{
	m_fTeleportTimer += fTimeDelta;

	if (m_bTeleportAnimStarted == false)
	{
		m_pAnimDriver->Play_Action(AnimAction::Teleport_Start);
		m_bTeleportAnimStarted = true;
	}

	if (m_fTeleportTimer < TELEPORT_DELAY)
		return;

	m_bTeleportPending = false;
	m_bTeleportAnimStarted = false;

	if (m_bRespawnRequested) return;

	ChangeChapter_EventDesc Desc = m_TeleportEventDesc;
	//텔레포트 전환 시작 
	m_pGameInstance->Broadcast(EVENT_TYPE::TELEPORT_START, &Desc);
}

void CPlayer::Face_DialogueSpeaker(Event_Desc* pEventDesc)
{
	const CNPC::NPC_DIALOGUE_EVENT_DESC* pDesc = static_cast<const CNPC::NPC_DIALOGUE_EVENT_DESC*>(pEventDesc);

	if (nullptr == pDesc || nullptr == m_pTransformCom)
		return;

	const _vector vSpeakerPos = XMLoadFloat3(&pDesc->vSpeakerLookPos);
	const _vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);

	if (XMVectorGetX(XMVector3LengthSq(XMVectorSetY(XMVectorSubtract(vSpeakerPos, vMyPos), 0.f))) < 1e-6f)
		return;

	m_pTransformCom->LookAt(XMVectorSetY(vSpeakerPos, XMVectorGetY(vMyPos)));

	if (false == pDesc->bLockCamera)
		return;

	m_vDialogueLookPos = pDesc->vSpeakerLookPos;
	m_bDialogueCameraLocked = true;
}

void CPlayer::Release_DialogueCamera()
{
	if (false == m_bDialogueCameraLocked)
		return;

	m_bDialogueCameraLocked = false;
	m_pGameInstance->Try_Carmera_LockOn(false);
}

void CPlayer::Drive_Effect(const PLAYER_FX_EVENT& fxEvent)
{
	switch (fxEvent.eType)
	{
	case PLAYER_FX::Parry:
	{

		m_pGameInstance->Play_Effect("Effect_Perfect_Guard", fxEvent.vWorldPos);
		break;
	}
	case PLAYER_FX::GuardHit:
	{
		m_pGameInstance->Play_Effect("Effect_Player_Normal_Guard", fxEvent.vWorldPos);
		break;
	}
	case PLAYER_FX::GrindLoop:
	{
		if (fxEvent.bActive && m_pChannel_GrinderFx == nullptr)
		{
			static const char* s_szGrinderStart[] =
			{
				"SE_PC_MT_Item_Grinder_Start_01",
				"SE_PC_MT_Item_Grinder_Start_02",
				"SE_PC_MT_Item_Grinder_Start_03",
			};
			const char* szStart = s_szGrinderStart[Random(0, 2)];
			m_pGameInstance->Play_PlayerSound(szStart, 2.2f * Random(0.95f, 1.05f), false, false, Random(0.95f, 1.05f));

			m_pGameInstance->Play_PlayerSound("SE_PC_MT_Item_Grinder_Loop_01", 0.8f, true, true, 1.f);
			m_pGameInstance->Play_PlayerSound("SE_PC_MT_Item_Grinder_Loop_02", 0.3f, true, true, 0.625f);

			const _float4x4* pMatPos = m_pTransformCom->Get_WorldMatrixPtr();
			m_pChannel_GrinderFx = m_pGameInstance->Play_Effect("Effect_Repair", m_pMasterModelCom->Get_Bone_CombinedMatPtr("Bn_L_Mecanic_001"), pMatPos, _float3(0.f, 1.2f, -0.2f));
			m_bGrinding = true;
		}
		else if (!fxEvent.bActive && m_pChannel_GrinderFx)
		{
			m_pGameInstance->StopSound_ByKey("SE_PC_MT_Item_Grinder_Loop_01");
			m_pGameInstance->StopSound_ByKey("SE_PC_MT_Item_Grinder_Loop_02");
			m_pGameInstance->Play_PlayerSound("SE_PC_MT_Item_Grinder_End_01", Random(0.95f, 1.05f), false, false, Random(0.95f, 1.05f));
			m_pChannel_GrinderFx->Stop();
			m_pChannel_GrinderFx = nullptr;
			m_bGrinding = false;
		}
		break;
	}

	default:
		break;
	}
}

void CPlayer::Drive_ItemSlot()
{
	if (m_pInputCom->Consume(PlayerActionID::Heal))
		m_eItemSlot = ITEM_SLOT::Heal;
	else if (m_pInputCom->Consume(PlayerActionID::Grinder))
		m_eItemSlot = ITEM_SLOT::Grinder;

	if (m_pAnimDriver)
	{
		m_pAnimDriver->Set_ItemSlot(m_eItemSlot);

		if (m_pInputCom->Peek(PlayerActionID::Lamp) && m_pLamp)
		{
			const PlayerAnimEvent eLampEvent = m_pLamp->Is_LightOn()
				? PlayerAnimEvent::Turn_Off_Lamp : PlayerAnimEvent::Turn_On_Lamp;

			if(m_pAnimDriver->Requst_Overlap(eLampEvent))
				m_pInputCom->Consume(PlayerActionID::Lamp);
		}
	}


}

_bool CPlayer::Handle_GetItem(ITEM_TYPE eItemType, _uint64 iItemID)
{
	if (nullptr == m_pInventory)
		return false;

	const ITEM_DATA* pItemData = CItemTable::Find(iItemID);
	
	if (pItemData == nullptr)
	{
		MSG_BOX("CPlayer ::Handle_GetItem : Invalid ItemID");
		return false;
	}

	return m_pInventory->Acquire(iItemID);
}

void CPlayer::OnEvent(EVENT_TYPE etype, Event_Desc* pData)
{
	switch (etype)
	{
		case  EVENT_TYPE::CAMERA_ACTION_BEGIN :
		{
			m_pStatusCom->Set_InputLock(true);	
			m_pStatusCom->Set_CamMoveLock(true);
			break;
		}
		case EVENT_TYPE::CAMERA_ACTION_END :
		{
			m_pStatusCom->Set_InputLock(false);
			m_pStatusCom->Set_CamMoveLock(false);
			break;
		}
		case EVENT_TYPE::START_CHAPTER :
		{
			m_pAnimDriver->Play_Action(AnimAction::Restart);
			m_pAnimDriver->Update(0.f);
			break;
		}
		case EVENT_TYPE::TELEPORT_BEGIN:
		{
			if (pData == nullptr || m_bTeleportPending || m_pStatCom->Is_Dead())
				return;

		m_bTeleportPending = true;
		m_bTeleportAnimStarted = false;
		m_TeleportEventDesc = *static_cast<ChangeChapter_EventDesc*>(pData);
		m_fTeleportTimer = 0.f;

			m_pStatusCom->Set_InputLock(true);
			m_pAnimDriver->Play_Action(AnimAction::Teleport_Start);
			break;
		}
		case EVENT_TYPE::UI_END : 
		{
			m_pAnimDriver->End_InteractLoop();
		}

	}

}

void CPlayer::OnEvent(EVENT_TYPE_MAP_OBJECT etype, Event_Desc* pData)
{
	switch (etype)
	{
	case Engine::EVENT_TYPE_MAP_OBJECT::STARGAZER_REPAIR:
	{
		m_pAnimDriver->Play_Action(AnimAction::Stargazer_InActive);
		break;
	}
	case Engine::EVENT_TYPE_MAP_OBJECT::STARGAZER_ACTIVATED:
	{
		m_pAnimDriver->Play_Action(AnimAction::Stargazer_Active);
		break;
	}
	case Engine::EVENT_TYPE_MAP_OBJECT::LADDER:
	{
		CLadder::LADDER_EVENT_DESC* pDesc = static_cast<CLadder::LADDER_EVENT_DESC*>(pData);
		Begin_LadderClimb(pDesc->vBottomLandingPosition, pDesc->vTopLandingPosition, pDesc->vTopPosition, pDesc->TopWorldMatrix);
		break;
	}
	case Engine::EVENT_TYPE_MAP_OBJECT::LADDER_OPEN:
	{
		m_pAnimDriver->Play_Action(AnimAction::Ladder_Activate);
		break;
	}
	case Engine::EVENT_TYPE_MAP_OBJECT::NPC_TALK_START:
	{
		//Test0731
		if (nullptr == pData) return;

		Face_DialogueSpeaker(pData);
		break;
	}
	case Engine::EVENT_TYPE_MAP_OBJECT::NPC_TALK_END:
	{
		Release_DialogueCamera();
		break;
	}
	case Engine::EVENT_TYPE_MAP_OBJECT::OPEN_DOOR:
	{
		if (!pData) return;

		Activate_EventDesc* pDesc = static_cast<Activate_EventDesc*>(pData);
		INTERACTION_TYPE eDoorType = pDesc->eInteractionType;

		m_pCCT->Set_FootPosition(XMLoadFloat4(&pDesc->vPosition));
		m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat4(&pDesc->vPosition));
		m_pTransformCom->Set_Quat(XMLoadFloat4(&pDesc->vQuat));

		if (eDoorType == INTERACTION_TYPE::DOOR_SLIDE)
		{
			m_pAnimDriver->Play_Action(AnimAction::Open_SlidingDoor);
		}
		else if (eDoorType == INTERACTION_TYPE::DOOR_BIG)
		{
			m_pAnimDriver->Play_Action(AnimAction::Open_DoubleDoor_Big);
		}
		else if (eDoorType == INTERACTION_TYPE::DOOR_BOSS)
		{
			m_pAnimDriver->Play_Action(AnimAction::Open_DoubleDoor_Boss);
		}
		else if (eDoorType == INTERACTION_TYPE::DOOR_HOTEL)
		{
			m_pAnimDriver->Play_Action(AnimAction::Open_DoubleDoor_Hotel);
		}


		break;
	}
	case Engine::EVENT_TYPE_MAP_OBJECT::ELEVATOR_OPEN:
	{
		m_pAnimDriver->Play_Action(AnimAction::Labber);
		break;
	}

	case Engine::EVENT_TYPE_MAP_OBJECT::OPEN_CHEST:
	{
		m_pAnimDriver->Play_Action(AnimAction::Open_Chest);
		break;
	}
	case Engine::EVENT_TYPE_MAP_OBJECT::GET_ITEM:
	{
		if (pData == nullptr) return;

		m_pAnimDriver->Play_Action(AnimAction::Item_Grab);
		
		GET_ITEM_EVENT_DESC* pDesc = static_cast<GET_ITEM_EVENT_DESC*>(pData);
		if (pDesc->eItemType == ITEM_TYPE::ERGO)
		{
			m_pInventory->Add_Ergo(max(1, pDesc->iItemCount));
			return;
		}
		//그 외의 아이템 
		Handle_GetItem(pDesc->eItemType, pDesc->iItemID);

		break;
	}
	default:
		break;
	}

}

void CPlayer::OnEvent(EVENT_TYPE_UI etype, Event_Desc* pData)
{
	switch (etype)
	{
		//UI 이벤트에서 획득하는 경우 (상점..)
		case Engine::EVENT_TYPE_UI::GET_ITEM:
		{
			if (pData == nullptr) return;

			const GET_ITEM_EVENT_DESC* pDesc = static_cast<GET_ITEM_EVENT_DESC*>(pData);
			Handle_GetItem(pDesc->eItemType, pDesc->iItemID);
			break;
		}
	}

}

HIT_DIR CPlayer::Compute_HitDir(const HitInfo& hitInfo, _float& outForwardDot) const
{
	HIT_DIR hitDir = HIT_DIR::End;

	//피격 상세 방향 정하기 
	_vector vDirToHitPoint = XMLoadFloat3(&hitInfo.vHitPosition) - m_pTransformCom->Get_State(STATE::POSITION);
	vDirToHitPoint = XMVectorSetY(vDirToHitPoint, 0.f);

	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vDirToHitPoint)), 0.f))
		vDirToHitPoint = m_pTransformCom->Get_State(STATE::LOOK);

	//Vector3로하여 w값 제외. w = 1이 포함되어 Dot연산시 결과가 왜곡됨
	vDirToHitPoint = XMVector3Normalize(vDirToHitPoint);

	_vector vLookXY = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
	_vector vRightXY = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::RIGHT), 0.f));

	outForwardDot = XMVectorGetX(XMVector3Dot(vLookXY, vDirToHitPoint)); // cos
	_float fRightDot = XMVectorGetX(XMVector3Dot(vRightXY, vDirToHitPoint)); // sin

	//정면 기준 방위각도
	_float fAngle = XMConvertToDegrees(atan2f(fRightDot, outForwardDot));
	_float fAbs = fabsf(fAngle);

	if (fAbs <= 35.f) hitDir = HIT_DIR::Front;
	else if (fAbs >= 110.f) hitDir = HIT_DIR::Back;
	else if (fAbs <= 70.f)
	{
		if (fAngle > 0.f) hitDir = HIT_DIR::FrontRight;
		else hitDir = HIT_DIR::FrontLeft;
	}
	else
	{
		if (fAngle > 0.f) hitDir = HIT_DIR::Right;
		else hitDir = HIT_DIR::Left;
	}
	return hitDir;
}

_bool CPlayer::Is_Defenseless(_bool bFrontHit)
{
	if (!bFrontHit) return true;
	if (!m_pStatusCom->Is_GuardActive()) return true;
	return false;
}

_bool CPlayer::Can_Guard(const HitInfo& hitInfo) const
{
	if (hitInfo.eHitType == HIT_TYPE::KnockBack ||
		hitInfo.eHitType == HIT_TYPE::HeavyUnGuard ||
		hitInfo.eHitType == HIT_TYPE::NormalUnGuard ||
		hitInfo.eHitType == HIT_TYPE::Fury) return false;
	return true;
}

void CPlayer::React_NoStaminaGuard(const HitInfo& hitInfo, HIT_DIR hitDir)
{
	m_pAnimDriver->Play_HitAction(hitInfo.eHitType, hitDir, REACT_TYPE::GuardBreak);
	m_pStatCom->Apply_Damage(hitInfo.fDamage);
}

void CPlayer::React_NoGuard(const HitInfo& hitInfo, HIT_DIR hitDir)
{
	if (hitInfo.eHitType == HIT_TYPE::KnockBack)
		m_pAnimDriver->Play_HitAction(hitInfo.eHitType, hitDir, REACT_TYPE::KnockBack);
	else
		m_pAnimDriver->Play_HitAction(hitInfo.eHitType, hitDir, REACT_TYPE::Hit);
	_float3 HitPosOffset = hitInfo.vHitPosition;
	HitPosOffset.y -= 1.f;
	m_pGameInstance->Play_Effect("Effect_Take_Damage", HitPosOffset);
	m_pStatCom->Apply_Damage(hitInfo.fDamage);

	if (hitInfo.eSpecialAttackType == SPECIAL_ATTACK_TYPE::ELECTRIC && (!m_pElectricityFx || m_pElectricityFx->Is_Finished()))
	{
		m_pElectricityFx = Play_BodySurfaceEffect(STR_ELECT_EFFECT);
	}

}

void CPlayer::React_Unguarded(const HitInfo& hitInfo, HIT_DIR hitDir)
{
	_bool bParry = m_pStatusCom->Is_ParryActive();
	if (bParry)
	{
		React_Parry(hitInfo, hitDir);
		if (hitInfo.eHitType == HIT_TYPE::Fury)
		{
			Launch_Back(hitInfo);
			m_pAnimDriver->Play_HitAction(hitInfo.eHitType, hitDir, REACT_TYPE::Guard);
			return;
		}
	}
	else
	{
		m_pStatCom->Consume_Stamina(GUARD_STATMINA_COST);
		m_pStatCom->Apply_Damage(hitInfo.fDamage);
	}

	if (hitInfo.eHitType == HIT_TYPE::KnockBack || hitInfo.eHitType == HIT_TYPE::Fury)
	{
		m_pAnimDriver->Play_HitAction(hitInfo.eHitType, hitDir, REACT_TYPE::KnockBack);
	}
	else if (hitInfo.eHitType == HIT_TYPE::HeavyUnGuard)
	{
		m_pAnimDriver->Play_HitAction(hitInfo.eHitType, hitDir, REACT_TYPE::Heavy_UnGuard);
	}
	else if (hitInfo.eHitType == HIT_TYPE::NormalUnGuard)
	{
		m_pAnimDriver->Play_HitAction(hitInfo.eHitType, hitDir, REACT_TYPE::Normal_UnGuard);
	}

	m_pGameInstance->Play_Cue(SOUND_GUARD_BREAK_CUE, CHANNEL_ID::SOUND_PLAYER, hitInfo.vHitPosition);
	m_pGameInstance->Play_Effect("Effect_Take_Damage", hitInfo.vHitPosition);

	if (hitInfo.eSpecialAttackType == SPECIAL_ATTACK_TYPE::ELECTRIC && (!m_pElectricityFx || m_pElectricityFx->Is_Finished()))
	{
		m_pElectricityFx = Play_BodySurfaceEffect(STR_ELECT_EFFECT);
	}
}

void CPlayer::React_Parry(const HitInfo& hitInfo, HIT_DIR hitDir)
{
	if (hitInfo.pOtherActor == nullptr && hitInfo.pDamangeCauser != nullptr)
	{
		// TODO: 나중에 static cast 
		CProjectile* pProjectile = dynamic_cast<CProjectile*>(hitInfo.pDamangeCauser);
		if (nullptr != pProjectile)
		{
			pProjectile->On_Parry();
		}
	}

	CAIObject* pMonster = dynamic_cast<CAIObject*>(hitInfo.pOtherActor);
	if (nullptr != pMonster)
	{
		HitInfo tParryInfo{};
		tParryInfo.fDamage = 0.f;
		tParryInfo.fGroggy = PARRY_GROGGY;
		tParryInfo.eHitType = HIT_TYPE::Normal;
		tParryInfo.vHitPosition = hitInfo.vHitPosition;
		tParryInfo.pOtherActor = this;

		pMonster->Take_Damage(tParryInfo);
		pMonster->On_Parried();
	}

	CWeapon_Monster* pMonsterWeapon = dynamic_cast<CWeapon_Monster*>(hitInfo.pDamangeCauser);
	if (nullptr != pMonsterWeapon)
		pMonsterWeapon->Begin_ParryFlash();

	_float3 vEffectPos = hitInfo.vHitPosition;
	_matrix weaponMat;
	CWeapon* pWeapon = m_pEquipmentCom->Get_CurrentWeapon();

	if (pWeapon)
	{
		if (SUCCEEDED(pWeapon->Get_EffectBoneMatrix(weaponMat)))
			XMStoreFloat3(&vEffectPos, weaponMat.r[3]);
	}

	//Parry Effect
	PLAYER_FX_EVENT fxParryEvent = {};
	fxParryEvent.eType = PLAYER_FX::Parry;
	fxParryEvent.vWorldPos = vEffectPos;
	Drive_Effect(fxParryEvent);

	if (hitInfo.eHitType == HIT_TYPE::Fury)
		m_pGameInstance->Play_Cue(SOUND_PERFECT_GUARD_METAL_FURY_CUE, CHANNEL_ID::SOUND_PLAYER, hitInfo.vHitPosition);
	else 
		m_pGameInstance->Play_Cue(SOUND_PERFECT_GUARD_METAL_CUE,CHANNEL_ID::SOUND_PLAYER, hitInfo.vHitPosition);

	m_pGameInstance->Play_Shake(CAM_SHAKE_PARRY_FURY);

}

void CPlayer::React_Guard(const HitInfo& hitInfo, HIT_DIR hitDir)
{
	_bool bParry = m_pStatusCom->Is_ParryActive();

	if (bParry) React_Parry(hitInfo, hitDir);
	else
	{
		m_pStatCom->Consume_Stamina(GUARD_STATMINA_COST);

		_float3 vEffectPos = hitInfo.vHitPosition;
		_matrix weaponMat;
		CWeapon* pWeapon = m_pEquipmentCom->Get_CurrentWeapon();

		if (pWeapon)
		{
			if (SUCCEEDED(pWeapon->Get_EffectBoneMatrix(weaponMat)))
				XMStoreFloat3(&vEffectPos, weaponMat.r[3]);
		}

		PLAYER_FX_EVENT fxGuardEvent = {};
		fxGuardEvent.eType = PLAYER_FX::GuardHit;
		fxGuardEvent.vWorldPos = vEffectPos;
		Drive_Effect(fxGuardEvent);

		m_pStatCom->Apply_Damage(hitInfo.fDamage * GUARD_DAMAGE_REDUCTION);
	}

	if (hitInfo.eHitType == HIT_TYPE::Normal)
	{
		Launch_Back(hitInfo);
		m_pAnimDriver->Play_HitAction(hitInfo.eHitType, hitDir, REACT_TYPE::Guard);
		
		m_pGameInstance->Play_Cue(SOUND_GUARD_METAL_CUE, CHANNEL_ID::SOUND_PLAYER, hitInfo.vHitPosition);
	}
	else
	{
		m_pGameInstance->Play_Cue(SOUND_GUARD_METAL2_CUE, CHANNEL_ID::SOUND_PLAYER, hitInfo.vHitPosition);
		m_pAnimDriver->Play_HitAction(hitInfo.eHitType, hitDir, REACT_TYPE::GuardHeavy);
	}


}

void CPlayer::Launch_Back(const HitInfo& hitInfo)
{
	//밀어낼 기준점 : 공격자 - 공격물체 - 피격 위치 순 설정 
	_vector vSource = XMVectorSetW(XMLoadFloat3(&hitInfo.vHitPosition), 1.f);
	if (hitInfo.pOtherActor != nullptr)
		vSource = hitInfo.pOtherActor->Get_Transform()->Get_State(STATE::POSITION);
	else if (hitInfo.pDamangeCauser != nullptr && hitInfo.pDamangeCauser->Get_Transform() != nullptr)
		vSource = hitInfo.pDamangeCauser->Get_Transform()->Get_State(STATE::POSITION);

	_vector vBackDir = XMVectorSetY(m_pTransformCom->Get_State(STATE::POSITION) - vSource, 0.f);

	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vBackDir)), 0.f))
		vBackDir = XMVectorNegate(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));

	vBackDir = XMVector3Normalize(XMVectorSetY(vBackDir, 0.f));

	//현재는 Normal Dist만 사용. 
	_float fKnockDist = m_KnockBackDists[ETOUI(HIT_TYPE::Normal)];
	m_pMovementCom->Begin_KnockBack(vBackDir, fKnockDist, GUARD_KNOCKBACK_DURATION, EASE_TYPE::OUT_QUINT);
}

void CPlayer::Interact_Triggers()
{
	if (!Can_Interact()) return;

	for (CMapGameplayObject* pMapObject : m_TriggerObjects)
	{
		if (pMapObject == nullptr || !pMapObject->Can_Interact(this)) continue;

		//TODO : 중복 트리거가 나올경우 (인접한 경우가 추가된다면 ) 거리별 Best Target 를 찾도록 변경 
		if (false == pMapObject->Interact(this))
			continue;

		// E 입력으로 대상의 실제 상호작용이 성공한 경우에만 SpawnID에 등록된 타입 이벤트를 발행한다.
		pMapObject->Dispatch_InteractionEvent();

		// 대상이 상호작용 위치 보정을 사용하는 경우에만 Player 위치와 회전을 보정한다.
		if (false == pMapObject->Is_UseInteractPosition())
			break;

		_float4x4 InteractTransform = pMapObject->Get_InteractTransform(this);
		_matrix InteractWorldMat = XMLoadFloat4x4(&InteractTransform);
		_vector vScale, vQuat, vTranslation;
		if (XMMatrixDecompose(&vScale, &vQuat, &vTranslation, InteractWorldMat))
		{
			m_pCCT->Set_FootPosition(vTranslation);
			m_pTransformCom->Set_State(STATE::POSITION, vTranslation);
			m_pTransformCom->Set_Quat(vQuat);
		}

		break;
	}

}


HRESULT CPlayer::Throw(_fvector vCenter, _fvector vThrowDir, _float fThrowSpeed, _float fUpSpeed)
{
	m_pCCT->Set_Active(false);

	//Actor 회전 
	_vector vFace = XMVectorNegate(vThrowDir);
	_float fYaw = atan2f(XMVectorGetX(vFace), XMVectorGetZ(vFace));
	m_pTransformCom->Set_Quat(XMQuaternionRotationRollPitchYaw(0.f, fYaw, 0.f));

	PxRigidActor* pActor = m_pThrowBody->Get_RigidActor();
	if (pActor == nullptr) return E_FAIL;

	if (pActor->getScene() == nullptr)
		m_pGameInstance->Add_ActorToScene(pActor);

	m_pThrowBody->Set_GlobalTransform(vCenter, XMQuaternionIdentity());

	PxRigidDynamic* pBody = static_cast<PxRigidDynamic*>(pActor);
	pBody->wakeUp();

	_vector vLaunchVelocity = XMVectorAdd(XMVectorScale(vThrowDir, fThrowSpeed), XMVectorSet(0.f, fUpSpeed, 0.f, 0.f));
	pBody->setLinearVelocity(PxVec3(XMVectorGetX(vLaunchVelocity), XMVectorGetY(vLaunchVelocity), XMVectorGetZ(vLaunchVelocity)));
	pBody->setAngularVelocity(PxVec3(0.f, 0.f, 0.f));

	//eVELOCITY_CHANGE : 질량 무시하고 바로 Velocity만큼 즉시 속도 변화
	//m_pThrowBody->Add_Force(vLaunchVelocity, PxForceMode::eVELOCITY_CHANGE);
	m_eGrabState = GRAB_STATE::THROW;
	m_fGrabPhaseTime = 0.f;
	m_bThrowImpact = false;

	return S_OK;
}

//CCT 복구 + 초기화 
void CPlayer::Finish_Throw()
{
	if (m_eGrabState != GRAB_STATE::THROW) return;
	_vector vPos = XMVectorZero();
	_vector vQuat = XMVectorZero();

	m_pThrowBody->Get_GlobalTransform(vPos, vQuat);

	RemoveActor_FromScene();

	_vector vFootPos = XMVectorSetW(XMVectorSubtract(vPos, XMVectorSet(0.f, THROW_BODY_RADIUS, 0.f, 0.f)), 1.f);
	m_pCCT->Set_FootPosition(vFootPos);
	m_pCCT->Set_Active(true);
	m_pCCT->InvalidateCache();

	m_pTransformCom->Set_State(STATE::POSITION, vFootPos);
	m_pMovementCom->Clear();

	m_pAnimDriver->Set_Param(PARAM_GRABHOLD, false);
	m_pAnimDriver->Set_Param(PARAM_THROW, false);

	m_eGrabState = GRAB_STATE::END;
	m_fGrabPhaseTime = 0.f;
	m_bThrowImpact = false;
	m_pGrabber = nullptr;
}

void CPlayer::RemoveActor_FromScene()
{
	if (m_pThrowBody == nullptr || m_pRigidActor == nullptr) return;

	if (m_pRigidActor->getScene() != nullptr)
		m_pGameInstance->Remove_ActorFromScene(m_pRigidActor);
}


CPlayer* CPlayer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPlayer* pInstance = new CPlayer(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CPlayer");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CPlayer::Clone(void* pArg)
{
	CPlayer* pInstance = new CPlayer(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPlayer");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CPlayer::Free()
{
	m_pGameInstance->UnSubscribe_AllEvent(this);
	m_pElectricityFx = nullptr;

	Safe_Release(m_pLamp);
	Safe_Release(m_pLegionArm_Manager);
	Safe_Release(m_pAimCom);
	Safe_Release(m_pDissolveCom);
	Safe_Release(m_pTargetCom);
	Safe_Release(m_pAnimDriver);
	Safe_Release(m_pEquipmentCom);
	Safe_Release(m_pStatCom);
	Safe_Release(m_pStatusCom);
	Safe_Release(m_pMovementCom);
	m_pGameInstance->Set_FollowTarget(nullptr);
	Safe_Release(m_pSpringArm);
	Safe_Release(m_pInputCom);

	m_pRigidActor = nullptr;
	Safe_Release(m_pThrowBody);
	Safe_Release(m_pCCT);

	Safe_Release(m_pAnimator);
	Safe_Release(m_pInventory);
	Safe_Release(m_pPartSet);

	//MasterMode == Model이라 Model은 이미 상위에서 해제됨 
	m_pMasterModelCom = nullptr;
	__super::Free();

}
