#include "TreasureChest.h"

#include "GameInstance.h"
#include "Item.h"

CTreasureChest::CTreasureChest(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CTreasureChest::CTreasureChest(const CTreasureChest& Prototype)
	: CMapGameplayObject{ Prototype }
	, m_pItem{ nullptr }
	, m_vItemLocalOffset{ Prototype.m_vItemLocalOffset }
	, m_vItemScale{ Prototype.m_vItemScale }
	, m_eState{ CHEST_STATE::END }
	, m_bPlayerInTrigger{ false }
	, m_eVisiblePromptType{ OBJECT_TYPE::END }
{
}

HRESULT CTreasureChest::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CTreasureChest::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	TREASURE_CHEST_DESC Desc = *static_cast<TREASURE_CHEST_DESC*>(pArg);
	Ready_DefaultDesc(&Desc);
	m_vItemLocalOffset = Desc.vItemLocalOffset;
	m_vItemScale = Desc.vItemScale;

	if (FAILED(__super::Initialize(&Desc)))
		return E_FAIL;

	// TreasureChest 생성 시 내부 Item도 같은 위치 기준으로 함께 생성한다.
	if (FAILED(Ready_Item(Desc)))
		return E_FAIL;

	if (FAILED(Enter_Closed()))
		return E_FAIL;

	// 첫 프레임 전에 닫힘 포즈를 모델에 반영한다.
	Drive_Animator(0.f);

	return S_OK;
}

void CTreasureChest::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	// 하나의 외부 Trigger를 닫힌 상태에서는 상자, 열린 상태에서는 내부 Item 상호작용으로 사용한다.
	m_bPlayerInTrigger = true;

	if (CHEST_STATE::CLOSED == m_eState)
		Show_InteractionPrompt(OBJECT_TYPE::TREASURECHEST);
	else if (CHEST_STATE::OPENED == m_eState && nullptr != m_pItem && false == m_pItem->Is_Acquired())
		Show_InteractionPrompt(OBJECT_TYPE::ITEM);
}

void CTreasureChest::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	m_bPlayerInTrigger = false;
	Hide_InteractionPrompt();
}

void CTreasureChest::Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition)
{
	__super::Set_MapTransform(vScale, vRotationDegree, vPosition);

	if (nullptr != m_pItem)
		m_pItem->Set_MapTransform(m_vItemScale, vRotationDegree, Calculate_ItemWorldPosition());
}

HRESULT CTreasureChest::Begin_Open()
{
	if (CHEST_STATE::CLOSED != m_eState || nullptr == m_pItem)
		return E_FAIL;

	//TODO : 위치, 회전값 추가 
	m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::OPEN_CHEST, nullptr);
	return Enter_Opening();
}

_bool CTreasureChest::Can_Interact(CGameObject* pPlayer) const
{
	// 닫힌 상자 또는 열린 상자의 미획득 Item만 외부 Trigger 안에서 상호작용할 수 있다.
	if (nullptr == pPlayer || nullptr == m_pGameInstance)
		return false;

	if (false == m_bPlayerInTrigger || nullptr == m_pItem)
		return false;

	if (CHEST_STATE::CLOSED == m_eState)
		return true;

	return CHEST_STATE::OPENED == m_eState && false == m_pItem->Is_Acquired() && true == m_pItem->Is_PickupEnabled();
}

_bool CTreasureChest::Interact(CGameObject* pPlayer)
{
	if (false == Can_Interact(pPlayer))
		return false;

	if (CHEST_STATE::CLOSED == m_eState)
		return SUCCEEDED(Begin_Open());

	if (CHEST_STATE::OPENED == m_eState && SUCCEEDED(m_pItem->Acquire()))
	{
		Hide_InteractionPrompt();
		m_bPlayerInTrigger = false;

		// 내부 Item 획득 완료 시 상자를 가리키던 외부 Trigger도 함께 제거한다.
		Deactivate_LinkedTriggers();
		return true;
	}

	return false;
}

void CTreasureChest::Update_MapGameplayObject(_float fTimeDelta)
{
	// Update는 입력을 처리하지 않고 상자 애니메이션 상태만 진행한다.
	if (CHEST_STATE::OPENING == m_eState && true == Is_AnimationFinished())
		Enter_Opened();
}

void CTreasureChest::Ready_DefaultDesc(TREASURE_CHEST_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	pDesc->eVisualType = VISUAL_TYPE::SKINNED;
	pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_TreasureChest") : pDesc->strModelPrototypeTag;
	pDesc->strAnimFolderTag = pDesc->strAnimFolderTag.empty() ? TEXT("TreasureChest") : pDesc->strAnimFolderTag;
	pDesc->strAnimTag = pDesc->strAnimTag.empty() ? TEXT("TreasureChest") : pDesc->strAnimTag;
	pDesc->bControllerEnabled = false;
	pDesc->bPlayDefault = false;
	pDesc->bApplyRootMotion = false;
}

HRESULT CTreasureChest::Ready_Item(const TREASURE_CHEST_DESC& Desc)
{
	if (nullptr != m_pItem)
		return S_OK;

	if (nullptr == m_pGameInstance || UINT_MAX == Desc.iItemPrototypeLevel || UINT_MAX == Desc.iItemLayerLevel || Desc.strItemPrototypeTag.empty() || Desc.strItemLayerTag.empty())
		return E_FAIL;

	CItem::ITEM_DESC ItemDesc{};
	const _float3 vItemWorldPosition = Calculate_ItemWorldPosition();
	ItemDesc.iModelLevel = Desc.iItemPrototypeLevel;
	ItemDesc.vTranslate = _float4(vItemWorldPosition.x, vItemWorldPosition.y, vItemWorldPosition.z, 1.f);
	ItemDesc.vRotationDegree = Desc.vRotationDegree;
	ItemDesc.vScale = Desc.vItemScale;
	ItemDesc.eItemType = Desc.eItemType;
	ItemDesc.strItemName = Desc.strItemName;
	ItemDesc.iItemCount = Desc.iItemCount;
	ItemDesc.iErgoMinAmount = Desc.iErgoMinAmount;
	ItemDesc.iErgoMaxAmount = Desc.iErgoMaxAmount;
	// 상자가 열릴 때까지 내부 Item의 상호작용만 비활성화한다.
	ItemDesc.bPickupEnabled = false;

	m_pItem = m_pGameInstance->Clone_Prototype<CItem>(Desc.iItemPrototypeLevel, Desc.strItemPrototypeTag, &ItemDesc);

	if (nullptr == m_pItem)
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_GameObject(Desc.iItemLayerLevel, Desc.strItemLayerTag, m_pItem)))
	{
		Safe_Release(m_pItem);
		return E_FAIL;
	}

	// Layer 소유 참조와 별개로 TreasureChest가 내부 Item 참조를 보유한다.
	Safe_AddRef(m_pItem);
	return S_OK;
}

_float3 CTreasureChest::Calculate_ItemWorldPosition() const
{
	_float3 vItemWorldPosition = {};

	if (nullptr == m_pTransformCom)
		return vItemWorldPosition;

	_vector vWorldPosition = XMVector3TransformCoord(XMLoadFloat3(&m_vItemLocalOffset), XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr()));
	XMStoreFloat3(&vItemWorldPosition, vWorldPosition);
	return vItemWorldPosition;
}

HRESULT CTreasureChest::Enter_Closed()
{
	// TreasureChest_AnimClipSettings.json의 ExportName으로 닫힘 Clip을 재생한다.
	if (FAILED(Play_AnimClip(TEXT("Close_Idle"))))
		return E_FAIL;

	m_eState = CHEST_STATE::CLOSED;
	m_bPlayerInTrigger = false;
	return S_OK;
}

HRESULT CTreasureChest::Enter_Opening()
{
	// Open Clip의 마지막 Pose를 유지하고 완료 시 같은 외부 Trigger를 Item 용도로 전환한다.
	if (FAILED(Play_AnimClip(TEXT("Open"))))
		return E_FAIL;

	Hide_InteractionPrompt();
	m_eState = CHEST_STATE::OPENING;
	return S_OK;
}

HRESULT CTreasureChest::Enter_Opened()
{
	if (nullptr == m_pItem)
		return E_FAIL;

	m_pItem->Enable_Pickup();
	m_eState = CHEST_STATE::OPENED;

	// 상자 개봉 후 내부 Item 획득 시에는 Player 위치를 다시 보정하지 않는다.
	m_bUseInteractPosition = false;

	// 외부 Trigger 안에 계속 머문 플레이어에게 Item 안내를 즉시 전환한다.
	if (true == m_bPlayerInTrigger)
		Show_InteractionPrompt(OBJECT_TYPE::ITEM);

	return S_OK;
}

void CTreasureChest::Show_InteractionPrompt(OBJECT_TYPE ePromptType)
{
	if (OBJECT_TYPE::END == ePromptType || ePromptType == m_eVisiblePromptType)
		return;

	Hide_InteractionPrompt();
	Broadcast_ObjUI(true, ePromptType);
	m_eVisiblePromptType = ePromptType;
}

void CTreasureChest::Hide_InteractionPrompt()
{
	if (OBJECT_TYPE::END == m_eVisiblePromptType)
		return;

	Broadcast_ObjUI(false, m_eVisiblePromptType);
	m_eVisiblePromptType = OBJECT_TYPE::END;
}

CTreasureChest* CTreasureChest::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CTreasureChest* pInstance = new CTreasureChest(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CTreasureChest");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CTreasureChest::Clone(void* pArg)
{
	CTreasureChest* pInstance = new CTreasureChest(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTreasureChest");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CTreasureChest::Free()
{
	Hide_InteractionPrompt();

	if (nullptr != m_pItem)
		m_pItem->Set_InActive();

	Safe_Release(m_pItem);

	__super::Free();
}
