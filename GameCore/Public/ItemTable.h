#pragma once
#include "Core_Defines.h"
#include "Engine_Utility.h"

NS_BEGIN(GameCore)

typedef struct tagItemWeaponData
{
	WeaponType				eWeaponType = { WeaponType::End };

} ITEM_WEAPON_DATA;

typedef struct tagItemLegionArmData
{
	LegionArmType			eArmType = { LegionArmType::End };
	_bool					bIsUpgrade = { false };
} ITEM_LEGION_DATA;


typedef struct tagItemBodyPartData
{
	PLAYER_PART				ePartSlot = { PLAYER_PART::END };
	_bool					bAlphaClip = { false };

} ITEM_BODYPART_DATA;



typedef struct tagItemData
{
	_uint64					iItemID = { 0 };
	_uint					iPrice = { 0 };
	ITEM_TYPE				eType = { ITEM_TYPE::END };

	_wstring				strObjectProtoTag = {};
	_wstring				strProtoModelTag = {};
	_wstring				strDisplayName = {};
	_wstring				strDesc = {};
	
	union
	{
		ITEM_WEAPON_DATA	WeaponData;
		ITEM_LEGION_DATA	LegionArmData;
		ITEM_BODYPART_DATA	BodyPartData;
		_uint64				_Raw; //어떤 데이터든 모두 0으로 초기화 가능하도록 함. 다른 구조체가 8바이트보다 커진다면 수정하기
	};

	//_Raw 초기화용 생성자
	tagItemData() : _Raw{ 0 } {}

} ITEM_DATA;

namespace ItemID
{
	constexpr _uint64 BODY_BASE				= Hash_String(L"Item_Body_BaseCloth");
	constexpr _uint64 BODY_WHITE			= Hash_String(L"Item_Body_WhiteCloth");
	
	constexpr _uint64 FACE_BASE				= Hash_String(L"Item_Face_Base");
	constexpr _uint64 HAIR_BASE				= Hash_String(L"Item_Hair_Base");

	constexpr _uint64 HELMET_MADDONKEY		= Hash_String(L"Item_Helmet_MadDonkey");
	constexpr _uint64 HELMET_SANTA			= Hash_String(L"Item_Helmet_Santa");
	constexpr _uint64 HELMET_ALCHEMIST		= Hash_String(L"Item_Helmet_Alchemist");

	constexpr _uint64 ACC_EYEPATCH			= Hash_String(L"Item_Acc_EyePatch");
	constexpr _uint64 ACC_RUDOLF_NOSE		= Hash_String(L"Item_Acc_Rudolf_Nose");

	constexpr _uint64 WEAPON_GREATSWORD		= Hash_String(L"Item_Weapon_GreatSword");
	constexpr _uint64 WEAPON_SCISSOR		= Hash_String(L"Item_Weapon_Scissor");

	constexpr _uint64 LEGION_BASE			= Hash_String(L"Item_Legion_Base");
	constexpr _uint64 LEGION_GRAPPLE		= Hash_String(L"Item_Legion_Grapple");
	constexpr _uint64 LEGION_CHARGEGUN		= Hash_String(L"Item_Legion_ChargeGun");
}


class GAMECORE_DLL CItemTable final
{
private:
	CItemTable() = default;
	~CItemTable() = default;

public:
	static HRESULT							Load_ItemList();
	static void								Clear();

	static const ITEM_DATA*					Find(_uint64 iItemID);
	static const ITEM_DATA*					Find_ByArmType(LegionArmType eArmType);

	static const vector<ITEM_DATA>&			Get_ItemList() { return m_ItemList; }

private:
	static void								Add(const ITEM_DATA& Data);

private:
	static vector<ITEM_DATA>				m_ItemList;
	//아이템 ID당 인덱스 번호 
	static unordered_map<_uint64, _uint>	m_ItemIndices;
};

NS_END