#include "Inventory.h"
#include "GameInstance.h"
#include "Player.h"
#include "ItemTable.h"
#include "Equipment.h"
#include "PartSet.h"
#include "LegionArm_Manager.h"
#include "LegionArm.h"
#include "GrappleGun.h"

CInventory::CInventory(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CPlayer* pOwner)
    : CComponent{ pDevice, pContext }
	, m_pOwner{ pOwner }
{
}

HRESULT CInventory::Initialize_Prototype()
{
	if(FAILED(__super::Initialize_Prototype()))
		return E_FAIL;	

	if(m_pOwner == nullptr)
		return E_FAIL;

	m_pEquipment = m_pOwner->Get_EquipmentComponent();
	m_pLegionArmManager = m_pOwner->Get_LegionArmManager();
	m_pPartSet = m_pOwner->Get_PartSet();

	if (m_pEquipment == nullptr || m_pLegionArmManager == nullptr || m_pPartSet == nullptr)
	{
		MSG_BOX("CInventory::Initialize_Prototype : Owner's Component is nullptr");
		return E_FAIL;
	}

	return S_OK;
}

_bool CInventory::Acquire(_uint64 iItemID)
{
	if (iItemID == 0) return false;

	const ITEM_DATA* pItemData = CItemTable::Find(iItemID);
	if (pItemData == nullptr) return false;

	if (Is_Acquired(iItemID)) return false;

	m_AcquiredItems[ETOUI(pItemData->eType)].push_back(iItemID);
	Try_AutoEquip(iItemID, pItemData->eType);
	
	return true;
}

_bool CInventory::Is_Acquired(_uint64 iItemID) const
{
	const ITEM_DATA* pItemData = CItemTable::Find(iItemID);
	if (pItemData == nullptr) return false;

	const auto& ItemList = m_AcquiredItems[ETOUI(pItemData->eType)];
	auto iter = find(ItemList.begin(), ItemList.end(), iItemID);

	return iter != ItemList.end();
}

_bool CInventory::Equip(_uint64 iItemID, _uint iSlotIndex)
{
    if(Is_Acquired(iItemID) == false)
		return false;

	const ITEM_DATA* pItemData = CItemTable::Find(iItemID);
	if (pItemData == nullptr) return false;

	switch(pItemData->eType)
	{
		case ITEM_TYPE::WEAPON:
		{
			if (iSlotIndex >= WEAPON_SLOT_MAX) return false;
			
			//다른 슬롯에 꽂혀있는거면 패스 
			const _uint iOther = (0 == iSlotIndex) ? 1 : 0;

			if (Get_EquippedID(ITEM_TYPE::WEAPON, iOther) == iItemID)
				return false;

			if (m_pEquipment && SUCCEEDED(m_pEquipment->Set_Slot(iSlotIndex, iItemID)))
				return true;
			 
			return false;

		}
		case ITEM_TYPE::LEGION_ARM:
		{
			m_pLegionArmManager->Change_Arm(pItemData->LegionArmData.eArmType);
			//실제로 장착이 됐으면 true반환함
			return Get_EquippedID(ITEM_TYPE::LEGION_ARM) == iItemID;
		}
		case ITEM_TYPE::BODY_PART:
		{
			m_pPartSet->Equip(iItemID);
			return true;
		}
	}

	return false;
}

void CInventory::UnEquip_Part(PLAYER_PART eSlot)
{
	if (eSlot == PLAYER_PART::FACE || eSlot == PLAYER_PART::HAIR)
		return;

	m_pPartSet->UnEquip(eSlot);
}

_bool CInventory::Is_LegionArm_Upgraded(_uint64 iItemID) const
{
	const ITEM_DATA* pItemData = CItemTable::Find(iItemID);

	if (pItemData == nullptr || pItemData->eType != ITEM_TYPE::LEGION_ARM)
		return false;

	if (false == Is_Acquired(iItemID))
		return false;

	return m_pLegionArmManager->Is_Arm_Upgraded(pItemData->LegionArmData.eArmType);
}

_bool CInventory::Upgrade_LegionArm(_uint64 iItemID, _uint iUpgradeCost)
{
	const ITEM_DATA* pItemData = CItemTable::Find(iItemID);

	if (pItemData == nullptr || pItemData->eType != ITEM_TYPE::LEGION_ARM)
		return false;

	//  에르고 검사까지 추가
	if (false == Is_Acquired(iItemID) || Is_LegionArm_Upgraded(iItemID))
		return false;

	if (m_iErgo < iUpgradeCost)
		return false;

	if (!m_pLegionArmManager->Upgrade_Arm(pItemData->LegionArmData.eArmType))
		return false;

	m_iErgo -= iUpgradeCost;
	return true;
}

void CInventory::Unlock_Grapple_Mobility()
{
	CGrappleGun* pGun = m_pLegionArmManager->Get_GrappleGun();
	if(pGun)
		pGun->Unlock_Mobility();
}

_uint64 CInventory::Get_EquippedID(ITEM_TYPE eType, _uint iSlotIndex) const
{
	switch(eType)
	{
		case ITEM_TYPE::WEAPON:
		{
			if (iSlotIndex >= WEAPON_SLOT_MAX) return 0;
			return m_pEquipment->Get_EquippedItemID(iSlotIndex);
		}
		case ITEM_TYPE::LEGION_ARM:
		{
			if (m_pLegionArmManager->Get_CurrentArm() == nullptr) return 0;
			return m_pLegionArmManager->Get_CurrentArm()->Get_ItemID();
		}
		case ITEM_TYPE::BODY_PART:
		{
			MSG_BOX("CInventory : Body Parts는 Get_EquippedPartID로 찾으세요 ");
			return 0;
		}
	}
	return 0;
}

_uint64	CInventory::Get_EquippedPartID(PLAYER_PART eSlot) const
{
	return m_pPartSet->Get_EquippedPartID(eSlot);
}

_bool CInventory::Is_Equipped(_uint64 iItemID) const
{
	if (iItemID == 0) return false;

	const ITEM_DATA* pItemData = CItemTable::Find(iItemID);
	if (pItemData == nullptr) return false;

	switch(pItemData->eType)
	{
		case ITEM_TYPE::WEAPON:
		{
			for (_uint i = 0; i < WEAPON_SLOT_MAX; ++i)
			{
				if (Get_EquippedID(ITEM_TYPE::WEAPON, i) == iItemID)
					return true;
			}
			return false;
		}
		case ITEM_TYPE::LEGION_ARM:
		{
			return (Get_EquippedID(ITEM_TYPE::LEGION_ARM) == iItemID);
		}
		case ITEM_TYPE::BODY_PART:
		{
			return Get_EquippedPartID(pItemData->BodyPartData.ePartSlot) == iItemID;
		}
	}
	return false;
}

CBodyPart* CInventory::Get_EquippedPart(PLAYER_PART eSlot) const
{
	if (m_pPartSet == nullptr) return nullptr;
	return m_pPartSet->Get_EquippedPart(eSlot);
}

_bool CInventory::Buy(_uint64 iItemID)
{
	const ITEM_DATA* pItemData = CItemTable::Find(iItemID);
	if (pItemData == nullptr) return false;

	if (Is_Acquired(iItemID)) return false;
	if (Consume_Ergo(pItemData->iPrice) == false) return false;

	return Acquire(iItemID);
}

//무기의 자동 장착
void CInventory::Try_AutoEquip(_uint64 iItemID, ITEM_TYPE eType)
{
	if (ITEM_TYPE::WEAPON != eType) return;

	for(_uint i=0; i< WEAPON_SLOT_MAX; ++i)
	{
		//비어있으면 
		if (Get_EquippedID(ITEM_TYPE::WEAPON, i) == 0)
		{
			Equip(iItemID, i);
			return;
		}
	}
}

HRESULT CInventory::Get_ItemIDs(ITEM_TYPE eType, vector<_uint64>& OutVector) const
{
	if(eType == ITEM_TYPE::END)
		return E_FAIL;

	OutVector = m_AcquiredItems[ETOUI(eType)];
	return S_OK;
}

_bool CInventory::Consume_Ergo(_uint iErgo)
{
	if (m_iErgo < iErgo)
		return false;

	m_iErgo -= iErgo;
	return true;
}


CInventory* CInventory::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CPlayer* pOwner)
{
	CInventory* pInstance = new CInventory(pDevice, pContext, pOwner);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CInventory");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CInventory::Free()
{
	__super::Free();

	for (_uint i = 0; i < ETOUI(ITEM_TYPE::END); ++i)
		m_AcquiredItems[i].clear();
}
