#include "BossArenaBarrier.h"

#include "Core_Struct.h"
#include "GameInstance.h"
#include "HotelDoor.h"
#include "RenderStruct.h"
#include "Transform.h"

CBossArenaBarrier::CBossArenaBarrier(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CBossArenaBarrier::CBossArenaBarrier(const CBossArenaBarrier& Prototype)
	: CMapGameplayObject{ Prototype }
{
}

HRESULT CBossArenaBarrier::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CBossArenaBarrier::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	BOSS_ARENA_BARRIER_DESC Desc = *static_cast<BOSS_ARENA_BARRIER_DESC*>(pArg);
	Ready_DefaultDesc(&Desc);

	// 에디터 프리뷰는 보스 이벤트를 구독하지 않으므로 런타임 전용 보스 타입을 요구하지 않는다.
	if (BARRIER_ROLE::END == Desc.eRole || Desc.fDissolveDuration <= 0.f ||
		(false == Desc.bEditorPreview && (SPAWN_MONSTER_TYPE::END == Desc.eClearBossType || SPAWN_MONSTER_TYPE::END == Desc.eStartBossType)))
		return E_FAIL;

	m_eRole = Desc.eRole;
	m_eClearBossType = Desc.eClearBossType;
	m_eStartBossType = Desc.eStartBossType;
	m_fDissolveSpeed = 1.f / Desc.fDissolveDuration;
	m_vDissolveColor = Desc.vDissolveColor;

	if (FAILED(__super::Initialize(&Desc)))
		return E_FAIL;

	if (true == Desc.bEditorPreview)
	{
		// 에디터에서는 이벤트와 충돌 없이 완전히 나타난 검은 장벽만 배치 미리보기로 사용한다.
		m_eState = BARRIER_STATE::CLOSED;
		m_fDissolveAmount = 0.f;
		return S_OK;
	}

	// 최초 보스 입장 전에는 완전히 사라져 있고 통과할 수 있다.
	m_eState = BARRIER_STATE::OPEN;
	m_fDissolveAmount = 1.f;
	m_bEncounterStarted = false;
	m_bPassageEnabled = false;
	m_bPlayerInPassageTrigger = false;
	m_bPermanentlyOpen = false;

	m_pGameInstance->Subscribe(EVENT_TYPE::BOSS_START, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::REQUEST_RESPAWN, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR, this);

	return S_OK;
}

void CBossArenaBarrier::Ready_DefaultDesc(BOSS_ARENA_BARRIER_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	pDesc->eVisualType = VISUAL_TYPE::STATIC;
	pDesc->iModelLevel = ETOUI(LEVEL::GAMEPLAY);
	pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_BossArenaBarrier") : pDesc->strModelPrototypeTag;

	// 에디터 미리보기는 시각 배치만 필요하므로 Rigidbody 자체를 만들지 않는다.
	pDesc->bUsePhysics = false == pDesc->bEditorPreview;
	pDesc->bPhysicsStartRegistered = false;

	// Boss_Barrier 메시의 실제 로컬 반경은 X=0.02, Y=1, Z=1이다.
	pDesc->vPhysicsHalfExtents = _float3(0.02f * fabsf(pDesc->vScale.x), 1.f * fabsf(pDesc->vScale.y), 1.f * fabsf(pDesc->vScale.z));
	pDesc->vPhysicsLocalOffset = _float3(0.f, 0.f, 0.f);
	pDesc->iPhysicsFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
	pDesc->iPhysicsFilterWord1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER);
}

void CBossArenaBarrier::Update_MapGameplayObject(_float fTimeDelta)
{
	if (true == m_bBossStartRequested)
	{
		m_bBossStartRequested = false;
		m_bPassageEnabled = false;

		// 재도전 때 실제로 벽 반대편까지 통과한 경우에만 전투를 시작한다.
		BossClear_EventDesc StartDesc{};
		StartDesc.eMonsterType = m_eStartBossType;

		m_pGameInstance->Broadcast(EVENT_TYPE::BOSS_START, &StartDesc);
	}

	// Physics Scene 등록과 제거는 PhysX Trigger 콜백 밖인 Update에서만 처리한다.
	Apply_PendingStateRequests();
	Update_Dissolve(fTimeDelta);
}

void CBossArenaBarrier::Attach_LegacyDoor(CHotelDoor* pLegacyDoor)
{
	// 입구 장벽만 재도전 시 기존 보스 문을 대체한다.
	if (BARRIER_ROLE::ENTRANCE != m_eRole || nullptr == pLegacyDoor)
		return;

	m_pLegacyDoor = pLegacyDoor;
}

void CBossArenaBarrier::Apply_PendingStateRequests()
{
	const _bool bOpenRequested = m_bOpenRequested;
	const _bool bCloseRequested = m_bCloseRequested;
	const _bool bLegacyDoorDeactivateRequested = m_bLegacyDoorDeactivateRequested;

	m_bOpenRequested = false;
	m_bCloseRequested = false;
	m_bLegacyDoorDeactivateRequested = false;

	if (true == bLegacyDoorDeactivateRequested && nullptr != m_pLegacyDoor)
		m_pLegacyDoor->Deactivate_ForBossRetry();

	if (true == m_bPermanentlyOpen)
	{
		Begin_Open();
		return;
	}

	if (true == bOpenRequested)
		Begin_Open();

	// 같은 프레임에 Open과 Close가 모두 들어온 경우에는 안전하게 닫힘 요청을 우선한다.
	if (true == bCloseRequested)
		Begin_Close();
}

void CBossArenaBarrier::Begin_Open()
{
	if (BARRIER_STATE::OPEN == m_eState || BARRIER_STATE::OPENING == m_eState)
		return;

	// 디졸브가 시작되는 즉시 플레이어가 통과할 수 있도록 충돌부터 제거한다.
	UnRegister_Physics();
	m_eState = BARRIER_STATE::OPENING;
}

void CBossArenaBarrier::Begin_Close()
{
	if (true == m_bPermanentlyOpen || BARRIER_STATE::CLOSED == m_eState || BARRIER_STATE::CLOSING == m_eState)
		return;

	// 벽이 생성되기 시작하는 순간부터 보스방 이탈을 차단한다.
	if (FAILED(Register_Physics()))
		return;

	m_eState = BARRIER_STATE::CLOSING;
}

void CBossArenaBarrier::Update_Dissolve(_float fTimeDelta)
{
	if (fTimeDelta <= 0.f)
		return;

	if (BARRIER_STATE::OPENING == m_eState)
	{
		m_fDissolveAmount += m_fDissolveSpeed * fTimeDelta;

		if (m_fDissolveAmount >= 1.f)
		{
			m_fDissolveAmount = 1.f;
			m_eState = BARRIER_STATE::OPEN;
		}

		return;
	}

	if (BARRIER_STATE::CLOSING == m_eState)
	{
		m_fDissolveAmount -= m_fDissolveSpeed * fTimeDelta;

		if (m_fDissolveAmount <= 0.f)
		{
			m_fDissolveAmount = 0.f;
			m_eState = BARRIER_STATE::CLOSED;
		}
	}
}

void CBossArenaBarrier::Configure_RenderCommand(Engine::RenderCommand* pCommand) const
{
	__super::Configure_RenderCommand(pCommand);

	if (nullptr == pCommand)
		return;

	// 검은 장벽이므로 디졸브 가장자리 발광도 검은색과 Alpha 0으로 유지한다.
	pCommand->fDissolveAmount = m_fDissolveAmount;
	pCommand->vDissolveColor = m_vDissolveColor;
}

_float CBossArenaBarrier::Get_SignedPassageSide(CGameObject* pInstigator) const
{
	if (nullptr == pInstigator || nullptr == m_pTransformCom)
		return 0.f;

	const _float4x4* pInstigatorWorld = pInstigator->Get_WorldTransform();

	if (nullptr == pInstigatorWorld)
		return 0.f;

	const _vector vBarrierPosition = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vBarrierNormal = m_pTransformCom->Get_State(STATE::RIGHT);

	if (XMVectorGetX(XMVector3LengthSq(vBarrierNormal)) <= 1e-8f)
		return 0.f;

	vBarrierNormal = XMVector3Normalize(vBarrierNormal);

	const _vector vInstigatorPosition = XMVectorSet(pInstigatorWorld->_41, pInstigatorWorld->_42, pInstigatorWorld->_43, 1.f);
	return XMVectorGetX(XMVector3Dot(vInstigatorPosition - vBarrierPosition, vBarrierNormal));
}

_bool CBossArenaBarrier::Can_Interact(CGameObject* pPlayer) const
{
	if (nullptr == pPlayer || BARRIER_ROLE::ENTRANCE != m_eRole || false == m_bPassageEnabled || false == m_bPlayerInPassageTrigger || true == m_bPermanentlyOpen)
		return false;

	// 닫혀 있거나 닫히는 중인 재도전 입구 장벽만 E 상호작용을 허용한다.
	return BARRIER_STATE::CLOSED == m_eState || BARRIER_STATE::CLOSING == m_eState;
}

_bool CBossArenaBarrier::Interact(CGameObject* pPlayer)
{
	if (false == Can_Interact(pPlayer))
		return false;

	// UI는 입력 즉시 닫고 장벽은 기존 디졸브 속도로 개방한다. OPEN_DOOR 이벤트를 발행하지 않아 플레이어 애니메이션은 재생되지 않는다.
	Hide_InteractionPrompt();
	Begin_Open();

	return true;
}

void CBossArenaBarrier::Show_InteractionPrompt()
{
	if (true == m_bInteractionPromptVisible)
		return;

	Broadcast_ObjUI(true, OBJECT_TYPE::DOOR);
	m_bInteractionPromptVisible = true;
}

void CBossArenaBarrier::Hide_InteractionPrompt()
{
	if (false == m_bInteractionPromptVisible)
		return;

	Broadcast_ObjUI(false, OBJECT_TYPE::DOOR);
	m_bInteractionPromptVisible = false;
}

void CBossArenaBarrier::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator || BARRIER_ROLE::ENTRANCE != m_eRole || false == m_bPassageEnabled || true == m_bPermanentlyOpen)
		return;

	// 외부 Trigger의 PLAYER 필터가 타입을 보장한다.
	m_bPlayerInPassageTrigger = true;
	m_fPassageEntrySide = Get_SignedPassageSide(pInstigator);

	// 재도전 입구 장벽은 자동 개방하지 않고 E 입력을 안내한다.
	if (true == Can_Interact(pInstigator))
		Show_InteractionPrompt();
}

void CBossArenaBarrier::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	if (nullptr == pInstigator || BARRIER_ROLE::ENTRANCE != m_eRole || false == m_bPlayerInPassageTrigger)
		return;

	const _float fPassageExitSide = Get_SignedPassageSide(pInstigator);
	const _bool bCrossedBarrier = (m_fPassageEntrySide < -0.05f && fPassageExitSide > 0.05f) || (m_fPassageEntrySide > 0.05f && fPassageExitSide < -0.05f);

	Hide_InteractionPrompt();
	m_bPlayerInPassageTrigger = false;
	m_fPassageEntrySide = 0.f;
	m_bCloseRequested = true;

	if (true == bCrossedBarrier)
	{
		// 반대편으로 통과했을 때만 재도전 전투를 시작한다.
		m_bPassageEnabled = false;
		m_bBossStartRequested = true;
	}
}

void CBossArenaBarrier::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{
	if (true == m_bPermanentlyOpen)
		return;

	switch (eType)
	{
	case EVENT_TYPE::BOSS_START:
		if (nullptr == pData)
			return;

		{
			const BossClear_EventDesc* pStartDesc = static_cast<const BossClear_EventDesc*>(pData);
			const _bool bIsPrimaryStart = m_eStartBossType == pStartDesc->eMonsterType;
			const _bool bIsRaxasiaPhase2Start = SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P1 == m_eStartBossType && SPAWN_MONSTER_TYPE::BOSS_RAXASIA_P2 == pStartDesc->eMonsterType;

			// 락사시아 장벽은 P1/P2 시작만 받고, 다른 보스의 전역 시작 이벤트는 무시한다.
			if (false == bIsPrimaryStart && false == bIsRaxasiaPhase2Start)
				return;
		}

		Hide_InteractionPrompt();
		m_bEncounterStarted = true;
		m_bPassageEnabled = false;

		// 통과 도중 들어온 시작 이벤트는 Trigger End에서 안전하게 닫는다.
		if (BARRIER_ROLE::ENTRANCE == m_eRole && true == m_bPlayerInPassageTrigger)
			return;

		m_bCloseRequested = true;
		break;

	case EVENT_TYPE::REQUEST_RESPAWN:
		if (false == m_bEncounterStarted)
			return;

		Hide_InteractionPrompt();
		m_bPlayerInPassageTrigger = false;
		m_fPassageEntrySide = 0.f;
		m_bBossStartRequested = false;
		m_bOpenRequested = false;
		m_bCloseRequested = true;

		// 첫 전투에서 사용한 애니메이션 문은 재도전부터 장벽으로 완전히 교체한다.
		if (BARRIER_ROLE::ENTRANCE == m_eRole)
			m_bLegacyDoorDeactivateRequested = true;

		// 재도전 시에는 입구 장벽만 통과 요청을 받는다.
		m_bPassageEnabled = BARRIER_ROLE::ENTRANCE == m_eRole;
		break;

	default:
		break;
	}
}

void CBossArenaBarrier::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
	if (EVENT_TYPE_MAP_OBJECT::BOSS_CLEAR != eType || nullptr == pData)
		return;

	BossClear_EventDesc* pClearDesc = static_cast<BossClear_EventDesc*>(pData);

	// 락사시아 1페이즈 클리어는 무시하고 지정된 최종 페이즈만 처리한다.
	if (m_eClearBossType != pClearDesc->eMonsterType)
		return;

	Hide_InteractionPrompt();
	m_bPermanentlyOpen = true;
	m_bPassageEnabled = false;
	m_bPlayerInPassageTrigger = false;
	m_bBossStartRequested = false;
	m_bCloseRequested = false;
	m_bOpenRequested = true;
}

CBossArenaBarrier* CBossArenaBarrier::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBossArenaBarrier* pInstance = new CBossArenaBarrier(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBossArenaBarrier");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CBossArenaBarrier::Clone(void* pArg)
{
	CBossArenaBarrier* pInstance = new CBossArenaBarrier(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBossArenaBarrier");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBossArenaBarrier::Free()
{
	// 트리거 종료 콜백 없이 언로드돼도 문 열기 안내 UI가 남지 않게 한다.
	Hide_InteractionPrompt();

	// EventManager가 Listener 생포인터를 보관하므로 객체 해제 전에 모든 구독을 제거한다.
	if (nullptr != m_pGameInstance)
		m_pGameInstance->UnSubscribe_AllEvent(this);

	__super::Free();
}
