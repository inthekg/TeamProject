#include "Equipment.h"
#include "Weapon.h"
#include "GameInstance.h"
#include "Transform.h"
#include "Player.h"
#include "ItemTable.h"
#include "Inventory.h"

CEquipment::CEquipment(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CPlayer* pOwner)
	: CComponent{ pDevice, pContext }, m_pOwner{ pOwner }
{
}
	
void CEquipment::Priority_Update(_float fTimeDelta)
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->Priority_Update(fTimeDelta);
}

void CEquipment::Update(_float fTimeDelta)
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon == nullptr) return;

	if (m_pAnimator) pCurWeapon->Drive_ActionWindows(m_pAnimator);
	pCurWeapon->Update(fTimeDelta);
	

}

void CEquipment::Late_Update(_float fTimeDelta)
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->Late_Update(fTimeDelta);
}

void CEquipment::Execute_FableArts(_uint iIndex)
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->Execute_FableArts(iIndex);
}

void CEquipment::On_Collision(AttackType eType, _uint iStep, PIECE_TYPE ePieceType)
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->On_Collision(eType, iStep, ePieceType);
}

void CEquipment::Off_Collision(PIECE_TYPE ePieceType)
{
	//Collision Disable
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->Off_Collision(ePieceType);

}

void CEquipment::Set_AttackContext(AttackType eType, _int iStep)
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->Set_AttackContext(eType, iStep);
}

void CEquipment::Set_Split(_bool bSplit)
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->Set_Split(bSplit);
}

void CEquipment::Set_Visibility(_bool bVisibility)
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->Set_Visibility(bVisibility);
}

void CEquipment::Set_Dissolve(_float fAmount, const _float4& vColor)
{
	if (CWeapon* pWeapon = Get_CurrentWeapon())
		pWeapon->Set_Dissolve(fAmount, vColor);
}

CWeapon* CEquipment::Get_CurrentWeapon() const
{
	if (m_iCurSlotIndex < 0 || m_iCurSlotIndex >= WEAPON_SLOT_MAX) return nullptr;
	return m_pEquipped[m_iCurSlotIndex];
}

_uint CEquipment::Get_WeaponTypeID() const
{
	if (m_iCurSlotIndex < 0 || m_iCurSlotIndex >= WEAPON_SLOT_MAX) return 0;
	return m_pEquipped[m_iCurSlotIndex]->Get_WeaponTypeID();
}

WeaponType CEquipment::Get_WeaponType() const
{
	if (m_iCurSlotIndex < 0 || m_iCurSlotIndex >= WEAPON_SLOT_MAX) return WeaponType::UnArmed;
	return m_pEquipped[m_iCurSlotIndex]->Get_WeaponType();
}

WeaponType CEquipment::Get_WeaponType(_uint iIndex) const
{
	if (iIndex >= m_Weapons.size()) return WeaponType::UnArmed;
	else return m_Weapons[iIndex].pWeapon->Get_WeaponType();
}

const WeaponData* CEquipment::Get_WeaponData(_uint iIndex) const
{
	if (iIndex >= m_Weapons.size()) return nullptr;
	else return &m_Weapons[iIndex].pWeapon->Get_WeaponData();
}

const ActionData* CEquipment::Get_ActionData(AttackType eAttackType, _uint iStep) const
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) return pCurWeapon->Get_ActionData(eAttackType, iStep);
	return nullptr;
}

_float CEquipment::Get_Durability() const
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) return pCurWeapon->Get_Durability();
	return 0.f;
}

_uint64 CEquipment::Get_EquippedItemID(_uint iSlotIndex) const
{
	if (iSlotIndex >= WEAPON_SLOT_MAX || nullptr == m_pEquipped[iSlotIndex])
		return 0;

	return m_pEquipped[iSlotIndex]->Get_ItemID();
}

HRESULT CEquipment::Ready_Weapon(const string& strRightSocketName, const string& strLeftSocketName)
{
	for (const ITEM_DATA& Data : CItemTable::Get_ItemList())
	{
		if (ITEM_TYPE::WEAPON != Data.eType)
			continue;
		CEquipment::Equip_Desc EquipDesc{};
		EquipDesc.iLevelIndex = ETOUI(LEVEL::GAMEPLAY);
		EquipDesc.strWeaponProtoTag = Data.strObjectProtoTag;
		EquipDesc.strSocketBoneName = strRightSocketName;
		EquipDesc.iItemID = Data.iItemID;

		if (FAILED(Acquire_Weapon(EquipDesc, false)))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CEquipment::Acquire_Weapon(const Equip_Desc& tEquipDesc, _bool bEquip, _uint iSlot)
{
	if (m_pOwner == nullptr) return E_FAIL;
	if (Has_WeaponTag(tEquipDesc.strWeaponProtoTag)) return E_FAIL;

	CWeapon::WEAPON_DESC tWeaponDesc = {};
	tWeaponDesc.pOwner = m_pOwner;
	//tWeaponDesc.strModelProtoTag = tEquipDesc.strModelProtoTag;
	tWeaponDesc.iLevelIndex = tEquipDesc.iLevelIndex;
	tWeaponDesc.pParentMatrix = m_pOwner->Get_Transform()->Get_WorldMatrixPtr();
	tWeaponDesc.pSocketBoneMatrix = m_pOwner->Get_Bone_CombinedMatPtr(tEquipDesc.strSocketBoneName.c_str());

	CWeapon* pWeapon = m_pGameInstance->Clone_Prototype<CWeapon>(tEquipDesc.iLevelIndex, tEquipDesc.strWeaponProtoTag, &tWeaponDesc);
	if (pWeapon == nullptr)
	{
		MSG_BOX("CEquipment : Acquire_Weapon Failed");
		return E_FAIL;
	}

	pWeapon->Set_ItemID(tEquipDesc.iItemID);

	m_Weapons.emplace_back(tEquipDesc.strWeaponProtoTag, pWeapon);
	if (bEquip && iSlot < WEAPON_SLOT_MAX)
	{
		m_pEquipped[iSlot] = pWeapon;
		//장착된게 없으면 장착함
		if (m_iCurSlotIndex < 0) m_iCurSlotIndex = iSlot;
	}

	return S_OK;
}

HRESULT CEquipment::Set_Slot(_uint iSlotIndex, _uint64 iItemID)
{
	if (iSlotIndex >= WEAPON_SLOT_MAX || iItemID == 0) return E_FAIL;
	
	for(const Weapon_Slot& weaponSlot : m_Weapons)
	{
		if (weaponSlot.pWeapon && weaponSlot.pWeapon->Get_ItemID() == iItemID)
		{
			m_pEquipped[iSlotIndex] = weaponSlot.pWeapon;

			if (m_iCurSlotIndex < 0) m_iCurSlotIndex = static_cast<_int>(iSlotIndex);
			return S_OK;
		}
	}
	return E_FAIL;
}

HRESULT CEquipment::Set_Slot(_uint iSlotIndex, const _wstring& strWeaponProtoTag)
{
	if (iSlotIndex >= WEAPON_SLOT_MAX) return E_FAIL;
	_uint iIndex = Find_WeaponIndex(strWeaponProtoTag);
	if (iIndex == -1) return E_FAIL;

	m_pEquipped[iSlotIndex] = m_Weapons[iIndex].pWeapon;
	if (m_iCurSlotIndex < 0) m_iCurSlotIndex = static_cast<_int>(iSlotIndex);
	return S_OK;
}

void CEquipment::Regen_Durability(_float fAmountPerSec, _float fDeltaTime)
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();

	if (pCurWeapon)
		pCurWeapon->Regen_Durability(fAmountPerSec, fDeltaTime);
}

_bool CEquipment::Can_Swap() const
{
	if (m_iCurSlotIndex < 0 || m_iCurSlotIndex >= WEAPON_SLOT_MAX) return false;

	const _uint iNextIndex = (m_iCurSlotIndex == 0) ? 1 : 0;
	return m_pEquipped[iNextIndex] != nullptr;
}

void CEquipment::Swap_NextWeapon()
{
	if (m_iCurSlotIndex == -1) return;
	_uint iNextIndex = (m_iCurSlotIndex == 0) ? 1 : 0;
	if (m_pEquipped[iNextIndex] == nullptr) return;

	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->On_Unequipped();
		
	m_iCurSlotIndex = iNextIndex;
	Swap_Begin();
}

void CEquipment::Swap_Begin()
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon)
	{
		//혹시 꺼져있으면 켜주기 
		pCurWeapon->Set_Visibility(true);
		pCurWeapon->On_Swap_Begin();
	}
}

void CEquipment::Swap_End()
{
	CWeapon* pCurWeapon = Get_CurrentWeapon();
	if (pCurWeapon) pCurWeapon->On_Swap_End();
}

_bool CEquipment::Has_WeaponTag(const _wstring& strWeaponProtoTag) const
{
	_uint iIndex = Find_WeaponIndex(strWeaponProtoTag);
	if (iIndex == -1) return false;
	else return true;
}

_uint CEquipment::Find_WeaponIndex(const _wstring& strProtoTag) const
{
	for (_uint i = 0; i < m_Weapons.size(); i++)
	{
		if (m_Weapons[i].strWeaponProtoTag == strProtoTag) return i;
	}
	return -1;
}



CEquipment* CEquipment::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CPlayer* pOwner)
{
	CEquipment* pInstance = new CEquipment(pDevice, pContext, pOwner);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CEquipment");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CEquipment::Free()
{
	__super::Free();

	for (Weapon_Slot& weaponSlot : m_Weapons)
		Safe_Release(weaponSlot.pWeapon);	

}
