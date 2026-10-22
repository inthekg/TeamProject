#pragma once
#include "Monster_Melee.h"
#include "BT_Node.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CMonster_Elite final : public CMonster_Melee
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CMonster_Elite)
#endif

public:
	typedef struct tagEliteDesc : public CMonster_Melee::MELEE_DESC
	{
		_float		fFarAttackRange = { 7.f };
		_float		fRunChaseRange = { 6.f };
		_float		fFatalFront = { 1.2f };
		_float		fFatalBack = { 1.2f };
	} MONSTER_DESC;

private:
	CMonster_Elite(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Elite(const CMonster_Elite& Prototype);
	virtual ~CMonster_Elite() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;

	virtual void				On_CheckDead() override;
	virtual void				Set_FatalMode(_bool bMode) override;

private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				On_Hit_Post(const HitInfo& hitInfo) override;
	virtual _bool				Blocks_Damage_While_Groggy() const override;

	virtual HRESULT				Ready_AttackData() override;
	virtual _float				Get_AttackRange() const override;
	virtual _bool				Wants_RunChase(_float fDist) const override;
	virtual void				Drive_MoveToTarget(_float fTimeDelta) override;
	virtual const _wstring&		Get_KnockbackAction(_int iHitInfo) const override;

	void						Update_PatternSelect(_float fTimeDelta);
	_int						Select_LongPattern();

private:
	_float						m_fFarAttackRange = { 7.f };
	_float						m_fRunChaseRange = { 6.f };

public:
	static CMonster_Elite* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END
