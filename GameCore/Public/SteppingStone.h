#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(Engine)
class CDynamicRigidBody;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CSteppingStone final : public CMapGameplayObject
{
public:
	enum class STEPPING_STONE_STATE
	{
		READY,
		PRESSING,
		PRESSED,
		END
	};

public:
	typedef struct tagSteppingStoneDesc : public MAP_GAMEPLAY_OBJECT_DESC
	{
		// 기존 PressurePlate와 비슷한 속도로 Y축 1만큼 눌린다.
		_float3 vPressOffset = { 0.f, -0.1f, 0.f };
		_float fPressDuration = { 0.15f };

		// SteppingStone 본체의 단순 Box Collider다.
		_float3 vPlatformHalfExtents = { 1.f, 0.1f, 1.f };
		_float3 vPlatformLocalOffset = { 0.f, 0.1f, 0.f };

		// 기존 PressurePlate와 비슷한 크기의 플레이어 감지 Trigger다.
		_float3 vTriggerHalfExtents = { 0.5f, 0.5f, 0.5f };
		_float3 vTriggerLocalOffset = { 0.f, 0.f, 0.f };
	} STEPPING_STONE_DESC;

private:
	CSteppingStone(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CSteppingStone(const CSteppingStone& Prototype);
	virtual ~CSteppingStone() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Physical_Update(_float fFixedTime) override;

	virtual void Begin_Overlap(CGameObject* pOther) override;

	virtual void Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition) override;

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

public:
	STEPPING_STONE_STATE Get_State() const { return m_eState; }

private:
	void Ready_DefaultDesc(STEPPING_STONE_DESC* pDesc);
	HRESULT Ready_DynamicPhysics(const STEPPING_STONE_DESC& Desc);
	HRESULT Start_Pressing();
	void Update_MovePositions();
	void Finish_Pressing();

private:
	Engine::CDynamicRigidBody* m_pDynamicRigidBody = { nullptr };

	_uint m_iTargetSpawnID = { UINT_MAX };

	_float3 m_vPressOffset = {};
	_float3 m_vStartPosition = {};
	_float3 m_vPressedPosition = {};
	_float3 m_vPhysicsPosition = {};

	_float m_fPressDuration = { 0.15f };
	_bool m_bCutSceneRequestPending = { false };

	STEPPING_STONE_STATE m_eState = { STEPPING_STONE_STATE::END };

public:
	static CSteppingStone* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
