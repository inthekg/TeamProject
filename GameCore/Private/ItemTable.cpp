#include "ItemTable.h"

vector<ITEM_DATA>				CItemTable::m_ItemList = {};
unordered_map<_uint64, _uint>	CItemTable::m_ItemIndices = {};

void CItemTable::Add(const ITEM_DATA& Data)
{
	if (0 == Data.iItemID || ITEM_TYPE::END == Data.eType) return;
	if (m_ItemIndices.find(Data.iItemID) != m_ItemIndices.end()) return;

	switch (Data.eType)
	{
		case ITEM_TYPE::WEAPON:
		{
			if(Data.strObjectProtoTag.empty() || Data.WeaponData.eWeaponType == WeaponType::End)
				return;
			break;
		}
		case ITEM_TYPE::LEGION_ARM:
		{
			if (Data.LegionArmData.eArmType == LegionArmType::End)
				return;
			break;
		}

		case ITEM_TYPE::BODY_PART:
		{
			if (Data.strProtoModelTag.empty() || Data.strObjectProtoTag.empty() ||  Data.BodyPartData.ePartSlot == PLAYER_PART::END)
				return;
			break;
		}
		default:
			return;
	}

	m_ItemIndices.emplace(Data.iItemID, static_cast<_uint>(m_ItemList.size()));
	m_ItemList.push_back(Data);
}


const ITEM_DATA* CItemTable::Find(_uint64 iItemID)
{
	auto iter = m_ItemIndices.find(iItemID);
	if (iter == m_ItemIndices.end())
		return nullptr;

	return &m_ItemList[iter->second];
}


const ITEM_DATA* CItemTable::Find_ByArmType(LegionArmType eArmType)
{
	if (eArmType == LegionArmType::End)
		return nullptr;

	for (const ITEM_DATA& Data : m_ItemList)
	{
		if (Data.eType != ITEM_TYPE::LEGION_ARM) continue;

		if (Data.LegionArmData.eArmType == eArmType)
			return &Data;
	}

	return nullptr;
}


// TODO : 시간되면 JSON으로 아이템 테이블을 만들어서 로드하도록 변경
HRESULT CItemTable::Load_ItemList()
{
	Clear();
#pragma region BODY_PARAT
	{
		ITEM_DATA Data{};
		Data.eType = ITEM_TYPE::BODY_PART;
		Data.strObjectProtoTag = L"Prototype_GameObject_BodyPart";

		Data.iItemID = ItemID::BODY_BASE;
		Data.strDisplayName = L"하얀 셔츠";
		Data.strDesc = L"편하게 입을 수 있는 평상복이다.\n누군가의 자상한 배려가 느껴진다.";
		Data.BodyPartData.ePartSlot = PLAYER_PART::BODY;
		Data.strProtoModelTag = L"Prototype_Component_Model_PlayerBody";
		Data.BodyPartData.bAlphaClip = false;
		Data.iPrice = 0; // TODO : 상점에서 팔 아이템은 가격 설정하기 
		Add(Data);

		Data.iItemID = ItemID::BODY_WHITE;
		Data.strDisplayName = L"하얀 여인의 사냥복";
		Data.strDesc = L"하얀 여인이 입었던 사냥복.\n노래를 잃은 여인은 인형들의 피로 무대를 꾸몄다.";
		Data.BodyPartData.ePartSlot = PLAYER_PART::BODY;
		Data.strProtoModelTag = L"Prototype_Component_Model_PlayerBody2";
		Data.BodyPartData.bAlphaClip = false;
		Data.iPrice = 1000;
		Add(Data);


		Data.iItemID = ItemID::FACE_BASE;
		Data.strDisplayName = L"얼굴";
		Data.strDesc = L"얼굴은 얼굴이야.";
		Data.BodyPartData.ePartSlot = PLAYER_PART::FACE;
		Data.strProtoModelTag = L"Prototype_Component_Model_Player_Facial";
		Data.BodyPartData.bAlphaClip = false;
		Add(Data);

		Data.iItemID = ItemID::HAIR_BASE;
		Data.strDisplayName = L"기본 머리";
		Data.BodyPartData.ePartSlot = PLAYER_PART::HAIR;
		Data.strProtoModelTag = L"Prototype_Component_Model_Player_Hair";
		Data.BodyPartData.bAlphaClip = true;
		Add(Data);

		Data.iItemID = ItemID::HELMET_ALCHEMIST;
		Data.strDisplayName = L"연금술사 모자";
		Data.strDesc = L"얼굴은 얼굴이야.";
		Data.BodyPartData.ePartSlot = PLAYER_PART::HELMET;
		Data.strProtoModelTag = L"Prototype_Component_Model_Player_Helmet_Alchemist";
		Data.BodyPartData.bAlphaClip = false;
		Add(Data);


		Data.iItemID = ItemID::HELMET_MADDONKEY;
		Data.strDisplayName = L"분노하는 동키 가면";
		Data.BodyPartData.ePartSlot = PLAYER_PART::HELMET;
		Data.strProtoModelTag = L"Prototype_Component_Model_Player_Helmet_MadDonkey";
		Data.BodyPartData.bAlphaClip = false;
		Add(Data);

		Data.iItemID = ItemID::HELMET_SANTA;
		Data.strDisplayName = L"산타 모자";
		Data.BodyPartData.ePartSlot = PLAYER_PART::HELMET;
		Data.strProtoModelTag = L"Prototype_Component_Model_Player_Helmet_Santa";
		Data.BodyPartData.bAlphaClip = false;
		Add(Data);


		Data.iItemID = ItemID::ACC_EYEPATCH;
		Data.strDisplayName = L"검은 안대";
		Data.BodyPartData.ePartSlot = PLAYER_PART::ACC;
		Data.strProtoModelTag = L"Prototype_Component_Model_Player_Acc_Eyepatch";
		Data.BodyPartData.bAlphaClip = false;
		Add(Data);

		Data.iItemID = ItemID::ACC_RUDOLF_NOSE;
		Data.strDisplayName = L"빨간 루돌프 코";
		Data.BodyPartData.ePartSlot = PLAYER_PART::ACC;
		Data.strProtoModelTag = L"Prototype_Component_Model_Player_Acc_Rudolf_Nose";
		Data.BodyPartData.bAlphaClip = false;
		Add(Data);
	}

#pragma endregion

#pragma region WEAPON
	{
		ITEM_DATA Data{};
		Data.eType = ITEM_TYPE::WEAPON;
		
		Data.iItemID = ItemID::WEAPON_SCISSOR;
		Data.strDisplayName = L"인간성의 증거";
		Data.WeaponData.eWeaponType = WeaponType::OneHand;
		Data.strObjectProtoTag = L"Prototype_Weapon_Scissor";
		Add(Data);


		Data.iItemID = ItemID::WEAPON_GREATSWORD;
		Data.strDisplayName = L"대검";
		Data.WeaponData.eWeaponType = WeaponType::TwoHand;
		Data.strObjectProtoTag = L"Prototype_Weapon_GreatSword";
		Add(Data);

	}
#pragma endregion

#pragma region REGION_ARM
	{
		ITEM_DATA Data{};
		Data.eType = ITEM_TYPE::LEGION_ARM;
		Data.LegionArmData.bIsUpgrade = false;

		Data.iItemID = ItemID::LEGION_BASE;
		Data.LegionArmData.eArmType = LegionArmType::ProtoArm;
		Data.strDisplayName = L"프로토타입 리전암";
		Add(Data);


		Data.iItemID = ItemID::LEGION_GRAPPLE;
		Data.LegionArmData.eArmType = LegionArmType::Grapple;
		Data.strDisplayName = L"퍼펫 스트링";
		Add(Data);

		Data.iItemID = ItemID::LEGION_CHARGEGUN;
		Data.LegionArmData.eArmType = LegionArmType::ChargeGun;
		Data.strDisplayName = L"캐논";
		Add(Data);

	}
#pragma endregion


	return S_OK;
}

void CItemTable::Clear()
{
	m_ItemList.clear();
	m_ItemIndices.clear();
}