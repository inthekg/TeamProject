#pragma once
#include "Weapon_Monster.h"

NS_BEGIN(Engine)
class CEffect;
#ifdef _DEBUG
class CDebugCapsule;
#endif // _DEBUG
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CWeapon_Melee final : public CWeapon_Monster
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CWeapon_Melee)
#endif // _DEBUG
public:
	typedef struct tagMeleeDesc : public CWeapon_Monster::WEAPON_DESC
	{
		string strTrailTopBoneName = {};
		string strTrailBottomBoneName = {};
		string strTrailFxTag = { "Effect_Monster_Trail" };
		_float fSweepRadius = { 0.3f };
		_float fSweepHalfHeight = { 0.5f };
		_bool  bTrailUse = { true };
	} MELEE_DESC;

private:
	CWeapon_Melee(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CWeapon_Melee(const CWeapon_Melee& Prototype);
	virtual ~CWeapon_Melee() = default;

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

#ifdef _DEBUG
	HRESULT					Set_SweepRadius(_float fAmount) { m_fSweepRadius = fAmount; return S_OK; }
	HRESULT					Set_SweepHeight(_float fHeight) { m_fSweepHalfHeight = fHeight; return S_OK; }
	void					Rebuild_DebugCapsule();
#endif // _DEBUG

public:
	HRESULT					Get_TrailMatrix(_matrix& OutTop, _matrix& OutBottom);
	void					Start_Trail();
	void					Stop_Trail();

private:
	CEffect* m_pTrail = { nullptr };
	_bool					m_bTrailUse = { true };
	_int					m_iTrailTopBoneIndex = { -1 };
	_int					m_iTrailBottomBoneIndex = { -1 };
	_float					m_fSweepRadius = { 0.3f };
	_float					m_fSweepHalfHeight = { 0.5f };

	string					m_strTrailTop = {};
	string					m_strTrailBot = {};
	string					m_strTrailFx = { "Effect_Monster_Trail" };

	unordered_set<CGameObject*>	m_CollisionTargets;
	_bool					m_bCollisionActive = { false };
	_bool					m_bPrevCenterValid = { false };
	_float3					m_vPrevCenter = {};

#ifdef _DEBUG
	CDebugCapsule* m_pDebugCapsule = { nullptr };
	_float					m_fLastDbgRadius = { -1.f };
	_float					m_fLastDbgHalfHeight = { -1.f };
#endif

public:
	static CWeapon_Melee* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END
