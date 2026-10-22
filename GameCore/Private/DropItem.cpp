#include "DropItem.h"

#include "GameInstance.h"
#include "Trigger.h"
#include "Item.h"

CDropItem::CDropItem(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CDropItem::CDropItem(const CDropItem& Prototype)
	: CMapGameplayObject{ Prototype }
	, m_eItemType{ ITEM_TYPE::END }
	, m_strItemName{}
	, m_iItemCount{ 0 }
	, m_iErgoMinAmount{ 100 }
	, m_iErgoMaxAmount{ 200 }
	, m_bPlayerInTrigger{ false }
	, m_bInteractionPromptVisible{ false }
	, m_bAcquired{ false }
	, m_pSelfTrigger{ nullptr }
{
}

HRESULT CDropItem::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CDropItem::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	DROP_ITEM_DESC Desc = *static_cast<DROP_ITEM_DESC*>(pArg);
	Ready_DefaultDesc(&Desc);
	m_eItemType = Desc.eItemType;
	m_strItemName = Desc.strItemName;
	m_iItemCount = Desc.iItemCount;
	m_iErgoMinAmount = Desc.iErgoMinAmount;
	m_iErgoMaxAmount = Desc.iErgoMaxAmount;
	//주의 . Desc의 문자열이 ItemID와 완전일치해야함 
	m_iItemID = Hash_String(Desc.strItemName.c_str());

	if (FAILED(__super::Initialize(&Desc)))
		return E_FAIL;

	if (FAILED(Ready_SelfTrigger(Desc)))
		return E_FAIL;

	return S_OK;
}

void CDropItem::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CDropItem::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CDropItem::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	__super::Late_Update(fTimeDelta);

	if (VISUAL_TYPE::NONE == m_eVisualType || nullptr == m_pModelCom || nullptr == m_pGameInstance)
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::BLEND2, this);
}

HRESULT CDropItem::Render()
{
	return __super::Render();
}

void CDropItem::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator || true == m_bAcquired)
		return;

	m_bPlayerInTrigger = true;
	m_pOverlappedInstigator = pInstigator;

	if (false == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(true, OBJECT_TYPE::ITEM);
		m_bInteractionPromptVisible = true;
	}
}

void CDropItem::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	m_bPlayerInTrigger = false;

	if (pInstigator == m_pOverlappedInstigator)
		m_pOverlappedInstigator = nullptr;

	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, OBJECT_TYPE::ITEM);
		m_bInteractionPromptVisible = false;
	}
}

HRESULT CDropItem::Acquire()
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

	if (ITEM_TYPE::ERGO == m_eItemType)
		EventDesc.iItemCount = static_cast<_uint>(Random(static_cast<_int>(m_iErgoMinAmount), static_cast<_int>(m_iErgoMaxAmount)));

	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::GET_ITEM, &EventDesc);

	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, OBJECT_TYPE::ITEM);
		m_bInteractionPromptVisible = false;
	}

	m_bPlayerInTrigger = false;
	m_bAcquired = true;

	if (nullptr != m_pOverlappedInstigator)
	{
		m_pOverlappedInstigator->End_Overlap(this);
		m_pOverlappedInstigator = nullptr;
	}

	Deactivate_LinkedTriggers();
	Set_InActive();

	return S_OK;
}


_bool CDropItem::Can_Interact(CGameObject* pPlayer) const
{
	if (nullptr == pPlayer || nullptr == m_pGameInstance)
		return false;

	return false == m_bAcquired && true == m_bPlayerInTrigger;
}

_bool CDropItem::Interact(CGameObject* pPlayer)
{
	if (false == Can_Interact(pPlayer))
		return false;

	Acquire();
	return true;
}

void CDropItem::Update_MapGameplayObject(_float fTimeDelta)
{
	if (true == m_bAcquired || nullptr == m_pTransformCom)
		return;

	m_pTransformCom->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), fTimeDelta);
}

void CDropItem::Ready_DefaultDesc(DROP_ITEM_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	pDesc->eVisualType = VISUAL_TYPE::STATIC;
	pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Item") : pDesc->strModelPrototypeTag;
	pDesc->bControllerEnabled = false;
	pDesc->bPlayDefault = false;
	pDesc->bApplyRootMotion = false;
	pDesc->fRotationPerSec = XMConvertToRadians(15.f);
	pDesc->bUsePhysics = false;

	if (ITEM_TYPE::END == pDesc->eItemType)
		pDesc->eItemType = ITEM_TYPE::ERGO;

	if (pDesc->strItemName.empty())
		pDesc->strItemName = TEXT("Ergo");

	if (0 == pDesc->iItemCount)
		pDesc->iItemCount = 1;

	if (pDesc->iErgoMaxAmount < pDesc->iErgoMinAmount)
	{
		const _uint iPreviousMinAmount = pDesc->iErgoMinAmount;
		pDesc->iErgoMinAmount = pDesc->iErgoMaxAmount;
		pDesc->iErgoMaxAmount = iPreviousMinAmount;
	}
}

HRESULT CDropItem::Ready_SelfTrigger(const DROP_ITEM_DESC& Desc)
{
	if (nullptr == m_pGameInstance || nullptr == m_pTransformCom)
		return E_FAIL;

	_float3 vPosition;
	XMStoreFloat3(&vPosition, m_pTransformCom->Get_State(STATE::POSITION));

	Engine::CTrigger::TRIGGER_DESC TriggerDesc{};
	TriggerDesc.vTranslate = _float4(vPosition.x, vPosition.y, vPosition.z, 1.f);
	TriggerDesc.vHalfExtents = Desc.vTriggerHalfExtents;
	TriggerDesc.iTargetFilter = ETOUI(FILTER_GROUP::PLAYER);
	TriggerDesc.bOnceTrigger = false;
	TriggerDesc.pLogicalOwner = this;
	TriggerDesc.OnBeginOverlap = [this](Engine::CGameObject* pOther) { On_LinkedTriggerBegin(pOther); };
	TriggerDesc.OnEndOverlap = [this](Engine::CGameObject* pOther) { On_LinkedTriggerEnd(pOther); };

	m_pSelfTrigger = m_pGameInstance->Clone_Prototype<Engine::CTrigger>(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Trigger"), &TriggerDesc);
	if (nullptr == m_pSelfTrigger)
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_GameObject(static_cast<_uint>(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_Trigger"), m_pSelfTrigger)))
	{
		Safe_Release(m_pSelfTrigger);
		m_pSelfTrigger = nullptr;
		return E_FAIL;
	}

	Bind_LinkedTriggerDeactivate([this]()
		{
			if (nullptr != m_pSelfTrigger)
				m_pSelfTrigger->Set_InActive();
		});

	return S_OK;
}

CDropItem* CDropItem::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CDropItem* pInstance = new CDropItem(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CDropItem");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CDropItem::Clone(void* pArg)
{
	CDropItem* pInstance = new CDropItem(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CDropItem");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CDropItem::Free()
{
	if (true == m_bInteractionPromptVisible)
	{
		Broadcast_ObjUI(false, OBJECT_TYPE::ITEM);
		m_bInteractionPromptVisible = false;
	}

	m_pOverlappedInstigator = nullptr;
	m_pSelfTrigger = nullptr;

	__super::Free();
}
