#include "UIBossClearHUDController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "Core_Struct.h"

HRESULT CUIBossClearHUDController::Initialize(void* pArg)
{
	if (pArg == nullptr)
		return E_FAIL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	// 보스가 사망하면서 보내는 클리어 이벤트를 구독한다.
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR, this);

	return S_OK;
}

void CUIBossClearHUDController::Update_ViewModel(_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);

	if (!m_bPlaying || m_pUIObject == nullptr)
		return;

	// 비루프 애니메이션이 종료되면 UI 전체를 다시 닫는다.
	if (m_pUIObject->Is_AnimationFinished())
		Finish_BossClearAnimation();
}

void CUIBossClearHUDController::Apply_ViewModel()
{
	// 현재 UI는 런타임 데이터 바인딩 없이 애니메이션만 재생한다.
}

void CUIBossClearHUDController::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
	if (eType != EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR || pData == nullptr)
		return;

	BossClear_EventDesc* pDesc = static_cast<BossClear_EventDesc*>(pData);

	// 락사시아 1페이즈 종료는 최종 보스 클리어가 아니므로 연출을 재생하지 않는다.
	if (pDesc->eMonsterType == SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P1)
		return;

	// 같은 프레임 또는 연출 도중 중복 이벤트가 들어와도 다시 시작하지 않는다.
	if (m_bPlaying)
		return;

	Begin_BossClearAnimation();
}

void CUIBossClearHUDController::Begin_BossClearAnimation()
{
	if (FAILED(m_pGameInstance->Open_UIObject(m_hUI)))
		return;

	if (m_pUIObject == nullptr || !m_pUIObject->Play_Animation("Kill_Boss", true))
	{
		m_pGameInstance->Close_UIObject(m_hUI);
		return;
	}

	m_pGameInstance->Play_Sound("SE_UI_AlertKill_02", SOUND_SFX, 1.f, false, false);

	m_bPlaying = true;
}

void CUIBossClearHUDController::Finish_BossClearAnimation()
{
	m_bPlaying = false;

	if (m_pUIObject != nullptr)
		m_pUIObject->Stop_Animation();

	m_pGameInstance->Close_UIObject(m_hUI);
}

CUIBossClearHUDController* CUIBossClearHUDController::Create()
{
	return new CUIBossClearHUDController;
}

void CUIBossClearHUDController::Free()
{
	__super::Free();

	m_bPlaying = false;
}