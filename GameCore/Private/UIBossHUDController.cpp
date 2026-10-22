#include "UIBossHUDController.h"
#include "GameInstance.h"
#include "Layer.h"
#include "AIObject.h"
#include "Boss_Festival.h"
#include "StatComponent_AI.h"
#include "UIObject.h"
#include "UIProgressBar.h"
#include "BossHUDViewModel.h"
#include "UIText.h"
#include "Player.h"
#include "AnimDriver.h"

CUIBossHUDController::CUIBossHUDController()
{
}

HRESULT CUIBossHUDController::Initialize(void* pArg)
{
	BOSSHUD_CONTROLLER_DESC* pDesc = static_cast<BOSSHUD_CONTROLLER_DESC*>(pArg);

	if (pDesc == nullptr)
		return E_FAIL;

	if (pDesc->strBossLayerTag.empty())
		return E_FAIL;

	m_strBossLayerTag = pDesc->strBossLayerTag;
	m_vFatalScreenOffset = pDesc->vFatalScreenOffset;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_pViewModel = CBossHUDViewModel::Create();

	if (m_pViewModel == nullptr)
		return E_FAIL;

	m_pGameInstance->Subscribe(EVENT_TYPE::BOSS_START, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::ACTOR_DESPAWN, this);

	return S_OK;
}

HRESULT CUIBossHUDController::Bind_UI()
{
	if (FAILED(__super::Bind_UI()))
		return E_FAIL;

	m_pHPBar = static_cast<CUIProgressBar*>(m_pUIObject->Find_Element(m_strHPBarElementTag));
	if (m_pHPBar == nullptr)
		return E_FAIL;

	m_iFillSlotID = m_pHPBar->Find_SlotID(m_strFillSlotName);
	if (m_iFillSlotID < 0)
		return E_FAIL;

	m_pGroggy = m_pUIObject->Find_Element(m_strGroggyElementTag);
	m_pFatal = m_pUIObject->Find_Element(m_strFatalElementTag);
	m_pDamageText = dynamic_cast<CUIText*>(m_pUIObject->Find_Element(m_strDamageTextElementTag));
	m_pGroggy2 = m_pUIObject->Find_Element(m_strGroggyElementTag2);
	m_pGroggy3 = m_pUIObject->Find_Element(m_strGroggyElementTag3);
	m_pBossNameText = dynamic_cast<CUIText*>(m_pUIObject->Find_Element(m_strBossNameElementTag));

	if (m_pGroggy2 == nullptr || m_pGroggy3 == nullptr || m_pBossNameText == nullptr)
		return E_FAIL;

	if (m_pGroggy == nullptr || m_pFatal == nullptr || m_pDamageText == nullptr)
		return E_FAIL;

	// JSON에 지정한 원래 색상을 페이탈 가능 색상으로 보관한다.
	m_vFatalAvailableColor = m_pFatal->Get_RenderData().vColor;


	m_pGroggy->Set_Visible(false);
	m_pGroggy2->Set_Visible(false);
	m_pGroggy3->Set_Visible(false);
	m_pFatal->Set_Visible(false);
	m_pDamageText->Set_Visible(false);

	// 이벤트 전에는 보스 전체 UI 숨김
	m_pUIObject->Set_Visible(false);

	return S_OK;
}

void CUIBossHUDController::Update_ViewModel(_float fTimeDelta)
{
	if (!m_bBossStarted)
		return;

	if (m_pBoss == nullptr || m_pBossStatCom == nullptr || m_pHPBar == nullptr)
	{
		Change_HUDAnimation(BOSS_HUD_ANIM_STATE::NONE);
		m_bWasGroggy = false;
		m_pViewModel->Set_Visible(false);
		m_pViewModel->Set_GroggyVisible(false);
		m_pViewModel->Set_FatalVisible(false);
		return;
	}

	m_pViewModel->Update_Damage(m_pBossStatCom->Get_CurrentHP(), fTimeDelta);

	if (m_pBossStatCom->Is_Dead())
	{
		Change_HUDAnimation(BOSS_HUD_ANIM_STATE::NONE);
		m_bWasGroggy = false;
		m_pViewModel->Set_Visible(false);
		m_pViewModel->Set_GroggyVisible(false);
		m_pViewModel->Set_FatalVisible(false);
		return;
	}

	CAnimDriver* pAnimDriver = nullptr;
	CAIObject* pFatalPossibleTarget = nullptr;
	_bool bFatalActionStarted = false;

	if (CPlayer* pPlayer = dynamic_cast<CPlayer*>(m_pGameInstance->Get_Player()))
	{
		pAnimDriver = pPlayer->Get_AnimDriverComponent();

		if (pAnimDriver != nullptr)
		{
			bFatalActionStarted = pAnimDriver->Is_FatalStart() || pAnimDriver->Is_ActionActive(AnimAction::Fatal);

			if (!bFatalActionStarted)
				pFatalPossibleTarget = pAnimDriver->Find_FatalPossibleTarget();
		}
	}

	const _bool bStaggerable = m_pBossStatCom->Is_Staggerable();
	const _bool bGroggy = m_pBoss->Is_Groggy();
	const _bool bGroggyJustSucceeded = bGroggy && !m_bWasGroggy;
	const _bool bFatalVisible = bGroggy && !bFatalActionStarted;
	const _bool bFatalPossible = bFatalVisible && pFatalPossibleTarget == m_pBoss;

	m_bWasGroggy = bGroggy;

	m_pViewModel->Set_Visible(true);
	m_pViewModel->Set_HPRatio(m_pBossStatCom->Get_HPPercent());
	m_pViewModel->Set_GroggyVisible(bStaggerable);

	// 그로기 성공 이미지를 먼저 1회 재생한다.
	if (bGroggyJustSucceeded)
	{
		Change_HUDAnimation(BOSS_HUD_ANIM_STATE::GROGGY_SUCCESS);
	}
	// 성공 애니메이션 0.2초 동안은 다른 루프 애니메이션으로 덮어쓰지 않는다.
	else if (m_eHUDAnimState == BOSS_HUD_ANIM_STATE::GROGGY_SUCCESS && !m_pUIObject->Is_AnimationFinished())
	{
	}
	else if (bFatalPossible)
	{
		Change_HUDAnimation(BOSS_HUD_ANIM_STATE::FATAL_POSSIBLE);
	}
	else if (bFatalVisible)
	{
		Change_HUDAnimation(BOSS_HUD_ANIM_STATE::FATAL_ACTIVE);
	}
	else if (bStaggerable)
	{
		Change_HUDAnimation(BOSS_HUD_ANIM_STATE::GROGGY);
	}
	else
	{
		Change_HUDAnimation(BOSS_HUD_ANIM_STATE::NONE);
	}

	if (!bFatalVisible)
	{
		m_pViewModel->Set_FatalVisible(false);
		return;
	}

	_vector vLockOnPos = {};
	_float2 vScreenPos = {};

	if (!m_pBoss->Get_LockOnWorldPosition(vLockOnPos) || !WorldToScreen(vLockOnPos, vScreenPos))
	{
		m_pViewModel->Set_FatalVisible(false);
		return;
	}

	vScreenPos.x += m_vFatalScreenOffset.x;
	vScreenPos.y += m_vFatalScreenOffset.y;

	m_pViewModel->Set_FatalPosition(vScreenPos);
	m_pViewModel->Set_FatalVisible(true);
}

void CUIBossHUDController::Apply_ViewModel()
{
	if (m_pViewModel == nullptr || !m_pViewModel->Is_Dirty())
		return;

	const _bool bVisible = m_pViewModel->Is_Visible();

	if (m_pUIObject != nullptr)
		m_pUIObject->Set_Visible(bVisible);

	if (m_pHPBar != nullptr)
	{
		m_pHPBar->Set_Visible(bVisible);
		m_pHPBar->Set_SlotRange(m_iFillSlotID, 0.f, m_pViewModel->Get_HPRatio());
	}

	const _bool bGroggyVisible = bVisible && m_pViewModel->Is_GroggyVisible();

	if (m_pGroggy != nullptr)
		m_pGroggy->Set_Visible(bGroggyVisible);

	if (m_pGroggy2 != nullptr)
		m_pGroggy2->Set_Visible(bGroggyVisible);

	if (m_pGroggy3 != nullptr)
		m_pGroggy3->Set_Visible(bGroggyVisible);

	if (m_pFatal != nullptr)
	{
		m_pFatal->Set_Visible(bVisible && m_pViewModel->Is_FatalVisible());
		m_pFatal->Set_Position(m_pViewModel->Get_FatalPosition());
	}

	if (m_pDamageText != nullptr)
	{
		const _bool bDamageVisible = bVisible && m_pViewModel->Is_DamageVisible();

		m_pDamageText->Set_Visible(bDamageVisible);

		if (bDamageVisible)
		{
			const _uint iDamage = static_cast<_uint>(ceilf(m_pViewModel->Get_AccumulatedDamage()));
			m_pDamageText->Set_Text(to_wstring(iDamage));
		}
	}

	m_pViewModel->Clear_Dirty();
}

void CUIBossHUDController::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{
	if (EVENT_TYPE::BOSS_START == eType)
	{
		m_bBossStarted = true;

		if (m_pBoss == nullptr)
			Bind_Boss(pData);

		if (m_pViewModel != nullptr)
			m_pViewModel->Set_Visible(m_pBoss != nullptr && m_pBossStatCom != nullptr);
	}
	else if (EVENT_TYPE::ACTOR_DESPAWN == eType)
	{
		if (pData == nullptr) return;

		ActorDespawn_EventDesc* pDesc = static_cast<ActorDespawn_EventDesc*>(pData);
		if (pDesc->eActorType != ACTOR_TYPE::BOSS || pDesc->pActor != m_pBoss)
			return;

		if (m_pViewModel != nullptr)
		{
			m_pViewModel->Set_Visible(false);
			m_pViewModel->Set_GroggyVisible(false);
			m_pViewModel->Set_FatalVisible(false);
		}

		// 이벤트 처리 후 화면에서도 제거
		Apply_ViewModel();

		m_pBoss = nullptr;
		m_pBossStatCom = nullptr;
		m_bBossStarted = false;
	}

}

HRESULT CUIBossHUDController::Bind_Boss(Event_Desc* pData)
{
	
	// 동적 현재 레벨로 탐색
	CLayer* pLayer = m_pGameInstance->Get_CurrentLevel_Layer(m_strBossLayerTag);
	// 레벨 지정 탐색	
	//CLayer* pLayer = m_pGameInstance->Get_Layer(ETOUI(LEVEL::GAMEPLAY), m_strBossLayerTag);

	if (pLayer == nullptr)
		return E_FAIL;

	
	SPAWN_MONSTER_TYPE pTargetType = SPAWN_MONSTER_TYPE::END;
	if (pData != nullptr)
	{
		BossClear_EventDesc* pDesc = static_cast<BossClear_EventDesc*>(pData);
		pTargetType = pDesc->eMonsterType;
	}

	for (CGameObject* pObject : pLayer->Get_Objects())
	{
		CAIObject* pBoss = static_cast<CAIObject*>(pObject);

		if (pBoss->Get_ActorType() != ACTOR_TYPE::BOSS)
			continue;

		if (pTargetType != pBoss->Get_MonsterType())
			continue;

		m_pBoss = pBoss;
		m_pBossStatCom = pBoss->Get_StatComponent();
		m_strBossName = pBoss->Get_MonsterName();

		if (m_pBossNameText != nullptr)
			m_pBossNameText->Set_Text(m_strBossName);

		if (m_pBossStatCom == nullptr)
			return E_FAIL;

		return S_OK;
	}

	return E_FAIL;
}

void CUIBossHUDController::Change_HUDAnimation(BOSS_HUD_ANIM_STATE eNextState)
{
	if (m_pUIObject == nullptr || m_eHUDAnimState == eNextState)
		return;

	Reset_HUDAnimation();
	m_eHUDAnimState = eNextState;

	switch (eNextState)
	{
	case BOSS_HUD_ANIM_STATE::GROGGY:
		m_pUIObject->Play_Animation("Groggy", true);
		break;

	case BOSS_HUD_ANIM_STATE::GROGGY_SUCCESS:
		m_pUIObject->Play_Animation("Groggy_Success", true);
		break;

	case BOSS_HUD_ANIM_STATE::FATAL_ACTIVE:
		m_pUIObject->Play_Animation("Fatal_Active", true);
		break;

	case BOSS_HUD_ANIM_STATE::FATAL_POSSIBLE:
		m_pUIObject->Play_Animation("Fatal_Possible", true);
		break;

	case BOSS_HUD_ANIM_STATE::NONE:
	default:
		break;
	}
}

void CUIBossHUDController::Reset_HUDAnimation()
{
	if (m_pUIObject != nullptr)
		m_pUIObject->Stop_Animation();

	if (m_pGroggy != nullptr)
		m_pGroggy->Set_Scale(_float2(1.f, 1.f));

	if (m_pFatal != nullptr)
	{
		m_pFatal->Set_Scale(_float2(1.f, 1.f));
		m_pFatal->Set_Color(m_vFatalAvailableColor);
	}
}


CUIBossHUDController* CUIBossHUDController::Create()
{
	return new CUIBossHUDController;
}

void CUIBossHUDController::Free()
{
	//if (m_pGameInstance != nullptr)
	//	m_pGameInstance->UnSubscribe(EVENT_TYPE::BOSS_START, this);

	m_pGameInstance->UnSubscribe_AllEvent(this);

	Safe_Release(m_pViewModel);

	m_pBoss = nullptr;
	m_pBossStatCom = nullptr;
	m_pHPBar = nullptr;
	m_pFatal = nullptr;
	m_pGroggy = nullptr;
	m_pGroggy2 = nullptr;
	m_pGroggy3 = nullptr;
	m_pBossNameText = nullptr;

	__super::Free();
}
