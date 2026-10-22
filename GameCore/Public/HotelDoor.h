#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(Engine)

class CDynamicRigidBody;

NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CHotelDoor final : public CMapGameplayObject
{
public:
	enum class DOOR_STATE
	{
		CLOSE_IDLE,
		OPENING,
		OPEN_IDLE,
		END
	};

private:
	CHotelDoor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CHotelDoor(const CHotelDoor& Prototype);
	virtual ~CHotelDoor() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Physical_Update(_float fFixedTime) override;

public:
	HRESULT Begin_Open();
	void Deactivate_ForBossRetry();

	// 플레이어가 입력을 받은 뒤 문 상호작용 가능 여부와 실행을 요청한다.
	virtual _bool Can_Interact(CGameObject* pPlayer) const override;
	virtual _bool Interact(CGameObject* pPlayer) override;

	_bool Is_Opened() const { return m_eState == DOOR_STATE::OPEN_IDLE; }
	DOOR_STATE Get_DoorState() const { return m_eState; }
	void Set_PlayerInTrigger(_bool bPlayerInTrigger) { m_bPlayerInTrigger = bPlayerInTrigger; }

	virtual void				On_LinkedTriggerBegin(CGameObject* pInstigator)override;
	virtual void				On_LinkedTriggerEnd(CGameObject* pInstigator)override;
	virtual void				On_LinkedActivate() override;

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	void Ready_DefaultDesc(MAP_GAMEPLAY_OBJECT_DESC* pDesc);
	static INTERACTION_TYPE Resolve_InteractionType(const _wstring& strAnimFolderTag);
	HRESULT Ready_DynamicPhysics(const MAP_GAMEPLAY_OBJECT_DESC& Desc);
	void UnRegister_DynamicPhysics();
	HRESULT Enter_CloseIdle();
	HRESULT Enter_Open();
	HRESULT Enter_OpenIdle();

private:
	static constexpr _uint MAX_DOOR_RIGID_BODY_COUNT = 2;

	Engine::CDynamicRigidBody* m_pDoorRigidBodies[MAX_DOOR_RIGID_BODY_COUNT] = {};
	_uint m_iDoorRigidBodyCount = {};

	// Player에게 전달할 문 상호작용 종류를 Animation 폴더 기준으로 보관한다.
	INTERACTION_TYPE m_eInteractionType = { INTERACTION_TYPE::END };
	DOOR_STATE m_eState = { DOOR_STATE::END };
	_bool m_bPlayerInTrigger = { false };

	// 실제로 DOOR 상호작용 안내를 표시했는지 보관한다.
	_bool m_bInteractionPromptVisible = { false };


public:
	static CHotelDoor* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
