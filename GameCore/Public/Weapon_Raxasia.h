#pragma once
#include "Weapon_Monster.h"

NS_BEGIN(Engine)
#ifdef _DEBUG
class CDebugCapsule;
#endif
NS_END


NS_BEGIN(GameCore)

class GAMECORE_DLL CWeapon_Raxasia final : public CWeapon_Monster
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CWeapon_Raxasia)
#endif

public:
	typedef struct tagRaxasiaWeaponDesc : public CWeapon_Monster::WEAPON_DESC
	{
		string	strBladeTopBoneName = {};
		string	strBladeBottomBoneName = {};
		_float	fSweepRadius = { 0.3f };
		_float	fSweepHalfHeight = { 0.5f };
		_float	fGroundRayExtend = { 1.f };
		_bool	bPhase2 = { false };
	} RAXASIA_WEAPON_DESC;

protected:
	CWeapon_Raxasia(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CWeapon_Raxasia(const CWeapon_Raxasia& Prototype);
	virtual ~CWeapon_Raxasia() = default;

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

public:
	void					Set_Phase(_bool bPhase2);
	void					Set_MeshVisible(const _wstring& strMeshKey, _bool bVisible);

#ifdef _DEBUG
public:
	HRESULT					Set_SweepRadius(_float fAmount) { m_fSweepRadius = fAmount; return S_OK; }
	HRESULT					Set_SweepHeight(_float fHeight) { m_fSweepHalfHeight = fHeight; return S_OK; }
	HRESULT					Set_GroundRayExtend(_float fExtend) { m_fGroundRayExtend = fExtend; return S_OK; }
private:
	void					Rebuild_DebugCapsule();
#endif

public:
	HRESULT					Get_BladeMatrix(_matrix& OutTop, _matrix& OutBottom);
	_bool					Get_BladeTipWorld(_vector& OutPos);
	_bool					Get_BladeGrabSocket(_float4x4& OutMat);
	_float					Get_GroundRayExtend() const { return m_fGroundRayExtend; }
	vector<_int>			Find_MeshIndices(const _wstring& strMeshKey) const;

public:
	void					Start_Trail();
	void					Start_Elec_Trail();
	void					Stop_Trail();

private:
	typedef unordered_set<CGameObject*> COLLISION_TARGET;

private:
	_int					m_iBladeTopBoneIndex = { -1 };
	_int					m_iBladeBottomBoneIndex = { -1 };
	_float					m_fSweepRadius = { 0.3f };
	_float					m_fSweepHalfHeight = { 0.5f };
	_float					m_fGroundRayExtend = { 1.f };

	CEffect*				m_pTrail = { nullptr };
	string					m_strTrailTop = {};
	string					m_strTrailBot = {};

	COLLISION_TARGET		m_CollisionTargets;
	_bool					m_bCollisionActive = { false };
	_bool					m_bPrevCenterValid = { false };
	_float3					m_vPrevCenter = {};

	vector<_bool>			m_MeshVisibles = {};

#ifdef _DEBUG
	CDebugCapsule* m_pDebugCapsule = { nullptr };
	_float					m_fLastDbgRadius = { -1.f };
	_float					m_fLastDbgHalfHeight = { -1.f };
#endif

public:
	static CWeapon_Raxasia* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END
