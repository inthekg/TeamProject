#include "UIPlayerDeadController.h"
#include "GameInstance.h"
#include "UIObject.h"

HRESULT CUIPlayerDeadController::Initialize(void* pArg)
{
	if (pArg == nullptr)
		return E_FAIL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	// 플레이어가 사망 UI 재생을 요청하는 이벤트를 구독한다.
	m_pGameInstance->Subscribe(EVENT_TYPE_UI::PLAYER_DEAD_OPEN_REQUEST, this);

	return S_OK;
}

void CUIPlayerDeadController::Update_ViewModel(_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);

	if (!m_bPlaying || m_pUIObject == nullptr)
		return;

	// 비루프 PlayerDead 애니메이션이 완전히 끝난 뒤 리스폰으로 넘어간다.
	if (m_pUIObject->Is_AnimationFinished())
		Finish_PlayerDeadAnimation();
}

void CUIPlayerDeadController::Apply_ViewModel()
{
	// 사망 UI는 런타임 데이터 바인딩 없이 애니메이션만 재생한다.
}

void CUIPlayerDeadController::OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData)
{
	UNREFERENCED_PARAMETER(pData);

	if (eType != EVENT_TYPE_UI::PLAYER_DEAD_OPEN_REQUEST)
		return;

	// 중복 사망 이벤트가 들어와도 애니메이션을 다시 시작하지 않는다.
	if (m_bPlaying)
		return;

	Begin_PlayerDeadAnimation();
}

void CUIPlayerDeadController::Begin_PlayerDeadAnimation()
{
	if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
	{
		Request_RespawnFallback();
		return;
	}

	if (m_pUIObject == nullptr || !m_pUIObject->Play_Animation("PlayerDead", true))
	{
		m_pGameInstance->Close_UIObject(m_hUI);
		Request_RespawnFallback();
		return;
	}

	m_pGameInstance->Play_Sound("SE_UI_AlertDeath_01", SOUND_SFX, 1.f, false, false);

	// 사망 연출 중에는 플레이어·몬스터·락온 등의 일반 HUD를 숨긴다.
	m_pGameInstance->Acquire_UILayerSuppression("HUD");
	m_bHUDSuppressed = true;
	m_bPlaying = true;
}

void CUIPlayerDeadController::Finish_PlayerDeadAnimation()
{
	m_bPlaying = false;

	if (m_pUIObject != nullptr)
		m_pUIObject->Stop_Animation();

	// Level_GamePlay가 이 이벤트를 받아 로딩 UI와 리스폰 절차를 시작한다.
	m_pGameInstance->Broadcast(EVENT_TYPE::REQUEST_RESPAWN, nullptr);

	// REQUEST_RESPAWN 처리에서 로딩 UI가 열린 뒤 사망 UI를 닫는다.
	m_pGameInstance->Close_UIObject(m_hUI);

	if (m_bHUDSuppressed)
	{
		m_pGameInstance->Release_UILayerSuppression("HUD");
		m_bHUDSuppressed = false;
	}
}

void CUIPlayerDeadController::Request_RespawnFallback()
{
	// UI 에셋이나 애니메이션 로드가 실패해도 사망 상태에 영구 정지하지 않도록 한다.
	m_bPlaying = false;
	m_pGameInstance->Broadcast(EVENT_TYPE::REQUEST_RESPAWN, nullptr);
}

CUIPlayerDeadController* CUIPlayerDeadController::Create()
{
	return new CUIPlayerDeadController;
}

void CUIPlayerDeadController::Free()
{
	if (m_bHUDSuppressed && m_pGameInstance != nullptr)
	{
		m_pGameInstance->Release_UILayerSuppression("HUD");
		m_bHUDSuppressed = false;
	}

	m_bPlaying = false;

	__super::Free();
}