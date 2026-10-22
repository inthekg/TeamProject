#pragma once
#include "AIObject.h"
#include "BT_Node.h"

NS_BEGIN(Engine)
class CDebugCapsule;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CMonster_Training final : public CAIObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CMonster_Training)
#endif // _DEBUG

public:
	typedef struct tagTrainingDesc : public CAIObject::AIOBJECT_DESC
	{
	}MONSTER_DESC;

private:
	CMonster_Training(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Training(const CMonster_Training& Prototype);
	virtual ~CMonster_Training() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

public:
	virtual void				On_Freeze(_bool bFreeze) override;
	virtual void				Action_Notify(_uint64 iHashKey) override;
	virtual void				Spawn() override;
	virtual void				Despawn() override;
	virtual void				On_Groggy(_bool bGroggy) override;
#ifdef _DEBUG
public:
	void						Rebuild_FistDebugCapsule();
	HRESULT						Set_FistSweepRadius(_float fRadius) { m_fFistSweepRadius = fRadius; return S_OK; }
	HRESULT						Set_FistSweepHalfHeight(_float fHalfHeight) { m_fFistSweepHalfHeight = fHalfHeight; return S_OK; }
#endif // _DEBUG

private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	virtual void				On_Perceive_Attacker(_bool bPreFatal) override;
	virtual _bool				Uses_Full_Hit_Reaction() const override;
	virtual void				Apply_ChargeHit(_float fDamage, _float fGroggy) override;
	HRESULT						Ready_FistBones();
	void						Drive_FistCollision(_float fTimeDelta);
	void						Sweep_RightFist();

private:
	_bool						m_bFront = { false };
	_int						m_iFistTopBoneIndex = { -1 };
	_int						m_iFistBotBoneIndex = { -1 };
	_float						m_fFistSweepRadius = { 0.2f };
	_float						m_fFistSweepHalfHeight = { 0.2f };
	_float						m_fFistDamage = { 400.f };

	_bool						m_bFistActive = { false };
	_bool						m_bPrevHitActive = { false };
	_bool						m_bFistPrevValid = { false };
	_float3						m_vFistPrevCenter = {};
	unordered_set<CGameObject*>	m_FistTargets;

#ifdef _DEBUG
	CDebugCapsule* m_pFistDebugCapsule = { nullptr };
	_float						m_fFistLastDbgRadius = { -1.f };
	_float						m_fFistLastDbgHalf = { -1.f };
#endif

public:
	static CMonster_Training* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;

};

NS_END