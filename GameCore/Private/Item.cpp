#include "Item.h"

#include "GameInstance.h"

CItem::CItem(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CItem::CItem(const CItem& Prototype)
	: CMapGameplayObject{ Prototype }
	, m_eItemType{ ITEM_TYPE::END }
	, m_strItemName{}
	, m_iItemCount{ 0 }
	, m_iErgoMinAmount{ 100 }
	, m_iErgoMaxAmount{ 500 }
	, m_bPickupEnabled{ false }
	, m_bPlayerInTrigger{ false }
	, m_bInteractionPromptVisible{ false }
	, m_bAcquired{ false }
{
}

HRESULT CItem::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CItem::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	ITEM_DESC Desc = *static_cast<ITEM_DESC*>(pArg);
	Ready_DefaultDesc(&Desc);
	m_eItemType = Desc.eItemType;
	m_strItemName = Desc.strItemName;
	m_iItemCount = Desc.iItemCount;
	m_iErgoMinAmount = Desc.iErgoMinAmount;
	m_iErgoMaxAmount = Desc.iErgoMaxAmount;
	m_bPickupEnabled = Desc.bPickupEnabled;

	if (FAILED(__super::Initialize(&Desc)))
		return E_FAIL;

	return S_OK;
}

void CItem::Late_Update(_float fTimeDelta)
{
	// 획득 프레임에는 부모의 NONBLEND 등록도 생략하고 Layer 제거만 진행한다.
	if (true == Is_InActive())
		return;

	__super::Late_Update(fTimeDelta);

	if (VISUAL_TYPE::NONE == m_eVisualType || nullptr == m_pModelCom || nullptr == m_pGameInstance)
		return;

	// Lily의 반투명 머테리얼을 위해 아이템만 BLEND2 패스에 추가 등록한다.
	// 부모가 등록한 NONBLEND 패스에서는 머테리얼 RenderGroup 검사로 실제 Draw가 생략된다.
	m_pGameInstance->Add_RenderGroup(RENDERID::BLEND2, this);
}

void CItem::Event_GetItems(const vector<GET_ITEM_EVENT_DESC>& Items)
{
	CGameInstance* pGameInstance = CGameInstance::Get_Instance();
	if (nullptr == pGameInstance)
		return;

	for (GET_ITEM_EVENT_DESC Desc : Items)
	{
		if (!Desc.strItemName.empty())
			Desc.iItemID = Hash_String(Desc.strItemName.c_str());

		pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::GET_ITEM, &Desc);
	}
}

void CItem::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator || false == m_bPickupEnabled || true == m_bAcquired)
		return;

	// 외부 Map Trigger의 PLAYER 필터가 상호작용 대상을 보장한다.
	m_bPlayerInTrigger = true;

	if (false == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(true, OBJECT_TYPE::ITEM);
		m_bInteractionPromptVisible = true;
	}
}

void CItem::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	// Trigger 밖에서는 E를 눌러도 아이템을 획득하지 않는다.
	m_bPlayerInTrigger = false;

	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, OBJECT_TYPE::ITEM);
		m_bInteractionPromptVisible = false;
	}
}

HRESULT CItem::Acquire()
{
	if (true == m_bAcquired)
		return S_OK;

	if (nullptr == m_pGameInstance || ITEM_TYPE::END == m_eItemType || m_strItemName.empty())
		return E_FAIL;

	GET_ITEM_EVENT_DESC EventDesc{};
	EventDesc.eItemType = m_eItemType;
	EventDesc.strItemName = m_strItemName;
	EventDesc.iItemCount = m_iItemCount;
	//ItemTable.h - ItemID 참고. 해당 이름으로 줘야 획득 가능함 
	EventDesc.iItemID = Hash_String(m_strItemName.c_str());

	// Ergo는 획득하는 순간 설정된 최소~최대 범위에서 실제 지급량을 한 번 확정한다.
	if (ITEM_TYPE::ERGO == m_eItemType)
		EventDesc.iItemCount = static_cast<_uint>(Random(static_cast<_int>(m_iErgoMinAmount), static_cast<_int>(m_iErgoMaxAmount)));

	// Event Manager는 동기 호출이므로 수신자는 필요한 값을 OnEvent 안에서 복사한다.
	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::GET_ITEM, &EventDesc);

	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, OBJECT_TYPE::ITEM);
		m_bInteractionPromptVisible = false;
	}

	m_bPlayerInTrigger = false;
	m_bPickupEnabled = false;
	m_bAcquired = true;

	// 획득한 Item을 가리키는 모든 외부 Trigger와 Player 후보군을 함께 제거한다.
	Deactivate_LinkedTriggers();
	Set_InActive();

	return S_OK;
}

void CItem::Enable_Pickup()
{
	if (true == m_bAcquired)
		return;

	m_bPickupEnabled = true;
}

void CItem::Disable_Pickup()
{
	m_bPickupEnabled = false;
	m_bPlayerInTrigger = false;
}

_bool CItem::Can_Interact(CGameObject* pPlayer) const
{
	// 획득되지 않은 활성 아이템만 외부 Map Trigger 안에서 상호작용할 수 있다.
	if (nullptr == pPlayer || nullptr == m_pGameInstance)
		return false;

	return false == m_bAcquired && true == m_bPickupEnabled && true == m_bPlayerInTrigger;
}

_bool CItem::Interact(CGameObject* pPlayer)
{
	if (false == Can_Interact(pPlayer))
		return false;

	// 플레이어 Update에서 호출되므로 외부 Trigger 비활성화와 후보군 정리를 안전하게 수행한다.
	return SUCCEEDED(Acquire());
}

void CItem::Update_MapGameplayObject(_float fTimeDelta)
{
	if (true == m_bAcquired || nullptr == m_pTransformCom)
		return;

	// Update는 입력을 처리하지 않고 Lily 모델 회전만 담당한다.
	m_pTransformCom->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), fTimeDelta);
}

void CItem::Ready_DefaultDesc(ITEM_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	// Lily는 Bone이 없는 Static 모델이며 기존 전용 머테리얼을 그대로 사용한다.
	pDesc->eVisualType = VISUAL_TYPE::STATIC;
	pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Item") : pDesc->strModelPrototypeTag;
	pDesc->bControllerEnabled = false;
	pDesc->bPlayDefault = false;
	pDesc->bApplyRootMotion = false;
	// 초당 30도, 12초에 한 바퀴 회전한다.
	pDesc->fRotationPerSec = XMConvertToRadians(15.f);

	// 아이템 상호작용은 외부 Map Trigger가 담당하므로 부모의 Static Rigidbody는 생성하지 않는다.
	pDesc->bUsePhysics = false;

	if (ITEM_TYPE::END == pDesc->eItemType)
		pDesc->eItemType = ITEM_TYPE::ERGO;

	if (pDesc->strItemName.empty())
	{
		switch (pDesc->eItemType)
		{
		case ITEM_TYPE::WEAPON:
			pDesc->strItemName = TEXT("Weapon");
			break;

		default:
			pDesc->strItemName = TEXT("Ergo");
			break;
		}
	}

	if (0 == pDesc->iItemCount)
		pDesc->iItemCount = 1;

	if (pDesc->iErgoMaxAmount < pDesc->iErgoMinAmount)
	{
		const _uint iPreviousMinAmount = pDesc->iErgoMinAmount;
		pDesc->iErgoMinAmount = pDesc->iErgoMaxAmount;
		pDesc->iErgoMaxAmount = iPreviousMinAmount;
	}
}

CItem* CItem::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CItem* pInstance = new CItem(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CItem");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CItem::Clone(void* pArg)
{
	CItem* pInstance = new CItem(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CItem");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CItem::Free()
{
	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, OBJECT_TYPE::ITEM);
		m_bInteractionPromptVisible = false;
	}

	__super::Free();
}
