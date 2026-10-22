#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(Engine)
class CDynamicRigidBody;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CCage final : public CMapGameplayObject
{
public:
	enum class CAGE_STATE
	{
		CLOSED,
		OPENING,
		OPENED,
		END
	};

private:
	CCage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCage(const CCage& Prototype);
	virtual ~CCage() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

public:
	// 외부 Trigger 진입 또는 다른 Map Object의 활성화 요청으로 Cage를 연다.
	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) override;
	virtual void On_LinkedActivate() override;

	HRESULT Open();
	CAGE_STATE Get_State() const { return m_eState; }

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	void Ready_DefaultDesc(MAP_GAMEPLAY_OBJECT_DESC* pDesc);
	HRESULT Ready_FloorPhysics(const MAP_GAMEPLAY_OBJECT_DESC& Desc);
	void Enter_Opened();

private:
	CAGE_STATE m_eState = { CAGE_STATE::END };
	_uint m_iTargetMonsterSpawnID = { UINT_MAX };
	Engine::CDynamicRigidBody* m_pCageDynamicRigidBody = { nullptr };

public:
	static CCage* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
