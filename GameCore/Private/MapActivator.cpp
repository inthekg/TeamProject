#include "MapActivator.h"
#include "GameInstance.h"

CMapActivator::CMapActivator(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ACTIVATOR_TYPE eActivatorType)
	: CMapGameplayObject{ pDevice, pContext }
	, m_eActivatorType{ eActivatorType }
{
}

CMapActivator::CMapActivator(const CMapActivator& Prototype)
	: CMapGameplayObject{ Prototype }
	, m_eActivatorType{ Prototype.m_eActivatorType }
	, m_eState{ ACTIVATOR_STATE::END }
	, m_iTargetSpawnID{ UINT_MAX }
	, m_fInteractionHeight{ 0.f }
	, m_bPlayerInTrigger{ false }
	, m_bInteractionPromptVisible{ false }
{
}

HRESULT CMapActivator::Initialize_Prototype()
{
	if (ACTIVATOR_TYPE::END == m_eActivatorType)
		return E_FAIL;

	return S_OK;
}

HRESULT CMapActivator::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	if (ACTIVATOR_TYPE::END == m_eActivatorType)
		return E_FAIL;

	MAP_GAMEPLAY_OBJECT_DESC Desc = *static_cast<MAP_GAMEPLAY_OBJECT_DESC*>(pArg);

	m_iTargetSpawnID = Desc.iTargetSpawnID;
	Ready_DefaultDesc(&Desc);

	// Elevator Controller 상호작용은 외부 Trigger만 사용하고, LadderActivater는 기존 본체 Collider를 유지한다.
	if (ACTIVATOR_TYPE::ELEVATOR_CONTROLLER == m_eActivatorType)
		Desc.bUsePhysics = false;

	if (FAILED(__super::Initialize(&Desc)))
		return E_FAIL;

	if (FAILED(Enter_CloseIdle()))
		return E_FAIL;

	// 최초 Controller Entry를 미리 반영해 첫 E 입력의 Opening 요청이 제거되지 않도록 한다.
	Drive_Animator(0.f);

	return S_OK;
}

void CMapActivator::Ready_DefaultDesc(MAP_GAMEPLAY_OBJECT_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	switch (m_eActivatorType)
	{
	case ACTIVATOR_TYPE::ELEVATOR_CONTROLLER:
		// Elevator Controller 모델과 Controller를 설정한다.
		pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Elevator_Controller00") : pDesc->strModelPrototypeTag;
		pDesc->strAnimFolderTag = pDesc->strAnimFolderTag.empty() ? TEXT("Elevator_Controller00") : pDesc->strAnimFolderTag;
		pDesc->strAnimTag = pDesc->strAnimTag.empty() ? TEXT("Elevator_Controller00") : pDesc->strAnimTag;
		break;

	case ACTIVATOR_TYPE::LADDER_ACTIVATER:
		// Ladder Activater 모델과 Controller를 설정한다.
		pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_LadderActivater") : pDesc->strModelPrototypeTag;
		pDesc->strAnimFolderTag = pDesc->strAnimFolderTag.empty() ? TEXT("LadderActivater") : pDesc->strAnimFolderTag;
		pDesc->strAnimTag = pDesc->strAnimTag.empty() ? TEXT("LadderActivater") : pDesc->strAnimTag;
		break;

	default:
		return;
	}

	pDesc->bControllerEnabled = true;
	pDesc->bPlayDefault = false;
	pDesc->bApplyRootMotion = false;
	pDesc->eVisualType = VISUAL_TYPE::SKINNED;
}

_wstring CMapActivator::Get_CloseParameter() const
{
	switch (m_eActivatorType)
	{
	case ACTIVATOR_TYPE::ELEVATOR_CONTROLLER:
		return TEXT("Elevator_Controller_Idle");

	case ACTIVATOR_TYPE::LADDER_ACTIVATER:
		return TEXT("LadderActivater_CloseIdle");

	default:
		return TEXT("");
	}
}

_wstring CMapActivator::Get_OpenParameter() const
{
	switch (m_eActivatorType)
	{
	case ACTIVATOR_TYPE::ELEVATOR_CONTROLLER:
		return TEXT("Elevator_Controller_Open");

	case ACTIVATOR_TYPE::LADDER_ACTIVATER:
		return TEXT("LadderActivater_Open");

	default:
		return TEXT("");
	}
}

_wstring CMapActivator::Get_OpenIdleParameter() const
{
	switch (m_eActivatorType)
	{
	case ACTIVATOR_TYPE::ELEVATOR_CONTROLLER:
		return TEXT("Elevator_Controller_OpenIdle");

	case ACTIVATOR_TYPE::LADDER_ACTIVATER:
		return TEXT("LadderActivater_OpenIdle");

	default:
		return TEXT("");
	}
}

HRESULT CMapActivator::Begin_Open()
{
	if (ACTIVATOR_STATE::CLOSE_IDLE != m_eState)
		return S_OK;

	return Enter_Opening();
}

HRESULT CMapActivator::Reset_ForReuse()
{
	// 사다리 Activator는 일회성 기존 동작을 유지한다.
	if (ACTIVATOR_TYPE::ELEVATOR_CONTROLLER != m_eActivatorType)
		return S_OK;

	if (ACTIVATOR_STATE::CLOSE_IDLE != m_eState)
	{
		// 다음 E 입력을 받을 수 있도록 엘리베이터 컨트롤러를 초기 상태로 전환한다.
		if (FAILED(Enter_CloseIdle()))
			return E_FAIL;
	}

	// 이동이 끝난 뒤에도 플레이어가 외부 Trigger 안에 있으면 안내를 다시 표시한다.
	if (true == m_bPlayerInTrigger && false == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(true, Get_ObjUIType());
		m_bInteractionPromptVisible = true;
	}

	return S_OK;
}

HRESULT CMapActivator::Enter_CloseIdle()
{
	const _wstring strCloseParameter = Get_CloseParameter();
	const _wstring strOpenParameter = Get_OpenParameter();
	const _wstring strOpenIdleParameter = Get_OpenIdleParameter();

	if (strCloseParameter.empty() || strOpenParameter.empty() || strOpenIdleParameter.empty())
		return E_FAIL;

	m_eState = ACTIVATOR_STATE::CLOSE_IDLE;

	Set_AnimBool(strCloseParameter, true);
	Set_AnimBool(strOpenParameter, false);
	Set_AnimBool(strOpenIdleParameter, false);

	return S_OK;
}

HRESULT CMapActivator::Enter_Opening()
{
	const _wstring strCloseParameter = Get_CloseParameter();
	const _wstring strOpenParameter = Get_OpenParameter();
	const _wstring strOpenIdleParameter = Get_OpenIdleParameter();

	if (strCloseParameter.empty() || strOpenParameter.empty() || strOpenIdleParameter.empty())
		return E_FAIL;

	m_eState = ACTIVATOR_STATE::OPENING;

	Set_AnimBool(strCloseParameter, false);
	Set_AnimBool(strOpenIdleParameter, false);
	Set_AnimBool(strOpenParameter, true);
	Request_AnimState(strOpenParameter);

	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::ELEVATOR_OPEN, nullptr);

	return S_OK;
}

HRESULT CMapActivator::Enter_OpenIdle()
{
	const _wstring strCloseParameter = Get_CloseParameter();
	const _wstring strOpenParameter = Get_OpenParameter();
	const _wstring strOpenIdleParameter = Get_OpenIdleParameter();

	if (strCloseParameter.empty() || strOpenParameter.empty() || strOpenIdleParameter.empty())
		return E_FAIL;

	if (ACTIVATOR_TYPE::LADDER_ACTIVATER == m_eActivatorType)
	{
		if (false == Set_MeshVisible(LADDER_ACTIVATER_ANCHOR_MESH_INDEX, false))
			return E_FAIL;
	}

	m_eState = ACTIVATOR_STATE::OPEN_IDLE;

	Set_AnimBool(strOpenParameter, false);
	Set_AnimBool(strCloseParameter, false);
	Set_AnimBool(strOpenIdleParameter, true);

	return S_OK;
}

void CMapActivator::Update_MapGameplayObject(_float fTimeDelta)
{
	// Update는 Activator 열림 애니메이션 완료만 처리한다.
	if (ACTIVATOR_STATE::OPENING != m_eState || false == Is_AnimationFinished())
		return;

	if (FAILED(Enter_OpenIdle()))
		return;

	// Elevator는 Controller의 Open 애니메이션이 끝난 뒤 이동을 시작한다.
	if (ACTIVATOR_TYPE::ELEVATOR_CONTROLLER == m_eActivatorType && nullptr != m_pGameInstance && UINT_MAX != m_iTargetSpawnID)
	{
		MAP_GAMEPLAY_ACTIVATE_EVENT_DESC EventDesc{};
		EventDesc.iTargetSpawnID = m_iTargetSpawnID;
		EventDesc.fActivatorHeight = m_fInteractionHeight;

		m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::ELEVATOR_START, &EventDesc);
	}
}

_bool CMapActivator::Can_Interact(CGameObject* pPlayer) const
{
	// 닫힌 Activator가 유효한 연결 대상을 가진 경우에만 상호작용을 허용한다.
	if (nullptr == pPlayer || nullptr == m_pGameInstance)
		return false;

	return ACTIVATOR_STATE::CLOSE_IDLE == m_eState && true == m_bPlayerInTrigger && UINT_MAX != m_iTargetSpawnID;
}

_bool CMapActivator::Interact(CGameObject* pPlayer)
{
	if (false == Can_Interact(pPlayer))
		return false;

	if (ACTIVATOR_TYPE::ELEVATOR_CONTROLLER == m_eActivatorType)
	{
		const _float4x4* pPlayerWorld = pPlayer->Get_WorldTransform();

		// 상호작용한 플레이어의 현재 발 위치를 엘리베이터 이동 기준 높이로 저장한다.
		if (nullptr != pPlayerWorld)
			m_fInteractionHeight = pPlayerWorld->_42;
	}

	if (FAILED(Begin_Open()))
		return false;

	// Elevator는 애니메이션 완료 분기에서 이벤트를 전달한다.
	if (ACTIVATOR_TYPE::ELEVATOR_CONTROLLER == m_eActivatorType)
	{
		// 입력을 처리한 즉시 안내를 숨기고, 재사용 준비가 끝나면 다시 표시한다.
		if (true == m_bInteractionPromptVisible)
		{
			Broadcast_ObjUI(false, Get_ObjUIType());
			m_bInteractionPromptVisible = false;
		}

		return true;
	}

	// Ladder Activator는 사용 즉시 체류 상태를 끝내고 사다리를 연다.
	if (ACTIVATOR_TYPE::LADDER_ACTIVATER == m_eActivatorType)
	{
		// 한 번 사용한 액티베이터는 현재 체류 및 안내 상태를 즉시 종료한다.
		m_bPlayerInTrigger = false;

		if (true == m_bInteractionPromptVisible)
		{
			Broadcast_ObjUI(false, OBJECT_TYPE::LADDER_ACTIVATOR);
			m_bInteractionPromptVisible = false;
		}

		MAP_GAMEPLAY_ACTIVATE_EVENT_DESC EventDesc{};
		EventDesc.iTargetSpawnID = m_iTargetSpawnID;

		m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::LADDER_OPEN, &EventDesc);
	}

	return true;
}
void CMapActivator::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	// 이미 사용한 액티베이터는 외부 트리거가 다시 감지해도 무시한다.
	if (ACTIVATOR_STATE::CLOSE_IDLE != m_eState)
		return;

	// Linked Trigger의 PLAYER 필터가 상호작용 대상을 보장한다.
	if (nullptr == pInstigator)
		return;

	const _float4x4* pInstigatorWorld = pInstigator->Get_WorldTransform();

	if (nullptr != pInstigatorWorld)
		m_fInteractionHeight = pInstigatorWorld->_42;

	m_bPlayerInTrigger = true;

	if (false == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(true, Get_ObjUIType());
		m_bInteractionPromptVisible = true;
	}
}

void CMapActivator::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	// Linked Trigger의 PLAYER 필터가 상호작용 대상을 보장한다.
	if (nullptr == pInstigator)
		return;

	m_bPlayerInTrigger = false;

	if (false == m_bInteractionPromptVisible)
		return;

	Broadcast_ObjUI(false, Get_ObjUIType());
	m_bInteractionPromptVisible = false;
}

void CMapActivator::Apply_AttachedInitialDisplacement(_fvector vDisplacement)
{
	if (nullptr == m_pTransformCom || XMVectorGetX(XMVector3LengthSq(vDisplacement)) <= FLT_EPSILON)
		return;

	// Controller 모델만 Elevator 시작 끝점으로 옮기고 외부 Trigger Box는 고정 위치를 유지한다.
	_vector vNextPosition = m_pTransformCom->Get_State(STATE::POSITION) + vDisplacement;
	m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vNextPosition, 1.f));
}

void CMapActivator::Apply_AttachedDisplacement(_fvector vDisplacement)
{
	if (nullptr == m_pTransformCom)
		return;

	// 이동 중에도 Controller 모델만 발판을 따라가며 위/아래 외부 Trigger Box에는 영향을 주지 않는다.
	_vector vNextPosition = m_pTransformCom->Get_State(STATE::POSITION) + vDisplacement;
	m_pTransformCom->Set_State(STATE::POSITION, vNextPosition);
}

OBJECT_TYPE CMapActivator::Get_ObjUIType() const
{
	switch (m_eActivatorType)
	{
	case ACTIVATOR_TYPE::ELEVATOR_CONTROLLER:
		return OBJECT_TYPE::ELEVATOR_CONTROLLER;

	case ACTIVATOR_TYPE::LADDER_ACTIVATER:
		return OBJECT_TYPE::LADDER_ACTIVATOR;

	default:
		return OBJECT_TYPE::END;
	}
}

CMapActivator* CMapActivator::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ACTIVATOR_TYPE eActivatorType)
{
	if (ACTIVATOR_TYPE::END == eActivatorType)
		return nullptr;

	CMapActivator* pInstance = new CMapActivator(pDevice, pContext, eActivatorType);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMapActivator");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CMapActivator::Clone(void* pArg)
{
	CMapActivator* pInstance = new CMapActivator(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMapActivator");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMapActivator::Free()
{
	m_bPlayerInTrigger = false;

	// 트리거 종료 콜백 없이 언로드돼도 안내 UI가 남지 않게 한다.
	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, Get_ObjUIType());
		m_bInteractionPromptVisible = false;
	}

	__super::Free();
}
