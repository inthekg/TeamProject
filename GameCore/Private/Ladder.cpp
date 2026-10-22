#include "Ladder.h"

#include "GameInstance.h"
#include "Model.h"
#include "Transform.h"

CLadder::CLadder(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CLadder::CLadder(const CLadder& Prototype)
	: CMapGameplayObject{ Prototype }
	, m_eLadderType{ LADDER_TYPE::END }
	, m_eState{ LADDER_STATE::END }
	, m_iBottomLandingBoneIndex{ -1 }
	, m_iTopLandingBoneIndex{ -1 }
	, m_iTopBoneIndex{ -1 }
	, m_bPlayerInTrigger{ false }
	, m_eVisiblePromptType{ OBJECT_TYPE::END }
	, m_bClimbRequested{ false }
{
}

HRESULT CLadder::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	// 사다리 종류는 Clone Descriptor에서 결정한다.
	return S_OK;
}

HRESULT CLadder::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	LADDER_DESC* pDesc = static_cast<LADDER_DESC*>(pArg);

	if (LADDER_TYPE::END == pDesc->eLadderType)
		return E_FAIL;

	m_eLadderType = pDesc->eLadderType;

	if (FAILED(Ready_DefaultDesc(pDesc)))
		return E_FAIL;

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	// Static과 Skinned 모두 동일한 이름의 모델 마커를 조회한다.
	if (FAILED(Ready_ClimbAnchors()))
		return E_FAIL;

	// Static 사다리는 개방 애니메이션 없이 즉시 등반 가능하다.
	if (VISUAL_TYPE::STATIC == Get_VisualType())
	{
		m_eState = LADDER_STATE::OPEN_IDLE;
		return S_OK;
	}

	if (VISUAL_TYPE::SKINNED != Get_VisualType())
		return E_FAIL;

	if (false == pDesc->bStartOpen)
	{
		if (FAILED(Enter_CloseIdle()))
			return E_FAIL;

		Drive_Animator(0.f);
		return S_OK;
	}

	// Controller Entry를 먼저 반영한 뒤 OpenIdle로 전환한다.
	Drive_Animator(0.f);

	if (FAILED(Enter_OpenIdle()))
		return E_FAIL;

	Drive_Animator(0.f);

	return S_OK;
}

void CLadder::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);

	if (false == m_bClimbRequested)
		return;

	// 요청은 한 번만 처리한다.
	m_bClimbRequested = false;

	if (LADDER_STATE::OPEN_IDLE != m_eState || false == m_bPlayerInTrigger)
		return;

	if (nullptr == m_pGameInstance)
		return;

	LADDER_EVENT_DESC EventDesc{};

	if (FAILED(Build_ClimbEventDesc(EventDesc)))
		return;

	// Broadcast는 동기 호출이므로 수신자는 이벤트 처리 중 필요한 값을 복사한다.
	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::LADDER, &EventDesc);
}

HRESULT CLadder::Ready_DefaultDesc(LADDER_DESC* pDesc)
{
	if (nullptr == pDesc)
		return E_FAIL;

	pDesc->iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	switch (m_eLadderType)
	{
	case LADDER_TYPE::STATIC:
		if (pDesc->strModelPrototypeTag.empty())
			return E_FAIL;

		pDesc->eVisualType = VISUAL_TYPE::STATIC;
		pDesc->bControllerEnabled = false;
		pDesc->bPlayDefault = false;
		pDesc->bApplyRootMotion = false;
		break;

	case LADDER_TYPE::METALWOOD:
		pDesc->eVisualType = VISUAL_TYPE::SKINNED;
		pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Ladder_MetalWood") : pDesc->strModelPrototypeTag;
		pDesc->strAnimFolderTag = pDesc->strAnimFolderTag.empty() ? TEXT("Ladder_MetalWood") : pDesc->strAnimFolderTag;
		pDesc->strAnimTag = pDesc->strAnimTag.empty() ? TEXT("Ladder_MetalWood") : pDesc->strAnimTag;
		pDesc->bStaticAnimation = true;
		pDesc->bControllerEnabled = true;
		pDesc->bPlayDefault = false;
		pDesc->bApplyRootMotion = false;
		break;

	case LADDER_TYPE::METALWOOD_6M:
		pDesc->eVisualType = VISUAL_TYPE::SKINNED;
		pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Ladder_MetalWood6M") : pDesc->strModelPrototypeTag;
		pDesc->strAnimFolderTag = pDesc->strAnimFolderTag.empty() ? TEXT("Ladder_MetalWood6M") : pDesc->strAnimFolderTag;
		pDesc->strAnimTag = pDesc->strAnimTag.empty() ? TEXT("Ladder_MetalWood6M") : pDesc->strAnimTag;
		pDesc->bStaticAnimation = true;
		pDesc->bControllerEnabled = true;
		pDesc->bPlayDefault = false;
		pDesc->bApplyRootMotion = false;
		break;

	default:
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLadder::Ready_ClimbAnchors()
{
	if (nullptr == m_pModelCom)
		return E_FAIL;

	// 모든 Static/Skinned 사다리 모델이 동일한 마커 이름을 사용한다.
	m_iBottomLandingBoneIndex = m_pModelCom->Get_BoneIndex("Ladder_BottomLanding");
	m_iTopLandingBoneIndex = m_pModelCom->Get_BoneIndex("Ladder_TopLanding");
	m_iTopBoneIndex = m_pModelCom->Get_BoneIndex("Ladder_Top");

	if (m_iBottomLandingBoneIndex < 0 || m_iTopLandingBoneIndex < 0 || m_iTopBoneIndex < 0)
		return E_FAIL;

	// Static 모델은 Animator가 없으므로 Bind Transform을 한 번 누적한다.
	if (VISUAL_TYPE::STATIC == Get_VisualType())
		m_pModelCom->Update_Bones();

	return S_OK;
}

HRESULT CLadder::Build_AnchorWorldMatrix(_int iBoneIndex, _float4x4& OutWorldMatrix) const
{
	if (iBoneIndex < 0 || nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return E_FAIL;

	const _float4x4* pBoneCombinedMatrix = m_pModelCom->Get_Bone_CombinedMatPtr(ETOUI(iBoneIndex));

	if (nullptr == pBoneCombinedMatrix)
		return E_FAIL;

	const _matrix BoneCombinedMatrix = XMLoadFloat4x4(pBoneCombinedMatrix);
	// 펼쳐지는 사다리는 발판 마커의 앞뒤가 트리거 배치와 반대이므로 로컬 Y축 기준으로 되돌린다.
	const _bool bReverseAnimatedAnchor = LADDER_TYPE::METALWOOD == m_eLadderType || LADDER_TYPE::METALWOOD_6M == m_eLadderType;
	const _matrix AnchorDirectionCorrectionMatrix = true == bReverseAnimatedAnchor ? XMMatrixRotationY(XM_PI) : XMMatrixIdentity();
	const _matrix ObjectWorldMatrix = XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
	const _matrix AnchorWorldMatrix = BoneCombinedMatrix * AnchorDirectionCorrectionMatrix * ObjectWorldMatrix;

	_vector vScale = {};
	_vector vRotation = {};
	_vector vTranslation = {};

	if (false == XMMatrixDecompose(&vScale, &vRotation, &vTranslation, AnchorWorldMatrix))
		return E_FAIL;

	// 플레이어 정렬 행렬에는 사다리의 Scale을 전달하지 않는다.
	_matrix ResultWorldMatrix = XMMatrixRotationQuaternion(XMQuaternionNormalize(vRotation));
	ResultWorldMatrix.r[3] = XMVectorSetW(vTranslation, 1.f);

	XMStoreFloat4x4(&OutWorldMatrix, ResultWorldMatrix);

	return S_OK;
}

HRESULT CLadder::Build_ClimbEventDesc(LADDER_EVENT_DESC& OutEventDesc) const
{
	_float4x4 BottomLandingWorldMatrix{};
	_float4x4 TopLandingWorldMatrix{};
	_float4x4 TopWorldMatrix{};

	// 아래 발판 중앙, 아래쪽 / 등반 시작 또는 등반 종료 위치 
	if (FAILED(Build_AnchorWorldMatrix(m_iBottomLandingBoneIndex, BottomLandingWorldMatrix)))
		return E_FAIL;

	// 위족 발판 중앙, 위쪽 / 하강 시작위치 또는 상승 후 끝나는 위치 
	if (FAILED(Build_AnchorWorldMatrix(m_iTopLandingBoneIndex, TopLandingWorldMatrix)))
		return E_FAIL;

	// 최상단 위치와 플레이어 정렬 방향 0
	if (FAILED(Build_AnchorWorldMatrix(m_iTopBoneIndex, TopWorldMatrix)))
		return E_FAIL;

	XMStoreFloat3(&OutEventDesc.vBottomLandingPosition, XMLoadFloat4x4(&BottomLandingWorldMatrix).r[3]);
	XMStoreFloat3(&OutEventDesc.vTopLandingPosition, XMLoadFloat4x4(&TopLandingWorldMatrix).r[3]);
	XMStoreFloat3(&OutEventDesc.vTopPosition, XMLoadFloat4x4(&TopWorldMatrix).r[3]);

	// Top 마커의 방향은 플레이어를 사다리에 정렬할 때 사용한다.
	OutEventDesc.TopWorldMatrix = TopWorldMatrix;

	return S_OK;
}

HRESULT CLadder::Begin_Open()
{
	// Static 사다리는 항상 열린 상태다.
	if (VISUAL_TYPE::STATIC == Get_VisualType())
		return S_OK;

	if (VISUAL_TYPE::SKINNED != Get_VisualType())
		return E_FAIL;

	if (LADDER_STATE::CLOSE_IDLE != m_eState)
		return S_OK;

	return Enter_Opening();
}

HRESULT CLadder::Enter_CloseIdle()
{
	if (VISUAL_TYPE::SKINNED != Get_VisualType())
		return E_FAIL;

	m_eState = LADDER_STATE::CLOSE_IDLE;

	// 닫힘 상태만 활성화한다.
	Set_AnimBool(TEXT("Ladder_CloseIdle"), true);
	Set_AnimBool(TEXT("Ladder_Open"), false);
	Set_AnimBool(TEXT("Ladder_OpenIdle"), false);

	return S_OK;
}

HRESULT CLadder::Enter_Opening()
{
	if (VISUAL_TYPE::SKINNED != Get_VisualType())
		return E_FAIL;

	m_eState = LADDER_STATE::OPEN;

	// Ladder_Open State를 요청한다.
	Set_AnimBool(TEXT("Ladder_CloseIdle"), false);
	Set_AnimBool(TEXT("Ladder_OpenIdle"), false);
	Set_AnimBool(TEXT("Ladder_Open"), true);
	Request_AnimState(TEXT("Ladder_Open"));

	return S_OK;
}

HRESULT CLadder::Enter_OpenIdle()
{
	if (VISUAL_TYPE::SKINNED != Get_VisualType())
		return E_FAIL;

	m_eState = LADDER_STATE::OPEN_IDLE;

	// 열림 애니메이션 종료 후 OpenIdle을 유지한다.
	Set_AnimBool(TEXT("Ladder_Open"), false);
	Set_AnimBool(TEXT("Ladder_CloseIdle"), false);
	Set_AnimBool(TEXT("Ladder_OpenIdle"), true);

	return S_OK;
}

void CLadder::Update_MapGameplayObject(_float fTimeDelta)
{
	// Update는 사다리 열림 애니메이션 상태만 진행한다.
	if (LADDER_STATE::OPEN == m_eState && true == Is_AnimationFinished())
	{
		if (FAILED(Enter_OpenIdle()))
			return;
	}

	// 상호작용 불가능한 상태로 바뀌면 대기 중인 등반 요청을 취소한다.
	if (LADDER_STATE::OPEN_IDLE != m_eState || false == m_bPlayerInTrigger)
		m_bClimbRequested = false;
}

_bool CLadder::Can_Interact(CGameObject* pPlayer) const
{
	// 열린 사다리의 Linked Trigger 안에 있는 플레이어만 등반할 수 있다.
	if (nullptr == pPlayer || nullptr == m_pGameInstance)
		return false;

	return LADDER_STATE::OPEN_IDLE == m_eState && true == m_bPlayerInTrigger;
}

_bool CLadder::Interact(CGameObject* pPlayer)
{
	if (false == Can_Interact(pPlayer))
		return false;

	// 최신 Animator Pose가 Model Bone에 반영된 뒤 이벤트를 보내도록 요청만 저장한다.
	m_bClimbRequested = true;
	return true;
}

void CLadder::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	const OBJECT_TYPE ePromptType = Resolve_InteractionPromptType(pInstigator);

	if (OBJECT_TYPE::END == ePromptType)
		return;

	m_bPlayerInTrigger = true;

	// 같은 안내가 이미 표시 중이면 중복 이벤트를 보내지 않는다.
	if (m_eVisiblePromptType == ePromptType)
		return;

	// 다른 방향 안내가 남아 있다면 새 안내보다 먼저 숨긴다.
	if (OBJECT_TYPE::END != m_eVisiblePromptType)
		Broadcast_ObjUI(false, m_eVisiblePromptType);

	m_eVisiblePromptType = ePromptType;
	Broadcast_ObjUI(true, m_eVisiblePromptType);
}

void CLadder::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	m_bPlayerInTrigger = false;
	m_bClimbRequested = false;

	// 진입할 때 실제로 표시했던 방향과 같은 타입으로 안내를 숨긴다.
	if (OBJECT_TYPE::END != m_eVisiblePromptType)
	{
		Broadcast_ObjUI(false, m_eVisiblePromptType);
		m_eVisiblePromptType = OBJECT_TYPE::END;
	}
}

OBJECT_TYPE CLadder::Resolve_InteractionPromptType(CGameObject* pInstigator) const
{
	if (nullptr == pInstigator)
		return OBJECT_TYPE::END;

	CTransform* pInstigatorTransform = pInstigator->Get_Transform();

	if (nullptr == pInstigatorTransform)
		return OBJECT_TYPE::END;

	_float4x4 BottomLandingWorldMatrix{};
	_float4x4 TopLandingWorldMatrix{};

	if (FAILED(Build_AnchorWorldMatrix(m_iBottomLandingBoneIndex, BottomLandingWorldMatrix)) || FAILED(Build_AnchorWorldMatrix(m_iTopLandingBoneIndex, TopLandingWorldMatrix)))
		return OBJECT_TYPE::END;

	const _vector vPlayerPosition = pInstigatorTransform->Get_State(STATE::POSITION);
	const _vector vBottomLandingPosition = XMLoadFloat4x4(&BottomLandingWorldMatrix).r[3];
	const _vector vTopLandingPosition = XMLoadFloat4x4(&TopLandingWorldMatrix).r[3];
	const _float fBottomDistanceSq = XMVectorGetX(XMVector3LengthSq(vPlayerPosition - vBottomLandingPosition));
	const _float fTopDistanceSq = XMVectorGetX(XMVector3LengthSq(vPlayerPosition - vTopLandingPosition));

	// 위쪽 발판에 가까우면 내려가기, 아래쪽 발판에 가까우면 올라가기다.
	return fTopDistanceSq < fBottomDistanceSq ? OBJECT_TYPE::LADDER_DOWN : OBJECT_TYPE::LADDER_UP;
}

void CLadder::On_LinkedActivate()
{
	// 연결된 LadderActivater가 작동했을 때 닫힌 사다리만 Opening으로 전환한다.
	Begin_Open();
}

HRESULT CLadder::Resolve_ActorType(const _wstring& strActorType, LADDER_DESC* pDesc)
{
	if (nullptr == pDesc || strActorType.empty())
		return E_FAIL;

	const _bool bStaticLadder = L"Ladder_Static_3_5m" == strActorType || L"Ladder_Static_5m" == strActorType || L"Ladder_Static_7m" == strActorType || L"Ladder_Static_7_5m" == strActorType;

	if (true == bStaticLadder)
	{
		pDesc->eLadderType = LADDER_TYPE::STATIC;
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_") + strActorType;
		return S_OK;
	}

	if (L"Ladder_MetalWood" == strActorType)
	{
		pDesc->eLadderType = LADDER_TYPE::METALWOOD;
		return S_OK;
	}

	if (L"Ladder_MetalWood6M" == strActorType)
	{
		pDesc->eLadderType = LADDER_TYPE::METALWOOD_6M;
		return S_OK;
	}

	return E_FAIL;
}

CLadder* CLadder::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLadder* pInstance = new CLadder(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CLadder");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CLadder::Clone(void* pArg)
{
	CLadder* pInstance = new CLadder(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CLadder");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CLadder::Free()
{
	// 트리거 종료 콜백 없이 언로드돼도 마지막 방향 안내가 남지 않게 한다.
	if (OBJECT_TYPE::END != m_eVisiblePromptType)
	{
		Broadcast_ObjUI(false, m_eVisiblePromptType);
		m_eVisiblePromptType = OBJECT_TYPE::END;
	}

	__super::Free();
}
