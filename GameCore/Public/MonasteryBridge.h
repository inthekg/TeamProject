#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(Engine)
class CDynamicRigidBody;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CMonasteryBridge final : public CMapGameplayObject
{
public:
	enum class BRIDGE_STATE
	{
		READY,
		RISING,
		ARRIVED,
		END
	};

public:
	typedef struct tagMonasteryBridgeDesc : public MAP_GAMEPLAY_OBJECT_DESC
	{
		// Bridge는 월드 Y축으로 8만큼 5초 동안 상승한다.
		_float3 vMoveOffset = { 0.f, 8.f, 0.f };
		_float fMoveDuration = { 7.f };

		// Monastery_Bridge 모델 전체를 감싸는 단순 Box Collider다.
		_float3 vPlatformHalfExtents = { 15.f, 4.f, 8.f };
		_float3 vPlatformLocalOffset = { 15.f, 4.f, -6.f };
	} MONASTERY_BRIDGE_DESC;

private:
	CMonasteryBridge(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonasteryBridge(const CMonasteryBridge& Prototype);
	virtual ~CMonasteryBridge() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Physical_Update(_float fFixedTime) override;

	virtual void On_LinkedActivate() override;

	virtual void Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition) override;

public:
	HRESULT Start_Rising();
	BRIDGE_STATE Get_State() const { return m_eState; }

private:
	void Ready_DefaultDesc(MONASTERY_BRIDGE_DESC* pDesc);
	HRESULT Ready_DynamicPhysics(const MONASTERY_BRIDGE_DESC& Desc);
	void Update_MovePositions();
	void Finish_Rising();

private:
	Engine::CDynamicRigidBody* m_pDynamicRigidBody = { nullptr };

	_float3 m_vMoveOffset = {};
	_float3 m_vStartPosition = {};
	_float3 m_vEndPosition = {};
	_float3 m_vPhysicsPosition = {};

	_float m_fMoveDuration = { 7.f };

	BRIDGE_STATE m_eState = { BRIDGE_STATE::END };

public:
	static CMonasteryBridge* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
