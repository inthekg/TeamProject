#pragma once
#include "Weapon_Monster.h"

NS_BEGIN(Engine)
class CDynamicRigidBody;
class CDebugCapsule;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CShield_Raxasia final : public CWeapon_Monster
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CShield_Raxasia)
#endif

public:
	typedef struct tagShieldDesc : public CWeapon_Monster::WEAPON_DESC
	{
		string	strBashTopBoneName = {};
		string	strBashBottomBoneName = {};
		_float	fSweepRadius = { 0.3f };
		_float	fSweepHalfHeight = { 0.5f };
		_float3	vColliderHalf = { 0.3f, 0.5f, 0.1f };
		_float	fShieldHP = { 100.f };
		_float	fBlockedRatio = { 0.2f };
	} SHIELD_DESC;

private:
	CShield_Raxasia(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CShield_Raxasia(const CShield_Raxasia& Prototype);
	virtual ~CShield_Raxasia() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	virtual void			Attack_Begin(_float fDamage = -1.f, HIT_TYPE eType = HIT_TYPE::Normal) override;
	virtual void			Attack_End() override;
	virtual void			Update_Attack(_float fTimeDelta) override;
	virtual _bool			Take_Damage(const HitInfo& hitInfo) override;

public:
	void					Set_Physics_InScene(_bool bIn);
	void					Set_Broke(_bool bBroke);
	void					Reset_Shield();
	_bool					Is_NoHp() const { return m_bBroken; }

#ifdef _DEBUG
	HRESULT					Set_SweepRadius(_float fAmount) { m_fSweepRadius = fAmount; return S_OK; }
	HRESULT					Set_SweepHeight(_float fHeight) { m_fSweepHalfHeight = fHeight; return S_OK; }
	HRESULT					Set_BlockedRatio(_float fRatio) { m_fBlockedRatio = fRatio; return S_OK; }
	HRESULT					Set_ColliderHalfX(_float fValue);
	HRESULT					Set_ColliderHalfY(_float fValue);
	HRESULT					Set_ColliderHalfZ(_float fValue);
#endif

private:
	HRESULT					Ready_RigidBody(const SHIELD_DESC& Desc);
	HRESULT					Get_BashMatrix(_matrix& OutTop, _matrix& OutBottom);
	vector<_int>			Find_MeshIndices(const _wstring& strMeshKey) const;
#ifdef _DEBUG
	void					Resize_Collider();
	void					Rebuild_DebugCapsule();
#endif

private:
	CDynamicRigidBody* m_pRigidBody = { nullptr };

	_int					m_iBashTopBoneIndex = { -1 };
	_int					m_iBashBottomBoneIndex = { -1 };
	_float					m_fSweepRadius = { 0.8f };
	_float					m_fSweepHalfHeight = { 0.5f };

	unordered_set<CGameObject*>	m_CollisionTargets;
	_bool					m_bCollisionActive = { false };
	_bool					m_bPrevCenterValid = { false };
	_float3					m_vPrevCenter = {};

	vector<_bool>			m_MeshVisibles = {};
	_bool					m_bBrokeMesh = { false };

	_float					m_fColHalfX = { 0.5f };
	_float					m_fColHalfY = { 0.3f };
	_float					m_fColHalfZ = { 0.8f };

	_float					m_fShieldHP = { 100.f };
	_float					m_fShieldMaxHP = { 100.f };
	_float					m_fBlockedRatio = { 0.2f };
	_bool					m_bBroken = { false };

#ifdef _DEBUG
	CDebugCapsule* m_pDebugCapsule = { nullptr };
	_float					m_fLastDbgRadius = { -1.f };
	_float					m_fLastDbgHalfHeight = { -1.f };
#endif

public:
	static CShield_Raxasia* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END
