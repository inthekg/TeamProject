#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CAnimator;
NS_END

NS_BEGIN(GameCore)
class CPlayer;
class CWeapon;
class CEquipment final : public CComponent
{
public:
	//무기를 얻을 때 필요한 정보들
	typedef struct tagEquipWeaponDesc
	{
		_uint			iLevelIndex;			//Weapon, Model Level은 동일하다고 가정 
		_wstring		strWeaponProtoTag;
		_wstring		strModelProtoTag;
		string			strSocketBoneName;
		_uint64			iItemID = {0};
	} Equip_Desc;


	typedef struct tagWeaponSlot
	{
		_wstring		strWeaponProtoTag;
		CWeapon*		pWeapon = { nullptr };
		tagWeaponSlot(_wstring str, CWeapon* p) : strWeaponProtoTag(str), pWeapon(p) {}
	} Weapon_Slot;
private:
	enum { WEAPON_SLOT_MAX = 2 };

private:
	CEquipment(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CPlayer* pOwner);
	virtual ~CEquipment() = default;

	CEquipment(const CEquipment& Prototype) = delete;
	CEquipment& operator=(const CEquipment& rhs) = delete;

public:
	void					Priority_Update(_float fTimeDelta);
	void					Update(_float fTimeDelta);
	void					Late_Update(_float fTimeDelta);
	void					Execute_FableArts(_uint iIndex);

public:
	CWeapon*				Get_CurrentWeapon() const;

	_uint					Get_WeaponCount() const { return (_uint)m_Weapons.size(); }
	//인자 없으면 현재 장착중인 무기 데이터 
	_uint					Get_WeaponTypeID() const;
	WeaponType				Get_WeaponType() const;
	WeaponType				Get_WeaponType(_uint iIndex) const;
	const WeaponData*		Get_WeaponData(_uint iIndex) const;

	//현재 장착중인 무기 슬롯 인덱스 반환
	const ActionData*		Get_ActionData(AttackType eAttackType, _uint iStep) const;
	_float					Get_Durability() const;

	_uint64					Get_EquippedItemID(_uint iSlotIndex) const;
public:
	HRESULT					Ready_Weapon(const string& strRightSocketName, const string& strLeftSocketName);

public:
	void					On_Collision(AttackType eType, _uint iStep, PIECE_TYPE ePieceType = PIECE_TYPE::PIECE_MERGE);
	void					Off_Collision(PIECE_TYPE ePieceType = PIECE_TYPE::PIECE_MERGE);

public:
	void					Set_Animator(class CAnimator* pAnimator) { m_pAnimator = pAnimator; }
	void					Set_AttackContext(AttackType eType, _int iStep);

	void					Set_Split(_bool bSplit);
	void 					Set_Visibility(_bool bVisibility);
	void					Set_Dissolve(_float fAmount, const _float4& vColor);

public:
	HRESULT					Acquire_Weapon(const Equip_Desc& tEquipDesc, _bool bEquip = false, _uint iSlot = 0);
	//슬롯 장착 
	//HRESULT				Equip(_uint iSlotIndex);
	//void					UnEquip(_uint iSlotIndex);

	HRESULT					Set_Slot(_uint iSlotIndex, _uint64 iItemID);
	HRESULT					Set_Slot(_uint iSlotIndex, const _wstring& strWeaponProtoTag);
	void					Regen_Durability(_float fAmountPerSec, _float fDeltaTime);
	
	_bool					Can_Swap() const;

	void					Swap_End();
	void					Swap_NextWeapon();
	_bool					Has_WeaponTag(const _wstring& strWeaponProtoTag) const;
private:
	_uint					Find_WeaponIndex(const _wstring& strWeaponProtoTag) const;
	void					Swap_Begin();


private:
	CPlayer*				m_pOwner = { nullptr };
	
	vector<Weapon_Slot>		m_Weapons = {};
	CWeapon*				m_pEquipped[WEAPON_SLOT_MAX] = { nullptr };

	_int					m_iCurSlotIndex = { -1 };

	CAnimator*				m_pAnimator = { nullptr };

public:
	static CEquipment*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CPlayer* pOwner);
	virtual void			Free() override;
};

NS_END