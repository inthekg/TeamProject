#pragma once
#include "MapGameplayObject.h"

NS_BEGIN(Engine)
class CTrigger;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CDropItem final : public CMapGameplayObject
{
public :
	typedef struct tagDropItemDesc : public MAP_GAMEPLAY_OBJECT_DESC
	{
		ITEM_TYPE	eItemType	= { ITEM_TYPE::ERGO };
		_wstring	strItemName = { TEXT("Ergo") };
		_uint		iItemCount	= { 1 };
		_uint		iErgoMinAmount = { 100 };
		_uint		iErgoMaxAmount = { 200 };
		_float3		vTriggerHalfExtents = { 2.f , 2.f, 2.f };
	} DROP_ITEM_DESC;

private :
	CDropItem(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CDropItem(const CDropItem& Prototype);
	virtual ~CDropItem() = default;

public :
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

public :
	_bool						Is_Acquired() const { return m_bAcquired; }
	ITEM_TYPE					Get_ItemType() const { return m_eItemType; }

public :
	HRESULT						Acquire();

	virtual _bool				Can_Interact(CGameObject* pPlayer) const override;
	virtual _bool				Interact(CGameObject* pPlayer) override;

	virtual void				On_LinkedTriggerBegin(CGameObject* pInstigator) override;
	virtual void				On_LinkedTriggerEnd(CGameObject* pInstigator) override;

private :
	virtual void				Update_MapGameplayObject(_float fTimeDelta) override;
	void						Ready_DefaultDesc(DROP_ITEM_DESC* pDesc);
	HRESULT						Ready_SelfTrigger(const DROP_ITEM_DESC& Desc);

private :
	CTrigger*					m_pSelfTrigger					= { nullptr };
	CGameObject*				m_pOverlappedInstigator			= { nullptr };

	ITEM_TYPE					m_eItemType						= { ITEM_TYPE::END };
	_wstring					m_strItemName					= { };
	_uint						m_iItemCount					= { 0 };
	_uint						m_iErgoMinAmount				= { 100 };
	_uint						m_iErgoMaxAmount				= { 200 };
	_bool						m_bPlayerInTrigger				= { false };
	_bool						m_bInteractionPromptVisible		= { false };
	_bool						m_bAcquired						= { false };
	_uint64						m_iItemID						= {};
public :
	static CDropItem*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*		Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END