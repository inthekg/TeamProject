#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(Engine)
class CDynamicRigidBody;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CMapTrap final : public CMapGameplayObject
{
public:
	enum class TRAP_STATE
	{
		IDLE,
		ACTIVE,
		END
	};

public:
	// 벽 트랩의 끝 피해 Trigger와 원통 본체 Blocking Capsule 설정이다.
	typedef struct tagMapTrapDesc : public MAP_GAMEPLAY_OBJECT_DESC
	{
		_float fDamage = { 10.f };
		_float fPushDistance = { 0.5f };
		_float fBlockingRadius = { 0.9f };
		_float fBlockingHalfHeight = { 2.44f };
		_float3 vBlockingLocalOffset = { 3.14f, 0.f, 0.f };
	} MAP_TRAP_DESC;

private:
	CMapTrap(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMapTrap(const CMapTrap& Prototype);
	virtual ~CMapTrap() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Physical_Update(_float fFixedTime) override;
	virtual void Begin_Overlap(CGameObject* pOther) override;

public:
	HRESULT Activate();

	void Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition) override;

	TRAP_STATE Get_TrapState() const { return m_eState; }

	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) override;
	virtual void On_LinkedTriggerEnd(CGameObject* pInstigator) override;
	virtual void On_LinkedActivate() override;

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	void Ready_DefaultDesc(MAP_TRAP_DESC* pDesc);
	HRESULT Ready_DynamicPhysics(const MAP_TRAP_DESC& Desc);
	void Apply_PendingPhysicsState();
	HRESULT Enter_Idle();
	HRESULT Enter_Active();

private:
	Engine::CDynamicRigidBody* m_pDynamicRigidBody = { nullptr };
	Engine::CDynamicRigidBody* m_pBlockingRigidBody = { nullptr };
	unordered_set<CGameObject*> m_HitTargets = {};

	_float m_fDamage = { 10.f };
	_float m_fPushDistance = { 0.5f };
	_float m_fReactivateTimer = { 0.f };

	TRAP_STATE m_eState = { TRAP_STATE::END };
	_bool m_bActiveAnimationEntered = { false };
	_bool m_bPhysicsActivatePending = { false };
	_bool m_bPhysicsDeactivatePending = { false };
	_bool m_bPlayerInsideActivationTrigger = { false };

	static constexpr _float REACTIVATE_DELAY = { 1.5f };

public:
	static CMapTrap* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
