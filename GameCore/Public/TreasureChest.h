#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(GameCore)

class CItem;

class GAMECORE_DLL CTreasureChest final : public CMapGameplayObject
{
public:
	enum class CHEST_STATE
	{
		CLOSED,
		OPENING,
		OPENED,
		END
	};

	typedef struct tagTreasureChestDesc final : public MAP_GAMEPLAY_OBJECT_DESC
	{
		// TreasureChest가 내부 Item을 함께 생성할 때 사용하는 정보다.
		_uint iItemPrototypeLevel = { UINT_MAX };
		_uint iItemLayerLevel = { UINT_MAX };
		_wstring strItemPrototypeTag = { TEXT("Prototype_GameObject_Item") };
		_wstring strItemLayerTag = { TEXT("Layer_MapObject") };
		_float3 vItemLocalOffset = { 0.f, 0.3f, 0.f };
		_float3 vItemScale = { 1.f, 1.f, 1.f };

		// 상자 내부 Lily가 E 입력 시 전달할 실제 보상 정보다.
		ITEM_TYPE eItemType = { ITEM_TYPE::ERGO };
		_wstring strItemName = { TEXT("Ergo") };
		_uint iItemCount = { 1 };
		_uint iErgoMinAmount = { 100 };
		_uint iErgoMaxAmount = { 500 };
	} TREASURE_CHEST_DESC;

private:
	CTreasureChest(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTreasureChest(const CTreasureChest& Prototype);
	virtual ~CTreasureChest() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

	virtual void Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition) override;

public:
	HRESULT Begin_Open();

	// 플레이어가 입력을 받은 뒤 상자 상호작용 가능 여부와 실행을 요청한다.
	virtual _bool Can_Interact(CGameObject* pPlayer) const override;
	virtual _bool Interact(CGameObject* pPlayer) override;
	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) override;
	virtual void On_LinkedTriggerEnd(CGameObject* pInstigator) override;

	CHEST_STATE Get_ChestState() const { return m_eState; }

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	void Ready_DefaultDesc(TREASURE_CHEST_DESC* pDesc);
	HRESULT Ready_Item(const TREASURE_CHEST_DESC& Desc);
	_float3 Calculate_ItemWorldPosition() const;
	void Show_InteractionPrompt(OBJECT_TYPE ePromptType);
	void Hide_InteractionPrompt();
	HRESULT Enter_Closed();
	HRESULT Enter_Opening();
	HRESULT Enter_Opened();

private:
	CItem* m_pItem = { nullptr };
	_float3 m_vItemLocalOffset = { 0.f, 0.6f, 0.f };
	_float3 m_vItemScale = { 1.f, 1.f, 1.f };
	CHEST_STATE m_eState = { CHEST_STATE::END };
	_bool m_bPlayerInTrigger = { false };
	OBJECT_TYPE m_eVisiblePromptType = { OBJECT_TYPE::END };

public:
	static CTreasureChest* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
