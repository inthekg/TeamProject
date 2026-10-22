#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CMasterModel;
NS_END

NS_BEGIN(GameCore)
class CBodyPart;

/* Body Parts 전체 소유자 */
class GAMECORE_DLL CPartSet final : public CComponent
{
public:
	typedef struct tagPartSetDesc
	{
		CMasterModel* pMasterModel = { nullptr };
		const _float4x4* pParentMatrix = { nullptr };
	} PARTSET_DESC;

private:
	CPartSet(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CPartSet() = default;

	CPartSet(const CPartSet& Prototype) = delete;
	CPartSet& operator=(const CPartSet& rhs) = delete;

public:
	HRESULT					Initialize(const PARTSET_DESC& Desc);

public:
	_bool					Has_Part(_uint64 iItemID) const;
	_bool					Equip(_uint64 iItemID);
	void					UnEquip(PLAYER_PART eSlot);

	_uint64					Get_EquippedPartID(PLAYER_PART eSlot) const;
	CBodyPart*				Get_EquippedPart(PLAYER_PART eSlot) const;

public:
	HRESULT					Render();
	HRESULT					Render_Shadow();

	void					Set_Dissolve(_float fAmount, const _float4& vColor);

private:
	HRESULT					Ready_PartObjets();
private:
	CMasterModel*			m_pMasterModel = { nullptr };
	const _float4x4*		m_pParentMatrix = { nullptr };

private:
	//CBodyPart*				m_pEquippedParts[ETOUI(PLAYER_PART::END)] = { nullptr };
	//_uint64					m_iEquippedPartIDs[ETOUI(PLAYER_PART::END)] = { 0 };

	// 바디 파츠 전체 
	unordered_map<_uint64, CBodyPart*> m_OwnedParts = {};
	//파츠별 현재 장착무기의 아이디 
	_uint64					m_EquippedID[ETOUI(PLAYER_PART::END)] = { 0 };
public:
	static CPartSet*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const PARTSET_DESC& Desc);
	virtual void			Free() override;
};

NS_END

