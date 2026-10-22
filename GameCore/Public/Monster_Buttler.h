#pragma once
#include "Monster_Melee.h"
#include "BT_Node.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CMonster_Buttler : public CMonster_Melee
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CMonster_Buttler)
#endif

public:
	typedef struct tagButtlerDesc : public CMonster_Melee::MELEE_DESC
	{
		_int		iPatternCount = { 3 };
		_int		iSpawnNumber = { 0 };
		_float		fFatalFront = { 1.2f };
		_float		fFatalBack = { 1.2f };
	} MONSTER_DESC;

protected:
	CMonster_Buttler(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Buttler(const CMonster_Buttler& Prototype);
	virtual ~CMonster_Buttler() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;

	virtual void				On_CheckDead() override;
	virtual void				Set_FatalMode(_bool bFatal) override;

private:
	virtual void				Apply_SpawnDesc(void* pArg) override;
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				On_Hit_Pre(const HitInfo& hitInfo) override;

	virtual HRESULT				Ready_AttackData() override;
	virtual const _wstring&		Get_KnockbackAction(_int iHitInfo) const override;
	virtual void				On_Mirror_Extra() override;
	virtual void				On_Spawn_Post() override;

	void						Update_PatternSelect(_float fTimeDelta);

private:
	_int						m_iSpawnNumber = { 0 };
	_int						m_iPatternCount = { 3 };

public:
	static CMonster_Buttler* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END
