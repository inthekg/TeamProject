#pragma once
#include "Core_Defines.h"
#include "PoolableObject.h"
#include <memory>
#include <float.h>

NS_BEGIN(Engine)
class CModel;
class CEffect;
class CTransform;

#ifdef _DEBUG
class CDebugCapsule;
#endif

NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CProjectile abstract : public CPoolableObject
{
public:
	enum class MODE { BULLET, FAN, SHOT, GROUND_WAVE, END };

	typedef struct tagProjectileFireDesc
	{
		MODE			eMode = { MODE::BULLET };
		_float3			vOrigin = {};
		_float3			vDir = {};
		_float3			vScale = { 1.f, 1.f, 1.f };
		_float			fSpeed = { 40.f };
		_float			fAccel = { 0.f };
		_float			fRange = { 30.f };
		_float			fRadius = { 0.2f };
		_float			fFaceDist = { 3.f };
		_float			fReflectStraightTime = { 0.35f };
		_float			fHitAssistDist = { 0.6f };
		_float			fDamage = { 10.f };
		HIT_TYPE		eHitType = { HIT_TYPE::Heavy };
		CGameObject* pOwner = { nullptr };
		CGameObject* pTarget = { nullptr };
		shared_ptr<bool>	pVolleyHit = { nullptr };
		_bool			bNoLoopSound = { false };
	} FIRE_DESC;

	typedef struct tagTrailSpec
	{
		const char* pBoneA = { nullptr };
		const char* pBoneB = { nullptr };
		const char* pKey = { nullptr };
	} TRAIL_SPEC;

protected:
	CProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CProjectile(const CProjectile& Prototype);
	virtual ~CProjectile() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override PURE;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	virtual void			Activate(void* pArg = nullptr) override;
	virtual void			InActivate() override;
	void					On_Parry();

protected:
	virtual const _wstring& Get_ModelProtoTag() const;
	virtual void			On_ImpactFx(_fvector vPos);

	virtual MODE			Get_Mode() const PURE;
	virtual const char*		Get_LoopCue() const { return nullptr; }
	virtual const char*		Get_HitCue() const { return nullptr; }
	void					Apply_Mode(MODE eMode);

	virtual void			On_Explode(_fvector vPos);
	virtual void			On_Reflect();
	virtual void			Collect_Trails(vector<TRAIL_SPEC>& Out) const;

protected:

	void					Update_Standard(_float fTimeDelta);
	void					Update_Missile(_float fTimeDelta);
	void					Orient_ToDir();
	_vector					Get_HomePos(CGameObject* pHome);
	void					Home_Toward(_fvector vDesired, _float fRatio);
	void					Reflect();
	void					Spawn_Trails();
	void					Stop_Trails();

	void					Start_LoopSound();
	void					Stop_LoopSound();
	void					Update_LoopSound();
	void					Play_HitSound(const _float3& vWorldPos);

protected:
	CModel*					m_pModelCom = { nullptr };
	vector<CEffect*>		m_Trails;
	CGameObject*			m_pOwner = { nullptr };
	CGameObject*			m_pTarget = { nullptr };
	shared_ptr<bool>		m_pVolleyHit = { nullptr };

#ifdef _DEBUG
	CDebugCapsule*			m_pDbg = { nullptr };
#endif

	MODE					m_eMode = { MODE::BULLET };
	_float3					m_vDir = {};
	_float3					m_vScale = { 1.f, 1.f, 1.f };
	_float					m_fSpeed = { 40.f };
	_float					m_fAccel = { 0.f };
	_float					m_fRemain = { 0.f };
	_float					m_fRadius = { 0.2f };
	_float					m_fDamage = { 10.f };
	HIT_TYPE				m_eHitType = { HIT_TYPE::Heavy };

	_bool					m_bGround = { false };
	_bool					m_bReflectMode = { false };
	_bool					m_bHoming = { false };
	_bool					m_bPierce = { false };
	_bool					m_bReflected = { false };
	_bool					m_bHitConsumed = { false };
	_bool					m_bParriedPending = { false };
	_float					m_fGroundY = { 0.f };
	_float					m_fHomeRatio = { 0.15f };
	_float					m_fFaceDist = { 3.f };
	_float					m_fReflectStraightTime = { 0.35f };
	_float					m_fReflectStraight = { 0.f };
	_float					m_fHitAssistDist = { 0.6f };
	_float					m_fReflectHomeRatio = { 0.15f };
	_float					m_fReflectRange = { 100.f };
	_float					m_fPrevHomeDist = { FLT_MAX };

	_bool					m_bHitSoundPlayed = { false };
	_uint					m_hLoopSound = { 0 };
	_bool					m_bNoLoopSound = { false };
	string					m_strLoopCue;
	string					m_strHitCue;

public:
	virtual CGameObject* Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END
