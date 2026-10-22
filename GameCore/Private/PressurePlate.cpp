#include "PressurePlate.h"

#include "Core_Struct.h"
#include "GameInstance.h"
#include "Animator.h"

CPressurePlate::CPressurePlate(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CPressurePlate::CPressurePlate(const CPressurePlate& Prototype)
	: CMapGameplayObject{ Prototype }
{
}

HRESULT CPressurePlate::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CPressurePlate::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	PRESSURE_PLATE_DESC* pDesc = static_cast<PRESSURE_PLATE_DESC*>(pArg);

	Ready_DefaultDesc(pDesc);

	m_iTargetSpawnID = pDesc->iTargetSpawnID;
	m_fResetDelay = max(pDesc->fResetDelay, 0.f);

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	if (nullptr == Get_Animator())
		return E_FAIL;

	// Controller의 Entry State인 Trapfloor_Ready를 즉시 처리한다.
	// 첫 번째 Update 전에 Trigger가 들어와도 Active 요청이 소실되지 않는다.
	Get_Animator()->Update(0.f);

	if (false == Is_AnimInState(TEXT("Trapfloor_Ready")))
		return E_FAIL;

	m_eState = PLATE_STATE::READY;
	m_fResetTimer = 0.f;

	return S_OK;
}

void CPressurePlate::Ready_DefaultDesc(PRESSURE_PLATE_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	// Trapfloor 모델 및 Animation Controller 설정이다.
	pDesc->eVisualType = VISUAL_TYPE::SKINNED;

	if (true == pDesc->strModelPrototypeTag.empty())
		pDesc->strModelPrototypeTag = TEXT("Prototype_Component_Model_Trapfloor");

	if (true == pDesc->strAnimFolderTag.empty())
		pDesc->strAnimFolderTag = TEXT("Trapfloor");

	if (true == pDesc->strAnimTag.empty())
		pDesc->strAnimTag = TEXT("Trapfloor");

	pDesc->eAnimTimeUnit = ANIMATION_TIME_UNIT::MILLISEC;
	pDesc->bStaticAnimation = true;
	pDesc->bControllerEnabled = true;
	pDesc->bPlayDefault = false;
	pDesc->bApplyRootMotion = false;

	// 플레이어 감지는 외부 MapTrigger가 담당한다.
	pDesc->bUsePhysics = false;
}

void CPressurePlate::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	if (PLATE_STATE::READY != m_eState)
		return;

	Enter_Active();
}

void CPressurePlate::Update_MapGameplayObject(_float fTimeDelta)
{
	switch (m_eState)
	{
	case PLATE_STATE::ACTIVE:
		m_fResetTimer += fTimeDelta;

		if (m_fResetTimer >= m_fResetDelay)
			Begin_Return();

		break;

	case PLATE_STATE::RETURNING:
		// ActiveSequence가 끝나고 Controller가 Ready로 복귀했는지 확인한다.
		if (true == Is_AnimInState(TEXT("Trapfloor_Ready")))
			Finish_Return();

		break;

	default:
		break;
	}
}

void CPressurePlate::Enter_Active()
{
	if (PLATE_STATE::READY != m_eState)
		return;

	m_eState = PLATE_STATE::ACTIVE;
	m_fResetTimer = 0.f;

	// Press → DownIdle Sequence를 요청한다.
	Request_AnimState(TEXT("Trapfloor_Active"));

	if (nullptr == m_pGameInstance || UINT_MAX == m_iTargetSpawnID)
		return;

	MAP_GAMEPLAY_ACTIVATE_EVENT_DESC EventDesc{};
	EventDesc.iTargetSpawnID = m_iTargetSpawnID;

	// 기존에 정의되어 있지만 사용되지 않던 Trap 활성화 이벤트를 사용한다.
	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::TRAP_OPEN, &EventDesc);
}

void CPressurePlate::Begin_Return()
{
	if (PLATE_STATE::ACTIVE != m_eState)
		return;

	m_eState = PLATE_STATE::RETURNING;

	// DownIdle Loop를 종료하고 Return Step으로 이동한다.
	Request_AnimSequenceEnd();
}

void CPressurePlate::Finish_Return()
{
	m_eState = PLATE_STATE::READY;
	m_fResetTimer = 0.f;
}

CPressurePlate* CPressurePlate::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPressurePlate* pInstance = new CPressurePlate(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CPressurePlate");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CPressurePlate::Clone(void* pArg)
{
	CPressurePlate* pInstance = new CPressurePlate(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPressurePlate");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPressurePlate::Free()
{
	__super::Free();
}
