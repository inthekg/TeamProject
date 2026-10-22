#include "AIObject.h"
#include "Engine_Utility.h"

#include "GameInstance.h"
#include "Animator.h"
#include "Model.h"
#include "Bone.h"
#include "RenderStruct.h"
#include "CCT.h"

#include "Navigation.h"

#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "BT_NodeModel.h"
#include "BT_FunctionRegistry.h"
#include "BT_TreeCompiler.h"
#include "BT_JsonSerializer.h"
#include "HitUtil.h"
#include "Effect.h"

#include "Movement_AI.h"
#include "StatComponent_AI.h"
#include "DebugCapsule.h"
#include "HitBox_Helper.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CAIObject, CCharacter)
ADD_PROPERTY_SETTER(CAIObject, m_fCCTRadius, Float, Set_CCTRadius, _float)
ADD_PROPERTY_SETTER(CAIObject, m_fCCTHeight, Float, Set_CCTHeight, _float)
ADD_PROPERTY_SETTER(CAIObject, m_fDbgRange, Float, Set_DebugRange, _float)
ADD_PROPERTY(CAIObject, m_fLastDbgRange, Float)
END_REFLECTION(CAIObject)
#endif

static Engine::CBT_FunctionRegistry* s_pRegistry = nullptr;

namespace
{
	const _float DEFAULT_HIT_GROGGY = { 10.f };
}

CAIObject::CAIObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CCharacter{ pDevice, pContext }
{
	//추가 
	m_eActorType = ACTOR_TYPE::MONSTER;
}

CAIObject::CAIObject(const CAIObject& Prototype)
	: CCharacter{ Prototype }
	, m_NotifyKeys{ Prototype.m_NotifyKeys }
	, m_HitTypeKeys{ Prototype.m_HitTypeKeys }
	, m_strMonsterName{ Prototype.m_strMonsterName }
{
}

_bool CAIObject::Get_TargetPos(_vector& vOut) const
{
	if (nullptr == m_pTarget)
		return false;

	CTransform* pTransform = m_pTarget->Get_Transform();
	if (nullptr == pTransform)
		return false;

	vOut = pTransform->Get_State(STATE::POSITION);
	return true;
}

void CAIObject::Set_Freeze(_bool bFreeze)
{
	m_bFrozen = bFreeze;

	if (true == bFreeze)
	{
		if (false == Is_Dead())
		{
			Set_AnimFloat(L"Speed", 0.f);

			if (nullptr != m_pAnimator)
				m_pAnimator->Reset_PlaybackState();
		}

		if (nullptr != m_pBehaviorTree)
			m_pBehaviorTree->Reset();
	}

	On_Freeze(bFreeze);
}

_bool CAIObject::Is_Frozen() const
{
	return m_bFrozen;
}

void CAIObject::Cancel_Leash()
{
	m_bReturning = false;
	m_fReengageTimer = 0.f;
}

_bool CAIObject::Is_Groggy_Full() const
{
	if (m_pStatCom) return m_pStatCom->Is_GroggyFull();
	return false;
}

HRESULT CAIObject::Initialize_Prototype()
{
	m_HitTypeKeys[Hash_String(L"HitType_Normal")] = HIT_TYPE::Normal;
	m_HitTypeKeys[Hash_String(L"HitType_Heavy")] = HIT_TYPE::Heavy;
	m_HitTypeKeys[Hash_String(L"HitType_KnockBack")] = HIT_TYPE::KnockBack;
	m_HitTypeKeys[Hash_String(L"HitType_KnockDown")] = HIT_TYPE::KnockBack;
	m_HitTypeKeys[Hash_String(L"HitType_Charge")] = HIT_TYPE::Charge;
	m_HitTypeKeys[Hash_String(L"HitType_Fury")] = HIT_TYPE::Fury;
	m_HitTypeKeys[Hash_String(L"HitType_NormalUnGuard")] = HIT_TYPE::NormalUnGuard;
	m_HitTypeKeys[Hash_String(L"HitType_HeavyUnGuard")] = HIT_TYPE::HeavyUnGuard;

	return S_OK;
}

HRESULT CAIObject::Initialize(void* pArg)
{
	_float fInitMaxHP = 100.f;
	_float fInitHP = 100.f;
	_float fInitMaxGroggy = 100.f;
	_float fInitGroggyDecayDelay = 2.f;
	_float fInitGroggyDecayRate = 25.f;
	_float fInitGroggyCooldownMax = 1.f;

	if (nullptr != pArg)
	{
		auto pDesc = static_cast<AIOBJECT_DESC*>(pArg);
		fInitHP = pDesc->fHP;
		fInitMaxHP = pDesc->fMaxHP;
		fInitMaxGroggy = pDesc->fMaxGroggy;
		fInitGroggyDecayDelay = pDesc->fGroggyDecayDelay;
		fInitGroggyDecayRate = pDesc->fGroggyDecayRate;
		fInitGroggyCooldownMax = pDesc->fGroggyCooldownMax;
	}

	Apply_SpawnDesc(pArg);

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components()))
		return E_FAIL;

	if (nullptr != pArg)
		Bind_Navigation(static_cast<AIOBJECT_DESC*>(pArg)->pNavigation);

	m_pStatCom = CStatComponent_AI::Create(m_pDevice, m_pContext, fInitMaxHP, fInitMaxGroggy, fInitGroggyDecayDelay, fInitGroggyDecayRate, fInitGroggyCooldownMax);
	if (nullptr == m_pStatCom)
		return E_FAIL;
	m_pStatCom->Set_CurrentHP(fInitHP);

	if (FAILED(Ready_BehaviorTree()))
		return E_FAIL;

	return S_OK;
}

void CAIObject::Update(_float fTimeDelta)
{
	if (m_fHudHitTimer > 0.f)
		m_fHudHitTimer -= fTimeDelta;

#ifdef _DEBUG
	// M 키를 누르면 현재 활성화되어 Update 중인 AIObject에 10만 데미지를 적용해 기존 사망 처리 흐름으로 즉사시킨다.
	if (nullptr != m_pStatCom && false == m_pStatCom->Is_Dead() && true == m_pGameInstance->Key_Down(DIK_M))
		m_pStatCom->Apply_Damage(100000.f);
#endif

	Drive_SuperArmor();

	m_pStatCom->Update_StatComponent(fTimeDelta);
	Tick_GroggyWindow(fTimeDelta);
	if (true == m_bFrozen)
	{
		Drive_Animator(fTimeDelta);
		return;
	}

	Update_BlackBoard(fTimeDelta);
	m_pBehaviorTree->Update();

	Drive_Animator(fTimeDelta);
}

void CAIObject::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CAIObject::Late_Update(_float fTimeDelta)
{
#ifdef _DEBUG
	if (true == m_pGameInstance->Get_DebugFlag() && nullptr != m_pTransformCom)
	{
		Debug_UpdateMoveState();

		if (nullptr == m_pDebugRange || m_fDbgRange != m_fLastDbgRange)
			Rebuild_DebugRange();

		if (nullptr != m_pDebugRange)
		{
			m_pDebugRange->Update_Pose(m_pTransformCom->Get_State(STATE::POSITION), XMQuaternionIdentity());
			m_pDebugRange->Late_Update(fTimeDelta);
		}
	}
#endif


	if (nullptr == m_pModelCom || nullptr == m_pAnimator)
		return;

	if (m_bSkipBoneUpdateOnce == false)
		m_pModelCom->Update_Bones(&m_pAnimator->Get_CurPose());

	m_bSkipBoneUpdateOnce = false;

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
	m_pGameInstance->Add_RenderGroup(RENDERID::SHADOW, this);
}

HRESULT CAIObject::Render()
{
	if (nullptr == m_pModelCom)
		return S_OK;

	const auto& units = m_pModelCom->Get_RenderUnits();
	const _uint iCount = static_cast<_uint>(units.size());
	const RENDERID eCurPass = m_pGameInstance->Get_CurRenderPass();
	const _float4x4* pWorld = m_pTransformCom->Get_WorldMatrixPtr();

	for (_uint i = 0; i < iCount; ++i)
	{
		CMesh* pMesh = units[i].pMesh;
		CMaterial* pMaterialCom = units[i].pMaterial;
		if (nullptr == pMesh || nullptr == pMaterialCom)
			continue;

		if (i < m_MeshVisibles.size() && false == m_MeshVisibles[i])
			continue;

		if (pMaterialCom->Get_RenderGroup() != eCurPass)
			continue;

		SkinnedCommand command{};
		command.header.etype = RENDER_TYPE::SKINNED;
		command.iBoneCount = pMesh->Get_BoneCount();
		command.header.iCommandSize = sizeof(SkinnedCommand);
		command.pVIBuffer = pMesh;
		command.pMaterial = pMaterialCom;
		command.worldMatrix = *pWorld;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		command.iAlphaClip = 0;
		command.fEmissiveAmount = m_fEmissiveAmount;
		command.iRimProfile = m_iRimProfile;
		command.fRimAmount = m_fRimAmount;
		command.fDissolveAmount = m_fDissolveAmount;
		command.vDissolveColor = m_vDissolveColor;
		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

HRESULT CAIObject::Render_Shadow()
{
	if (nullptr == m_pModelCom)
		return S_OK;

	const auto& units = m_pModelCom->Get_RenderUnits();
	const _uint iCount = static_cast<_uint>(units.size());
	const _float4x4* pWorld = m_pTransformCom->Get_WorldMatrixPtr();

	for (_uint i = 0; i < iCount; ++i)
	{
		CMesh* pMesh = units[i].pMesh;
		CMaterial* pMaterialCom = units[i].pMaterial;
		if (nullptr == pMesh || nullptr == pMaterialCom)
			continue;

		if (i < m_MeshVisibles.size() && false == m_MeshVisibles[i])
			continue;

		SkinnedCommand command{};
		command.header.etype = RENDER_TYPE::SKINNED;
		command.iBoneCount = pMesh->Get_BoneCount();
		command.header.iCommandSize = sizeof(SkinnedCommand);
		command.pVIBuffer = pMesh;
		command.pMaterial = pMaterialCom;
		command.worldMatrix = *pWorld;

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		m_pGameInstance->Submit_Skinned_Resolved(command);
	}

	return S_OK;
}

_uint CAIObject::Release()
{
	if (0 == m_iRefCnt)
	{
		Free();

		delete this;

		return 0;
	}
	else
	{
		m_iRefCnt--;

		if (m_iRefCnt == 0)
		{
			if (Has_OwnerPool())
			{
				return Return_ToPool();
			}
			else
			{
				Free();
				delete this;
				return 0;
			}
		}
	}
	return m_iRefCnt;
}

void CAIObject::Activate(void* pArg)
{
	if (false == m_bPoolReady)
	{
		if (FAILED(Initialize(pArg)))
		{
			MSG_BOX("Pool Activate Failed : CAIObject");
			return;
		}
		m_bPoolReady = true;
		Set_Active();
		return;
	}

	_vector vSpawnPos = Get_SpawnPos();
	CNavigation* pReuseNavi = nullptr;

	if (nullptr != pArg)
	{
		AIOBJECT_DESC* pDesc = static_cast<AIOBJECT_DESC*>(pArg);
		vSpawnPos = XMLoadFloat4(&pDesc->vTranslate);
		pReuseNavi = pDesc->pNavigation;
	}

	m_bSpawnCaptured = false;

	if (nullptr != m_pTransformCom)
		m_pTransformCom->Set_State(STATE::POSITION, vSpawnPos);

	if (nullptr != pReuseNavi)
		Bind_Navigation(pReuseNavi);

	Apply_SpawnDesc(pArg);

	Set_Active();

	Spawn();
}

void CAIObject::InActivate()
{
	Set_InActive();
	Despawn();
}

void CAIObject::Set_InActive()
{
	m_fHudHitTimer = 0.f;

	Stop_AllCamShakes();
	Stop_AllLoopSounds();

	__super::Set_InActive();
}

void CAIObject::Request_Despawn()
{
	Set_InActive();
	//Despawn();
}

void CAIObject::Set_Dissolve(_float fAmount, const _float4& vColor)
{
	m_fDissolveAmount = fAmount;
	m_vDissolveColor = vColor;
}

void CAIObject::Turn_On_AI()
{
	_vector vPos = (nullptr != m_pTransformCom) ? m_pTransformCom->Get_State(STATE::POSITION) : XMVectorSet(0.f, 0.f, 0.f, 1.f);

	if (nullptr != m_pCCT)
	{
		m_pCCT->Set_FootPosition(vPos);
		m_pCCT->Set_Active(true);
	}

	if (nullptr != m_pNavigation)
	{
		_float3 vNaviPos = {};
		XMStoreFloat3(&vNaviPos, vPos);
		m_pNavigation->Set_CurrentCell(vNaviPos, &m_iNaviCurCellIndex);
	}

	if (nullptr != m_pMovementCom)
		m_pMovementCom->Reset_Motion();

	if (nullptr != m_pAnimator)
	{
		m_pAnimator->Reset_PlaybackState();
		m_pAnimator->Set_ControllerEnabled(true);
		m_pAnimator->Resume();
	}

	if (nullptr != m_pBehaviorTree)
		m_pBehaviorTree->Reset();
}

void CAIObject::Turn_Off_AI()
{
	if (nullptr != m_pBehaviorTree)
		m_pBehaviorTree->Reset();

	if (nullptr != m_pAnimator)
	{
		m_pAnimator->Set_ControllerEnabled(false);
		//m_pAnimator->Stop();
	}

	if (nullptr != m_pMovementCom)
		m_pMovementCom->Stop_MoveTo();

	Cancel_Leash();
	m_iNaviCurCellIndex = -1;

	if (nullptr != m_pCCT)
		m_pCCT->Set_Active(false);
}

void CAIObject::Apply_SpawnDesc(void* pArg)
{
	// Pool 재사용 경로는 Initialize를 다시 부르지 않으므로 배치마다 달라지는 값은 전부 여기서 반영한다.
	if (nullptr == pArg)
		return;

	auto pDesc = static_cast<AIOBJECT_DESC*>(pArg);

	m_fDetectRange = pDesc->fDetectRange;
	m_fFrontCos = pDesc->fFrontCos;
	m_fAttackRange = pDesc->fAttackRange;
	m_fLeashRange = pDesc->fLeashRange;
	m_fReengageRange = pDesc->fReaggroRange;
	m_fReengageDelay = pDesc->fReengageDelay;
	m_eMonsterType = pDesc->eMonsterType;
}

_bool CAIObject::Is_MyBossStart(Engine::Event_Desc* pData) const
{
	if (nullptr == pData)
		return false;

	return m_eMonsterType == static_cast<BossClear_EventDesc*>(pData)->eMonsterType;
}

void CAIObject::Spawn()
{
	Turn_On_AI();

	if (nullptr != m_pStatCom)
		m_pStatCom->Reset_OnSpawn();

	Cancel_Leash();
	m_bReturningPrev = false;
	m_bDetected = false;
	m_bFrozen = false;
	m_bAggro = false;
	m_bSpawned = false;
	m_bFatalReady = false;
	m_bSuperArmorPrev = false;
	m_iCurAnimStateHash = 0;
	m_fLostTimer = 0.f;
	m_fHitTimer = 0.f;
	m_iHitDir = 0;
	m_eCurHitType = HIT_TYPE::Normal;
	m_iPendingPatternID = -1;
	m_fPendingAttackLength = 0.f;
	m_bApproaching = false;

	if (nullptr != m_pBlackBoard)
	{
		m_pBlackBoard->Set<_bool>(L"bDead", false);
		m_pBlackBoard->Set<_bool>(L"bFatal", false);
		m_pBlackBoard->Set<_bool>(L"bGroggy", false);
	}

	// 추가
	ActorSpawn_EventDesc Desc = {};
	Desc.pActor = this;
	Desc.eActorType = m_eActorType;

	m_pGameInstance->Broadcast(EVENT_TYPE::ACTOR_SPAWN, &Desc);
}

void CAIObject::Despawn()
{
	Set_RimAmount(0.f);
	Turn_Off_AI();
}

void CAIObject::Set_Groggy(_bool bGroggy)
{
	_bool bCur = (nullptr != m_pStatCom) ? m_pStatCom->Is_Groggy() : false;

	if (bGroggy != bCur)
	{
		if (true == bGroggy)
		{
			if (false == m_pStatCom->Try_EnterGroggy())
				return;

			Begin_GroggyWindow(m_fGroggyWindowFallback);
		}
		else
		{
			m_pStatCom->Leave_Groggy();
			m_bGroggyWindowActive = false;
		}

		On_Groggy(bGroggy);
	}

	m_pBlackBoard->Set<_bool>(L"bGroggy", bGroggy);
}

_bool CAIObject::Is_Groggy() const
{
	return (nullptr != m_pStatCom) ? m_pStatCom->Is_Groggy() : false;
}

void CAIObject::On_SetDead()
{
	Despawn();
}

void CAIObject::Play_Animation(const _wstring& strKey)
{
	if (m_pAnimator)
		m_pAnimator->Request_State(strKey);
}

string CAIObject::Resolve_NotifyKey(const ANIM_NOTIFY* pNotify, const string& strPrefix) const
{
	if (nullptr == pNotify)
		return string();

	if (false == pNotify->strPayload.empty())
		return pNotify->strPayload;

	if (0 == pNotify->strName.rfind(strPrefix, 0))
		return pNotify->strName.substr(strPrefix.size());

	return pNotify->strName;
}

void CAIObject::Handle_SoundNotify(const ANIM_NOTIFY* pNotify)
{
	static const string strSoundPrefix = "Sound_";

	const string strCueKey = Resolve_NotifyKey(pNotify, strSoundPrefix);

	if (true == strCueKey.empty() || nullptr == m_pTransformCom)
		return;

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Play_Cue(strCueKey, SOUND_MONSTER, vWorldPos);
}

void CAIObject::Handle_FootStepNotify(const ANIM_NOTIFY* pNotify)
{
	static const string strFootStepPrefix = "FootStep_";

	const string strEventName = Resolve_NotifyKey(pNotify, strFootStepPrefix);

	if (true == strEventName.empty() || nullptr == m_pTransformCom)
		return;

	const string strCueKey = "SE_NPC_" + strEventName + "_" + m_strFootStepSurface + "_Cue";

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Play_Cue(strCueKey, SOUND_MONSTER, vWorldPos);
}

void CAIObject::Handle_CamShakeNotify(const ANIM_NOTIFY* pNotify)
{
	static const string strShakePrefix = "CamShake_";
	static const string strClassSuffix = "_C";

	string strShakeKey = Resolve_NotifyKey(pNotify, strShakePrefix);

	if (true == strShakeKey.empty())
		return;

	const size_t iSuffixSize = strClassSuffix.size();

	if (iSuffixSize <= strShakeKey.size()
		&& 0 == strShakeKey.compare(strShakeKey.size() - iSuffixSize, iSuffixSize, strClassSuffix))
		strShakeKey.resize(strShakeKey.size() - iSuffixSize);

	if (true == strShakeKey.empty())
		return;

	Calculate_ViewDist();

	if (m_fCamShakeMaxDist < Get_ViewDist())
		return;

	m_pGameInstance->Play_Shake(strShakeKey);
}

_bool CAIObject::Handle_CustomNotify(const ANIM_NOTIFY* pNotify)
{
	if (nullptr == pNotify || true == pNotify->strPayload.empty())
		return false;

	CHitBox_Helper* pHitBoxes = Get_HitBoxes();
	if (nullptr == pHitBoxes)
		return false;

	_float3 vOrigin = {};
	_float  fRadius = 0.f;
	if (false == pHitBoxes->Get_GroundProbe(pNotify->iNameHash, this, vOrigin, fRadius))
		return false;

	const _float3 vDown = { 0.f, -1.f, 0.f };

	CollisionResult tGround{};
	if (true == m_pGameInstance->Raycast(vOrigin, vDown, fRadius + m_fGroundFxExtend,
		FILTER_GROUP::GROUND, tGround, PxQueryFlag::eSTATIC))
		m_pGameInstance->Play_Effect(pNotify->strPayload, tGround.tRaycastData.vHitPosition);

	return true;
}


void CAIObject::Sound_Notify(string& payLoad)
{
	m_pGameInstance->Play_Sound(payLoad, CHANNEL_ID::SOUND_MONSTER);
}

_bool CAIObject::Is_AnimationFinished() const
{
	return m_pAnimator ? m_pAnimator->Is_Ended() : true;
}

void CAIObject::Drive_LoopSounds()
{
	static const string strLoopPrefix = "LoopSound_";

	if (nullptr == m_pAnimator || nullptr == m_pTransformCom)
	{
		Stop_AllLoopSounds();
		return;
	}

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	unordered_set<_uint64> ActiveKeys;

	for (const ANIM_WINDOW* pWindow : m_pAnimator->Get_ActiveCustomWindows())
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
		const _uint hHandle = m_pGameInstance->Play_Cue(strCueKey, SOUND_MONSTER, vWorldPos, 1.f, true);

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

void CAIObject::Drive_CamShakeWindows()
{
	static const string strShakePrefix = "BP_Shake_";

	if (nullptr == m_pAnimator || nullptr == m_pTransformCom)
	{
		Stop_AllCamShakes();
		return;
	}

	Calculate_ViewDist();

	if (m_fCamShakeMaxDist < Get_ViewDist())
	{
		Stop_AllCamShakes();
		return;
	}

	unordered_set<_uint64> ActiveKeys;

	for (const ANIM_WINDOW* pWindow : m_pAnimator->Get_ActiveCustomWindows())
	{
		if (nullptr == pWindow || 0 == pWindow->iPayloadHash)
			continue;

		if (0 != pWindow->strPayload.rfind(strShakePrefix, 0))
			continue;

		ActiveKeys.emplace(pWindow->iPayloadHash);

		if (m_CamShakes.end() != m_CamShakes.find(pWindow->iPayloadHash))
			continue;

		if (SUCCEEDED(m_pGameInstance->Play_Shake(pWindow->iPayloadHash)))
			m_CamShakes.emplace(pWindow->iPayloadHash);
	}

	for (auto iter = m_CamShakes.begin(); iter != m_CamShakes.end(); )
	{
		if (ActiveKeys.end() != ActiveKeys.find(*iter))
		{
			++iter;
			continue;
		}

		m_pGameInstance->Stop_Shake(*iter);
		iter = m_CamShakes.erase(iter);
	}
}

void CAIObject::Stop_AllCamShakes()
{
	for (_uint64 iKeyHash : m_CamShakes)
		m_pGameInstance->Stop_Shake(iKeyHash);

	m_CamShakes.clear();
}


void CAIObject::Stop_AllLoopSounds()
{
	//for (auto& Pair : m_LoopSounds)
	//	m_pGameInstance->Stop_SoundSource(Pair.second);

	m_LoopSounds.clear();
}


void CAIObject::Set_AnimBool(const _wstring& strKey, _bool bValue)
{
	if (m_pAnimator) m_pAnimator->Set_Bool(strKey, bValue);
}

void CAIObject::Set_AnimInt(const _wstring& strKey, _int iValue)
{
	if (m_pAnimator) m_pAnimator->Set_Int(strKey, iValue);
}

void CAIObject::Set_AnimFloat(const _wstring& strKey, _float fValue)
{
	if (m_pAnimator) m_pAnimator->Set_Float(strKey, fValue);
}

void CAIObject::Set_AnimTrigger(const _wstring& strKey)
{
	if (m_pAnimator) m_pAnimator->Set_Trigger(strKey);
}

void CAIObject::Request_AnimAction(const _wstring& strAction)
{
	if (m_pAnimator) m_pAnimator->Request_Action(strAction);
}

void CAIObject::Request_AnimState(const _wstring& strState)
{
	if (m_pAnimator) m_pAnimator->Request_State(strState);
}

void CAIObject::Request_AnimSequenceEnd()
{
	if (m_pAnimator) m_pAnimator->Request_SequenceEnd();
}

_bool CAIObject::Was_AnimActionAccepted() const
{
	return m_pAnimator ? m_pAnimator->Was_ActionAcceptedThisFrame() : false;
}

_bool CAIObject::Was_AnimStateChanged() const
{
	return m_pAnimator ? m_pAnimator->Was_StateChangedThisFrame() : false;
}

_bool CAIObject::Is_AnimInState(const _wstring& strState) const
{
	if (nullptr == m_pAnimator) return false;
	const ANIM_CONTROLLER_STATE* pState = m_pAnimator->Get_CurControllerState();
	return (nullptr != pState) ? (pState->iNameHash == Hash_String(strState.c_str())) : false;
}

_bool CAIObject::Can_AnimMove() const
{
	return m_pAnimator ? m_pAnimator->Get_GameplayFlags().bCanMove : true;
}

_bool CAIObject::Can_AnimRotate() const
{
	return m_pAnimator ? m_pAnimator->Get_GameplayFlags().bCanRotate : true;
}

_bool CAIObject::Is_HitActive() const
{
	return m_pAnimator ? m_pAnimator->Get_GameplayFlags().bHitActive : false;
}

_bool CAIObject::Is_SuperArmor() const
{
	return m_pAnimator ? m_pAnimator->Get_GameplayFlags().bSuperArmor : false;
}

_bool CAIObject::Is_Dead() const
{
	return (nullptr != m_pStatCom) ? m_pStatCom->Is_Dead() : false;
}

_bool CAIObject::Is_GroundSweepActive() const
{
	if (nullptr == m_pAnimator)
		return false;

	return m_pAnimator->Is_InCustomWindow(L"GroundSweep");
}

_bool CAIObject::Take_Damage(const HitInfo& hitInfo)
{
	m_fHudHitTimer = m_fHudHitShowTime;

	const _float fGroggyAmount = (hitInfo.fGroggy > 0.f) ? hitInfo.fGroggy : DEFAULT_HIT_GROGGY;

	Record_HitType(hitInfo);
	On_Perceive_Attacker(true);

	if (HIT_TYPE::Fatal == hitInfo.eHitType)
	{
		Compute_HitDir(XMLoadFloat3(&hitInfo.vHitPosition));
		Set_AnimInt(L"Direction", m_iHitDir);
		Set_FatalMode(true);
		On_Fatal_Begin();
		m_pStatCom->Apply_Damage(hitInfo.fDamage);
		return true;
	}

	if (HIT_TYPE::FatalEnd == hitInfo.eHitType && true == Handles_FatalEnd())
	{
		m_pStatCom->Apply_Damage(hitInfo.fDamage);
		Set_FatalMode(false);
		return true;
	}

	if (Is_Dead())
		return true;

	if (true == Should_Ignore_Damage(hitInfo))
		return true;

	On_Perceive_Attacker(false);

	On_Hit_Pre(hitInfo);

	if (HIT_TYPE::Charge == hitInfo.eHitType)
	{
		Compute_HitDir(XMLoadFloat3(&hitInfo.vHitPosition));
		Set_AnimInt(L"Direction", m_iHitDir);
		Apply_ChargeHit(hitInfo.fDamage, fGroggyAmount);
		return true;
	}

	m_pStatCom->Apply_Damage(hitInfo.fDamage);
	if (Is_Dead())
	{
		if (true == Uses_Full_Hit_Reaction())
			m_pBlackBoard->Set<_bool>(L"bDead", true);
		return true;
	}

	if (true == Blocks_Damage_While_Groggy() && true == Is_Groggy())
		return true;

	if (true == Uses_Full_Hit_Reaction())
		On_Hit_Post(hitInfo);

	m_pStatCom->Add_Groggy(fGroggyAmount);
	m_pStatCom->Set_SinceHit(0.f);
	if (m_pStatCom->Is_GroggyFull())
		m_pStatCom->Set_Staggerable(true);

	if (true == Uses_Full_Hit_Reaction() && false == Is_SuperArmor() && false == Is_Groggy())
	{
		m_pBlackBoard->Set<_bool>(L"bHit", true);
		m_bHitRefreshPending = true;
	}
	return true;
}

void CAIObject::Apply_RootMotion(const _float3& vTranslation, const _float4& vRotation)
{
	if (nullptr == m_pTransformCom)
		return;

	_vector vWorldMove = XMVector3Rotate(XMLoadFloat3(&vTranslation), m_pTransformCom->Get_Quat());
	if (XMVectorGetX(XMVector3LengthSq(vWorldMove)) > 1e-12f)
	{
		_vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
		_vector vTarget = XMVectorAdd(vPos, vWorldMove);

		if (nullptr != m_pNavigation && m_iNaviCurCellIndex >= 0)
		{
			_float3 vT = {};
			XMStoreFloat3(&vT, vTarget);

			_float3 vOut = {};
			if (true == m_pNavigation->Move_OnNavigation(vT, &m_iNaviCurCellIndex, &vOut))
				m_pTransformCom->Set_State(STATE::POSITION, vTarget);
		}
		else
		{
			m_pTransformCom->Set_State(STATE::POSITION, vTarget);
		}
	}

	_vector vRootRot = XMQuaternionNormalize(XMLoadFloat4(&vRotation));
	_vector vAxis = XMVectorZero();
	_float  fAngle = 0.f;
	XMQuaternionToAxisAngle(&vAxis, &fAngle, vRootRot);
	const _float fYaw = fAngle * XMVectorGetY(vAxis);
	if (fabsf(fYaw) > 1e-5f)
	{
		_vector qYaw = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), fYaw);
		_vector qNew = XMQuaternionNormalize(XMQuaternionMultiply(qYaw, m_pTransformCom->Get_Quat()));
		m_pTransformCom->Set_Quat(qNew);
	}
}

_bool CAIObject::Get_FatalPoint_Front(_vector& vOut) const
{
	if (false == Can_FatalFront() || nullptr == m_pTransformCom)
		return false;

	// 로컬 오프셋을 Facing 으로 회전 + 위치 가산
	_vector vOffsetWorld = XMVector3Rotate(XMLoadFloat3(&m_vFatalOffsetFront), m_pTransformCom->Get_Quat());
	vOut = XMVectorAdd(m_pTransformCom->Get_State(STATE::POSITION), vOffsetWorld);
	return true;
}

_bool CAIObject::Get_FatalPoint_Back(_vector& vOut) const
{
	if (false == Can_FatalBack() || nullptr == m_pTransformCom)
		return false;

	_vector vOffsetWorld = XMVector3Rotate(XMLoadFloat3(&m_vFatalOffsetBack), m_pTransformCom->Get_Quat());
	vOut = XMVectorAdd(m_pTransformCom->Get_State(STATE::POSITION), vOffsetWorld);
	return true;
}

_bool CAIObject::Can_FatalFront() const
{
	return m_bFatalReady;
}

_bool CAIObject::Can_FatalBack() const
{
	return true;
}

void CAIObject::Set_FatalOffset(const _float3& vFront, const _float3& vBack)
{
	m_vFatalOffsetFront = vFront;
	m_vFatalOffsetBack = vBack;
}

void CAIObject::Set_FatalMode(_bool bMode)
{
	if (false == bMode && true == Is_Dead())
	{
		m_pStatCom->Set_FatalDying(true);
	}
	else if (bMode)
	{
		Set_Groggy(false);
	}
	m_pBlackBoard->Set<_bool>(L"bFatal", bMode);
}

void CAIObject::Fatal_Start()
{
	m_bFrozen = false;

	Set_FatalMode(true);

	Set_AnimFloat(L"Speed", 0.f);
	On_Fatal_Begin();

	Request_AnimAction(L"FATAL");
	Set_AnimBool(L"Fatal", true);
}

void CAIObject::Fatal_End()
{
	Set_FatalMode(false);
	Set_AnimBool(L"Fatal", false);
	m_bFrozen = false;
}

void CAIObject::Fatal_Wait()
{
	if (true == Is_Groggy())
	{
		m_bFrozen = true;
		if (nullptr != m_pBehaviorTree)
			m_pBehaviorTree->Reset();
		On_Freeze(true);
	}
	else
	{
		Set_Freeze(true);
	}
}

const string* CAIObject::Get_GroundSparkFx() const
{
	if (nullptr == m_pAnimator)
		return nullptr;

	static const _uint64 iNameKey = Hash_String(L"GroundSpark");

	for (const ANIM_WINDOW* pWindow : m_pAnimator->Get_ActiveCustomWindows())
	{
		if (nullptr == pWindow || pWindow->iNameHash != iNameKey)
			continue;

		if (false == pWindow->strPayload.empty())
			return &pWindow->strPayload;
	}

	return nullptr;
}

_bool CAIObject::Hit_SweepSphere(_float fRadius, _fvector vPrev, _fvector vCur, HitInfo tHitInfo, unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget)
{
	tHitInfo.pOtherActor = this;
	tHitInfo.pDamangeCauser = this;
	return CHitUtil::Sweep_Sphere(m_pGameInstance, fRadius, vPrev, vCur, tHitInfo, Targets, eTarget);
}

_bool CAIObject::Hit_SweepCapsule(_float fRadius, _float fHalfHeight, _fvector vPrev, _fvector vCur, _fvector vQuat, HitInfo tHitInfo, unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget)
{
	tHitInfo.pOtherActor = this;
	tHitInfo.pDamangeCauser = this;
	return CHitUtil::Sweep_Capsule(m_pGameInstance, fRadius, fHalfHeight, vPrev, vCur, vQuat, tHitInfo, Targets, eTarget);
}

void CAIObject::Hit_OverlapSphere(_float fRadius, _fvector vCenter, HitInfo tHitInfo, unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget)
{
	tHitInfo.pOtherActor = this;
	tHitInfo.pDamangeCauser = this;
	CHitUtil::Overlap_Sphere(m_pGameInstance, fRadius, vCenter, tHitInfo, Targets, eTarget);
}

_bool CAIObject::Make_BoneCapsule(_int iBoneTop, _int iBoneBot, _vector& OutCenter, _vector& OutQuat)
{
	if (iBoneTop < 0 || iBoneBot < 0)
		return false;

	_matrix TopWorld, BotWorld;
	if (FAILED(Get_BoneWorldMatrix(static_cast<_uint>(iBoneTop), TopWorld)))
		return false;
	if (FAILED(Get_BoneWorldMatrix(static_cast<_uint>(iBoneBot), BotWorld)))
		return false;

	CHitUtil::Make_CapsulePose(TopWorld.r[3], BotWorld.r[3], OutCenter, OutQuat);
	return true;
}

_bool CAIObject::Update_Leash(_float fTimeDelta, _float fDistToTarget)
{
	if (nullptr == m_pTransformCom)
		return false;

	_vector vSpawn = XMLoadFloat4(&m_vSpawnPos);
	_vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
	_float fDist = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(vSpawn, vPos), 0.f)));

	if (false == m_bReturning)
	{
		if (fDist > m_fLeashRange && fDistToTarget > m_fReengageRange)
		{
			m_bReturning = true;
			m_fReengageTimer = 0.f;
		}
		else
			return false;
	}

	if (fDist <= 0.5f)
	{
		m_bReturning = false;
		m_fReengageTimer = 0.f;
		return false;
	}

	if (fDistToTarget <= m_fReengageRange)
	{
		m_fReengageTimer += fTimeDelta;

		if (m_fReengageTimer >= m_fReengageDelay)
		{
			m_bReturning = false;
			m_fReengageTimer = 0.f;
			return false;
		}
	}
	else
		m_fReengageTimer = 0.f;

	return true;
}

_bool CAIObject::Has_LineOfSight(_fvector vFrom, _fvector vTo, FILTER_GROUP eBlockers, _float fRadius) const
{
	if (nullptr == m_pGameInstance)
		return true;

	_vector vDelta = XMVectorSubtract(vTo, vFrom);
	_float fDist = XMVectorGetX(XMVector3Length(vDelta));
	if (fDist < 0.7f)
		return true;

	_vector vDir = XMVector3Normalize(vDelta);
	_vector vStart = XMVectorAdd(vFrom, XMVectorScale(vDir, 0.6f));
	_float fLen = fDist - 0.6f;

	CollisionResult tResult{};
	_bool bHit = false;

	if (fRadius <= 0.f)
	{
		_float3 vRayOrigin, vRayDir;
		XMStoreFloat3(&vRayOrigin, vStart);
		XMStoreFloat3(&vRayDir, vDir);
		bHit = m_pGameInstance->Raycast(vRayOrigin, vRayDir, fLen, eBlockers, tResult, PxQueryFlag::eSTATIC);
	}
	else
	{
		bHit = m_pGameInstance->Sweep_Sphere(fRadius, vStart, vDir, fLen, eBlockers, tResult, PxQueryFlag::eSTATIC);
	}

	return (false == bHit);
}

_bool CAIObject::Can_See(CGameObject* pTarget, _float fEyeHeight, FILTER_GROUP eBlockers, _float fRadius) const
{
	if (nullptr == pTarget || nullptr == m_pTransformCom)
		return false;

	CTransform* pTargetTransform = pTarget->Get_Transform();
	if (nullptr == pTargetTransform)
		return false;

	_vector vEyeOffset = XMVectorSet(0.f, fEyeHeight, 0.f, 0.f);
	_vector vEye = XMVectorAdd(m_pTransformCom->Get_State(STATE::POSITION), vEyeOffset);
	_vector vAim = XMVectorAdd(pTargetTransform->Get_State(STATE::POSITION), vEyeOffset);

	return Has_LineOfSight(vEye, vAim, eBlockers, fRadius);
}


CBT_FunctionRegistry* CAIObject::Registry()
{
	if (nullptr == s_pRegistry)
	{
		s_pRegistry = Engine::CBT_FunctionRegistry::Create();
		if (nullptr != s_pRegistry)
			Register_ProjectFunctions(s_pRegistry);
	}

	return s_pRegistry;
}

void CAIObject::Register_ProjectFunctions(Engine::CBT_FunctionRegistry* pReg)
{
	if (nullptr == pReg)
		return;

	using Status = Engine::CBT_Node::Status;
	using Params = Engine::BT_PARAMS;
	auto Param = &Engine::CBT_FunctionRegistry::Param;

	pReg->Register_Condition("Cond_BBBool",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->_bool {
			if (nullptr == pBB) return false;
			return pBB->Get<_bool>(StringToWString(Param(p, "key", "")));
		});

	pReg->Register_Task("Task_SetBB",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			pBB->Set<_bool>(StringToWString(Param(p, "key", "")), Param(p, "value", "") == "true");
			return Status::Success;
		});

	pReg->Register_Task("Task_AnimSetBool",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			auto* pSelf = pBB->Get_Object<CAIObject>(L"Self");
			if (nullptr == pSelf) return Status::Failure;
			pSelf->Set_AnimBool(StringToWString(Param(p, "key", "")), Param(p, "value", "") == "true");
			return Status::Success;
		});

	pReg->Register_Task("Task_AnimSetInt",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			auto* pSelf = pBB->Get_Object<CAIObject>(L"Self");
			if (nullptr == pSelf) return Status::Failure;
			pSelf->Set_AnimInt(StringToWString(Param(p, "key", "")), atoi(Param(p, "value", "0").c_str()));
			return Status::Success;
		});

	pReg->Register_Task("Task_AnimSetFloat",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			auto* pSelf = pBB->Get_Object<CAIObject>(L"Self");
			if (nullptr == pSelf) return Status::Failure;
			pSelf->Set_AnimFloat(StringToWString(Param(p, "key", "")), (_float)atof(Param(p, "value", "0").c_str()));
			return Status::Success;
		});

	pReg->Register_Task("Task_AnimFaceTarget",
		[](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			auto* pSelf = pBB->Get_Object<CAIObject>(L"Self");
			if (nullptr == pSelf) return Status::Failure;
			_vector vPos;
			if (false == pSelf->Get_TargetPos(vPos)) return Status::Failure;
			if (pSelf->Can_AnimRotate() && nullptr != pSelf->Get_MovementCom())
				pSelf->Get_MovementCom()->TurnTo(vPos, pBB->Get<_float>(L"fTimeDelta"));
			return Status::Running;
		});

	pReg->Register_Task("Task_AnimMoveToTarget",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			auto* pSelf = pBB->Get_Object<CAIObject>(L"Self");
			if (nullptr == pSelf) return Status::Failure;
			const _float fSpeed = (_float)atof(Param(p, "speed", "1.0").c_str());
			pSelf->Set_AnimFloat(L"Speed", fSpeed);
			_vector vPos;
			if (false == pSelf->Get_TargetPos(vPos)) return Status::Failure;
			if (pSelf->Can_AnimMove() && nullptr != pSelf->Get_MovementCom())
				pSelf->Get_MovementCom()->MoveTo_Navi(vPos, pBB->Get<_float>(L"fTimeDelta"));
			return Status::Running;
		});

	pReg->Register_Task("Task_AnimAction",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			auto* pSelf = pBB->Get_Object<CAIObject>(L"Self");
			if (nullptr == pSelf) return Status::Failure;
			const _wstring strAction = StringToWString(Param(p, "action", ""));
			const _wstring strTrigger = StringToWString(Param(p, "trigger", ""));
			const _wstring strIntKey = StringToWString(Param(p, "intKey", ""));
			const _int     iIntVal = atoi(Param(p, "int", "0").c_str());
			if (false == strIntKey.empty())  pSelf->Set_AnimInt(strIntKey, iIntVal);
			if (false == strTrigger.empty()) pSelf->Set_AnimTrigger(strTrigger);
			if (false == strAction.empty())  pSelf->Request_AnimAction(strAction);
			return Status::Success;
		});

	pReg->Register_Task("Task_HoldBB",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			return pBB->Get<_bool>(StringToWString(Param(p, "key", ""))) ? Status::Running : Status::Success;
		});

	pReg->Register_Task("Task_AnimWaitState",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			auto* pSelf = pBB->Get_Object<CAIObject>(L"Self");
			if (nullptr == pSelf) return Status::Failure;
			const _wstring strState = StringToWString(Param(p, "state", ""));
			if (false == pSelf->Is_AnimInState(strState)) return Status::Running;
			return pSelf->Is_AnimationFinished() ? Status::Success : Status::Running;
		});

	pReg->Register_Task("Task_Revive",
		[](Engine::CBT_BlackBoard* pBB, const Params& p)->Status {
			if (nullptr == pBB) return Status::Failure;
			auto* pSelf = pBB->Get_Object<CAIObject>(L"Self");
			if (nullptr == pSelf) return Status::Failure;
			pSelf->Revive();
			return Status::Success;
		});

	pReg->Register_Condition("Cond_AnimInState",
		[Param](Engine::CBT_BlackBoard* pBB, const Params& p)->_bool {
			if (nullptr == pBB) return false;
			auto* pSelf = pBB->Get_Object<CAIObject>(L"Self");
			return pSelf ? pSelf->Is_AnimInState(StringToWString(Param(p, "state", ""))) : false;
		});
}

void CAIObject::Release_Registry()
{
	Safe_Release(s_pRegistry);
}

void CAIObject::Begin_GrappleGrab()
{
	On_Perceive_Attacker(false);
	Set_Freeze(true);

	if (false == m_strGrappleStartCue.empty() && nullptr != m_pTransformCom)
	{
		_float3 vWorldPos;
		XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

		m_pGameInstance->Play_Cue(m_strGrappleStartCue, SOUND_MONSTER, vWorldPos);
	}
}

void CAIObject::On_GrapplePullStart()
{
	if (true == m_strGrapplePullCue.empty() || nullptr == m_pTransformCom)
		return;

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Play_Cue(m_strGrapplePullCue, SOUND_MONSTER, vWorldPos);
}

void CAIObject::On_GrapplePulled(_fvector vTargetPos)
{
	m_pCCT->Set_FootPosition(vTargetPos);
	m_pTransformCom->Set_State(STATE::POSITION, vTargetPos);
}

void CAIObject::End_GrappleGrab()
{
	Set_Freeze(false);
}
_float CAIObject::Get_CCTRadius() const
{
	if (!m_pCCT) return 0.f;
	return m_pCCT->Get_Radius();
}

#ifdef _DEBUG
HRESULT CAIObject::Set_CCTRadius(_float fRadius)
{
	m_fCCTRadius = fRadius;
	m_pCCT->Resize_Radius(fRadius);
	return S_OK;
}

HRESULT CAIObject::Set_CCTHeight(_float fHeight)
{
	m_fCCTHeight = fHeight;
	m_pCCT->Resize(fHeight);
	return S_OK;
}
HRESULT CAIObject::Set_DebugRange(_float fRadius)
{
	m_fDbgRange = fRadius;
	return S_OK;
}
const vector<_float3>& CAIObject::Get_DebugNaviPath() const
{
	return m_pMovementCom->Get_DebugNaviPath();
}

_uint CAIObject::Get_DebugNaviPathIndex() const
{
	return m_pMovementCom->Get_DebugNaviPathIndex();
}
static _float4 Debug_StateColor(_int iState)
{
	switch (static_cast<CAIObject::DBG_MOVE>(iState))
	{
	case CAIObject::DBG_MOVE::IDLE:     return _float4(1.f, 1.f, 1.f, 1.f);
	case CAIObject::DBG_MOVE::CHASE:    return _float4(0.f, 1.f, 0.f, 1.f);
	case CAIObject::DBG_MOVE::APPROACH: return _float4(0.f, 1.f, 1.f, 1.f);
	case CAIObject::DBG_MOVE::STRAFE:   return _float4(1.f, 1.f, 0.f, 1.f);
	case CAIObject::DBG_MOVE::COMBAT:   return _float4(1.f, 0.f, 0.f, 1.f);
	case CAIObject::DBG_MOVE::LEASH:    return _float4(0.3f, 0.5f, 1.f, 1.f);
	case CAIObject::DBG_MOVE::GROGGY:   return _float4(1.f, 0.f, 1.f, 1.f);
	case CAIObject::DBG_MOVE::HIT:      return _float4(1.f, 0.5f, 0.f, 1.f);
	case CAIObject::DBG_MOVE::FATAL:    return _float4(0.2f, 0.2f, 0.2f, 1.f);
	case CAIObject::DBG_MOVE::SPAWN:    return _float4(0.5f, 0.5f, 0.5f, 1.f);
	case CAIObject::DBG_MOVE::FROZEN:   return _float4(0.f, 0.6f, 0.9f, 1.f);
	case CAIObject::DBG_MOVE::DEAD:     return _float4(0.1f, 0.1f, 0.1f, 1.f);
	}
	return _float4(1.f, 1.f, 1.f, 1.f);
}

static const char* Debug_StateName(_int iState)
{
	switch (static_cast<CAIObject::DBG_MOVE>(iState))
	{
	case CAIObject::DBG_MOVE::IDLE:     return "IDLE";
	case CAIObject::DBG_MOVE::CHASE:    return "CHASE";
	case CAIObject::DBG_MOVE::APPROACH: return "APPROACH";
	case CAIObject::DBG_MOVE::STRAFE:   return "STRAFE";
	case CAIObject::DBG_MOVE::COMBAT:   return "COMBAT";
	case CAIObject::DBG_MOVE::LEASH:    return "LEASH";
	case CAIObject::DBG_MOVE::GROGGY:   return "GROGGY";
	case CAIObject::DBG_MOVE::HIT:      return "HIT";
	case CAIObject::DBG_MOVE::FATAL:    return "FATAL";
	case CAIObject::DBG_MOVE::SPAWN:    return "SPAWN";
	case CAIObject::DBG_MOVE::FROZEN:   return "FROZEN";
	case CAIObject::DBG_MOVE::DEAD:     return "DEAD";
	}
	return "IDLE";
}

_int CAIObject::Get_DebugMoveState() const
{
	if (nullptr == m_pBlackBoard)
		return static_cast<_int>(DBG_MOVE::IDLE);

	if (true == Is_Dead() || true == m_pBlackBoard->Get<_bool>(L"bDead"))
		return static_cast<_int>(DBG_MOVE::DEAD);

	if (true == m_pBlackBoard->Get<_bool>(L"bFatal"))
		return static_cast<_int>(DBG_MOVE::FATAL);

	if (true == m_bFrozen)
		return static_cast<_int>(DBG_MOVE::FROZEN);

	if (true == m_pBlackBoard->Get<_bool>(L"bGroggy"))
		return static_cast<_int>(DBG_MOVE::GROGGY);

	if (true == m_pBlackBoard->Get<_bool>(L"bHit"))
		return static_cast<_int>(DBG_MOVE::HIT);

	if (false == m_bSpawned)
		return static_cast<_int>(DBG_MOVE::SPAWN);

	if (true == m_bReturning)
		return static_cast<_int>(DBG_MOVE::LEASH);

	if (false == m_bAggro)
		return static_cast<_int>(DBG_MOVE::IDLE);

	if (false == m_pBlackBoard->Get<_bool>(L"bInAttackRange"))
		return static_cast<_int>(DBG_MOVE::CHASE);

	if (true == m_bApproaching)
		return static_cast<_int>(DBG_MOVE::APPROACH);

	return static_cast<_int>(DBG_MOVE::COMBAT);
}

_float CAIObject::Get_DebugReachRadius() const
{
	return (nullptr != m_pMovementCom) ? m_pMovementCom->Get_ReachRadius() : 0.f;
}

void CAIObject::Debug_UpdateMoveState()
{
	//	const _int iState = Get_DebugMoveState();
	//
	//	const ANIM_CONTROLLER_STATE* pAnimState = (nullptr != m_pAnimator) ? m_pAnimator->Get_CurControllerState() : nullptr;
	//	const _uint64 iAnimHash = (nullptr != m_pAnimator) ? m_pAnimator->Get_CurControllerStateHash() : 0;
	//
	//	if (iState == m_iLastDbgMoveState && iAnimHash == m_iLastDbgAnimHash)
	//		return;
	//
	//	if (iState != m_iLastDbgMoveState)
	//	{
	//		m_vDbgColor = Debug_StateColor(iState);
	//		Rebuild_DebugRange();
	//	}
	//
	//	m_iLastDbgMoveState = iState;
	//	m_iLastDbgAnimHash = iAnimHash;
	//
	//	_vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
	//	const char* szAnim = (nullptr != pAnimState) ? pAnimState->strName.c_str() : "-";
	//	char szBuf[192] = {};
	//	sprintf_s(szBuf, 192, "[AI %p] %-9s anim=%-18s pos(%.1f, %.1f, %.1f)\n",
	//		static_cast<const void*>(this), Debug_StateName(iState), szAnim,
	//		XMVectorGetX(vPos), XMVectorGetY(vPos), XMVectorGetZ(vPos));
	//	OutputDebugStringA(szBuf);
}

void CAIObject::Rebuild_DebugRange()
{
	Safe_Release(m_pDebugRange);

	CDebugCapsule::DEBUGCAPSULE_OBJ_DESC Desc{};
	Desc.fRadius = (m_fDbgRange > 0.f) ? m_fDbgRange : 1.f;
	Desc.fHalfHeight = 1.f;
	Desc.vColor = m_vDbgColor;
	Desc.iSegment = 32;

	m_pDebugRange = CDebugCapsule::Create(m_pDevice, m_pContext, Desc);
	m_fLastDbgRange = m_fDbgRange;
}

#endif

HRESULT CAIObject::Ready_Character(const CHARACTER_SETUP_DESC& Desc)
{
	if (FAILED(__super::Add_Component(Desc.iModelLevel, Desc.strModelPrototypeTag,
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	m_MeshVisibles.assign(m_pModelCom->Get_RenderUnits().size(), true);

	CAnimator::ANIMATOR_DESC AnimDesc{};
	AnimDesc.BasePose = *m_pModelCom->Get_BindPosePtr();
	AnimDesc.RawAnimationLoadDesc.strFolderName = Desc.strAnimFolderTag;
	AnimDesc.RawAnimationLoadDesc.strAnimationBaseName = Desc.strAnimTag;
	AnimDesc.RawAnimationLoadDesc.eTimeUnit = ANIMATION_TIME_UNIT::MILLISEC;
	AnimDesc.RawAnimationLoadDesc.bStaticAnimation = true;

	const vector<CBone*>& Bones = m_pModelCom->Get_Bones();
	AnimDesc.Bones.clear();
	AnimDesc.Bones.reserve(Bones.size());
	for (_uint i = 0; i < Bones.size(); ++i)
	{
		CAnimator::ANIMATOR_BONE_DESC BoneDesc{};
		if (nullptr != Bones[i])
		{
			BoneDesc.strName = Bones[i]->Get_Name();
			_wstring strBoneName;
			strBoneName.assign(BoneDesc.strName.begin(), BoneDesc.strName.end());
			BoneDesc.iNameHash = Hash_String(strBoneName.c_str());
			BoneDesc.iParentIndex = Bones[i]->Get_ParentIndex();
		}
		AnimDesc.Bones.push_back(BoneDesc);
	}
	AnimDesc.PreTransformMatrix = m_pModelCom->Get_PreTransformMatrix();

	m_pAnimator = CAnimator::Create(AnimDesc);
	if (nullptr == m_pAnimator)
		return E_FAIL;

	const _int iRootBone = m_pModelCom->Get_BoneIndex(Desc.pRootBoneName);
	m_pAnimator->Set_RootBoneIndex(iRootBone);

	if (true == Desc.bUseCCT)
	{
		CCT::CONTROLLER_DESC CCTDesc{};
		CCTDesc.eControllerType = Desc.eControllerType;
		CCTDesc.fRadius = Desc.fCCTRadius;
		CCTDesc.fHeight = Desc.fCCTHeight;
		CCTDesc.vBoxHalf = Desc.vCCTBoxHalf;
		CCTDesc.fStepOffset = Desc.fCCTStepOffset;
		CCTDesc.fSlopeLimit = cosf(XMConvertToRadians(Desc.fCCTSlopeLimitDeg));
		CCTDesc.pOwner = this;
		XMStoreFloat3(&CCTDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
		XMStoreFloat4(&CCTDesc.vInitQuat, m_pTransformCom->Get_Quat());
		CCTDesc.iFilterWord0 = ETOUI(FILTER_GROUP::MONSTER);
		CCTDesc.iFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT);
		if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_CCT"),
			TEXT("Com_Physics"), reinterpret_cast<CComponent**>(&m_pCCT), &CCTDesc)))
			return E_FAIL;
	}

	if (nullptr != m_pCCT)
	{
		CMovement_AI::MOVEMENT_DESC MoveDesc{};
		MoveDesc.pOwner = this;
		MoveDesc.pTransform = m_pTransformCom;
		MoveDesc.pNavigation = m_pNavigation;
		MoveDesc.fRunSpeed = Desc.fRunSpeed;
		MoveDesc.fSprintSpeed = Desc.fSprintSpeed;
		MoveDesc.fAcceleration = Desc.fMoveAccel;
		MoveDesc.fDeceleration = Desc.fMoveDecel;
		MoveDesc.fGroundStick = Desc.fGroundStick;
		MoveDesc.fNaviRepathInterval = 0.f;
		MoveDesc.fRotationPerSec = 0.f;
		MoveDesc.fAgentRadius = (Desc.eControllerType == CCT_TYPE::BOX) ? Desc.vCCTBoxHalf.x : Desc.fCCTRadius;
		if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Movement_AI"),
			TEXT("Com_Movement"), reinterpret_cast<CComponent**>(&m_pMovementCom), &MoveDesc)))
			return E_FAIL;
	}

	return S_OK;
}


HRESULT CAIObject::Ready_Components()
{
	m_pBehaviorTree = CBehaviorTree::Create();
	if (nullptr == m_pBehaviorTree)
	{
		MSG_BOX("Failed to Create : CBehaviorTree (CAIObject)");
		return E_FAIL;
	}

	m_pBlackBoard = m_pBehaviorTree->Get_BlackBoard();

	return S_OK;
}

HRESULT CAIObject::Bind_Navigation(CNavigation* pNavigation)
{
	if (m_pNavigation == pNavigation)
		return S_OK;

	Safe_Release(m_pNavigation);
	m_pNavigation = pNavigation;
	if (nullptr != m_pNavigation)
		Safe_AddRef(m_pNavigation);

	if (nullptr != m_pMovementCom)
		m_pMovementCom->Set_Navigation(m_pNavigation);

	m_iNaviCurCellIndex = -1;
	if (nullptr != m_pNavigation && nullptr != m_pTransformCom)
	{
		_float3 vPos = {};
		XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));
		m_pNavigation->Set_CurrentCell(vPos, &m_iNaviCurCellIndex);
	}

	return S_OK;
}

HRESULT CAIObject::Ready_BehaviorTree()
{
	if (nullptr == m_pBehaviorTree)
		return E_FAIL;

	CBT_BlackBoard* pBlackBoard = m_pBehaviorTree->Get_BlackBoard();
	if (nullptr == pBlackBoard)
		return E_FAIL;

	pBlackBoard->Set_Object<CAIObject>(L"Self", this);

	Build_BehaviorTree();

	return S_OK;
}

void CAIObject::Build_BehaviorTree()
{
	if (false == m_strBTPath.empty())
		Load_BehaviorTree(m_strBTPath);
}

HRESULT CAIObject::Load_BehaviorTree(const _wstring& strPath)
{
	if (nullptr == m_pBehaviorTree || true == strPath.empty())
		return E_FAIL;

	int iSize = ::WideCharToMultiByte(CP_UTF8, 0, strPath.c_str(), (int)strPath.size(), nullptr, 0, nullptr, nullptr);
	std::string strUtf8Path(iSize, 0);
	::WideCharToMultiByte(CP_UTF8, 0, strPath.c_str(), (int)strPath.size(), &strUtf8Path[0], iSize, nullptr, nullptr);

	Engine::BT_NODE_MODEL* pModel = nullptr;
	if (false == Engine::CBT_JsonSerializer::Load(strUtf8Path, &pModel) || nullptr == pModel)
	{
		MSG_BOX("Failed to Load BT (CAIObject::Load_BehaviorTree)");
		return E_FAIL;
	}

	Engine::CBT_Node* pRoot = Engine::CBT_TreeCompiler::Compile(pModel, Registry());

	Safe_Delete(pModel);

	if (nullptr == pRoot)
	{
		MSG_BOX("Failed to Compile BT (CAIObject::Load_BehaviorTree)");
		return E_FAIL;
	}

	m_pBehaviorTree->Set_Root(pRoot);

	return S_OK;
}

void CAIObject::Update_BlackBoard(_float fTimeDelta)
{
	CBT_BlackBoard* pBlackBoard = m_pBehaviorTree->Get_BlackBoard();
	if (nullptr == pBlackBoard)
		return;

	Set_Groggy(pBlackBoard->Get<_bool>(L"bGroggy"));

	pBlackBoard->Set<_float>(L"fTimeDelta", fTimeDelta);

	if (nullptr != m_pTarget)
	{
		pBlackBoard->Set_Object<CGameObject>(L"pTarget", m_pTarget);

		if (nullptr != m_pTransformCom)
		{
			CTransform* pTargetTransform = m_pTarget->Get_Transform();
			if (nullptr != pTargetTransform)
			{
				_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
				_vector vTargetPos = pTargetTransform->Get_State(STATE::POSITION);
				_float  fDist = XMVectorGetX(XMVector3Length(XMVectorSubtract(vTargetPos, vMyPos)));
				pBlackBoard->Set<_float>(L"fDistToTarget", fDist);
			}
		}
	}
	else
	{
		pBlackBoard->Remove_Object(L"pTarget");
	}

	pBlackBoard->Set<_bool>(L"bDead", Is_Dead() && false == pBlackBoard->Get<_bool>(L"bFatal"));
}

void CAIObject::Update_Activation()
{
	_bool bInFront = false;
	if (nullptr != m_pTransformCom && nullptr != m_pTarget)
	{
		if (CTransform* pTT = m_pTarget->Get_Transform())
		{
			_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
			_vector vDir = XMVector3Normalize(XMVectorSetY(
				XMVectorSubtract(pTT->Get_State(STATE::POSITION), vMyPos), 0.f));
			_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
			bInFront = (XMVectorGetX(XMVector3Dot(vLook, vDir)) >= m_fFrontCos);
		}
	}
	m_pBlackBoard->Set<_bool>(L"bPlayerInFront", bInFront);
}

void CAIObject::Compute_HitDir(_fvector vHitPosition)
{
	if (nullptr == m_pTransformCom)
		return;

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vToHit = XMVectorSetY(XMVectorSubtract(vHitPosition, vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vToHit)) < 1e-6f)
		vToHit = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);

	vToHit = XMVector3Normalize(vToHit);

	_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
	_vector vRight = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::RIGHT), 0.f));

	const _float fForwardDot = XMVectorGetX(XMVector3Dot(vLook, vToHit));
	const _float fRightDot = XMVectorGetX(XMVector3Dot(vRight, vToHit));

	const _float fAngle = XMConvertToDegrees(atan2f(fRightDot, fForwardDot));

	const _float BACK_HIT_ANGLE_DEG = 110.f;
	if (fabsf(fAngle) >= BACK_HIT_ANGLE_DEG)
		m_iHitDir = static_cast<_int>(MON_DIRECTION::BACK);
	else
		m_iHitDir = (fAngle > 0.f)
		? static_cast<_int>(MON_DIRECTION::RIGHT)
		: static_cast<_int>(MON_DIRECTION::LEFT);
}

void CAIObject::Record_HitType(const HitInfo& hitInfo)
{
	m_pStatCom->Set_LastHitType(hitInfo.eHitType);
}

void CAIObject::On_Perceive_Attacker(_bool bPreFatal)
{
	if (false == bPreFatal)
	{
		m_bAggro = true;
		m_fLostTimer = 0.f;
		m_bSpawned = true;
		Cancel_Leash();
	}
}

void CAIObject::Apply_ChargeHit(_float fDamage, _float fGroggy)
{
	m_pStatCom->Apply_Damage(fDamage);
	if (Is_Dead())
	{
		if (true == Uses_Full_Hit_Reaction())
			m_pBlackBoard->Set<_bool>(L"bDead", true);
		return;
	}

	if (true == m_pStatCom->Is_Staggerable() && false == m_pStatCom->Is_OnGroggyCooldown())
	{
		m_pStatCom->Reset_Groggy();
		m_pStatCom->Set_Staggerable(false);
		Set_Groggy(true);
		Request_AnimAction(L"GROGGY");
		Set_AnimFloat(L"Speed", 0.f);
		On_Groggy_Triggered();
	}
	else if (false == Is_Groggy())
	{
		if (false == Is_SuperArmor() && true == Uses_Full_Hit_Reaction())
			m_pBlackBoard->Set<_bool>(L"bHit", true);
		m_pStatCom->Add_Groggy(fGroggy);
		if (m_pStatCom->Is_GroggyFull())
			m_pStatCom->Set_Staggerable(true);
	}
}

void CAIObject::Capture_SpawnPos()
{
	if (m_bSpawnCaptured || nullptr == m_pTransformCom)
		return;

	XMStoreFloat4(&m_vSpawnPos, m_pTransformCom->Get_State(STATE::POSITION));
	m_bSpawnCaptured = true;
}

void CAIObject::Drive_Animator(_float fTimeDelta)
{
	if (nullptr == m_pAnimator)
		return;

	m_pAnimator->Update(fTimeDelta);

#ifdef _DEBUG
	Debug_TraceAnim();
#endif

	const _uint64 iCurStateHash = Get_CurAnimStateHash();
	if (iCurStateHash != m_iCurAnimStateHash)
	{
		const _uint64 iPrevStateHash = m_iCurAnimStateHash;
		m_iCurAnimStateHash = iCurStateHash;
		On_AnimStateChanged(iPrevStateHash, iCurStateHash);
	}

	for (const ANIM_NOTIFY* pNotify : m_pAnimator->Get_TriggeredNotifies())
	{
		if (nullptr == pNotify)
			continue;

		switch (pNotify->eType)
		{
		case ANIM_NOTIFY_TYPE::CUSTOM:
			if (true == Handle_CustomNotify(pNotify))
				break;

			if (false == pNotify->strPayload.empty())
				Action_Notify(pNotify->iPayloadHash);
			break;

		case ANIM_NOTIFY_TYPE::SOUND:
			Handle_SoundNotify(pNotify);
			break;

		case ANIM_NOTIFY_TYPE::FOOTSTEP:
			Handle_FootStepNotify(pNotify);
			break;

		case ANIM_NOTIFY_TYPE::EFFECT:
			Handle_EffectNotify(pNotify);
			break;

		case ANIM_NOTIFY_TYPE::CAMERA_SHAKE:
			Handle_CamShakeNotify(pNotify);
			break;

		default:
			break;
		}
	}

	Drive_LoopSounds();
	Drive_CamShakeWindows();

	const ANIM_ROOT_MOTION_DELTA& Root = m_pAnimator->Get_RootMotionDelta();
	Apply_RootMotion(Root.vTranslation, Root.vRotation);
}

void CAIObject::Drive_SuperArmor()
{
	const _bool bSuperArmor = Is_SuperArmor();
	if (bSuperArmor != m_bSuperArmorPrev)
	{
		On_SuperArmor(bSuperArmor);
		m_bSuperArmorPrev = bSuperArmor;
	}
}

void CAIObject::Handle_EffectNotify(const ANIM_NOTIFY* pNotify)
{
	if (nullptr == pNotify || true == pNotify->strPayload.empty() || nullptr == m_pModelCom)
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

CEffect* CAIObject::Start_LoopEffect_World(_uint64 iKey, const string& strFxName, const _float3& vWorldPos, _bool bAnimScoped)
{
	Stop_LoopEffect(iKey);

	CEffect* pFx = m_pGameInstance->Play_Effect(strFxName, vWorldPos);
	if (nullptr != pFx)
	{
		LOOP_EFFECT_ENTRY Entry{};
		Entry.pEffect = pFx;
		Entry.bAnimScoped = bAnimScoped;
		Entry.iOwnerStateHash = Get_CurAnimStateHash();
		m_LoopEffects[iKey] = Entry;
	}

	return pFx;
}

CEffect* CAIObject::Start_LoopEffect(_uint64 iKey, const string& strFxName, const string& strBoneName, const _float3& vOffset, _bool bAnimScoped)
{
	Stop_LoopEffect(iKey);

	CEffect* pFx = nullptr;

	if (nullptr != m_pModelCom && false == strBoneName.empty())
		pFx = m_pGameInstance->Play_Effect(strFxName, m_pModelCom, strBoneName.c_str(),
			m_pTransformCom->Get_WorldMatrixPtr(), vOffset);
	else
		pFx = m_pGameInstance->Play_Effect(strFxName, m_pTransformCom->Get_WorldMatrixPtr(), nullptr, vOffset);

	if (nullptr != pFx)
	{
		LOOP_EFFECT_ENTRY Entry{};
		Entry.pEffect = pFx;
		Entry.bAnimScoped = bAnimScoped;
		Entry.iOwnerStateHash = Get_CurAnimStateHash();
		m_LoopEffects[iKey] = Entry;
	}

	return pFx;
}

void CAIObject::Stop_LoopEffect(_uint64 iKey)
{
	auto it = m_LoopEffects.find(iKey);
	if (m_LoopEffects.end() == it)
		return;

	if (nullptr != it->second.pEffect)
	{
		it->second.pEffect->Stop_Follow();
		it->second.pEffect->Stop();
	}

	m_LoopEffects.erase(it);
}

void CAIObject::Stop_LoopImmediate(_uint64 iKey)
{
	auto it = m_LoopEffects.find(iKey);
	if (m_LoopEffects.end() == it)
		return;

	if (nullptr != it->second.pEffect)
	{
		it->second.pEffect->Stop_Follow();
		it->second.pEffect->Stop_Immediate();
	}

	m_LoopEffects.erase(it);
}

void CAIObject::Stop_AllLoopEffects()
{
	for (auto& Pair : m_LoopEffects)
	{
		if (nullptr != Pair.second.pEffect)
		{
			Pair.second.pEffect->Stop_Follow();
			Pair.second.pEffect->Stop();
		}
	}

	m_LoopEffects.clear();
}

void CAIObject::Stop_AnimScopedLoopEffects(_uint64 iKeepStateHash)
{
	for (auto it = m_LoopEffects.begin(); it != m_LoopEffects.end(); )
	{
		LOOP_EFFECT_ENTRY& Entry = it->second;

		if (false == Entry.bAnimScoped || Entry.iOwnerStateHash == iKeepStateHash)
		{
			++it;
			continue;
		}

		if (nullptr != Entry.pEffect)
		{
			Entry.pEffect->Stop_Follow();
			Entry.pEffect->Stop();
		}

		it = m_LoopEffects.erase(it);
	}
}

_uint64 CAIObject::Get_CurAnimStateHash() const
{
	if (nullptr == m_pAnimator)
		return 0;

	const ANIM_CONTROLLER_STATE* pState = m_pAnimator->Get_CurControllerState();
	return (nullptr != pState) ? pState->iNameHash : 0;
}

void CAIObject::On_AnimStateChanged(_uint64 iPrevStateHash, _uint64 iCurStateHash)
{
	Stop_AnimScopedLoopEffects(iCurStateHash);
}

void CAIObject::Begin_GroggyWindow(_float fSeconds)
{
	m_fGroggyWindow = fSeconds;
	m_bGroggyWindowActive = true;
}

void CAIObject::Tick_GroggyWindow(_float fTimeDelta)
{
	if (false == m_bGroggyWindowActive)
		return;

	if (false == Is_Groggy())
	{
		m_bGroggyWindowActive = false;
		return;
	}

	m_fGroggyWindow -= fTimeDelta;

	if (m_fGroggyWindow <= 0.f)
	{
		m_bGroggyWindowActive = false;
		On_GroggyTimeout();
	}
}

_bool CAIObject::Can_Perceive_Target() const
{
	if (nullptr == m_pTarget || nullptr == m_pTransformCom)
		return false;

	CTransform* pTargetTransform = m_pTarget->Get_Transform();
	if (nullptr == pTargetTransform)
		return false;

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTo = XMVectorSetY(
		XMVectorSubtract(pTargetTransform->Get_State(STATE::POSITION), vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vTo)) > 1e-6f)
	{
		vTo = XMVector3Normalize(vTo);
		_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));

		const _float fCos = XMVectorGetX(XMVector3Dot(vLook, vTo));
		const _float fHalfCos = cosf(XMConvertToRadians(m_fPerceiveFOVDegree * 0.5f));
		if (fCos < fHalfCos)
			return false;
	}

	return Can_See(m_pTarget, m_fPerceiveEyeHeight, m_ePerceiveBlockers, m_fPerceiveLOSRadius);
}

void CAIObject::Update_Aggro_WithLOS(_float fDist, _float fTimeDelta)
{
	const _bool bInAcquireRange = (fDist <= m_fDetectRange);
	const _bool bInKeepRange = (fDist <= m_fDetectRange * m_fAggroKeepScale);

	if (false == m_bAggro)
	{
		if (true == bInAcquireRange && true == Can_Perceive_Target())
		{
			m_bAggro = true;
			m_fLostTimer = 0.f;
		}
	}
	else
	{
		if (true == bInKeepRange)
			m_fLostTimer = 0.f;
		else
		{
			m_fLostTimer += fTimeDelta;
			if (m_fLostTimer >= m_fLoseDelay)
			{
				m_bAggro = false;
				m_fLostTimer = 0.f;
			}
		}
	}
}



_float CAIObject::Calc_RootMotionScale() const
{
	if (nullptr == m_pAnimator)
		return 1.f;

	const vector<const ANIM_WINDOW*>& vecActiveWindows = m_pAnimator->Get_ActiveCustomWindows();

	_float	fScale = 1.f;
	_bool	bHasMax = false;
	_bool	bHasOffset = false;
	_float	fMax = 1.f;
	_float	fOffset = 0.f;

	for (const ANIM_WINDOW* pWindow : vecActiveWindows)
	{
		if (nullptr == pWindow)
			continue;

		if (pWindow->iPayloadHash == Hash_String(L"RootMotionScale"))
			fScale *= pWindow->fValue;
		else if (pWindow->iPayloadHash == Hash_String(L"RootMotionScale_Max"))
		{
			bHasMax = true;
			fMax = pWindow->fValue;
		}
		else if (pWindow->iPayloadHash == Hash_String(L"RootMotionScale_Offset"))
		{
			bHasOffset = true;
			fOffset = pWindow->fValue * ROOTMOTION_OFFSET_TO_METER;
		}
	}

	if (true == bHasMax && true == bHasOffset && fOffset > 0.f)
	{
		_vector vTargetPos;

		if (true == Get_TargetPos(vTargetPos))
		{
			_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
			_float fDist = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f)));

			_float fDistScale = fDist / fOffset;
			fDistScale = max(1.f, min(fDistScale, fMax));

			fScale *= fDistScale;
		}
	}
	else if (true == bHasMax)
		fScale *= fMax;

	return fScale;
}

void CAIObject::Apply_Movment(_float fTimeDelta)
{
	if (nullptr == m_pMovementCom || nullptr == m_pCCT)
		return;

	if (true == m_bFrozen)
		return;

	_bool bHasRootMotion = false;
	if (nullptr != m_pAnimator)
	{
		const ANIM_CLIP_SETTING* pCurClip = m_pAnimator->Get_CurClip();
		bHasRootMotion = (nullptr != pCurClip) && pCurClip->bUseRootMotion;

		const ANIM_ROOT_MOTION_DELTA& RootDelta = m_pAnimator->Get_RootMotionDelta();

		const _float fRootMotionScale = Calc_RootMotionScale();

		TransformData tRootDelta = {};
		tRootDelta.vScale = _float4(1.f, 1.f, 1.f, 0.f);
		tRootDelta.vRot = RootDelta.vRotation;
		tRootDelta.vTranslation = _float4(RootDelta.vTranslation.x * fRootMotionScale, RootDelta.vTranslation.y * fRootMotionScale, RootDelta.vTranslation.z * fRootMotionScale, 0.f);

		m_pMovementCom->Push_RootMotion(bHasRootMotion, tRootDelta);
	}

	CMovement_AI::MOVEMENT_RESULT tMoveResult = m_pMovementCom->Compute_Movement(fTimeDelta);

	if (true == tMoveResult.bChangedQuat)
		m_pTransformCom->Set_Quat(XMLoadFloat4(&tMoveResult.vMoveQuat));

	_vector vFinalMove = XMLoadFloat3(&tMoveResult.vMoveDir);

	if (false == Can_AnimMove() && false == bHasRootMotion && false == m_pMovementCom->Is_MoveActive()
		&& false == m_pMovementCom->Is_Launching())
		vFinalMove = XMVectorSet(0.f, XMVectorGetY(vFinalMove), 0.f, 0.f);

	CCTResult tCCTResult = m_pCCT->Move(vFinalMove, fTimeDelta);

	m_pMovementCom->Set_Grounded(tCCTResult.m_bGrounded);
	m_pMovementCom->Set_SideBlocked(tCCTResult.m_bSideHit);

	_vector vSimPos = XMLoadFloat4(&tCCTResult.vSimulatedPosition);

	if (nullptr != m_pNavigation && m_iNaviCurCellIndex >= 0
		&& false == m_pMovementCom->Is_Launching())
	{
		_float3 vProbe = {};
		XMStoreFloat3(&vProbe, vSimPos);

		_float3 vOut = {};
		if (false == m_pNavigation->Move_OnNavigation(vProbe, &m_iNaviCurCellIndex, &vOut))
		{
			_vector vHold = m_pTransformCom->Get_State(STATE::POSITION);
			vSimPos = XMVectorSetY(vHold, XMVectorGetY(vSimPos));
			m_pCCT->Set_FootPosition(vSimPos);
		}
	}

	m_pTransformCom->Set_State(STATE::POSITION, vSimPos);
}


void CAIObject::Cancel_All_Attacks()
{
	if (nullptr != m_pAnimator)
	{
		m_pAnimator->Reset_PlaybackState();
		m_pAnimator->Set_ControllerEnabled(true);
		m_pAnimator->Resume();
	}

	if (CBehaviorTree* pBT = Get_BehaviorTree())
		pBT->Reset();

	if (nullptr != m_pMovementCom)
		m_pMovementCom->Stop_MoveTo();

	Set_AnimFloat(L"Speed", 0.f);
	m_iPendingPatternID = -1;
	m_bApproaching = false;
}

void CAIObject::Begin_Pattern(_int iPatternID, _float fAttackLength)
{
	m_iPendingPatternID = iPatternID;
	m_fPendingAttackLength = fAttackLength;
	m_bApproaching = false;
}

#ifdef _DEBUG
_bool CAIObject::s_bTraceAnim = false;

void CAIObject::Debug_TraceAnim()
{
	if (false == s_bTraceAnim || nullptr == m_pAnimator)
		return;

	const ANIM_CLIP_SETTING* pClip = m_pAnimator->Get_CurClip();
	if (nullptr == pClip)
		return;

	_tchar szLog[512] = {};

	if (m_iDebugLastClipHash != pClip->iClipHash)
	{
		m_iDebugLastClipHash = pClip->iClipHash;

		const ANIM_CONTROLLER_STATE* pState = m_pAnimator->Get_CurControllerState();

		const wstring strClipName = StringToWString(pClip->strSourceName);
		const wstring strStateName = (nullptr != pState) ? StringToWString(pState->strName) : wstring(L"-");

		swprintf_s(szLog, L"[Anim] %-36s state=%-14s frames=%u\n",
			strClipName.c_str(),
			strStateName.c_str(),
			m_pAnimator->Get_CurClipFrameCount());

		OutputDebugStringW(szLog);
	}

	for (const ANIM_NOTIFY* pNotify : m_pAnimator->Get_TriggeredNotifies())
	{
		if (nullptr == pNotify)
			continue;

		const wstring strNotifyName = StringToWString(pNotify->strName);

		swprintf_s(szLog, L"    [Notify] f%-4u (now f%-4u) type=%d  %s\n",
			pNotify->iFrame,
			m_pAnimator->Get_LocalFrame(),
			static_cast<_int>(pNotify->eType),
			strNotifyName.c_str());

		OutputDebugStringW(szLog);
	}
}

void CAIObject::Toggle_TraceAnim()
{
	s_bTraceAnim = !s_bTraceAnim;
	OutputDebugStringW(s_bTraceAnim ? L"[Anim] trace ON\n" : L"[Anim] trace OFF\n");
}

#endif

void CAIObject::Free()
{
	Stop_AllLoopSounds();

	__super::Free();

	Safe_Release(m_pStatCom);
	Safe_Release(m_pAnimator);
	Safe_Release(m_pCCT);
	Safe_Release(m_pMovementCom);
	Safe_Release(m_pNavigation);
	Safe_Release(m_pBehaviorTree);
	m_pBlackBoard = nullptr;

#ifdef _DEBUG
	Safe_Release(m_pDebugRange);
#endif
}
