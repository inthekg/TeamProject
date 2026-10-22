#include "PartSet.h"
#include "GameInstance.h"
#include "BodyPart.h"
#include "ItemTable.h"

CPartSet::CPartSet(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{
}

HRESULT CPartSet::Initialize(const PARTSET_DESC& Desc)
{
	if(Desc.pMasterModel == nullptr || Desc.pParentMatrix == nullptr)
	{
		MSG_BOX("CPartSet::Initialize : Desc is invalid");
		return E_FAIL;
	}

	m_pMasterModel = Desc.pMasterModel;
	m_pParentMatrix = Desc.pParentMatrix;

	return Ready_PartObjets();
}


HRESULT CPartSet::Ready_PartObjets()
{
	for (const ITEM_DATA& Data : CItemTable::Get_ItemList())
	{
		if (Data.eType != ITEM_TYPE::BODY_PART) continue;

		CBodyPart::BODYPART_DESC PartDesc{};
		PartDesc.pMasterModel = m_pMasterModel;
		PartDesc.pParentMatrix = m_pParentMatrix;
		PartDesc.iLevelIndex = ETOUI(LEVEL::GAMEPLAY);
		PartDesc.strModelTag = Data.strProtoModelTag;
		PartDesc.bAlphaClip = Data.BodyPartData.bAlphaClip;

		CBodyPart* pPart = m_pGameInstance->Clone_Prototype<CBodyPart>(PartDesc.iLevelIndex, Data.strObjectProtoTag, &PartDesc);
		if (pPart == nullptr)
		{
			MSG_BOX("CPartSet::Ready_PartObjets : Failed to Clone_Prototype for BodyPart");
			return E_FAIL;
		}
		pPart->Set_ItemID(Data.iItemID);
		m_OwnedParts.emplace(Data.iItemID, pPart);
	}

	return S_OK;
}


_bool CPartSet::Has_Part(_uint64 iItemID) const
{
	return m_OwnedParts.find(iItemID) != m_OwnedParts.end();
}

_bool CPartSet::Equip(_uint64 iItemID)
{
	auto iter = m_OwnedParts.find(iItemID);
	if (iter == m_OwnedParts.end())  return false;

	const ITEM_DATA* pItemData = CItemTable::Find(iItemID);
	if(pItemData == nullptr || pItemData->eType != ITEM_TYPE::BODY_PART)
		return false;

	if (PLAYER_PART::END == pItemData->BodyPartData.ePartSlot)
		return false;

	const _uint iSlot = ETOUI(pItemData->BodyPartData.ePartSlot);
	
	m_EquippedID[ETOUI(pItemData->BodyPartData.ePartSlot)] = iItemID;

	return true;
}

void CPartSet::UnEquip(PLAYER_PART eSlot)
{
	if (eSlot == PLAYER_PART::END) return;

	m_EquippedID[ETOUI(eSlot)] = 0;
}


_uint64 CPartSet::Get_EquippedPartID(PLAYER_PART eSlot) const
{
	if (PLAYER_PART::END == eSlot)
		return 0;

	return m_EquippedID[ETOUI(eSlot)];
}

CBodyPart* CPartSet::Get_EquippedPart(PLAYER_PART eSlot) const
{
	if (PLAYER_PART::END == eSlot)
		return 0;

	const _uint64 iEquippedID = m_EquippedID[ETOUI(eSlot)];
	if (iEquippedID == 0) return nullptr;

	auto iter = m_OwnedParts.find(iEquippedID);
	return (iter != m_OwnedParts.end()) ? iter->second : nullptr;
}

HRESULT	CPartSet::Render()
{
	_bool bEquippedHelemet = (Get_EquippedPartID(PLAYER_PART::HELMET) != 0);

	for (_uint i = 0; i < ETOUI(PLAYER_PART::END); ++i)
	{
		if(PLAYER_PART::HAIR == static_cast<PLAYER_PART>(i) && bEquippedHelemet)
			continue;

		CBodyPart* pPart = Get_EquippedPart(static_cast<PLAYER_PART>(i));

		if (pPart)
			pPart->Render();
	}

	return S_OK;
}

HRESULT	CPartSet::Render_Shadow()
{
	for (_uint i = 0; i < ETOUI(PLAYER_PART::END); ++i)
	{
		CBodyPart* pPart = Get_EquippedPart(static_cast<PLAYER_PART>(i));

		if (pPart)
			pPart->Render_Shadow();
	}

	return S_OK;
}

void CPartSet::Set_Dissolve(_float fAmount, const _float4& vColor)
{
	for (_uint i = 0; i < ETOUI(PLAYER_PART::END); ++i)
	{
		CBodyPart* pPart = Get_EquippedPart(static_cast<PLAYER_PART>(i));
		if (pPart) pPart->Set_Dissolve(fAmount, vColor);
	};
}

CPartSet* CPartSet::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const PARTSET_DESC& Desc)
{
	CPartSet* pInstance = new CPartSet(pDevice, pContext);
	if(FAILED(pInstance->Initialize(Desc)))
	{
		MSG_BOX("CPartSet::Create : Failed to Initialize");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CPartSet::Free()
{
	__super::Free();
	for (auto& Pair : m_OwnedParts)
		Safe_Release(Pair.second);

	m_OwnedParts.clear();
}
