#include "AnimDriver.h"
#include "Animator.h"
#include "InputComponent.h"
#include "Status.h"
#include "StatComponent.h"
#include "Movement.h"
#include "Equipment.h"
#include "TargetComponent.h"
#include "AIObject.h"
#include "Transform.h"
#include "Model.h"

#include "GameInstance.h"
#include "Player.h"
#include "LegionArm_Manager.h"
#include "GrappleGun.h"

#include "CutScene_Data.h"

CAnimDriver::CAnimDriver(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{
}

void CAnimDriver::Ready_ActionTable()
{
	auto RegisterAction = [&](AnimAction eAction, const wchar_t* szActionName, ActionCategory category)
		{
			m_ActionTable[eAction] = { Hash_String(szActionName), category };
		};

	//				AnimAction						/ Animator ActionMap 이름	 / ActionCategory
	RegisterAction(AnimAction::Move,					L"Move",					ActionCategory::Action);
	RegisterAction(AnimAction::Dodge,					L"Dodge",					ActionCategory::Action);
	RegisterAction(AnimAction::Jump,					L"Jump",					ActionCategory::Action);
	RegisterAction(AnimAction::KnockBack,				L"KnockBack",				ActionCategory::Action);
	RegisterAction(AnimAction::Guard,					L"Guard",					ActionCategory::Action);

	RegisterAction(AnimAction::Light,					L"Light",					ActionCategory::Attack);
	RegisterAction(AnimAction::DashLight,				L"DashLight",				ActionCategory::Attack);

	RegisterAction(AnimAction::Heavy,					L"Heavy",					ActionCategory::Attack);
	RegisterAction(AnimAction::DashHeavy,				L"DashHeavy",				ActionCategory::Attack);
	RegisterAction(AnimAction::ChargeHeavy,				L"ChargeHeavy",				ActionCategory::Attack);

	RegisterAction(AnimAction::Fable,					L"Fable",					ActionCategory::Attack);
	RegisterAction(AnimAction::FableEX,					L"FableEX",					ActionCategory::Attack);
	RegisterAction(AnimAction::Fatal,					L"Fatal",					ActionCategory::Attack);

	RegisterAction(AnimAction::Hit,						L"Hit",						ActionCategory::Reaction);
	RegisterAction(AnimAction::Heal,					L"Heal",					ActionCategory::Reaction);
	RegisterAction(AnimAction::Dead,					L"Dead",					ActionCategory::Reaction);

	RegisterAction(AnimAction::Lamp_On,					L"Lamp_On",					ActionCategory::Reaction);
	RegisterAction(AnimAction::Lamp_Off,				L"Lamp_Off",				ActionCategory::Reaction);
	RegisterAction(AnimAction::SwapWeapon,				L"SwapWeapon",				ActionCategory::Reaction);
	RegisterAction(AnimAction::Labber,					L"Labber",					ActionCategory::Reaction);
	RegisterAction(AnimAction::Grapple,					L"Grapple",					ActionCategory::Reaction);
	RegisterAction(AnimAction::Grapple_Upgrade,			L"Grapple_Upgrade",			ActionCategory::Reaction);
	RegisterAction(AnimAction::Grapple_Aim,				L"Grapple_Aim",				ActionCategory::Reaction);

	RegisterAction(AnimAction::Raxasia_Grab,			L"Raxasia_Grab",			ActionCategory::Reaction);

	RegisterAction(AnimAction::ChargeGun,				L"ChargeGun",				ActionCategory::Reaction);

	RegisterAction(AnimAction::Stand_Up,				L"Stand_Up",				ActionCategory::Reaction);

	//Grinder는 BlendPose라 액션 맵없음 .대신 PoseLayer 값으로 사용함
	RegisterAction(AnimAction::TwoHand_Grinder,			L"TwoHand_Grinder",			ActionCategory::Reaction);
	RegisterAction(AnimAction::OneHand_Grinder,			L"OneHand_Grinder",			ActionCategory::Reaction);

	RegisterAction(AnimAction::Blast_Launch,			L"Blast_Launch",			ActionCategory::Reaction);

	/* Ladder */
	RegisterAction(AnimAction::Ladder_StartDown,		L"Ladder_StartDown",		ActionCategory::Reaction);
	RegisterAction(AnimAction::Ladder_StartUp,			L"Ladder_StartUp",			ActionCategory::Reaction);

	RegisterAction(AnimAction::Ladder_UpLeft,			L"Ladder_UpLeft",			ActionCategory::Reaction);
	RegisterAction(AnimAction::Ladder_UpRight,			L"Ladder_UpRight",			ActionCategory::Reaction);
	
	RegisterAction(AnimAction::Ladder_DownLeft,			L"Ladder_DownLeft",			ActionCategory::Reaction);
	RegisterAction(AnimAction::Ladder_DownRight,		L"Ladder_DownRight",		ActionCategory::Reaction);
	
	RegisterAction(AnimAction::Ladder_IdleLeft,			L"Ladder_IdleLeft",			ActionCategory::Reaction);
	RegisterAction(AnimAction::Ladder_IdleRight,		L"Ladder_IdleRight",		ActionCategory::Reaction);
	
	RegisterAction(AnimAction::Ladder_EndUpLeft,		L"Ladder_EndUpLeft",		ActionCategory::Reaction);
	RegisterAction(AnimAction::Ladder_EndUpRight,		L"Ladder_EndUpRight",		ActionCategory::Reaction);
	
	RegisterAction(AnimAction::Ladder_EndDownLeft,		L"Ladder_EndDownLeft",		ActionCategory::Reaction);
	RegisterAction(AnimAction::Ladder_EndDownRight,		L"Ladder_EndDownRight",		ActionCategory::Reaction);

	RegisterAction(AnimAction::Ladder_ExitIdle,			L"Ladder_ExitIdle",			ActionCategory::Reaction);
	RegisterAction(AnimAction::Ladder_Activate,			L"Ladder_Activate",			ActionCategory::Reaction);

	/* Activate */
	RegisterAction(AnimAction::Stargazer_Active,		L"Stargazer_Active",		ActionCategory::Reaction); // 활성화 별바라기
	RegisterAction(AnimAction::Stargazer_InActive,		L"Stargazer_InActive",		ActionCategory::Reaction); // 비활성화 별바라기 

	RegisterAction(AnimAction::Item_Grab,				L"Item_Grab",				ActionCategory::Reaction);
	RegisterAction(AnimAction::Elevator_Activate,		L"Elevator_Activate",		ActionCategory::Reaction);

	RegisterAction(AnimAction::Open_Chest,				L"Open_Chest",				ActionCategory::Reaction);

	RegisterAction(AnimAction::Door_OpenCheck,			L"Door_Check",				ActionCategory::Reaction);
	RegisterAction(AnimAction::Door_Unlock,				L"Door_Key_Unlock",			ActionCategory::Reaction);
	RegisterAction(AnimAction::Open_SlidingDoor,		L"Open_SlidingDoor",		ActionCategory::Reaction);
	RegisterAction(AnimAction::Open_DoubleDoor_Big,		L"Open_DoubleDoor_Big",		ActionCategory::Reaction);
	RegisterAction(AnimAction::Open_DoubleDoor_Boss,	L"Open_DoubleDoor_Boss",	ActionCategory::Reaction);
	RegisterAction(AnimAction::Open_DoubleDoor_Hotel,	L"Open_DoubleDoor_Hotel",	ActionCategory::Reaction);
	RegisterAction(AnimAction::Open_DoubleDoor_Slide,	L"Open_DoubleDoor_Slide",	ActionCategory::Reaction);
	RegisterAction(AnimAction::Crash_BossDoor,			L"Crash_BossDoor",			ActionCategory::Reaction);
	RegisterAction(AnimAction::Open_Sophia_Door,		L"Open_Sophia_Door",		ActionCategory::Reaction);

	/* Game */
	RegisterAction(AnimAction::Restart,					L"Restart",					ActionCategory::Reaction); // Teleport End 와 같은 애니메이션 
	RegisterAction(AnimAction::Teleport_Start,			L"Teleport_Start",			ActionCategory::Reaction);

	/* Stat Room */
	RegisterAction(AnimAction::StatRoom_Weapon,			L"StatRoom_Weapon",			ActionCategory::Reaction);
	RegisterAction(AnimAction::StatRoom_LegionArm,		L"StatRoom_LegionArm",		ActionCategory::Reaction);

	/* Scene */
	RegisterAction(AnimAction::Sophia_Cine_HandStart,	L"Sophia_Cine_HandStart",	ActionCategory::Reaction);
	RegisterAction(AnimAction::Sophia_Cine_HandEnd,		L"Sophia_Cine_HandEnd",		ActionCategory::Reaction);
	RegisterAction(AnimAction::Sophia_Cine_Idle,		L"Sophia_Cine_Idle",		ActionCategory::Reaction);
	RegisterAction(AnimAction::Sophia_Cine_Walk,		L"Sophia_Cine_Walk",		ActionCategory::Reaction);
	RegisterAction(AnimAction::Open_Sophia_Door,		L"Open_Sophia_Door",		ActionCategory::Reaction);
}


HRESULT CAnimDriver::Initialize(const ANIMDRIVER_DESC& Desc)
{
	if (Desc.pAnimator == nullptr || Desc.pInput == nullptr || Desc.pStatus == nullptr)
		return E_FAIL;

	m_pTransformCom = Desc.pTransform;
	m_pAnimatorCom = Desc.pAnimator;
	m_pInputCom = Desc.pInput;
	m_pStatusCom = Desc.pStatus;
	m_pStatCom = Desc.pStat;
	m_pMovementCom = Desc.pMovement;
	m_pEquipmentCom = Desc.pEquipment;
	m_pTargetCom = Desc.pTarget;
	m_pModelCom = Desc.m_pModel;

	Ready_ActionTable();

	return S_OK;
}

void CAnimDriver::Update(_float fTimeDelta)
{
	m_FxEvents.clear();
	m_LegionArmEvents.clear();
	m_PlayerAnimEvents.clear();

	if (m_pAnimatorCom == nullptr) return;

	// 명시적인 컷신 Section은 매니저 시간이 Pose를 직접 평가하고, Idle만 시간에 따라 진행한다.
	if (m_bCutSceneControlled)
	{
		if (0 == m_iCutSceneClipHash)
			m_pAnimatorCom->Update(fTimeDelta);

		return;
	}

	m_bStaminaEmpty = false;

	Drive_Intent(fTimeDelta);
	m_pAnimatorCom->Update(fTimeDelta);
	Apply_Result(fTimeDelta);

	On_Notify();

	Clear_OverlayRequest();
}

void CAnimDriver::Play_Action(AnimAction eAction)
{
	auto iter = m_ActionTable.find(eAction);
	if (iter == m_ActionTable.end()) return;
	
	m_ePending_PlayAction = eAction;
}

void CAnimDriver::Play_HitAction(HIT_TYPE eHitType, HIT_DIR eHitDir, REACT_TYPE eReact)
{
	m_bHitRequested = true;
	m_ePendingHitType = eHitType;
	m_ePendingHitDir = eHitDir;
	m_ePendingReactType = eReact;
}

_bool CAnimDriver::Is_GrappleActive() const
{
	const ANIM_CONTROLLER_STATE* pState = m_pAnimatorCom->Get_CurControllerState();
	if (pState == nullptr)
		return false;

	for (_uint64 iHash : GRAPPLE_STATE_HASHES)
		if (pState->iNameHash == iHash)
			return true;
	return false;
}

_bool CAnimDriver::Is_ActionActive(AnimAction eAction) const
{
	const ANIM_CONTROLLER_STATE* pState = m_pAnimatorCom->Get_CurControllerState();
	if (pState == nullptr) return false;

	auto iter = m_ActionTable.find(eAction);
	if (iter == m_ActionTable.end()) return false;

	return (pState->iNameHash == iter->second.iActionHash);
}

_bool CAnimDriver::Is_Locomotion() const
{
	if (m_eCurrentAction != AnimAction::End ) return false;
	return true;
}

void CAnimDriver::Clear()
{
	Reset_CamShakeWindow();
	Stop_AllLoopSounds();

	m_PendingActions.clear();

	m_eCurrentAction = AnimAction::End;
	m_eCurrentAttack = AttackType::End;

	m_ePending_PlayAction = AnimAction::End;

	m_iComboStep = -1;
	m_pCurActionData = nullptr;

	m_bGrindPrev = false;
	
	m_bHitRequested = false;
	m_bCharged = false;
	m_fChargeHoldTime = 0.f;
	m_fGuardElapsed = 0.f;
	m_bCanCamMove = true;
	m_bApproaching = false;
	m_pFatalTarget = nullptr;
	m_FxEvents.clear();
	m_PlayerAnimEvents.clear();
	m_LegionArmEvents.clear();

	Clear_OverlayRequest();
	m_ePendingVerify = PlayerAnimEvent::End;
	m_pAnimatorCom->Clear_Requests();
}

void CAnimDriver::Reset_CamShakeWindow()
{
	mShakeWindowSync.Clear(m_pGameInstance);
}

void CAnimDriver::Drive_LoopSounds()
{
	static const string strLoopPrefix = "LoopSound_";

	if (nullptr == m_pAnimatorCom || nullptr == m_pTransformCom)
	{
		Stop_AllLoopSounds();
		return;
	}

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	unordered_set<_uint64> ActiveKeys;

	for (const ANIM_WINDOW* pWindow : m_pAnimatorCom->Get_ActiveCustomWindows())
	{
		if (nullptr == pWindow || 0 != pWindow->strPayload.rfind(strLoopPrefix, 0))
			continue;

		ActiveKeys.emplace(pWindow->iPayloadHash);

		auto iter = m_LoopSounds.find(pWindow->iPayloadHash);

		if (iter != m_LoopSounds.end())
		{
			if (true == m_pGameInstance->Is_SoundSourcePlaying(iter->second))
			{
				m_pGameInstance->Update_SoundSourcePos(iter->second, vWorldPos);
				continue;
			}

			m_LoopSounds.erase(iter);
		}

		const string strCueKey = pWindow->strPayload.substr(strLoopPrefix.size());
		const _uint hHandle = m_pGameInstance->Play_Cue(strCueKey, SOUND_PLAYER, vWorldPos, 1.f, true);

		if (0 != hHandle)
			m_LoopSounds.emplace(pWindow->iPayloadHash, hHandle);
	}

	for (auto iter = m_LoopSounds.begin(); iter != m_LoopSounds.end(); )
	{
		if (ActiveKeys.find(iter->first) != ActiveKeys.end())
		{
			++iter;
			continue;
		}

		m_pGameInstance->Stop_SoundSource(iter->second);
		iter = m_LoopSounds.erase(iter);
	}
}

void CAnimDriver::Stop_AllLoopSounds()
{
	for (auto& Pair : m_LoopSounds)
		m_pGameInstance->Stop_SoundSource(Pair.second);

	m_LoopSounds.clear();
}

_bool CAnimDriver::Is_FatalStart()
{
	return m_bApproaching;
}

void CAnimDriver::End_InteractLoop()
{
	if (const wchar_t* szLoopParam = Get_InteractLoopParam(m_eCurrentAction))
		Set_Param(szLoopParam, true);
}

void CAnimDriver::Set_Param(const wchar_t* szName, _bool bValue)
{
	m_pAnimatorCom->Set_Bool(szName, bValue);
}

void CAnimDriver::Set_Param(const wchar_t* szName, _int iValue)
{
	m_pAnimatorCom->Set_Int(szName, iValue);
}

void CAnimDriver::Set_Param(const wchar_t* szName, _float fValue)
{
	m_pAnimatorCom->Set_Float(szName, fValue);
}

_uint64 CAnimDriver::Find_HashName(AnimAction eAction)
{
	auto it = m_ActionTable.find(eAction);
	return (it != m_ActionTable.end()) ? it->second.iActionHash : 0;
}

CAIObject* CAnimDriver::Find_FatalPossibleTarget()
{
	if (m_pTargetCom == nullptr || m_pTransformCom == nullptr)
		return nullptr;

	CAIObject* pTarget = m_pTargetCom->Find_FatalTarget();
	if (pTarget == nullptr || pTarget->Get_Transform() == nullptr)
		return nullptr;

	_vector vPlayerPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vMonsterPos = pTarget->Get_Transform()->Get_State(STATE::POSITION);
	_vector vTargetLook = XMVector3Normalize(XMVectorSetY(pTarget->Get_Transform()->Get_State(STATE::LOOK), 0.f));
	_vector vToPlayer = XMVectorSetY(vPlayerPos - vMonsterPos, 0.f);

	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vToPlayer)), 0.f))
		return nullptr;

	vToPlayer = XMVector3Normalize(vToPlayer);

	const _float fDot = XMVectorGetX(XMVector3Dot(vTargetLook, vToPlayer));

	_vector vFatalPoint = {};

	// 몬스터 후면에서 페이탈 가능한지 검사한다.
	if (fDot <= -FATAL_ANGLE_COS && pTarget->Get_FatalPoint_Back(vFatalPoint))
		return pTarget;

	// 몬스터 정면에서 페이탈 가능한지 검사한다.
	if (fDot >= FATAL_ANGLE_COS && pTarget->Get_FatalPoint_Front(vFatalPoint))
		return pTarget;

	return nullptr;
}

AttackType CAnimDriver::Get_AttackType(AnimAction eAction) const
{
	switch (eAction)
	{
	case GameCore::AnimAction::Light:
		return AttackType::Light;

	case GameCore::AnimAction::Heavy:
		return AttackType::Heavy;

	case GameCore::AnimAction::DashLight:
		return AttackType::DashLight;

	case GameCore::AnimAction::DashHeavy:
		return AttackType::DashHeavy;

	case GameCore::AnimAction::ChargeHeavy:
		return AttackType::ChargeHeavy;

	case GameCore::AnimAction::Fable:
		return AttackType::Fable;

	case GameCore::AnimAction::FableEX:
		return AttackType::FableEX;

	case GameCore::AnimAction::Fatal:
		return AttackType::Fatal;

	default:
		break;
	}
	return AttackType::End;
}


const wchar_t* CAnimDriver::Get_InteractLoopParam(AnimAction eAction) const
{
	switch (eAction)
	{
	case AnimAction::Open_Chest:			return PARAM_CHEST_LOOPEND;
	case AnimAction::Stargazer_Active:		return PARAM_STARGAZER_LOOPEND;
	case AnimAction::Stand_Up:				return PARAM_SIT_LOOPEND;
	default:								return nullptr;
	}
}

void CAnimDriver::Set_LadderMode(_bool bLadderMode)
{
	m_bLadderMode = bLadderMode;
	m_ePending_LadderAction = AnimAction::End;
}

void CAnimDriver::Play_LadderAction(AnimAction eAction)
{
	m_ePending_LadderAction = eAction;
}

_bool CAnimDriver::Requst_Overlap(PlayerAnimEvent eAction)
{
	if (m_pAnimatorCom == nullptr)
		return false;

	_wstring strParam = {};
	_uint64 iHash = 0;

	if (!Get_OverlayInfo(eAction, strParam, iHash))
		return false;

	//이전 오버레이 요청이 안끝났으면 무시 
	if (m_ePendingClear != PlayerAnimEvent::End)
		return false;

	if(Is_OverlayPlaying())
		return false;

	m_pAnimatorCom->Set_Bool(strParam, true);

	m_ePendingClear = eAction;
	m_ePendingVerify = eAction;

	return true;
}

_bool CAnimDriver::Is_OverlayPlaying()
{
	if (nullptr == m_pAnimatorCom)
		return false;

	for(_uint i=0; i < ETOUI(PlayerAnimEvent::End); ++i)
	{
		_wstring strParam = {};
		_uint64 iHash = 0;
		if (Get_OverlayInfo(static_cast<PlayerAnimEvent>(i), strParam, iHash) && m_pAnimatorCom->Is_PoseLayerActive(iHash))
			return true;
	}

	return false;
}

_bool CAnimDriver::Get_OverlayInfo(PlayerAnimEvent eAction, _wstring& strOutParam, _uint64& iOutHash)
{
	switch (eAction)
	{
		case PlayerAnimEvent::Heal:
		{
			strOutParam = PARAM_REQUEST_HEAL;
			iOutHash = Find_HashName(AnimAction::Heal);
			break;
		}
		case PlayerAnimEvent::Turn_On_Lamp:
		{
			strOutParam = PARAM_REQUEST_LAMP_ON;
			iOutHash = Find_HashName(AnimAction::Lamp_On);
			break;
		}

		case PlayerAnimEvent::Turn_Off_Lamp:
		{
			strOutParam = PARAM_REQUEST_LAMP_OFF;
			iOutHash = Find_HashName(AnimAction::Lamp_Off);
			break;
		}
		default:
			return false;
	}
	return iOutHash != 0;
}

void CAnimDriver::Verify_OverlayStarted()
{
	if (m_ePendingVerify == PlayerAnimEvent::End)
		return;

	_wstring strParam = {};
	_uint64 iHash = 0;

	if (Get_OverlayInfo(m_ePendingVerify, strParam, iHash) && m_pAnimatorCom->Is_PoseLayerActive(iHash))
	{
		m_PlayerAnimEvents.push_back(m_ePendingVerify);
	}

	m_ePendingVerify = PlayerAnimEvent::End;
}

//처리 확인 후 Clear
void CAnimDriver::Clear_OverlayRequest()
{
	if (m_ePendingClear == PlayerAnimEvent::End)
		return;

	_wstring strParam = {};
	_uint64 iHash = 0;

	if (Get_OverlayInfo(m_ePendingClear, strParam, iHash))
		m_pAnimatorCom->Set_Bool(strParam, false);

	m_ePendingClear = PlayerAnimEvent::End;
}



void CAnimDriver::Begin_CutSceneAnimation()
{
	if (m_bCutSceneControlled || nullptr == m_pAnimatorCom)
		return;

	// 게임 플레이 중 예약된 공격, 피격, 콤보 요청을 제거한다.
	Clear();

	m_bCutSceneControlled = true;
	m_iCutSceneSectionID = UINT_MAX;
	m_iCutSceneClipHash = 0;
	m_fCutSceneLastTime = 0.f;
	m_bCutSceneRootMotionGenerated = false;

	m_pAnimatorCom->Reset_PlaybackState();
	m_pAnimatorCom->Set_ControllerEnabled(false);
	Play_CutSceneIdle();
}

void CAnimDriver::Play_CutSceneIdle()
{
	if (nullptr == m_pAnimatorCom)
		return;

	m_pAnimatorCom->Reset_PlaybackState();
	m_pAnimatorCom->Set_ControllerEnabled(false);
	m_iCutSceneClipHash = 0;

	if (FAILED(m_pAnimatorCom->Play_Clip(Hash_String(m_strCutSceneIdleClip.c_str()))))
		m_pAnimatorCom->Play_Default();

	m_pAnimatorCom->Set_Loop(true);
	m_pAnimatorCom->Set_PlayRate(1.f);
	m_pAnimatorCom->Set_UseRootMotion(false);
}

_bool CAnimDriver::Apply_CutSceneAnimation(const CUTSCENE_ANIMATION_SECTION* pSection, _float fCutSceneTime, _bool bSeek)
{
	m_bCutSceneRootMotionGenerated = false;

	if (nullptr == m_pAnimatorCom)
		return false;

	if (false == m_bCutSceneControlled)
		Begin_CutSceneAnimation();

	if (nullptr == pSection || pSection->strClipName.empty())
	{
		if (UINT_MAX != m_iCutSceneSectionID)
		{
			m_iCutSceneSectionID = UINT_MAX;
			Play_CutSceneIdle();
		}

		m_fCutSceneLastTime = fCutSceneTime;
		return false;
	}

	const _bool bSectionChanged = m_iCutSceneSectionID != pSection->iSectionID;

	if (bSectionChanged)
	{
		const _uint64 iAnimationHash = Hash_String(pSection->strClipName.c_str());
		auto ActionIter = find_if(m_ActionTable.begin(), m_ActionTable.end(), [iAnimationHash](const pair<const AnimAction, ACTION_ENTRY>& Pair)
			{
				return Pair.second.iActionHash == iAnimationHash;
			});

		m_iCutSceneSectionID = pSection->iSectionID;

		m_pAnimatorCom->Reset_PlaybackState();

		if (m_ActionTable.end() != ActionIter)
		{
			m_pAnimatorCom->Set_ControllerEnabled(true);

			// 등록된 게임 플레이 Action은 기존 Controller 전환 경로를 그대로 사용한다.
			m_pAnimatorCom->Update(0.f);
			m_pAnimatorCom->Request_Action(ActionIter->second.iActionHash);
			m_pAnimatorCom->Update(0.f);

			if (false == m_pAnimatorCom->Was_ActionAcceptedThisFrame() || m_pAnimatorCom->Get_AcceptedActionHash() != ActionIter->second.iActionHash)
			{
				const _wstring strMessage = L"[CutScene Player] Animation action was not accepted: " + pSection->strClipName + L"\n";
				OutputDebugStringW(strMessage.c_str());
				Play_CutSceneIdle();
				m_fCutSceneLastTime = fCutSceneTime;
				return false;
			}

			// Controller가 선택한 State와 Clip은 유지하고 컷신 절대 시간 평가 중 자동 전환만 막는다.
			m_pAnimatorCom->Set_ControllerEnabled(false);
			m_iCutSceneClipHash = ActionIter->second.iActionHash;
		}
		else
		{
			// 컷신 전용 AS_*처럼 ActionTable에 없는 실제 Clip은 Controller를 거치지 않고 직접 재생한다.
			m_pAnimatorCom->Set_ControllerEnabled(false);

			if (FAILED(m_pAnimatorCom->Play_Clip(iAnimationHash)))
			{
				const _wstring strMessage = L"[CutScene Player] Animation action or clip was not found: " + pSection->strClipName + L"\n";
				OutputDebugStringW(strMessage.c_str());
				Play_CutSceneIdle();
				m_fCutSceneLastTime = fCutSceneTime;
				return false;
			}

			m_iCutSceneClipHash = iAnimationHash;
		}

		m_pAnimatorCom->Set_Loop(pSection->bLoop);
		m_pAnimatorCom->Set_PlayRate(1.f);
		// Runtime Player Section만 명시적으로 Root Motion 권한을 가질 수 있다.
		m_pAnimatorCom->Set_UseRootMotion(pSection->bUseRootMotion);
	}

	const _float fClipDuration = max(0.f, m_pAnimatorCom->Get_CurClipDuration());
	const _float fClipStartTime = min(max(0.f, pSection->fClipStartTime * 1000.f), fClipDuration);
	_float fClipEndTime = fClipDuration;

	if (pSection->fClipEndTime > pSection->fClipStartTime)
		fClipEndTime = min(pSection->fClipEndTime * 1000.f, fClipDuration);

	const _float fClipLength = max(0.f, fClipEndTime - fClipStartTime);
	const _float fPlayRate = max(0.0001f, pSection->fPlayRate);
	_float fLocalTime = fClipStartTime + max(0.f, fCutSceneTime - pSection->fStartTime) * fPlayRate * 1000.f;

	if (pSection->bLoop && fClipLength > FLT_EPSILON)
		fLocalTime = fClipStartTime + fmodf(max(0.f, fLocalTime - fClipStartTime), fClipLength);
	else
		fLocalTime = min(fLocalTime, fClipEndTime);

	if (false == pSection->bUseRootMotion)
	{
		// Root Motion을 사용하지 않는 Section은 기존처럼 컷신 절대 시간으로 Pose만 평가한다.
		m_pAnimatorCom->Set_LocalTime(fLocalTime);
		m_fCutSceneLastTime = fCutSceneTime;
		return false;
	}

	if (bSeek || bSectionChanged || fCutSceneTime < m_fCutSceneLastTime)
	{
		// Seek 또는 Section 진입 시 기준 Pose에서 현재 시간까지 한 번 평가해 누적 Root Delta를 만든다.
		m_pAnimatorCom->Set_LocalTime(fClipStartTime);
		const _float fAdvanceTime = max(0.f, fLocalTime - fClipStartTime) * 0.001f;

		if (fAdvanceTime > FLT_EPSILON)
			m_pAnimatorCom->Update(fAdvanceTime);

		m_fCutSceneLastTime = fCutSceneTime;
		m_bCutSceneRootMotionGenerated = fAdvanceTime > FLT_EPSILON;
		return m_bCutSceneRootMotionGenerated;
	}

	// 연속 재생은 지난 컷신 시간만큼만 전진시켜 이번 프레임의 Root Delta를 생성한다.
	const _float fAdvanceTime = max(0.f, fCutSceneTime - m_fCutSceneLastTime) * fPlayRate;

	if (fAdvanceTime > FLT_EPSILON)
		m_pAnimatorCom->Update(fAdvanceTime);

	m_fCutSceneLastTime = fCutSceneTime;
	m_bCutSceneRootMotionGenerated = fAdvanceTime > FLT_EPSILON;
	return m_bCutSceneRootMotionGenerated;
}

_bool CAnimDriver::Get_CutSceneRootMotionDelta(_float3* pTranslation, _float4* pRotation) const
{
	if (false == m_bCutSceneRootMotionGenerated || nullptr == m_pAnimatorCom || nullptr == pTranslation || nullptr == pRotation)
		return false;

	// Player 내부를 건드리지 않고 CutScene Manager가 현재 Animator의 Root Delta만 가져갈 수 있게 한다.
	const ANIM_ROOT_MOTION_DELTA& RootDelta = m_pAnimatorCom->Get_RootMotionDelta();
	*pTranslation = RootDelta.vTranslation;
	*pRotation = RootDelta.vRotation;
	return true;
}

void CAnimDriver::Clear_CutSceneAnimation()
{
	if (false == m_bCutSceneControlled || nullptr == m_pAnimatorCom)
		return;

	m_bCutSceneControlled = false;
	m_iCutSceneSectionID = UINT_MAX;
	m_iCutSceneClipHash = 0;
	m_fCutSceneLastTime = 0.f;
	m_bCutSceneRootMotionGenerated = false;

	m_pAnimatorCom->Reset_PlaybackState();
	m_pAnimatorCom->Set_ControllerEnabled(true);
	Clear();
}

void CAnimDriver::Set_CurrentAction(AnimAction eNext)
{
	if (m_eCurrentAction == AnimAction::Fatal && eNext != AnimAction::Fatal)
		Cancel_Fatal();

	m_eCurrentAction = eNext;
}

void CAnimDriver::Set_LegionArmManager(CLegionArm_Manager* pLegionArm)
{
	m_pLegionArmCom = pLegionArm;
}


void CAnimDriver::Request_OnceAction(AnimAction eAction)
{
	if (eAction == AnimAction::End) return;
	const ACTION_ENTRY& eEntry = m_ActionTable[eAction];
	
	if (eEntry.iActionHash == 0) return;
	if (!Check_StaminaCost()) return;

	m_PendingActions[eEntry.iActionHash] = { eAction, ACTION_STAMINA_COST, -1 };
	m_pAnimatorCom->Request_Action(eEntry.iActionHash);
}

void CAnimDriver::Request_Attack(AnimAction eAction)
{
	if (eAction == AnimAction::End) return;

	_int iNextStep = (m_eCurrentAction == eAction) ? m_iComboStep + 1 : 0;

	const ACTION_ENTRY& eEntry = m_ActionTable[eAction];
	if (eEntry.iActionHash == 0 ) return;
	if (!Check_StaminaCost()) return;
	if (!Check_FableCost(eAction)) return;


	const AttackType eAttackType = Get_AttackType(eAction);
	const ActionData* pActionData = m_pEquipmentCom->Get_ActionData(eAttackType, iNextStep);

	if (pActionData == nullptr			// 마지막 공격이후
		&& eAction == m_eCurrentAction  // 같은 입력
		&& m_pAnimatorCom->Can_ComboInput()) // 콤보윈도우라면 1타로 회귀한다
	{
		iNextStep = 0;
		pActionData = m_pEquipmentCom->Get_ActionData(eAttackType, iNextStep);
	}

	if (pActionData == nullptr) return;

	m_PendingActions[eEntry.iActionHash] = { eAction, pActionData->fStaminaCost, iNextStep };

	m_pAnimatorCom->Set_Int(PARAM_COMBO, iNextStep);
	m_pAnimatorCom->Request_Action(eEntry.iActionHash);
}

void CAnimDriver::Request_Reaction(AnimAction eAction)
{
	if (eAction == AnimAction::End) return;

	const ACTION_ENTRY& eEntry = m_ActionTable[eAction];
	if (eEntry.iActionHash == 0) return;

	m_PendingActions[eEntry.iActionHash] = { eAction, 0.f, -1 };
	m_pAnimatorCom->Request_Action(eEntry.iActionHash);
}

void CAnimDriver::Drive_Intent(_float fTimeDelta)
{
	m_PendingActions.clear();

	if (m_pAnimatorCom->Is_Ended())
	{
		if (m_eCurrentAction == AnimAction::SwapWeapon)
			m_pEquipmentCom->Swap_End();

		m_iComboStep = -1;
		m_eCurrentAttack = AttackType::End;
		Set_CurrentAction(AnimAction::End);

		m_fChargeHoldTime = 0.f;
		m_bCharged = false;

		Set_Param(PARAM_CHEST_LOOPEND, false);
		Set_Param(PARAM_SIT_LOOPEND, false);
		Set_Param(PARAM_STARGAZER_LOOPEND, false);
		Set_Param(PARAM_CHARGE, false);
	}
	
	if (m_ePending_PlayAction != AnimAction::End)
	{
		const AnimAction ePlay = m_ePending_PlayAction;
		m_ePending_PlayAction = AnimAction::End;

		if (ePlay == AnimAction::Stargazer_Active && m_pStatCom)
			m_pStatCom->Clear();

		if (const wchar_t* szLoopParam = Get_InteractLoopParam(ePlay))
			Set_Param(szLoopParam, false);

		switch (m_ActionTable[ePlay].eCategory)
		{
		case ActionCategory::Attack:	Request_Attack(ePlay);		break;
		case ActionCategory::Action:	Request_OnceAction(ePlay);	break;
		case ActionCategory::Reaction:	Request_Reaction(ePlay);	break;
		default: break;
		}
	}



	/* Set Animator Parameter */
	m_pAnimatorCom->Set_Bool(PARAM_HIT, false);
	m_pAnimatorCom->Set_Int(PARAM_HITTYPE, ETOUI(HIT_TYPE::End));
	m_pAnimatorCom->Set_Int(PARAM_HITDIR, ETOUI(HIT_DIR::End));

	//WeaponID = 무기 종류 -> 한 무기타입에 여러 종류의 무기가 생기면 추가하기 
	//m_pAnimatorCom->Set_Int(PARAM_WEAPONID, ETOUI(m_pEquipmentCom->Get_WeaponTypeID()));
	
	//WeaponType = 무기 카테고리(unarmed, 한손, 양손, 창 등)	
	m_pAnimatorCom->Set_Int(PARAM_WEAPONTYPE, ETOUI(m_pEquipmentCom->Get_WeaponType()));
	
	// LegionArm Type 
	if (m_pLegionArmCom)
	{
		m_pAnimatorCom->Set_Int(PARAM_LEGIONARM_TYPE, ETOUI(m_pLegionArmCom->Get_CurrentArmType()));
	}

	//락온상탠
	m_pAnimatorCom->Set_Bool(PARAM_LOCKON, m_pStatusCom->Is_LockedOn());
	
	m_pAnimatorCom->Set_Bool(PARAM_GROUNDED, m_pStatusCom->Is_Grounded());
	m_pAnimatorCom->Set_Bool(PARAM_FALLING, m_pMovementCom->Is_AirActive());
	if (!m_pMovementCom->Is_AirActive())
		m_pAnimatorCom->Set_Bool(PARAM_GRAPPLE_ZIPFALL, false);

	//Input 방향  
	_int iRawX = m_pInputCom->Get_RawAxisX();
	_int iRawY = m_pInputCom->Get_RawAxisY();

	DIR8 eDir8 = Compute_InputDir8(iRawX, iRawY);
	m_pAnimatorCom->Set_Int(PARAM_INPUTDIR, eDir8);

	DIRECTION eDir = Compute_Direction(iRawX, iRawY);
	m_pAnimatorCom->Set_Int(PARAM_MOVEDIR, ETOUI(eDir));

	if (m_bHitRequested) 
		Cancel_Fatal();

	/*Hit */
	if (Drive_Reaction(fTimeDelta)) 
		return;
	/*Ladder*/
	 
	if (m_bLadderMode)
	{
		if (AnimAction::End != m_ePending_LadderAction)
		{
			Request_Reaction(m_ePending_LadderAction);
			m_ePending_LadderAction = AnimAction::End;
		}
		return;
	}

	_bool bGrappleAiming = false;
	CGrappleGun* pGrappleGun = nullptr;
	if(m_pLegionArmCom) 
		pGrappleGun = m_pLegionArmCom->Get_GrappleGun();

	if(pGrappleGun)
		bGrappleAiming = (pGrappleGun->Get_GrappleState() == GrappleState::Aim);

	//공중이거나 Aim중이면 새 액션을 받지않는다
	if (m_pMovementCom->Is_AirActive() || bGrappleAiming)
	{
		m_fChargeHoldTime = 0.f;
		m_bCharged = false;
		m_pAnimatorCom->Set_Bool(PARAM_CHARGE, false);

		m_pAnimatorCom->Set_Float(PARAM_SPEED, 0.f);
		m_pAnimatorCom->Set_Bool(PARAM_SPRINT, false);
		m_pAnimatorCom->Set_Bool(PARAM_JUMP, false);
		m_pAnimatorCom->Set_Bool(PARAM_DODGE, false);
		m_pAnimatorCom->Set_Bool(PARAM_GUARD, false);
		m_pAnimatorCom->Set_Bool(PARAM_GRINDER, false);
		//다른 모션 나오지않도록 
		return;
	}

	if (Drive_Fatal(fTimeDelta))
	{
		m_bGuardPrev = false;
		return;
	}


	Drive_Locomotion(fTimeDelta);
	Drive_Actions(fTimeDelta);
	Drive_Stance(fTimeDelta);

}

void CAnimDriver::Apply_Result(_float fTimeDelta)
{
	Apply_GameplayFlags();
	Apply_ActionResult();

	_bool bGuard = m_pAnimatorCom->Get_GameplayFlags().bGuardActive;
	if (bGuard)
	{
		if (!m_bGuardPrev) m_fGuardElapsed = 0.f;
		else m_fGuardElapsed += fTimeDelta;
	}

	m_pStatusCom->Set_GuardActive(bGuard);
	m_pStatusCom->Set_ParryActive(bGuard && m_fGuardElapsed <= PARRY_WINDOW);
	m_bGuardPrev = bGuard;


	_bool bGrind = m_pAnimatorCom->Is_PoseLayerLoop(Find_HashName(AnimAction::TwoHand_Grinder));
	bGrind |= m_pAnimatorCom->Is_PoseLayerLoop(Find_HashName(AnimAction::OneHand_Grinder));

	if(bGrind && !m_bGrindPrev) m_FxEvents.push_back({ PLAYER_FX::GrindLoop, true});
	else if (!bGrind && m_bGrindPrev) m_FxEvents.push_back({ PLAYER_FX::GrindLoop, false });
	m_bGrindPrev = bGrind;
}

void CAnimDriver::Drive_ChargeHeavy(_float fTimeDelta)
{
	if (m_pInputCom->IsHold(PlayerActionID::Heavy_Attack))
	{
		//m_pInputCom->Consume(PlayerActionID::Heavy_Attack);
		m_fChargeHoldTime += fTimeDelta;
		if (m_fChargeHoldTime >= CHARGE_HOLD_TIME)
		{
			//바로 차지공격
			if (!m_bCharged && Can_Accept_Attack(AnimAction::ChargeHeavy))
			{
				m_bCharged = true;
				m_pAnimatorCom->Set_Bool(PARAM_CHARGE, true);
				Request_Attack(AnimAction::ChargeHeavy);
			}
		}
		return;
	}

	if (m_fChargeHoldTime > 0.f)
	{
		AnimAction eAttack = AnimAction::End;
		if (m_fChargeHoldTime >= CHARGE_HOLD_TIME) eAttack = AnimAction::ChargeHeavy;
		else if(m_bSprint) eAttack = AnimAction::DashHeavy;
		else eAttack = AnimAction::Heavy;
		
		if (Can_Accept_Attack(eAttack)) Request_Attack(eAttack);

		m_fChargeHoldTime = 0.f;
		m_bCharged = false;
		m_pAnimatorCom->Set_Bool(PARAM_CHARGE, false);
	}

}

_bool CAnimDriver::Drive_Fatal(_float fTimeDelta)
{
	//접근 중이라면 
	if (m_bApproaching)
	{
		const _bool bMoveCanceled = m_pMovementCom->Is_MoveActive() == false && m_pMovementCom->Is_MoveReached() == false;

		if (m_pFatalTarget == nullptr || m_pFatalTarget->Is_Dead() || bMoveCanceled)
		{
			Cancel_Fatal();
			return false;
		}

		if(m_pMovementCom->Is_MoveReached())
		{
			//TODO : 몬스터 Fatal 이벤트 호출 
			Request_Attack(AnimAction::Fatal);
			//Cancel_Fatal();
			//Camera Work때문에 FatalTarget을 남겨놔야하여 End로 변경 
			End_FatalApproach();
		}

		return true;
	}

	if (false == m_pInputCom->Peek(PlayerActionID::Fatal)) return false;
	if (false == Can_Start_Action(AnimAction::Fatal)) return false;
	m_pInputCom->Consume(PlayerActionID::Fatal);

	CAIObject* pTarget = m_pTargetCom->Find_FatalTarget();
	if (pTarget == nullptr) return false;

	_vector vPlayerPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vMonsterPos = pTarget->Get_Transform()->Get_State(STATE::POSITION);
	_vector vTargetLook = XMVector3Normalize(XMVectorSetY(pTarget->Get_Transform()->Get_State(STATE::LOOK), 0.f));
	_vector vToPlayer = XMVectorSetY(vPlayerPos - vMonsterPos, 0.f);

	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vToPlayer)), 0.f)) return false;
	vToPlayer = XMVector3Normalize(vToPlayer);

	_float fDot = XMVectorGetX(XMVector3Dot(vTargetLook, vToPlayer));
	
	_vector vPoint = {};
	_bool bCanFatal = {};

	//후면 
	if (fDot <= -FATAL_ANGLE_COS)
	{
		bCanFatal = pTarget->Get_FatalPoint_Back(vPoint);
	}
	//정면 
	else if (fDot >= FATAL_ANGLE_COS)
	{
		bCanFatal = pTarget->Get_FatalPoint_Front(vPoint);
	}

	if (!bCanFatal) return false;

	_vector vFaceDir = XMVectorSetY(vMonsterPos - vPlayerPos, 0.f);
	_vector vFaceQuat = m_pTransformCom->Get_Quat();
	if (false == IsEqual(XMVectorGetX(XMVector3LengthSq(vFaceDir)), 0.f))
	{
		vFaceDir = XMVector3Normalize(vFaceDir);
		vFaceQuat = XMQuaternionRotationRollPitchYaw(0.f, atan2f(XMVectorGetX(vFaceDir), XMVectorGetZ(vFaceDir)), 0.f);
	}

	_vector vCurLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
	_float  fAngle = acosf(min(max(XMVectorGetX(XMVector3Dot(vCurLook, vFaceDir)), -1.f), 1.f));
	_float  fDist = XMVectorGetX(XMVector3Length(XMVectorSetY(vPoint - vPlayerPos, 0.f)));
	_float  fDur = max(fDist / FATAL_APPROACH_SPEED, fAngle / FATAL_TURN_SPEED);

	m_pFatalTarget = pTarget;
	m_bApproaching = true;
	m_pStatusCom->Set_InputLock(true);
	m_pMovementCom->Begin_MoveTo(vPoint, vFaceQuat, fDur);
	return true;
}

void CAnimDriver::Drive_Locomotion(_float fTimeDelta)
{
	//초기화 
	m_pAnimatorCom->Set_Float(PARAM_SPEED, 0.f);
	m_pAnimatorCom->Set_Bool(PARAM_SPRINT, false);

	if (m_pInputCom->Has_MoveInput() == false) return;

	m_bSprint = m_pInputCom->IsHold(PlayerActionID::Run);
	_float fSpeed = m_pMovementCom->Get_CurrentSpeed();

	m_pAnimatorCom->Set_Float(PARAM_SPEED, fSpeed);
	m_pAnimatorCom->Set_Bool(PARAM_SPRINT, m_bSprint);
	
	m_pAnimatorCom->Request_Action(Find_HashName(AnimAction::Move));
}

void CAnimDriver::Drive_Actions(_float fTimeDelta)
{
	_bool bJump = false; 
	if (m_pInputCom->Peek(PlayerActionID::Jump) && Can_Start_Action(AnimAction::Jump))
	{
		bJump = true;
		m_pInputCom->Consume(PlayerActionID::Jump);
		Request_OnceAction(AnimAction::Jump);
	}
	m_pAnimatorCom->Set_Bool(PARAM_JUMP, bJump);

	_bool bDodge = false;
	if (m_pInputCom->Peek(PlayerActionID::Dodge) && Can_Start_Action(AnimAction::Dodge))
	{
		bDodge = true;
		m_pInputCom->Consume(PlayerActionID::Dodge);
		Request_OnceAction(AnimAction::Dodge);
	}
	m_pAnimatorCom->Set_Bool(PARAM_DODGE, bDodge);

	if (m_pInputCom->Peek(PlayerActionID::SwapWeapon) && Can_Start_Action(AnimAction::SwapWeapon))
	{
		if (m_pEquipmentCom->Can_Swap())
		{
			m_pInputCom->Consume(PlayerActionID::SwapWeapon);
			m_pEquipmentCom->Swap_NextWeapon();

			m_pAnimatorCom->Set_Int(PARAM_WEAPONTYPE, ETOUI(m_pEquipmentCom->Get_WeaponType()));

			Request_Reaction(AnimAction::SwapWeapon);
			return;
		}
	}

	if(m_bSprint)
		Try_Attack(PlayerActionID::Light_Attack, AnimAction::DashLight);
	else 
		Try_Attack(PlayerActionID::Light_Attack, AnimAction::Light);

	Drive_ChargeHeavy(fTimeDelta);
	
	Try_Attack(PlayerActionID::Fable_EX, AnimAction::FableEX);
	Try_Attack(PlayerActionID::Fable_Arts, AnimAction::Fable);

	/* Heal */
	if (m_pInputCom->Peek(PlayerActionID::Item_Use) && 
		m_eItemSlot == ITEM_SLOT::Heal && 
		m_pStatCom->Has_Potion())
	{
		if (Requst_Overlap(PlayerAnimEvent::Heal))
			m_pInputCom->Consume(PlayerActionID::Item_Use);
	}


}

void CAnimDriver::Drive_Stance(_float fTimeDelta)
{
	_bool bGuard = m_pInputCom->IsHold(PlayerActionID::Guard);

	m_pAnimatorCom->Set_Bool(PARAM_GUARD, bGuard);
	if(bGuard)
		m_pAnimatorCom->Request_Action(Find_HashName(AnimAction::Guard));

	_bool bGrinder = (m_eItemSlot == ITEM_SLOT::Grinder) && m_pInputCom->IsHold(PlayerActionID::Item_Use);
	m_pAnimatorCom->Set_Bool(PARAM_GRINDER, bGrinder);
}

_bool CAnimDriver::Drive_Reaction(_float fTimeDelta)
{
	if (!m_bHitRequested) return false;

	m_pAnimatorCom->Set_Bool(PARAM_HIT, true);
	m_pAnimatorCom->Set_Int(PARAM_HITDIR, ETOUI(m_ePendingHitDir));
	m_pAnimatorCom->Set_Int(PARAM_HITTYPE, ETOUI(m_ePendingHitType));
	m_pAnimatorCom->Set_Int(PARAM_REACT, ETOUI(m_ePendingReactType));


	m_iComboStep = -1;
	m_eCurrentAttack = AttackType::End;
	Set_CurrentAction(AnimAction::End);

	m_pAnimatorCom->Request_Action(Find_HashName(AnimAction::Hit));
	m_bHitRequested = false;
	return true;
}

void CAnimDriver::Apply_GameplayFlags()
{
	const ANIM_GAMEPLAY_FLAGS& Flags = m_pAnimatorCom->Get_GameplayFlags();

	m_pStatusCom->Set_MovementLock(!Flags.bCanMove);
	m_pStatusCom->Set_RotationLock(!Flags.bCanRotate);
	m_pStatusCom->Set_CanTargetTracking(Flags.bCanTargetTrack);
	m_pStatusCom->Set_Invincible(Flags.bInvincible);
	m_pStatusCom->Set_SuperArmor(Flags.bSuperArmor);
	m_pStatusCom->Set_HitActive(Flags.bHitActive);

	if (!m_bLadderMode)
		m_pStatusCom->Set_GravityDisabled(m_pAnimatorCom->Is_InCustomWindow(L"NoGravity"));

	_bool bMoveLock = m_pAnimatorCom->Is_InCustomWindow(L"CameraLock");
	//움직임 락 
	if (bMoveLock)
	{
		if (m_bCanCamMove)
		{
			m_bCanCamMove = false;
			m_pGameInstance->Set_CameraLock(bMoveLock);
		}
	}
	else
	{
		if (!m_bCanCamMove)
		{
			m_bCanCamMove = true;
			m_pGameInstance->Set_CameraLock(bMoveLock);
		}
	}

	mShakeWindowSync.Update(m_pAnimatorCom, m_pGameInstance);
	Drive_LoopSounds();
}

void CAnimDriver::Apply_ActionResult()
{
	//PoseLayer 
	Verify_OverlayStarted();

	if (m_pAnimatorCom->Was_ActionAcceptedThisFrame() == false) return;
	
	//Hit는 PendingAction이 없어서 여기서 처리 
	//현재 수락된 애니메이션의 ActionMap 데이터 반환 
	const _uint64 iAcceptedHash = m_pAnimatorCom->Get_AcceptedActionHash();

	/* Grapple Animation Accepted*/
	if (iAcceptedHash == Find_HashName(AnimAction::Grapple) || iAcceptedHash == Find_HashName(AnimAction::Grapple_Upgrade))
	{
		m_LegionArmEvents.push_back(LegionArmEvent::Grapple_Accepted);
	}

	if (iAcceptedHash == Find_HashName(AnimAction::Grapple_Aim))
	{
		m_LegionArmEvents.push_back(LegionArmEvent::Grapple_AimAccepted);
	}

	/* ChargeGun AnimationAccepted*/
	if (iAcceptedHash == Find_HashName(AnimAction::ChargeGun))
	{
		m_LegionArmEvents.push_back(LegionArmEvent::ChargeGun_Accepted);
	}

	/* Hit Camera Shake */
	if (iAcceptedHash == Find_HashName(AnimAction::Hit))
	{
		switch (m_ePendingReactType)
		{
		case GameCore::REACT_TYPE::Hit:
			m_pGameInstance->Play_Shake(CAM_SHAKE_HIT_NORMAL);
			break;
		case GameCore::REACT_TYPE::Guard:
			m_pGameInstance->Play_Shake(CAM_SHAKE_GUARD_NORMAL);
			break;
		case GameCore::REACT_TYPE::GuardHeavy:
			m_pGameInstance->Play_Shake(CAM_SHAKE_GUARD_STRONG);
			break;
		case GameCore::REACT_TYPE::GuardBreak:
			m_pGameInstance->Play_Shake(CAM_SHAKE_GUARD_BREAK);
			break;
		case GameCore::REACT_TYPE::KnockBack:
			m_pGameInstance->Play_Shake(CAM_SHAKE_HIT_KnockBack);
			break;
		case GameCore::REACT_TYPE::Normal_UnGuard:
		case GameCore::REACT_TYPE::Heavy_UnGuard:
			m_pGameInstance->Play_Shake(CAM_SHAKE_GUARD_STRONG);
			break;
		case GameCore::REACT_TYPE::End:
			break;
		default:
			break;
		}
	}

	// 같은 스테이트 재진입으로 애니가 바뀌지 않은 경우는 액션이 바뀌지 않음
	if (m_pAnimatorCom->Was_StateChangedThisFrame() == false) return;

	m_pMovementCom->Clear_RootMotionScale();

	auto it = m_PendingActions.find(iAcceptedHash);
	if (it == m_PendingActions.end()) // Auto 상태
	{
		m_eCurrentAttack = AttackType::End;
		m_iComboStep = -1;
		Set_CurrentAction(AnimAction::End);
		return;
	}

	m_pMovementCom->Restart_RootMotion();

	const PendingAction& pending = it->second;
	
	if (m_pStatCom && pending.fStaminaCost>0.f)
		m_pStatCom->Consume_Stamina(pending.fStaminaCost);

	m_eCurrentAction = pending.eAction;
	m_iComboStep = pending.iComboStep;
	m_eCurrentAttack = Get_AttackType(m_eCurrentAction);

	m_pEquipmentCom->Set_AttackContext(m_eCurrentAttack, m_iComboStep);

	if(m_ActionTable.find(m_eCurrentAction) != m_ActionTable.end())
	{
		const ACTION_ENTRY& eEntry = m_ActionTable[m_eCurrentAction];
		if (eEntry.eCategory == ActionCategory::Attack)
		{
			m_eCurrentAttack = Get_AttackType(m_eCurrentAction);
			if (pending.eAction == AnimAction::Fable)
			{
				if (m_pStatCom)
					m_pStatCom->Consume_Fable();
			}

			//현재 FableEX 만 특수 액션(버프..) 라 FableEX때 함수 호출함. 예외생기면 수정하기 
			else if (pending.eAction == AnimAction::FableEX)
			{
				if (m_pStatCom)
					m_pStatCom->Consume_FableEX();

				m_pEquipmentCom->Execute_FableArts(0);
			}
		}
	}
}

void CAnimDriver::Cancel_Fatal()
{
	End_FatalApproach();
	m_pFatalTarget = nullptr;
	m_pGameInstance->Stop_CameraWork();
}

void CAnimDriver::End_FatalApproach()
{
	if (m_bApproaching == false) return;
	m_bApproaching = false;
	m_pMovementCom->Stop_MoveTo();
	m_pStatusCom->Set_InputLock(false);
}


void CAnimDriver::Try_Attack(PlayerActionID eInput, AnimAction eAction)
{
	if (!m_pInputCom->Peek(eInput)|| !Can_Accept_Attack(eAction)) return;

	m_pInputCom->Consume(eInput);
	Request_Attack(eAction);
}

void CAnimDriver::On_Notify()
{
	for (const ANIM_NOTIFY* pNotify : m_pAnimatorCom->Get_TriggeredNotifies())
	{
		const string& strNotifyName = pNotify->strName;
		const string& strPayload = pNotify->strPayload;
		
		if (pNotify->eType == ANIM_NOTIFY_TYPE::EFFECT)
		{
			Play_NotifyEffect(pNotify);
			continue;
		}
		else if (pNotify->eType == ANIM_NOTIFY_TYPE::SOUND)
		{
			if (strPayload.empty())
				Play_CueNotify(strNotifyName);
			else
				m_pGameInstance->Play_PlayerSound(strPayload);
			continue;
		}

		else if (pNotify->eType == ANIM_NOTIFY_TYPE::FOOTSTEP)
		{
			if (strPayload.empty())
				Play_FootStepCue();
			else
				Handle_PlaySoundRule(strPayload);
			continue;
		}

		else if (strNotifyName == NOTIFY_CAMERA_SCENE)
		{
			string strPayload = pNotify->strPayload;
			//음수로 주면 기존 데이터의 duration
			m_pGameInstance->Play_Scene(strPayload.c_str(), 0.7f, 0.7f, 1.f);
		}

		else if (strNotifyName == NOTIFY_CAMERA_SHAKE)
		{
			if (strPayload.empty()) continue;
			m_pGameInstance->Play_Shake(strPayload.c_str());
			continue;
		}

		else if (strNotifyName == NOTIFY_LAST_FATAL_BEGIN)
		{
			m_pStatusCom->Set_FatalLastHit(true);
			continue;
		}

		else if (strNotifyName == NOTIFY_LAST_FATAL_END)
		{
			m_pStatusCom->Set_FatalLastHit(false);
			continue;
		}

		else if (strNotifyName == NOTIFY_ROOTMOTION_SCALING)
		{
			if (strPayload.empty()) continue;
			_float fScale = stof(strPayload);
			m_pMovementCom->Set_RootMotionScale(fScale);
			continue;
		}

		else if (strNotifyName == NOTIFY_ROOTMOTION_CLEAR)
		{
			m_pMovementCom->Clear_RootMotionScale();
			continue;
		}

		else if (strNotifyName == NOTIFY_WEAPON_SWAP_END)
		{
			m_pEquipmentCom->Swap_End();
			continue;
		}

		else if (strNotifyName == NOTIFY_GRAPPLE_WIRELAUNCH)
		{
			m_LegionArmEvents.push_back(LegionArmEvent::Grapple_WireLaunch);
			continue;
		}

		else if (strNotifyName == NOTIFY_GRAPPLE_PULL)
		{
			m_LegionArmEvents.push_back(LegionArmEvent::Grapple_Pull);
			continue;
		}

		else if (strNotifyName == NOTIFY_GRAPPLE_FAIL)
		{
			m_LegionArmEvents.push_back(LegionArmEvent::Grapple_Fail);
			continue;
		}

		else if (strNotifyName == NOTIFY_GRAPPLE_CHASE)
		{
			m_LegionArmEvents.push_back(LegionArmEvent::Grapple_Chase);
			continue;
		}

		else if (strNotifyName == NOTIFY_GRAPPLE_STRIKE)
		{
			m_LegionArmEvents.push_back(LegionArmEvent::Grapple_Smash_Strike);
			continue;
		}

		else if (strNotifyName == NOTIFY_GRAPPLE_SMASH)
		{
			m_LegionArmEvents.push_back(LegionArmEvent::Grapple_Smash_Jump);
			continue;
		}

		else if (strNotifyName == NOTIFY_CHARGEGUN_AIM_READY)
		{
			m_LegionArmEvents.push_back(LegionArmEvent::ChargeGun_AimReady);
			continue;
		}

		else if (strNotifyName == NOTIFY_CHARGEGUN_FIRE)
		{
			m_LegionArmEvents.push_back(LegionArmEvent::ChargeGun_Fire);
			continue;
		}

		else if (strNotifyName == NOTIFY_CHARGEGUN_EXIT)
		{
			m_LegionArmEvents.push_back(LegionArmEvent::ChargeGun_Exit);
			continue;
		}

		else if (strNotifyName == NOTIFY_CAMERA_WORK)
		{
			if (strPayload.empty()) continue;

			string strKey = strPayload;
			// 타켓선정 순위 : 1. 페이탈 타켓
			CGameObject* pTarget = m_pFatalTarget;
			//2. 락온 타겟
			if (pTarget == nullptr)
				pTarget = m_pTargetCom->Get_CurrentTarget();

			//예외가 있으면 Work이름@XXX로 입력
			size_t iAt = strPayload.find('@');
			if (iAt != string::npos)
			{
				strKey = strPayload.substr(0, iAt);
				const string strTarget = strPayload.substr(iAt + 1);

				if (strTarget == "Self")
					pTarget = m_pGameInstance->Get_Player();
			}

			//3. 그래플이면 그래플 타겟
			if (pTarget == nullptr && m_pLegionArmCom)
			{
				CGrappleGun* pGrappleGun = m_pLegionArmCom->Get_GrappleGun();

				if (pGrappleGun && pGrappleGun->Get_GrappleState() != GrappleState::Idle)
					pTarget = pGrappleGun->Get_GrappleTarget();
			}

			//4. 모두 없으면 플레이어
			if (pTarget == nullptr)
				pTarget = m_pGameInstance->Get_Player();

			m_pGameInstance->Play_CameraWork(strPayload.c_str(), pTarget);
			continue;
		}

		else if (strNotifyName == NOTIFY_CAMERA_WORK_END)
		{
			m_pGameInstance->Stop_CameraWork();
			m_pFatalTarget = nullptr;
			continue;
		}

		else if (strNotifyName == NOTIFY_HITSTOP)
		{
			if (strPayload.empty()) continue;
			size_t iCount = strPayload.find('@');

			if (iCount == string::npos || iCount == 0 || iCount + 1 >= strPayload.size())
				continue;

			_float fDuration = stof(strPayload.substr(0, iCount));
			_float fTimeScale = stof(strPayload.substr(iCount + 1));

			m_pGameInstance->Slow_MainTimer(fDuration, fTimeScale);

			continue;
		}
		else if (strNotifyName == NOTIFY_MODEL_EFFECT)
		{
			if (strPayload.empty() || !m_pTransformCom || !m_pModelCom) continue;

			_float3 vPos = {};
			XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));
			m_pGameInstance->Play_Effect_Model(strPayload.c_str(), m_pModelCom, m_pTransformCom->Get_WorldMatrixPtr());
		}

	}
}

void CAnimDriver::Handle_PlaySoundRule(const string& strPayload)
{
	const _float fVolume = 1.f;

	if (strPayload.find("Stone_Run") != string::npos)
	{
		if (!m_pGameInstance->Can_PlaySoundByInterval("Player_Footstep", 0.18f))
			return;

		const int iVariant = Random(1, 9);
		const string strNum = (iVariant < 10 ? "0" : "") + std::to_string(iVariant);

		string strKey = strPayload;
		const size_t iPos = strKey.find_last_of('_');
		const _bool bHasNumber = (iPos != string::npos && iPos + 1 < strKey.size()
			&& strKey[iPos + 1] >= '0' && strKey[iPos + 1] <= '9');

		if (bHasNumber)
			strKey = strKey.substr(0, iPos + 1) + strNum;
		else
			strKey += "_" + strNum;

		m_pGameInstance->Play_PlayerSound(strKey, fVolume, false, false);
		return;
	}

	if (strPayload.find("Grinder") != string::npos)
		return;

	m_pGameInstance->Play_PlayerSound(strPayload, fVolume, false, false);
}

void CAnimDriver::Play_CueNotify(const string& strNotifyName)
{
	static const string strSoundPrefix = "Sound_";

	const string strCueKey = (0 == strNotifyName.rfind(strSoundPrefix, 0))
		? strNotifyName.substr(strSoundPrefix.size())
		: strNotifyName;

	if (true == strCueKey.empty() || nullptr == m_pTransformCom)
		return;

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Play_Cue(strCueKey, CHANNEL_ID::SOUND_PLAYER, vWorldPos);
}

void CAnimDriver::Play_FootStepCue()
{
	if (nullptr == m_pTransformCom)
		return;

	const string strCueKey = "SE_PC_FS_" + m_strFootStepSurface + "_Run_Cue";

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Play_Cue(strCueKey, CHANNEL_ID::SOUND_PLAYER, vWorldPos);
}


//소량이라도 있으면 무조건 액션 가능 
_bool CAnimDriver::Check_StaminaCost() const
{
	if (m_pStatCom == nullptr || m_pStatCom->Is_StaminaEmpty()) return false;
	return true;
}

_bool CAnimDriver::Check_FableCost(AnimAction eAction) const
{
	if (m_pStatCom == nullptr) return false;
	if (eAction == AnimAction::Fable) return m_pStatCom->Has_Fable();
	else if (eAction == AnimAction::FableEX) return m_pStatCom->Has_FableEX();
	return true;
}


_bool CAnimDriver::Can_Accept_Attack(AnimAction eAction) const
{
	const _bool bInAttack = (m_eCurrentAction != AnimAction::End);
	const _bool bCombo = (m_eCurrentAction == eAction && m_pAnimatorCom->Can_ComboInput());
	const _bool bCross = (m_eCurrentAction != eAction && m_pAnimatorCom->Can_Cancel());
	if (bInAttack && !bCombo && !bCross) return false;
	return true;
}

_bool CAnimDriver::Can_Start_Action(AnimAction eAction) const
{
	const _bool bInAnimation = (m_eCurrentAction != AnimAction::End);

	//Dash Window 면 Dash 는 통과 
	const _bool bDashWindow = (eAction == AnimAction::Dodge && m_pAnimatorCom->Is_InWindow(ANIM_WINDOW_TYPE::DASH_INPUT));
	const _bool bGuardWindow = (eAction == AnimAction::Guard && m_pAnimatorCom->Is_InWindow(ANIM_WINDOW_TYPE::GUARD_INPUT));
	const _bool bCanCancel = (m_pAnimatorCom->Can_Cancel() || bDashWindow || bGuardWindow);

	if (bInAnimation && !bCanCancel) return false;
	//if (!m_pStatusCom->CanMove() && !bDashWindow && !bGuardWindow) return false;
	if (!bCanCancel && !m_pStatusCom->CanMove()) return false;

	return true;
}

CAnimDriver::DIR8 CAnimDriver::Compute_InputDir8(_int iRawX, _int iRawY)
{
	if (iRawX == 0 && iRawY == 1) return DIR_F;
	if (iRawX == 1 && iRawY == 1) return DIR_FR;
	if (iRawX == 1 && iRawY == 0) return DIR_R;
	if (iRawX == 1 && iRawY == -1) return DIR_BR;
	if (iRawX == 0 && iRawY == -1) return DIR_B;
	if (iRawX == -1 && iRawY == -1) return DIR_BL;
	if (iRawX == -1 && iRawY == 0) return DIR_L;
	if (iRawX == -1 && iRawY == 1) return DIR_FL;
	return DIR8::End;
}

DIRECTION CAnimDriver::Compute_Direction(_int iRawX, _int iRawY)
{
	if (iRawY == 1) return DIRECTION::FRONT;
	else if (iRawY == -1) return DIRECTION::BACK;
	else if (iRawX == 1) return DIRECTION::RIGHT;
	else if (iRawX == -1) return DIRECTION::LEFT;
	else return DIRECTION::END;
}

void CAnimDriver::Play_NotifyEffect(const ANIM_NOTIFY* pNotify)
{
	if (nullptr == pNotify || true == pNotify->strPayload.empty() || nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return;

	if (true == pNotify->bUseSlot && false == pNotify->Slot.strBoneName.empty())
	{
		m_pGameInstance->Play_Effect(pNotify->strPayload, m_pModelCom,
			pNotify->Slot.strBoneName.c_str(),
			m_pTransformCom->Get_WorldMatrixPtr(),
			pNotify->Slot.vOffsetPosition);
		return;
	}

	_float3 vPos;
	XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));
	m_pGameInstance->Play_Effect(pNotify->strPayload, vPos);

}



CAnimDriver* CAnimDriver::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const ANIMDRIVER_DESC& Desc)
{
	CAnimDriver* pInstance = new CAnimDriver(pDevice, pContext);
	if(FAILED(pInstance->Initialize(Desc)))
	{
		MSG_BOX("Failed to Created : CAnimDriver");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CAnimDriver::Free()
{
	__super::Free();
	Stop_AllLoopSounds();
}
