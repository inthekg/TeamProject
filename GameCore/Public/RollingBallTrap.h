#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(Engine)

class CDynamicRigidBody;

NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CRollingBallTrap final : public CMapGameplayObject
{
public:
	enum class BALL_STATE
	{
		ROLLING,
		HIDDEN,
		END
	};

public:
	// 공의 물리, 경로, 반복 설정을 한곳에서 전달한다.
	typedef struct tagRollingBallDesc : public MAP_GAMEPLAY_OBJECT_DESC
	{
		// 시작 위치 이후 공이 순서대로 향할 월드 좌표다.
		vector<_float3> Waypoints = {};

		// 마지막 지점 도착 후 숨겨진 상태에서 이동할 월드 좌표다.
		_float3 vResetPosition = {};

		_float fRadius = 0.5f;
		_float fTriggerRadius = 0.65f;
		_float fMass = 50.f;

		_float fMoveSpeed = 6.f;
		_float fSteerGain = 4.f;
		_float fMaxSteerAcceleration = 12.f;
		_float fLookAheadDistance = 2.f;

		_float fWaypointRadius = 0.75f;
		_float fFinishRadius = 1.f;
		_float fHiddenTime = 1.f;

		_float fDamage = 10.f;

		// 경로 이탈과 정지 상태를 복구하기 위한 안전장치다.
		_float fKillY = -1000.f;
		_float fStuckSpeed = 0.1f;
		_float fStuckTime = 3.f;
	} ROLLING_BALL_DESC;

private:
	CRollingBallTrap(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CRollingBallTrap(const CRollingBallTrap& Prototype);
	virtual ~CRollingBallTrap() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Physical_Update(_float fFixedTime) override;

	virtual void Begin_Overlap(CGameObject* pOther) override;
	virtual void End_Overlap(CGameObject* pOther) override;

public:
	BALL_STATE Get_BallState() const { return m_eState; }

private:
	_bool Is_ValidDesc(const ROLLING_BALL_DESC& Desc) const;

	HRESULT Ready_DynamicPhysics(_float fRadius, _float fTriggerRadius, _float fMass);
	HRESULT Configure_PhysicsShapes();

	void Advance_Waypoint(_fvector vPosition);
	_bool Has_ReachedFinish(_fvector vPosition) const;
	_vector Compute_GuidePoint(_fvector vPosition) const;
	void Drive_Rolling();
	void Update_StuckState(_float fFixedTime);

	void Enter_Hidden();
	void Reset_Cycle();
	void Reset_PhysicsPose();
	void Set_Visible(_bool bVisible);

private:
	Engine::CDynamicRigidBody* m_pDynamicRigidBody = { nullptr };

	vector<_float3> m_Waypoints = {};
	unordered_set<CGameObject*> m_HitTargets = {};

	_float3 m_vResetPosition = {};
	_float4 m_vResetQuaternion = { 0.f, 0.f, 0.f, 1.f };

	_uint m_iWaypointIndex = {};

	_float m_fRadius = 0.5f;
	_float m_fTriggerRadius = 0.65f;
	_float m_fMass = 50.f;

	_float m_fMoveSpeed = 6.f;
	_float m_fSteerGain = 4.f;
	_float m_fMaxSteerAcceleration = 12.f;
	_float m_fLookAheadDistance = 2.f;

	_float m_fWaypointRadius = 0.75f;
	_float m_fFinishRadius = 1.f;
	_float m_fHiddenTime = 1.f;

	_float m_fDamage = 10.f;

	_float m_fKillY = -1000.f;
	_float m_fStuckSpeed = 0.1f;
	_float m_fStuckTime = 3.f;

	_float m_fHiddenTimer = {};
	_float m_fStuckTimer = {};

	BALL_STATE m_eState = { BALL_STATE::END };

public:
	static CRollingBallTrap* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END