#include "NPC.h"

#include "GameInstance.h"
#include "Player.h"

CNPC::CNPC(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CNPC::CNPC(const CNPC& Prototype)
	: CMapGameplayObject{ Prototype }
{
}

HRESULT CNPC::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CNPC::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	NPC_DESC Desc = *static_cast<NPC_DESC*>(pArg);

	Ready_DefaultDesc(&Desc);

	if (UINT_MAX == Desc.iSpawnID || NPC_TYPE::END == Desc.eNPCType)
		return E_FAIL;

	m_iSpawnID = Desc.iSpawnID;
	m_eNPCType = Desc.eNPCType;
	m_fTalkMaxYawDegree = Desc.fTalkMaxYawDegree;

	if (FAILED(__super::Initialize(&Desc)))
		return E_FAIL;

	// 플레이어를 바라본 뒤 복구할 맵 배치 기준 회전을 보관한다.
	if (nullptr != m_pTransformCom)
		XMStoreFloat4(&m_vDefaultQuaternion, m_pTransformCom->Get_Quat());

	// NPC는 항상 Idle 상태로 시작한다.
	Enter_Idle();

	// 첫 프레임 전에 Controller Entry와 DialogueActive 초기값을 반영한다.
	Drive_Animator(0.f);

	// NPC는 UI가 보내는 종료 이벤트만 구독한다.
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::NPC_TALK_END, this);

	return S_OK;
}

void CNPC::Ready_DefaultDesc(NPC_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	pDesc->eVisualType = VISUAL_TYPE::SKINNED;
	pDesc->bControllerEnabled = true;
	pDesc->bPlayDefault = false;
	pDesc->bApplyRootMotion = false;

	// bUsePhysics는 NPC 몸체 충돌 여부이므로 여기서 임의 변경하지 않는다.
	// 근접 판정은 별도의 Map Trigger Box가 담당한다.
}

HRESULT CNPC::Resolve_ActorType(const _wstring& strActorType, NPC_DESC* pDesc)
{
	if (nullptr == pDesc || strActorType.empty())
		return E_FAIL;

	// NPC별 대화 회전 제한값은 각 ActorType 분기에서 독립적으로 설정한다.
	if (L"NPC_Sophia" == strActorType)
	{
		pDesc->eNPCType = NPC_TYPE::SOPHIA;
		pDesc->fTalkMaxYawDegree = 45.f;
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_NPC_Sophia");
		pDesc->strAnimFolderTag = TEXT("Sophia");
		pDesc->strAnimTag = TEXT("Sophia");
		return S_OK;
	}

	if (L"NPC_Antonia" == strActorType)
	{
		pDesc->eNPCType = NPC_TYPE::ANTONIA;
		pDesc->fTalkMaxYawDegree = 0.f;
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_NPC_Antonia");
		pDesc->strAnimFolderTag = TEXT("Antonia");
		pDesc->strAnimTag = TEXT("Antonia");
		return S_OK;
	}

	if (L"NPC_Polendina" == strActorType)
	{
		pDesc->eNPCType = NPC_TYPE::POLENDINA;
		pDesc->fTalkMaxYawDegree = 15.f;
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_NPC_Polendina");
		pDesc->strAnimFolderTag = TEXT("Polendina");
		pDesc->strAnimTag = TEXT("Polendina");
		return S_OK;
	}

	if (L"NPC_Vendor" == strActorType)
	{
		pDesc->eNPCType = NPC_TYPE::VENDOR;
		pDesc->fTalkMaxYawDegree = 45.f;
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_NPC_Vendor");
		pDesc->strAnimFolderTag = TEXT("Vendor");
		pDesc->strAnimTag = TEXT("Vendor");
		return S_OK;
	}

	if (L"NPC_Riddleking" == strActorType)
	{
		pDesc->eNPCType = NPC_TYPE::RIDDLEKING;
		pDesc->fTalkMaxYawDegree = 0.f;
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_NPC_Riddleking");
		pDesc->strAnimFolderTag = TEXT("Riddleking");
		pDesc->strAnimTag = TEXT("Riddleking");
		return S_OK;
	}

	if (L"NPC_Eugenie" == strActorType)
	{
		pDesc->eNPCType = NPC_TYPE::EUGENIE;
		pDesc->fTalkMaxYawDegree = 0.f;
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_NPC_Eugenie");
		pDesc->strAnimFolderTag = TEXT("Eugenie");
		pDesc->strAnimTag = TEXT("Eugenie");
		return S_OK;
	}

	return E_FAIL;
}

void CNPC::Update_MapGameplayObject(_float fTimeDelta)
{
	// 대화 상태와 관계없이 진행 중인 바라보기 회전을 매 프레임 보간한다.
	Update_FacingInterpolation(fTimeDelta);

	// Talk_Start가 끝나 Animator가 Talking으로 넘어간 시점에 논리 상태를 동기화한다.
	if (NPC_STATE::TALK_START == m_eState && Is_AnimInState(TEXT("Talking")))
	{
		Enter_Talking();
		return;
	}

	// 대화 종료 후 Animator가 Idle로 복귀한 시점에 다시 입력을 허용한다.
	if (NPC_STATE::TALK_END == m_eState && Is_AnimInState(TEXT("Idle")))
	{
		Enter_Idle();
	}
}

_bool CNPC::Can_Interact(CGameObject* pPlayer) const
{
	// 현재 Linked Trigger에 들어온 동일 플레이어만 Idle 상태에서 대화할 수 있다.
	if (nullptr == pPlayer || nullptr == m_pGameInstance)
		return false;

	return true == m_bPlayerInTrigger && NPC_STATE::IDLE == m_eState && pPlayer == m_pInteractionPlayer;
}

_bool CNPC::Interact(CGameObject* pPlayer)
{
	if (false == Can_Interact(pPlayer))
		return false;

	// 추가
	// 이미 ESC 메뉴, 대화창, 별바라기 같은 Screen이 활성화되어 있으면 대화를 시작하지 않는다.
	if (!m_pGameInstance->Can_PushUI(UI_INPUT_OWNER_KIND::SCREEN))
		return false;

	return Begin_Dialogue();
}

_bool CNPC::Begin_Dialogue()
{
	if (false == m_bPlayerInTrigger || NPC_STATE::IDLE != m_eState)
		return false;


	// 대화를 시작하면 Trigger 안내 문구는 즉시 숨긴다.
	Hide_InteractionPrompt();

	// 추가
	// NPC가 반응하기 전에 현재 상황에 맞는 대사 스크립트를 먼저 선택한다.
	const string strDialogueID = Resolve_DialogueID();

	// 실행할 대사가 없는 NPC는 회전, 애니메이션, 이벤트를 모두 시작하지 않는다.
	if (strDialogueID.empty())
		return false;

	// 대화를 건 플레이어 방향을 NPC별 최대 회전각 안에서 바라본다.
	Face_InteractionPlayer();

	// Broadcast 전에 먼저 TALK_START로 전환한다.
	// EventManager가 동기 호출이므로 UI가 즉시 END를 보내도 정상 처리할 수 있다.
	Enter_TalkStart();

	NPC_DIALOGUE_EVENT_DESC EventDesc{};
	EventDesc.iTargetSpawnID = m_iSpawnID;
	EventDesc.eNPCType = m_eNPCType;
	EventDesc.strDialogueID = strDialogueID;
	EventDesc.bLockCamera = false;

	if (nullptr != m_pTransformCom)
		XMStoreFloat3(&EventDesc.vSpeakerLookPos, m_pTransformCom->Get_State(STATE::POSITION));

	// UI는 NPC_TYPE에 맞는 대화창을 처리한다.
	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::NPC_TALK_START, &EventDesc);
	return true;
}

void CNPC::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{

}

void CNPC::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
	if (EVENT_TYPE_MAP_OBJECT::NPC_TALK_END != eType || nullptr == pData)
		return;

	const NPC_DIALOGUE_EVENT_DESC* pDesc = static_cast<const NPC_DIALOGUE_EVENT_DESC*>(pData);

	// 다른 NPC의 대화 종료 이벤트는 무시한다.
	if (m_iSpawnID != pDesc->iTargetSpawnID || m_eNPCType != pDesc->eNPCType)
		return;

	// 대화 시작 또는 진행 상태가 아니면 중복 종료 이벤트로 판단한다.
	if (NPC_STATE::TALK_START != m_eState && NPC_STATE::TALKING != m_eState)
		return;

	Enter_TalkEnd();
}

void CNPC::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	// Linked Trigger의 PLAYER 필터가 타입을 보장하므로 RTTI 검사 없이 포인터를 보관한다.
	CPlayer* pPlayer = static_cast<CPlayer*>(pInstigator);

	// 외부 Trigger Box가 활성화되면 E 입력을 허용한다.
	m_bPlayerInTrigger = true;
	m_pInteractionPlayer = pPlayer;

	// Idle 상태에서만 NPC 상호작용 안내를 표시한다.
	Show_InteractionPrompt();
}

void CNPC::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;


	CPlayer* pPlayer = static_cast<CPlayer*>(pInstigator);

	if (m_pInteractionPlayer != pPlayer)
		return;

	// Trigger Box 밖에서는 새 대화를 시작할 수 없다.
	m_bPlayerInTrigger = false;
	m_pInteractionPlayer = nullptr;
	Hide_InteractionPrompt();
}

void CNPC::Show_InteractionPrompt()
{
	if (true == m_bInteractionPromptVisible || false == m_bPlayerInTrigger || NPC_STATE::IDLE != m_eState)
		return;

	// OBJ_UI는 NPC 공통 상호작용 안내만 전달한다.
	Broadcast_ObjUI(true, OBJECT_TYPE::NPC);
	m_bInteractionPromptVisible = true;
}

void CNPC::Hide_InteractionPrompt()
{
	if (false == m_bInteractionPromptVisible)
		return;

	Broadcast_ObjUI(false, OBJECT_TYPE::NPC);
	m_bInteractionPromptVisible = false;
}

void CNPC::Enter_Idle()
{
	m_eState = NPC_STATE::IDLE;

	// Idle은 Controller의 Entry/Auto 상태이므로 파라미터만 초기화한다.
	Set_AnimBool(TEXT("DialogueActive"), false);

	// 대화 종료 후에도 Trigger 안에 있으면 다시 상호작용할 수 있음을 알린다.
	Show_InteractionPrompt();
}

void CNPC::Enter_TalkStart()
{
	m_eState = NPC_STATE::TALK_START;

	// 대화 조건을 켠 뒤 Talk_Start 단발 상태를 명시적으로 요청한다.
	Set_AnimBool(TEXT("DialogueActive"), true);
	Request_AnimState(TEXT("Talk_Start"));
}

void CNPC::Enter_Talking()
{
	// 실제 Talking 진입은 Animator Controller가 Talk_Start 종료 후 처리한다.
	m_eState = NPC_STATE::TALKING;
}

void CNPC::Enter_TalkEnd()
{
	m_eState = NPC_STATE::TALK_END;

	// UI에서 대화 종료를 알리면 맵에 배치된 원래 정면으로 돌아간다.
	Restore_DefaultFacing();

	// DialogueActive가 꺼지면 Controller가 자동으로 Idle을 선택한다.
	Set_AnimBool(TEXT("DialogueActive"), false);
}

void CNPC::Face_InteractionPlayer()
{
	if (nullptr == m_pTransformCom || nullptr == m_pInteractionPlayer || nullptr == m_pInteractionPlayer->Get_Transform())
		return;

	const _vector vDefaultQuaternion = XMQuaternionNormalize(XMLoadFloat4(&m_vDefaultQuaternion));
	_vector vDefaultLook = XMVectorSetY(XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.f, 0.f), vDefaultQuaternion), 0.f);
	_vector vDefaultRight = XMVectorSetY(XMVector3Rotate(XMVectorSet(1.f, 0.f, 0.f, 0.f), vDefaultQuaternion), 0.f);
	_vector vToPlayer = XMVectorSetY(m_pInteractionPlayer->Get_Transform()->Get_State(STATE::POSITION) - m_pTransformCom->Get_State(STATE::POSITION), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vDefaultLook)) < 1e-6f || XMVectorGetX(XMVector3LengthSq(vDefaultRight)) < 1e-6f || XMVectorGetX(XMVector3LengthSq(vToPlayer)) < 1e-6f)
		return;

	vDefaultLook = XMVector3Normalize(vDefaultLook);
	vDefaultRight = XMVector3Normalize(vDefaultRight);
	vToPlayer = XMVector3Normalize(vToPlayer);

	const _float fForwardDot = XMVectorGetX(XMVector3Dot(vDefaultLook, vToPlayer));
	const _float fRightDot = XMVectorGetX(XMVector3Dot(vDefaultRight, vToPlayer));
	const _float fMaxYaw = XMConvertToRadians(fabsf(m_fTalkMaxYawDegree));
	_float fTargetYaw = atan2f(fRightDot, fForwardDot);

	// 초기 정면에서 좌우 어느 쪽이든 NPC별 설정 각도까지만 회전한다.
	if (fTargetYaw < -fMaxYaw)
		fTargetYaw = -fMaxYaw;
	else if (fTargetYaw > fMaxYaw)
		fTargetYaw = fMaxYaw;

	const _vector vYawQuaternion = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), fTargetYaw);
	const _vector vTargetQuaternion = XMQuaternionNormalize(XMQuaternionMultiply(vDefaultQuaternion, vYawQuaternion));

	Begin_FacingInterpolation(vTargetQuaternion);
}

void CNPC::Restore_DefaultFacing()
{
	if (nullptr == m_pTransformCom)
		return;

	// 현재 회전부터 대화 시작 전 맵 배치 회전까지 부드럽게 복구한다.
	Begin_FacingInterpolation(XMQuaternionNormalize(XMLoadFloat4(&m_vDefaultQuaternion)));
}

void CNPC::Begin_FacingInterpolation(_fvector vTargetQuaternion)
{
	if (nullptr == m_pTransformCom)
		return;

	// 진행 중인 보간이 있어도 현재 보이는 회전을 새 시작점으로 사용한다.
	XMStoreFloat4(&m_vFacingStartQuaternion, XMQuaternionNormalize(m_pTransformCom->Get_Quat()));
	XMStoreFloat4(&m_vFacingTargetQuaternion, XMQuaternionNormalize(vTargetQuaternion));
	m_fFacingInterpolationTime = 0.f;
	m_bFacingInterpolating = true;
}

void CNPC::Update_FacingInterpolation(_float fTimeDelta)
{
	if (false == m_bFacingInterpolating || nullptr == m_pTransformCom)
		return;

	m_fFacingInterpolationTime += fTimeDelta;

	_float fRatio = m_fFacingInterpolationTime / FACING_INTERPOLATION_DURATION;

	if (fRatio >= 1.f)
	{
		fRatio = 1.f;
		m_bFacingInterpolating = false;
	}
	else if (fRatio < 0.f)
	{
		fRatio = 0.f;
	}

	// SmoothStep 비율로 시작과 끝의 급격한 회전 변화를 줄인다.
	const _float fSmoothRatio = fRatio * fRatio * (3.f - 2.f * fRatio);
	const _vector vInterpolatedQuaternion = XMQuaternionNormalize(XMQuaternionSlerp(XMLoadFloat4(&m_vFacingStartQuaternion), XMLoadFloat4(&m_vFacingTargetQuaternion), fSmoothRatio));

	m_pTransformCom->Set_Quat(vInterpolatedQuaternion);
}

string CNPC::Resolve_DialogueID() const
{
	// 대사 바뀌면 나중에 추가할 거
	// if (스토리 보스 처치 완료)
	//     return "NPC.Eugenie.Hotel.AfterBoss";

	// if (첫 대화)
	//     return "NPC.Eugenie.Hotel.First";

	return Resolve_DefaultDialogueID();
}

string CNPC::Resolve_DefaultDialogueID() const
{
	switch (m_eNPCType)
	{
	case NPC_TYPE::SOPHIA:
		return "NPC.Sophia.Hotel.Default";

	case NPC_TYPE::ANTONIA:
		return "NPC.Antonia.Hotel.Default";

	case NPC_TYPE::POLENDINA:
		return "NPC.Polendina.Hotel.Default";

	case NPC_TYPE::VENDOR:
		return "NPC.Vendor.Default";

	case NPC_TYPE::RIDDLEKING:
		return "NPC.RiddleKing.Default";

	case NPC_TYPE::EUGENIE:
		return "NPC.Eugenie.Hotel.Default";

	default:
		// 기본 대사가 등록되지 않은 NPC는 대화를 시작하지 않는다.
		return {};
	}
}

CNPC* CNPC::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CNPC* pInstance = new CNPC(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CNPC");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CNPC::Clone(void* pArg)
{
	CNPC* pInstance = new CNPC(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CNPC");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CNPC::Free()
{
	// 제거 시 남아 있는 NPC 상호작용 안내를 먼저 닫는다.
	Hide_InteractionPrompt();

	// EventManager가 Listener 생포인터를 보관하므로 먼저 구독을 해제한다.
	if (nullptr != m_pGameInstance)
		m_pGameInstance->UnSubscribe_AllEvent(this);

	m_bPlayerInTrigger = false;
	m_bInteractionPromptVisible = false;
	m_pInteractionPlayer = nullptr;
	m_bFacingInterpolating = false;

	__super::Free();
}
