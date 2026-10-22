#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(GameCore)
class CPlayer;
class CEquipment;
class CLegionArm_Manager;
class CPartSet;
class CBodyPart;

// 실제 객체는 PartSet, Equipment, LegionArm Manager가 소유함 
// 인벤토리는 획득 / 장착한 아이템 목록을 관리함

class GAMECORE_DLL CInventory final : public CComponent
{
private:
	CInventory(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CPlayer* pOwner);
	virtual ~CInventory() = default;

	CInventory(const CInventory& Prototype) = delete;
	CInventory& operator=(const CInventory& rhs) = delete;

public:
	virtual HRESULT			Initialize_Prototype() override;

public:
	_bool					Acquire(_uint64 iItemID);
	//소유중인가 
	_bool					Is_Acquired(_uint64 iItemID) const;

	_bool					Equip(_uint64 iItemID, _uint iSlotIndex = 0);
	void					UnEquip_Part(PLAYER_PART eSlot);

public:
	_bool					Is_LegionArm_Upgraded(_uint64 iItemID) const;

	_bool					Upgrade_LegionArm(_uint64 iItemID, _uint iUpgradeCost = 0);
	void					Unlock_Grapple_Mobility();

public:
	// 현재 소유중인 목록 리스트
	HRESULT					Get_ItemIDs(ITEM_TYPE eType, vector<_uint64>& OutVector) const;

	//장착중인 아이템 ID 반환 (Weapon은 슬롯 인덱스 필요)
	_uint64					Get_EquippedID(ITEM_TYPE eType, _uint iSlotIndex = 0) const;
	//Player BodyPart Item ID 는 여기서 확인
	_uint64					Get_EquippedPartID(PLAYER_PART eSlot) const;
	_bool					Is_Equipped(_uint64 iItemID) const;
	CBodyPart*				Get_EquippedPart(PLAYER_PART eSlot) const;
public:
	// Ergo 
	_uint					Get_Ergo() const { return m_iErgo; }
	void					Add_Ergo(_uint iErgo) { m_iErgo += iErgo; }
	_bool					Consume_Ergo(_uint iErgo);

public:
	//상점에서 아이템 구매 시 호출
	_bool					Buy(_uint64 iItemID);
private:
	void					Try_AutoEquip(_uint64 iItemID, ITEM_TYPE eType);
private:
	static constexpr _uint	WEAPON_SLOT_MAX = { 2 };

private:
	CPlayer*				m_pOwner = { nullptr };
	CEquipment*				m_pEquipment = { nullptr };
	CLegionArm_Manager*		m_pLegionArmManager = { nullptr };
	CPartSet*				m_pPartSet = { nullptr };

private:
	//타입별 획득 리스트 
	vector<_uint64>			m_AcquiredItems[ETOUI(ITEM_TYPE::END)];
	_uint					m_iErgo = { 30000 };

public:
	static CInventory*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CPlayer* pOwner);
	virtual void			Free() override;
};


NS_END