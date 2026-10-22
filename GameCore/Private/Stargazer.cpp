#include "Stargazer.h"
#include "Animator.h"
#include "Model.h"
#include "GameInstance.h"
#include "Layer.h"
#include "Player.h"
#include "Effect.h"
#include "StargazerManager.h"


CStargazer::CStargazer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CStargazer::CStargazer(const CStargazer& Prototype)
	: CMapGameplayObject{ Prototype }
{
}

HRESULT CStargazer::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CStargazer::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;


	STARGAZER_DESC* pDesc = static_cast<STARGAZER_DESC*>(pArg);
	Ready_DefaultDesc(pDesc);

	if (true == m_bActivated)
	{
		// 이미 활성화된 별바라기는 Normal 컨트롤러의 Entry/Open_Idle 흐름으로 시작한다.
		if (FAILED(__super::Initialize(&m_NormalDesc)))
			return E_FAIL;

		m_eState = STARGAZER_STATE::OPEN_IDLE;

		if (FAILED(Register_Physics()))
			return E_FAIL;

		return S_OK;
	}

	// 기본 상태는 Broken 컨트롤러의 Entry/Broken_Idle 흐름으로 시작한다.
	if (FAILED(__super::Initialize(&m_BrokenDesc)))
		return E_FAIL;

	m_eState = STARGAZER_STATE::BROKEN_IDLE;
	
	_float3 Pos;
	XMStoreFloat3(&Pos, m_pTransformCom->Get_State(STATE::POSITION));
	m_pEffect_Wait = m_pGameInstance->Play_Effect("Effect_Stargazer_Wait", Pos);
	//m_pEffect_Wait = m_pGameInstance->Play_Effect("Effect_Stargazer_Idle", m_pModelCom, "Audio", m_pTransformCom->Get_WorldMatrixPtr());
	Start_OpenLoopSound();

	return S_OK;
}

void CStargazer::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);



	// Drive_Animator 이후 별바라기 전용 상태 전환을 처리한다.
	Update_StargazerState(fTimeDelta);
}

void CStargazer::Ready_DefaultDesc(const STARGAZER_DESC* pDesc)
{
	m_BrokenDesc = *pDesc;
	m_NormalDesc = pDesc->NormalDesc;
	m_bActivated = true == pDesc->bUseHotelModel ? true : pDesc->bActivated;
	m_fInteractionRange = pDesc->fInteractionRange;
	m_pStargazerManager = pDesc->pStargazerManager;
	m_iStargazerID = pDesc->iStargazerID;

	// Stargazer 인스턴스가 사용하는 동안 Runtime Manager 수명을 보장한다.
	Safe_AddRef(m_pStargazerManager);

	// Normal 모델도 Broken 모델과 같은 배치 Transform을 사용한다.
	m_NormalDesc.vScale = pDesc->vScale;
	m_NormalDesc.vTranslate = pDesc->vTranslate;
	m_NormalDesc.vRotationDegree = pDesc->vRotationDegree;
	m_NormalDesc.fSpeedPerSec = pDesc->fSpeedPerSec;
	m_NormalDesc.fRotationPerSec = pDesc->fRotationPerSec;


	// 피직스 세팅 
	m_NormalDesc.bUsePhysics = m_BrokenDesc.bUsePhysics;
	m_NormalDesc.bPhysicsStartRegistered = m_BrokenDesc.bPhysicsStartRegistered;
	m_NormalDesc.vPhysicsHalfExtents = m_BrokenDesc.vPhysicsHalfExtents;
	m_NormalDesc.vPhysicsLocalOffset = m_BrokenDesc.vPhysicsLocalOffset;
	m_NormalDesc.vPhysicsLocalQuat = m_BrokenDesc.vPhysicsLocalQuat;
	m_NormalDesc.iPhysicsFilterWord0 = m_BrokenDesc.iPhysicsFilterWord0;
	m_NormalDesc.iPhysicsFilterWord1 = m_BrokenDesc.iPhysicsFilterWord1;

	if (0 == m_NormalDesc.iModelLevel)
		m_NormalDesc.iModelLevel = m_BrokenDesc.iModelLevel;

	// Broken 모델 컨트롤러 세팅.
	m_BrokenDesc.strModelPrototypeTag = m_BrokenDesc.strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Stargazer_Broken") : m_BrokenDesc.strModelPrototypeTag;
	m_BrokenDesc.strAnimFolderTag = m_BrokenDesc.strAnimFolderTag.empty() ? TEXT("Stargazer_Broken") : m_BrokenDesc.strAnimFolderTag;
	m_BrokenDesc.strAnimTag = m_BrokenDesc.strAnimTag.empty() ? TEXT("Stargazer_Broken") : m_BrokenDesc.strAnimTag;
	m_BrokenDesc.bControllerEnabled = true;
	m_BrokenDesc.bPlayDefault = false;

	if (true == pDesc->bUseHotelModel)
	{
		// 호텔 별바라기는 애니메이션이 없는 정적 모델로 렌더링한다.
		m_NormalDesc.eVisualType = CMapGameplayObject::VISUAL_TYPE::STATIC;
		m_NormalDesc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Stargazer_Hotel");
		m_NormalDesc.strAnimFolderTag.clear();
		m_NormalDesc.strAnimTag.clear();
		m_NormalDesc.bControllerEnabled = false;
		m_NormalDesc.bPlayDefault = false;
	}
	else
	{
		// 일반 Normal 모델 컨트롤러 세팅.
		m_NormalDesc.strModelPrototypeTag = m_NormalDesc.strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Stargazer") : m_NormalDesc.strModelPrototypeTag;
		m_NormalDesc.strAnimFolderTag = m_NormalDesc.strAnimFolderTag.empty() ? TEXT("Stargazer") : m_NormalDesc.strAnimFolderTag;
		m_NormalDesc.strAnimTag = m_NormalDesc.strAnimTag.empty() ? TEXT("Stargazer") : m_NormalDesc.strAnimTag;
		m_NormalDesc.bControllerEnabled = true;
		m_NormalDesc.bPlayDefault = false;
	}
}

_bool CStargazer::Can_Interact(CGameObject* pPlayer) const
{

	if (m_eState != STARGAZER_STATE::BROKEN_IDLE && m_eState != STARGAZER_STATE::OPEN_IDLE) return false;
	return true;
}

_bool CStargazer::Interact(CGameObject* pPlayer)
{
	if (nullptr == pPlayer)
		return false;

	if (!m_pGameInstance->Can_PushUI(UI_INPUT_OWNER_KIND::SCREEN))
		return false;

	return SUCCEEDED(Begin_Interaction(pPlayer));

}

_float4x4 CStargazer::Get_InteractTransform(CGameObject* pPlayer) const
{
	// 부서진 상태와 전환 애니메이션 중에는 플레이어의 현재 월드 행렬을 그대로 반환한다.
	_float4x4 ResultWorldMatrix = __super::Get_InteractTransform(pPlayer);

	if (false == m_bActivated || STARGAZER_STATE::OPEN_IDLE != m_eState || nullptr == pPlayer || nullptr == pPlayer->Get_Transform() || nullptr == m_pTransformCom)
		return ResultWorldMatrix;

	const _vector vPlayerPosition = pPlayer->Get_Transform()->Get_State(STATE::POSITION);
	const _vector vObjectPosition = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vLook = XMVectorSetY(vObjectPosition - vPlayerPosition, 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vLook)) <= 1e-8f)
		return ResultWorldMatrix;

	vLook = XMVector3Normalize(vLook);

	// 플레이어 위치는 그대로 두고 별바라기를 향하는 회전값만 행렬에 담아 제공한다.
	const _float fYaw = atan2f(XMVectorGetX(vLook), XMVectorGetZ(vLook));
	_matrix InteractWorldMatrix = XMMatrixRotationY(fYaw);
	InteractWorldMatrix.r[3] = XMVectorSetW(vPlayerPosition, 1.f);
	XMStoreFloat4x4(&ResultWorldMatrix, InteractWorldMatrix);

	return ResultWorldMatrix;
}

HRESULT CStargazer::Begin_Interaction(CGameObject* pPlayer)
{
	if (nullptr == pPlayer)
		return E_FAIL;

	if (false == m_bActivated && m_eState == STARGAZER_STATE::BROKEN_IDLE)
	{
		if (FAILED(Build_RespawnPoint(pPlayer, &m_PendingRepairRespawnPoint)))
			return E_FAIL;

		m_bHasPendingRepairRespawnPoint = true;
		m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::STARGAZER_REPAIR, nullptr);

		const HRESULT hResult = Enter_Restoring();

		if (FAILED(hResult))
			m_bHasPendingRepairRespawnPoint = false;

		return hResult;
	}

	if (true == m_bActivated && m_eState == STARGAZER_STATE::OPEN_IDLE)
	{
		RespawnPoint_EventDesc Desc = {};

		if (FAILED(Build_RespawnPoint(pPlayer, &Desc.SpawnPoint)))
			return E_FAIL;

		if (nullptr != m_pStargazerManager && INVALID_STARGAZER_ID != m_iStargazerID)
		{
			if (FAILED(m_pStargazerManager->Update_RespawnPoint(m_iStargazerID, Desc.SpawnPoint)))
				return E_FAIL;
		}

		m_pGameInstance->Broadcast(EVENT_TYPE::UPDATE_RESPAWN_POINT, &Desc);
		m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::STARGAZER_ACTIVATED, &Desc);

		return Begin_ActivatedInteraction();
	}

	
	return S_OK;
}

HRESULT CStargazer::Change_ModelAndAnimator(const CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC& Desc)
{
	// 모델과 Animator 교체 책임은 공통 MapGameplayObject 구현에 둔다.
	return Change_Visual(Desc);
}

HRESULT CStargazer::Enter_Restoring()
{
	// 복구 중에는 상호작용할 수 없으므로 Broken 안내를 제거한다.
	Hide_InteractionPrompt();

	m_eState = STARGAZER_STATE::RESTORING;

	// Broken 컨트롤러의 Broken_Restore State를 요청한다.
	Set_AnimBool(TEXT("Broken_Idle"), false);
	Set_AnimBool(TEXT("Broken_Restore"), true);
	Request_AnimState(TEXT("Broken_Restore"));

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Play_Cue(CUE_RESTORE, CHANNEL_ID::SOUND_SFX, vWorldPos);
	m_pGameInstance->Play_Cue(CUE_RESTORE_BASE, CHANNEL_ID::SOUND_SFX, vWorldPos);
	m_pGameInstance->Play_Cue(CUE_RESTORE_WHISPER, CHANNEL_ID::SOUND_SFX, vWorldPos);

	return S_OK;
}

void CStargazer::Start_OpenLoopSound()
{
	if (0 != m_hOpenLoopSound || nullptr == m_pTransformCom)
		return;

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_hOpenLoopSound = m_pGameInstance->Play_Cue(CUE_OPEN_LOOP, CHANNEL_ID::SOUND_SFX, vWorldPos, 1.f, true);
}


void CStargazer::Stop_OpenLoopSound()
{
	if (0 == m_hOpenLoopSound)
		return;

	m_pGameInstance->Stop_SoundSource(m_hOpenLoopSound);
	m_hOpenLoopSound = 0;
}


HRESULT CStargazer::Enter_Opening()
{
	CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC ChangeDesc = m_NormalDesc;
	ChangeDesc.bControllerEnabled = false;
	ChangeDesc.bPlayDefault = false;

	// 복구 중에는 Normal Controller의 Entry/Open_Idle 실행을 막고 모델만 교체한다.
	if (FAILED(Change_ModelAndAnimator(ChangeDesc)))
		return E_FAIL;

	m_pEffect_Wait->Stop();
	m_pGameInstance->Play_Effect("Effect_Stargazer_Activate", m_pModelCom, "Audio", m_pTransformCom->Get_WorldMatrixPtr());
	m_bActivated = true;
	m_eState = STARGAZER_STATE::OPENING;

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Play_Cue(CUE_OPEN, CHANNEL_ID::SOUND_SFX, vWorldPos);

	// Controller를 거치지 않고 Broken_Close_Open 시퀀스를 즉시 재생한다.
	if (FAILED(Play_AnimSequence(TEXT("Broken_Close_Open"))))
		return E_FAIL;

	return S_OK;
}

HRESULT CStargazer::Enter_OpenIdle()
{
	// 실패 시 활성화 상태가 부분 반영되지 않도록 Physics 등록을 먼저 끝낸다.
	if (FAILED(Register_Physics()))
		return E_FAIL;

	RespawnPoint_EventDesc RespawnDesc = {};
	_bool bHasRespawnPoint = false;

	if (nullptr != m_pStargazerManager && INVALID_STARGAZER_ID != m_iStargazerID)
	{
		if (false == m_bHasPendingRepairRespawnPoint)
			return E_FAIL;

		// 복구 완료 시 Runtime 활성화와 방문 상태를 함께 기록한다.
		if (FAILED(m_pStargazerManager->Activate_Stargazer(m_iStargazerID, m_PendingRepairRespawnPoint)))
			return E_FAIL;

		if (FAILED(m_pStargazerManager->Get_RespawnPoint(m_iStargazerID, &RespawnDesc.SpawnPoint)))
			return E_FAIL;

		bHasRespawnPoint = true;
	}

	m_bActivated = true;
	m_eState = STARGAZER_STATE::OPEN_IDLE;

	m_pEffect_Wait = m_pGameInstance->Play_Effect("Effect_Stargazer_Idle", m_pModelCom, "Audio", m_pTransformCom->Get_WorldMatrixPtr());

	// Broken_Close_Open 종료 후 기본 Entry/Open_Idle 조건을 설정한다.
	Set_AnimBool(TEXT("Broken_Close_Open"), false);
	Set_AnimBool(TEXT("Open_Idle"), true);

	if (nullptr != m_pAnimator)
	{
		// 복구 시퀀스가 끝난 뒤 Controller를 켜서 기본 Entry/Open_Idle로 진입한다.
		m_pAnimator->Set_ControllerEnabled(true);
		m_pAnimator->Update(0.f);
	}

	// Broken에서 Normal 복구가 완료된 시점에 사망 복귀 위치를 저장한다.
	if (true == bHasRespawnPoint)
		m_pGameInstance->Broadcast(EVENT_TYPE::UPDATE_RESPAWN_POINT, &RespawnDesc);

	m_bHasPendingRepairRespawnPoint = false;

	// 플레이어가 계속 트리거 안에 있으면 정상 Stargazer 안내를 표시한다.
	if (true == m_bPlayerInTrigger)
		Show_InteractionPrompt();

	return S_OK;
}
HRESULT CStargazer::Begin_ActivatedInteraction()
{
	if (nullptr == m_pStargazerManager || INVALID_STARGAZER_ID == m_iStargazerID)
		return E_FAIL;

	STARGAZER_UI_OPEN_DESC UIOpenDesc = {};

	if (FAILED(m_pStargazerManager->Build_UIOpenDesc(m_iStargazerID, &UIOpenDesc)))
		return E_FAIL;

	// EventManager Broadcast는 동기 호출이므로 UI는 Callback 안에서 데이터를 복사해야 한다.
	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::STARGAZER_UI_OPEN, &UIOpenDesc);

	return S_OK;
}

HRESULT CStargazer::Build_RespawnPoint(CGameObject* pPlayer, Respawn_Point* pOutPoint) const
{
	if (nullptr == pOutPoint)
		return E_FAIL;

	if (nullptr == pPlayer || nullptr == pPlayer->Get_Transform() || nullptr == m_pTransformCom)
		return E_FAIL;

	_vector vPlayerPosition = pPlayer->Get_Transform()->Get_State(STATE::POSITION);
	const _vector vStargazerPosition = m_pTransformCom->Get_State(STATE::POSITION);

	vPlayerPosition = XMVectorSetW(vPlayerPosition, 1.f);
	XMStoreFloat4(&pOutPoint->vPosition, vPlayerPosition);

	const _float fDeltaX = XMVectorGetX(vStargazerPosition) - XMVectorGetX(vPlayerPosition);
	const _float fDeltaZ = XMVectorGetZ(vStargazerPosition) - XMVectorGetZ(vPlayerPosition);

	pOutPoint->fYaw = IsEqual(fDeltaX, 0.f) && IsEqual(fDeltaZ, 0.f) ? 0.f : atan2f(fDeltaX, fDeltaZ);

	return S_OK;
}

void CStargazer::Update_MapGameplayObject(_float fTimeDelta)
{
	// 상호작용 입력은 Player가 처리하므로 Stargazer 전용 Update 로직은 없다.
}

void CStargazer::Update_StargazerState(_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);

	if (m_eState == STARGAZER_STATE::RESTORING && true == Is_AnimationFinished())
	{
		if (FAILED(Enter_Opening()))
		{
			Hide_InteractionPrompt();
			m_eState = STARGAZER_STATE::END;
			OutputDebugStringW(L"[Stargazer] Enter_Opening failed.\n");
		}

		return;
	}

	if (m_eState == STARGAZER_STATE::OPENING && true == Is_AnimationFinished())
	{
		if (FAILED(Enter_OpenIdle()))
		{
			Hide_InteractionPrompt();
			m_eState = STARGAZER_STATE::END;
			OutputDebugStringW(L"[Stargazer] Enter_OpenIdle failed.\n");
		}

		return;
	}
}

void CStargazer::On_AnimNotify(_uint64 iNotifyHash)
{
	// Effect_Stargazer, Sound_stargazer 같은 Notify 처리는 이후 여기서 연결한다.
}

OBJECT_TYPE CStargazer::Resolve_InteractionObjectType() const
{
	switch (m_eState)
	{
	case STARGAZER_STATE::BROKEN_IDLE:
		return OBJECT_TYPE::BROKEN_STARGAZER;

	case STARGAZER_STATE::OPEN_IDLE:
		return OBJECT_TYPE::STARGAZER;

	default:
		// 복구 및 열림 애니메이션 중에는 상호작용 안내를 표시하지 않는다.
		return OBJECT_TYPE::END;
	}
}

void CStargazer::Show_InteractionPrompt()
{
	const OBJECT_TYPE ePromptType = Resolve_InteractionObjectType();

	if (OBJECT_TYPE::END == ePromptType || m_eVisiblePromptType == ePromptType)
		return;

	// 다른 타입이 표시 중이면 기존 안내를 먼저 제거한다.
	Hide_InteractionPrompt();

	m_eVisiblePromptType = ePromptType;
	Broadcast_ObjUI(true, m_eVisiblePromptType);
}

void CStargazer::Hide_InteractionPrompt()
{
	if (OBJECT_TYPE::END == m_eVisiblePromptType)
		return;

	// 실제 Show 이벤트를 보냈던 타입과 동일한 타입으로 Hide한다.
	Broadcast_ObjUI(false, m_eVisiblePromptType);
	m_eVisiblePromptType = OBJECT_TYPE::END;
}


void CStargazer::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	Set_PlayerInTrigger(true);

	// 부서진 상태면 BROKEN_STARGAZER, 정상 상태면 STARGAZER를 표시한다.
	Show_InteractionPrompt();
}

void CStargazer::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	Set_PlayerInTrigger(false);

	// 현재 상태가 아니라 실제 표시했던 타입을 숨긴다.
	Hide_InteractionPrompt();
}


CStargazer* CStargazer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CStargazer* pInstance = new CStargazer(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CStargazer");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CStargazer::Clone(void* pArg)
{
	CStargazer* pInstance = new CStargazer(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CStargazer");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CStargazer::Free()
{	
	m_pEffect_Wait = nullptr;
	// 트리거 종료 콜백 없이 챕터가 언로드되어도 안내 UI가 남지 않게 한다.
	Hide_InteractionPrompt();

	Safe_Release(m_pStargazerManager);

	__super::Free();
}
