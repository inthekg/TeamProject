#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CMapActivator final : public CMapGameplayObject
{
public:
	enum class ACTIVATOR_TYPE
	{
		ELEVATOR_CONTROLLER,
		LADDER_ACTIVATER,
		END
	};

	enum class ACTIVATOR_STATE
	{
		CLOSE_IDLE,
		OPENING,
		OPEN_IDLE,
		END
	};

private:
	CMapActivator(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ACTIVATOR_TYPE eActivatorType);
	CMapActivator(const CMapActivator& Prototype);
	virtual ~CMapActivator() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

public:
	// 플레이어가 입력을 받은 뒤 Activator 상호작용 가능 여부와 실행을 요청한다.
	virtual _bool Can_Interact(CGameObject* pPlayer) const override;
	virtual _bool Interact(CGameObject* pPlayer) override;

	HRESULT Begin_Open();
	HRESULT Reset_ForReuse();
	void Apply_AttachedInitialDisplacement(_fvector vDisplacement);
	void Apply_AttachedDisplacement(_fvector vDisplacement);

	ACTIVATOR_TYPE Get_ActivatorType() const { return m_eActivatorType; }
	ACTIVATOR_STATE Get_ActivatorState() const { return m_eState; }

	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) override;
	virtual void On_LinkedTriggerEnd(CGameObject* pInstigator) override;

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	void Ready_DefaultDesc(MAP_GAMEPLAY_OBJECT_DESC* pDesc);

	HRESULT Enter_CloseIdle();
	HRESULT Enter_Opening();
	HRESULT Enter_OpenIdle();

	_wstring Get_CloseParameter() const;
	_wstring Get_OpenParameter() const;
	_wstring Get_OpenIdleParameter() const;
	OBJECT_TYPE  Get_ObjUIType() const;

private:
	static constexpr _uint LADDER_ACTIVATER_ANCHOR_MESH_INDEX = 1;

	ACTIVATOR_TYPE m_eActivatorType = { ACTIVATOR_TYPE::END };
	ACTIVATOR_STATE m_eState = { ACTIVATOR_STATE::END };
	_uint m_iTargetSpawnID = { UINT_MAX };
	_float m_fInteractionHeight = { 0.f };
	_bool m_bPlayerInTrigger = { false };

	// 외부 또는 자체 트리거로 표시한 Activator 안내 상태다.
	_bool m_bInteractionPromptVisible = { false };

public:
	static CMapActivator* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ACTIVATOR_TYPE eActivatorType);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
