#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(GameCore)

// GET_ITEM 수신자가 인벤토리 반영과 획득 UI에 사용하는 동기 이벤트 데이터다.
typedef struct tagGetItemEventDesc final : public Engine::Event_Desc
{
	ITEM_TYPE eItemType = { ITEM_TYPE::END };
	_wstring strItemName = {};
	_uint iItemCount = { 0 };
	_uint64 iItemID = { 0 }; // ItemTable.h 참고 
} GET_ITEM_EVENT_DESC;

class GAMECORE_DLL CItem final : public CMapGameplayObject
{
public:
	typedef struct tagItemDesc final : public MAP_GAMEPLAY_OBJECT_DESC
	{
		// Lily 외형과 별개로 실제 지급할 보상을 선택한다.
		ITEM_TYPE eItemType = { ITEM_TYPE::ERGO };
		_wstring strItemName = { TEXT("Ergo") };
		_uint iItemCount = { 1 };
		_uint iErgoMinAmount = { 100 };
		_uint iErgoMaxAmount = { 500 };

		// 상자 내부 아이템은 false로 시작하고 상자 Open 완료 시 상호작용을 허용한다.
		_bool bPickupEnabled = { true };
	} ITEM_DESC;

private:
	CItem(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CItem(const CItem& Prototype);
	virtual ~CItem() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Late_Update(_float fTimeDelta) override;

public:
	static void Event_GetItems(const vector<GET_ITEM_EVENT_DESC>& Items);

public:
	// 플레이어가 입력을 받은 뒤 아이템 상호작용 가능 여부와 실행을 요청한다.
	virtual _bool Can_Interact(CGameObject* pPlayer) const override;
	virtual _bool Interact(CGameObject* pPlayer) override;

	HRESULT Acquire();
	void Enable_Pickup();
	void Disable_Pickup();

	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) override;
	virtual void On_LinkedTriggerEnd(CGameObject* pInstigator) override;

	_bool Is_Acquired() const { return m_bAcquired; }
	_bool Is_PickupEnabled() const { return m_bPickupEnabled; }
	ITEM_TYPE Get_ItemType() const { return m_eItemType; }
	const _wstring& Get_ItemName() const { return m_strItemName; }

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	void Ready_DefaultDesc(ITEM_DESC* pDesc);

private:
	ITEM_TYPE m_eItemType = { ITEM_TYPE::END };
	_wstring m_strItemName = {};
	_uint m_iItemCount = { 0 };
	_uint m_iErgoMinAmount = { 100 };
	_uint m_iErgoMaxAmount = { 500 };
	_bool m_bPickupEnabled = { false };
	_bool m_bPlayerInTrigger = { false };
	_bool m_bInteractionPromptVisible = { false };
	_bool m_bAcquired = { false };

public:
	static CItem* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
