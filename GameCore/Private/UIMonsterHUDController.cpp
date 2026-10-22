#include "UIMonsterHUDController.h"
#include "GameInstance.h"
#include "Layer.h"
#include "AIObject.h"
#include "StatComponent_AI.h"
#include "UIObject.h"
#include "UIElement.h"
#include "UIProgressBar.h"
#include "Boss_Festival.h"
#include "MonsterHUDViewModel.h"
#include "UIText.h"
#include "Player.h"
#include "Status.h"
#include "AnimDriver.h"

CUIMonsterHUDController::CUIMonsterHUDController()
{
}

HRESULT CUIMonsterHUDController::Initialize(void* pArg)
{
	MONSTERHUD_DESC* pDesc = static_cast<MONSTERHUD_DESC*>(pArg);

	if (pDesc == nullptr)
		return E_FAIL;

	if (pDesc->strAssetID.empty() || pDesc->strInstancePrefix.empty() || pDesc->strMonsterLayerTag.empty())
		return E_FAIL;

	m_strAssetID = pDesc->strAssetID;
	m_strInstancePrefix = pDesc->strInstancePrefix;
	m_strMonsterLayerTag = pDesc->strMonsterLayerTag;

	m_vScreenOffset = pDesc->vScreenOffset;
	m_vFatalScreenOffset = pDesc->vFatalScreenOffset;
	m_vGroggyScreenOffset = pDesc->vGroggyScreenOffset;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_pGameInstance->Subscribe(EVENT_TYPE::ACTOR_SPAWN, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::ACTOR_DESPAWN, this);

	// UI 컨트롤러가 생성되기 전에 이미 스폰된 몬스터 보관
	Register_ExistingMonsters();

	return S_OK;
}

void CUIMonsterHUDController::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{
	if (eType == EVENT_TYPE::ACTOR_SPAWN)
	{
		if (pData == nullptr) 
			return;  
		
		ActorSpawn_EventDesc* pDesc = static_cast<ActorSpawn_EventDesc*>(pData);

		if (pDesc->eActorType != ACTOR_TYPE::MONSTER)
			return;

		CAIObject* pMonster = dynamic_cast<CAIObject*>(pDesc->pActor);

		Queue_MonsterSpawn(pMonster);

		return;
	}
	
	if (eType == EVENT_TYPE::ACTOR_DESPAWN)
	{
		if (pData == nullptr) 
			return;

		ActorDespawn_EventDesc* pDesc = static_cast<ActorDespawn_EventDesc*>(pData);
		
		CAIObject* pMonster = dynamic_cast<CAIObject*>(pDesc->pActor);
		
		if (pMonster != nullptr)
		{
			m_PendingSpawnActors.erase(pMonster);
			m_FailedRegistrations.erase(pMonster);
		}

		auto iterPending = m_PendingUIBindings.begin();

		while (iterPending != m_PendingUIBindings.end())
		{
			if (iterPending->pMonster != pMonster)
			{
				++iterPending;
				continue;
			}
		
			m_pGameInstance->Request_ReleaseUIObject(iterPending->hUI);

			iterPending = m_PendingUIBindings.erase(iterPending);
		}

		auto iter = std::find_if(m_Entries.begin(), m_Entries.end(), [pDesc](const MONSTER_HP_ENTRY& Entry)
		{
			return Entry.pMonster == pDesc->pActor;
		});

		if(iter != m_Entries.end())
		{
			// 실제 제거 전까지 화면에 남지 않도록 즉시 숨김
			if (iter->pUIObject != nullptr)
				iter->pUIObject->Set_Visible(false);

			// UI 매니저가 순회 중일 수 있으므로 핸들 기반 제거 예약
			m_pGameInstance->Request_ReleaseUIObject(iter->hUI);

			Safe_Release(iter->pViewModel);
			m_Entries.erase(iter);
		}
	}
}

HRESULT CUIMonsterHUDController::Bind_UI()
{ 
	// 이 컨트롤러는 특정 UIObject 하나에 묶이지 않는다.
	// 몬스터가 발견될 때마다 MonsterHUD 템플릿 UIObject를 새로 생성해서 관리한다.
	
	return S_OK;
}

void CUIMonsterHUDController::Update_ViewModel(_float fTimeDelta)
{
	// 이전 프레임에 실제 생성된 UI 바인딩
	Resolve_PendingUIBindings();

	// 이번 프레임 Spawn Event Actor의 UI 생성 요청
	Process_PendingSpawnActors();

	CGameObject* pLockedTarget = nullptr;
	CAIObject* pFatalPossibleTarget = nullptr;
	_bool bFatalActionStarted = false;

	if (CPlayer* pPlayer = dynamic_cast<CPlayer*>(m_pGameInstance->Get_Player()))
	{
		if (CStatusComponent* pStatusCom = pPlayer->Get_StatusComponent())
			pLockedTarget = pStatusCom->Get_LockedTarget();

		if (CAnimDriver* pAnimDriver = pPlayer->Get_AnimDriverComponent())
		{
			// 접근 시작 또는 실제 페이탈 애니메이션 진입부터 Possible 표시를 종료한다.
			bFatalActionStarted = pAnimDriver->Is_FatalStart() || pAnimDriver->Is_ActionActive(AnimAction::Fatal);

			if (!bFatalActionStarted)
				pFatalPossibleTarget = pAnimDriver->Find_FatalPossibleTarget();
		}
	}

	for (MONSTER_HP_ENTRY& Entry : m_Entries)
		Update_Entry(Entry, pLockedTarget, pFatalPossibleTarget, bFatalActionStarted, fTimeDelta);
}


void CUIMonsterHUDController::Apply_ViewModel()
{
	for (MONSTER_HP_ENTRY& Entry : m_Entries)
	{
		CMonsterHUDViewModel* pViewModel = Entry.pViewModel;

		if (pViewModel == nullptr || !pViewModel->Is_Dirty())
			continue;

		const _bool bVisible = pViewModel->Is_Visible();
		const _bool bHealthVisible = bVisible && pViewModel->Is_HealthVisible();

		if (Entry.pUIObject != nullptr)
			Entry.pUIObject->Set_Visible(bVisible);

		if (Entry.pHPBar != nullptr)
		{
			Entry.pHPBar->Set_Visible(bHealthVisible);
			Entry.pHPBar->Set_Position(pViewModel->Get_Position());
			Entry.pHPBar->Set_SlotRange(Entry.iFillSlotID, 0.f, pViewModel->Get_HPRatio());
		}

		if (Entry.pFrame != nullptr)
		{
			Entry.pFrame->Set_Visible(bHealthVisible);
			Entry.pFrame->Set_Position(pViewModel->Get_Position());
		}

		if (Entry.pFatal != nullptr)
		{
			Entry.pFatal->Set_Visible(bVisible && pViewModel->Is_FatalVisible());
			Entry.pFatal->Set_Position(pViewModel->Get_FatalPosition());
		}

		if (Entry.pGroggy != nullptr)
		{
			Entry.pGroggy->Set_Visible(bHealthVisible && pViewModel->Is_GroggyVisible());
			Entry.pGroggy->Set_Position(pViewModel->Get_GroggyPosition());
		}

		if (Entry.pDamageText != nullptr)
		{
			const _bool bDamageVisible = bHealthVisible && pViewModel->Is_DamageVisible();

			Entry.pDamageText->Set_Visible(bDamageVisible);
			Entry.pDamageText->Set_Position(_float2(pViewModel->Get_Position().x + Entry.vDamageTextOffset.x, pViewModel->Get_Position().y + Entry.vDamageTextOffset.y));

			if (bDamageVisible)
			{
				const _uint iDamage = static_cast<_uint>(ceilf(pViewModel->Get_AccumulatedDamage()));
				Entry.pDamageText->Set_Text(to_wstring(iDamage));
			}
		}

		pViewModel->Clear_Dirty();
	}
}

// Controller 생성 전에 이미 존재하던 몬스터를 한 번만 수집
void CUIMonsterHUDController::Register_ExistingMonsters()
{
	CLayer* pLayer = m_pGameInstance->Get_CurrentLevel_Layer(m_strMonsterLayerTag);
	if (pLayer == nullptr)
		return;

	for (CGameObject* pObject : pLayer->Get_Objects())
	{
		CAIObject* pMonster = dynamic_cast<CAIObject*>(pObject);
		if (pMonster == nullptr)
			continue;

		Queue_MonsterSpawn(pMonster);
	}
}

// 스폰 이벤트로 들어온 몬스터를 다음 업데이트 때 처리
void CUIMonsterHUDController::Process_PendingSpawnActors()
{
	for (CAIObject* pMonster : m_PendingSpawnActors)
	{
		if (pMonster == nullptr)
			continue;

		if (Is_Registered(pMonster))
			continue;

		if (FAILED(Request_MonsterHUD(pMonster)))
		{
			m_FailedRegistrations.emplace(pMonster);
		}
	}

	m_PendingSpawnActors.clear();
}

// 스폰 대기열 등록
void CUIMonsterHUDController::Queue_MonsterSpawn(CAIObject* pMonster)
{
	if (pMonster == nullptr)
		return;

	if (pMonster->Get_ActorType() != ACTOR_TYPE::MONSTER)
		return;

	if (Is_Registered(pMonster))
		return;

	if (Is_PendingRegistration(pMonster))
		return;

	if (m_PendingSpawnActors.find(pMonster) != m_PendingSpawnActors.end())
		return;

	if (m_FailedRegistrations.find(pMonster) != m_FailedRegistrations.end())
		return;

	m_PendingSpawnActors.emplace(pMonster);
}

void CUIMonsterHUDController::Update_Entry(MONSTER_HP_ENTRY& Entry, CGameObject* pLockedTarget, CAIObject* pFatalPossibleTarget, _bool bFatalActionStarted, _float fTimeDelta)
{
	if (Entry.pViewModel == nullptr)
		return;

	if (Entry.pMonster == nullptr || Entry.pStatCom == nullptr)
	{
		Update_FatalAnimation(Entry, MONSTER_FATAL_ANIM_STATE::NONE);
		Entry.pViewModel->Set_Visible(false);
		Entry.pViewModel->Set_HealthVisible(false);
		Entry.pViewModel->Set_FatalVisible(false);
		Entry.pViewModel->Set_GroggyVisible(false);
		return;
	}

	Entry.pViewModel->Update_Damage(Entry.pStatCom->Get_CurrentHP(), fTimeDelta);

	const _bool bLockedOn = pLockedTarget != nullptr && pLockedTarget == Entry.pMonster;
	const _bool bHealthRequested = bLockedOn || Entry.pMonster->Is_RecentlyHit();

	const _bool bGroggy = Entry.pMonster->Is_Groggy();

	// 그로기 여부와 관계없이 현재 플레이어가 실제 페이탈을 실행할 수 있는 대상인지 검사한다.
	const _bool bFatalPossible = !bFatalActionStarted && pFatalPossibleTarget == Entry.pMonster;

	// 그로기 상태지만 아직 실행 위치에 도달하지 않았을 때 일반 Fatal 표시를 사용한다.
	const _bool bFatalActive = bGroggy && !bFatalActionStarted && !bFatalPossible;
	const _bool bFatalRequested = bFatalPossible || bFatalActive;

	if (Entry.pStatCom->Is_Dead() || (!bHealthRequested && !bFatalRequested))
	{
		Update_FatalAnimation(Entry, MONSTER_FATAL_ANIM_STATE::NONE);
		Entry.pViewModel->Set_Visible(false);
		Entry.pViewModel->Set_HealthVisible(false);
		Entry.pViewModel->Set_FatalVisible(false);
		Entry.pViewModel->Set_GroggyVisible(false);
		return;
	}

	_bool bHealthVisible = false;

	if (bHealthRequested)
	{
		_vector vHeadPos = {};
		_float2 vScreenPos = {};

		if (Entry.pMonster->Get_HeadWorldPosition(vHeadPos) && WorldToScreen(vHeadPos, vScreenPos))
		{
			vScreenPos.x += m_vScreenOffset.x;
			vScreenPos.y += m_vScreenOffset.y;

			bHealthVisible = true;

			Entry.pViewModel->Set_Position(vScreenPos);
			Entry.pViewModel->Set_HPRatio(Entry.pStatCom->Get_HPPercent());

			const _bool bStaggerable = Entry.pStatCom->Is_Staggerable();

			Entry.pViewModel->Set_GroggyVisible(bStaggerable);
			Entry.pViewModel->Set_GroggyPosition(_float2(vScreenPos.x + m_vGroggyScreenOffset.x, vScreenPos.y + m_vGroggyScreenOffset.y));
		}
	}

	Entry.pViewModel->Set_HealthVisible(bHealthVisible);

	if (!bHealthVisible)
		Entry.pViewModel->Set_GroggyVisible(false);

	_bool bFatalVisible = false;

	if (bFatalRequested)
	{
		_vector vLockOnPos = {};
		_float2 vFatalScreenPos = {};

		if (Entry.pMonster->Get_LockOnWorldPosition(vLockOnPos) && WorldToScreen(vLockOnPos, vFatalScreenPos))
		{
			vFatalScreenPos.x += m_vFatalScreenOffset.x;
			vFatalScreenPos.y += m_vFatalScreenOffset.y;

			bFatalVisible = true;

			Entry.pViewModel->Set_FatalPosition(vFatalScreenPos);
			Update_FatalAnimation(Entry, bFatalPossible ? MONSTER_FATAL_ANIM_STATE::POSSIBLE : MONSTER_FATAL_ANIM_STATE::ACTIVE);
		}
	}

	if (!bFatalVisible)
		Update_FatalAnimation(Entry, MONSTER_FATAL_ANIM_STATE::NONE);

	Entry.pViewModel->Set_FatalVisible(bFatalVisible);

	// 체력 HUD 또는 페이탈 표시 중 하나라도 필요하면 최상위 HUD 객체는 유지한다.
	Entry.pViewModel->Set_Visible(bHealthVisible || bFatalVisible);
}

void CUIMonsterHUDController::Update_FatalAnimation(MONSTER_HP_ENTRY& Entry, MONSTER_FATAL_ANIM_STATE eNextState)
{
	if (Entry.pUIObject == nullptr || Entry.eFatalAnimState == eNextState)
		return;

	Entry.pUIObject->Stop_Animation();

	// Stop_Animation은 트랜스폼과 색상을 초기 상태로 돌려주지 않으므로 직접 복구한다.
	if (Entry.pFatal != nullptr)
	{
		Entry.pFatal->Set_Scale(_float2(1.f, 1.f));
		Entry.pFatal->Set_Color(Entry.vFatalBaseColor);
	}

	Entry.eFatalAnimState = eNextState;

	switch (eNextState)
	{
	case MONSTER_FATAL_ANIM_STATE::ACTIVE:
		Entry.pUIObject->Play_Animation("Fatal", true);
		break;

	case MONSTER_FATAL_ANIM_STATE::POSSIBLE:
		Entry.pUIObject->Play_Animation("Fatal_Possible", true);
		break;

	case MONSTER_FATAL_ANIM_STATE::NONE:
	default:
		break;
	}
}


_bool CUIMonsterHUDController::Is_Registered(CAIObject* pMonster) const
{
	for (const MONSTER_HP_ENTRY& Entry : m_Entries)
	{
		if (Entry.pMonster == pMonster)
			return true;
	}

	return false;
}

HRESULT CUIMonsterHUDController::Request_MonsterHUD(CAIObject* pMonster)
{
	if (pMonster == nullptr)
		return E_FAIL;

	if (pMonster->Get_StatComponent() == nullptr)
		return E_FAIL;

	_wstring strUITag = m_strInstancePrefix;

	strUITag += L"_";
	strUITag += to_wstring(m_iNextInstanceID++);

	UI_HANDLE hUI = {};

	if (FAILED(m_pGameInstance->Request_SpawnUIObject(m_strAssetID, strUITag, &hUI)))
		return E_FAIL;

	MONSTER_PENDING_ENTRY Pending = {};
	Pending.pMonster = pMonster;
	Pending.hUI = hUI;

	m_PendingUIBindings.push_back(Pending);

	return S_OK;
}

HRESULT CUIMonsterHUDController::Complete_MonsterHUD(const MONSTER_PENDING_ENTRY& Pending)
{
	CAIObject* pMonster = Pending.pMonster;

	if (pMonster == nullptr)
		return E_FAIL;

	CStatComponent_AI* pStatCom = pMonster->Get_StatComponent();
	if (pStatCom == nullptr)
		return E_FAIL;

	CUIObject* pUIObject = m_pGameInstance->Find_UIObject(Pending.hUI);

	if (pUIObject == nullptr)
		return E_FAIL;

	CUIElement* pHPElement = pUIObject->Find_Element(m_strHPBarElementTag);

	CUIProgressBar* pHPBar = dynamic_cast<CUIProgressBar*>(pHPElement);
	if (pHPBar == nullptr)
		return E_FAIL;

	CUIElement* pFrame = pUIObject->Find_Element(m_strFrameElementTag);
	CUIElement* pFatal = pUIObject->Find_Element(m_strFatalElementTag);
	CUIElement* pGroggy = pUIObject->Find_Element(m_strGroggyElementTag);
	CUIText* pDamageText = dynamic_cast<CUIText*>(pUIObject->Find_Element(m_strDamageTextElementTag));

	if (pFrame == nullptr || pFatal == nullptr || pGroggy == nullptr || pDamageText == nullptr)
		return E_FAIL;

	const _int iFillSlotID = pHPBar->Find_SlotID(m_strFillSlotName);
	if (iFillSlotID < 0)
		return E_FAIL;

	CMonsterHUDViewModel* pViewModel = CMonsterHUDViewModel::Create();
	if (pViewModel == nullptr)
		return E_FAIL;

	MONSTER_HP_ENTRY Entry = {};
	Entry.pMonster = pMonster;
	Entry.pStatCom = pStatCom;
	Entry.pViewModel = pViewModel;

	Entry.pUIObject = pUIObject;
	Entry.pHPBar = pHPBar;
	Entry.pFrame = pFrame;
	Entry.pFatal = pFatal;
	Entry.pGroggy = pGroggy;
	Entry.pDamageText = pDamageText;

	Entry.iFillSlotID = iFillSlotID;
	Entry.hUI = Pending.hUI;
	Entry.vDamageTextOffset = pDamageText->Get_UITransform().vPosition;
	Entry.vFatalBaseColor = pFatal->Get_RenderData().vColor;

	Entry.pDamageText->Set_Visible(false);
	Entry.pUIObject->Set_Visible(false);
	Entry.pFatal->Set_Visible(false);
	Entry.pGroggy->Set_Visible(false);

	m_Entries.push_back(Entry);

	return S_OK;
}

void CUIMonsterHUDController::Resolve_PendingUIBindings()
{
	auto iter = m_PendingUIBindings.begin();

	while (iter != m_PendingUIBindings.end())
	{
		// 실제 UI 생성 Command가 아직 처리되지 않음
		if (m_pGameInstance->Is_PendingSpawnUIObject(iter->hUI))
		{
			++iter;
			continue;
		}

		// Spawn Command가 실패하거나 핸들이 취소 됨
		if (!m_pGameInstance->Is_AliveUIObject(iter->hUI))
		{
			if (iter->pMonster != nullptr)
				m_FailedRegistrations.emplace(iter->pMonster);

			iter = m_PendingUIBindings.erase(iter);

			continue;
		}

		if (FAILED(Complete_MonsterHUD(*iter)))
		{
			m_pGameInstance->Request_ReleaseUIObject(iter->hUI);

			if (iter->pMonster != nullptr)
				m_FailedRegistrations.emplace(iter->pMonster);
		}

		iter = m_PendingUIBindings.erase(iter);
	}
}

_bool CUIMonsterHUDController::Is_PendingRegistration(CAIObject* pMonster) const
{
	for (const auto& Pending : m_PendingUIBindings)
	{
		if (Pending.pMonster == pMonster)
			return true;
	}

	return false;
}

CUIMonsterHUDController* CUIMonsterHUDController::Create()
{
	return new CUIMonsterHUDController;
}

void CUIMonsterHUDController::Free()
{
	for (MONSTER_PENDING_ENTRY& Pending
		: m_PendingUIBindings)
	{
		m_pGameInstance->Request_ReleaseUIObject(
			Pending.hUI);
	}

	for (MONSTER_HP_ENTRY& Entry : m_Entries)
	{
		if (Entry.pUIObject != nullptr)
			Entry.pUIObject->Set_Visible(false);

		m_pGameInstance->Request_ReleaseUIObject(Entry.hUI);

		Safe_Release(Entry.pViewModel);
	}

	m_PendingUIBindings.clear();
	m_PendingSpawnActors.clear();
	m_Entries.clear();
	m_PendingSpawnActors.clear();
	m_FailedRegistrations.clear();

	m_pGameInstance->UnSubscribe_AllEvent(this);

	__super::Free();
} 
