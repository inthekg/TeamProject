#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CNeedleLauncher final : public CMapGameplayObject
{
public:
	enum class LAUNCHER_STATE
	{
		READY,
		FIRING,
		END
	};

public:
	// NeedleHole 발사대 설정이다.
	typedef struct tagNeedleLauncherDesc : public MAP_GAMEPLAY_OBJECT_DESC
	{
		// Projectile을 추가할 실제 Chapter Layer Level이다.
		LEVEL eLayerLevel = { LEVEL::GAMEPLAY };

		_uint iBurstCount = { 3 };

		// 1초마다 한 번씩 총 3회 발사한다.
		_float fShotInterval = { 1.f };
		_float fNeedleSpeed = { 30.f };
		_float fNeedleRange = { 20.f };
		_float fNeedleRadius = { 0.08f };
		_float fNeedleDamage = { 15.f };

		// 런처 그룹 중심을 기준으로 한 NeedleHole 모델 3개의 로컬 위치다.
		vector<_float3> HoleLocalOffsets =
		{
			_float3(-1.f, 0.f, 0.f),
			_float3(0.f, 0.f, 0.f),
			_float3(1.f, 0.f, 0.f)
		};

		// 각 NeedleHole 중심에서 로컬 +Z 방향으로 떨어진 화살 생성 거리다.
		_float fMuzzleForwardOffset = { 0.5f };
	} NEEDLE_LAUNCHER_DESC;

private:
	CNeedleLauncher(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNeedleLauncher(const CNeedleLauncher& Prototype);
	virtual ~CNeedleLauncher() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual HRESULT Render() override;
	virtual HRESULT Render_Shadow() override;

	virtual void On_LinkedActivate() override;

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	void Ready_DefaultDesc(NEEDLE_LAUNCHER_DESC* pDesc);
	HRESULT Render_LauncherGroup(_bool bCheckMaterialPass);
	void Fire_Needle();
	void Finish_Burst();

private:
	LAUNCHER_STATE m_eState = { LAUNCHER_STATE::END };

	LEVEL m_eLayerLevel = { LEVEL::GAMEPLAY };

	_uint m_iBurstCount = { 3 };
	_uint m_iFiredCount = { 0 };

	_float m_fShotInterval = { 1.f };
	_float m_fShotTimer = { 0.f };

	_float m_fNeedleSpeed = { 30.f };
	_float m_fNeedleRange = { 20.f };
	_float m_fNeedleRadius = { 0.08f };
	_float m_fNeedleDamage = { 15.f };

	// 모델 렌더링과 화살 생성이 함께 사용하는 발사구 중심 위치다.
	vector<_float3> m_HoleLocalOffsets = {};

	_float m_fMuzzleForwardOffset = { 0.1f };

public:
	static CNeedleLauncher* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
