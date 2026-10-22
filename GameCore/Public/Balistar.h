#pragma once
#include "AIObject.h"

NS_BEGIN(GameCore)

class CShooter;

class GAMECORE_DLL CBalistar final : public CAIObject
{

#ifdef _DEBUG
	DECLARE_REFLECTION(CBalistar)
#endif

public:
	typedef struct tagBalistarDesc : public CAIObject::AIOBJECT_DESC
	{
		tagBalistarDesc()
		{
			fDetectRange = 20.f;
			fAttackRange = 20.f;
			fLeashRange = 30.f;
		}

		_wstring	strModelProtoTag = {};
		_wstring	strAnimFolderTag = { TEXT("Balistar") };
		string		strRootBoneName = { "Base" };
		_float		fAttackGap = { 2.f };
		_float		fBulletSpeed = { 40.f };
		_float		fBulletRange = { 30.f };
		_float		fBulletRadius = { 0.3f };
		_float		fBulletDamage = { 10.f };
		_int		iMaxHit = { 2 };
		_float		fVisionHalfCos = { 0.70710678f };
		_float		fLoseGrace = { 0.5f };
		string		strFireBone = { "MissileTurret_Muzzle" };
		string		strGazeBaseBone = { "BN_Top_Shooter_F" };
		_float		fMinRange = { 5.f };
	} BALISTAR_DESC;

private:
	enum class PHASE { IDLE, AIM, ATTACK, RELOAD, RETURN };

private:
	CBalistar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBalistar(const CBalistar& Prototype);
	virtual ~CBalistar() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;

protected:
	virtual void			Update_BlackBoard(_float fTimeDelta) override;
	virtual void			Action_Notify(_uint64 iHashKey) override;
	virtual _bool			Take_Damage(const HitInfo& hitInfo) override;
	virtual void			Spawn() override;

private:
	void					Drive_Aim(_float fTimeDelta, _bool bToPlayer, _bool bSnap);
	void					Apply_BoneRot(POSE& Pose, const string& strBone, _float4& refCur, _bool& refInit, _float fArc, _bool bSnap, _float fSlerpAmt);
	_bool					Get_AimArc(_float& OutArc);
	_bool					Get_AimPitch(_float& OutPitch);
	_bool					Is_AimNeutral() const;
	_bool					Perceive_Now();
	_bool					Get_TargetAimPos(_vector& OutAim);
	_bool					Get_GazeForward(_vector& OutOrigin, _vector& OutForward);
	void					Fire_Projectile();

	void					Enter_Idle();
	void					Enter_Aim(_bool bReseed);
	void					Enter_Return();
	void					Begin_Attack();
	void					Drive_Death();
	HRESULT					Ready_Shooter();

private:
	PHASE					m_ePhase = { PHASE::IDLE };

	string					m_strFireBone = { "MissileTurret_Muzzle" };
	string					m_strGazeBaseBone = { "BN_Top_Shooter_F" };
	string					m_strRootBone = { "Base" };

	string					m_strAimBone = { "BN_Body01" };
	string					m_strAimParentBone = { "BN_Body00" };
	_float4					m_vAimRotCur = { 0.f, 0.f, 0.f, 1.f };
	_bool					m_bAimInit = { false };
	_float					m_fAimSlerpRate = { 8.f };
	_float					m_fAimYawScale = { -1.f };
	_float					m_fAimYawOffset = { XMConvertToRadians(180.f) };
	_float					m_fAimErrorRad = { XMConvertToRadians(1.f) };
	_float					m_fGazeHalfRad = { XMConvertToRadians(60.f) };

	string					m_strPitchBone = { "BN_Body_Drum_R" };
	_float4					m_vPitchRotCur = { 0.f, 0.f, 0.f, 1.f };
	_bool					m_bPitchInit = { false };
	_float					m_fPitchScale = { -1.f };
	_float					m_fPitchOffset = { 0.f };
	_float					m_fPitchHalfRad = { XMConvertToRadians(30.f) };

	_float					m_fAimDuration = { 5.f };
	_float					m_fAimTime = { 0.f };
	_float					m_fLoseGrace = { 0.5f };
	_float					m_fLoseTimer = { 0.f };
	_float					m_fPhaseTimer = { 0.f };
	_float					m_fPhaseTimeout = { 4.f };
	_float					m_fDebugTimer = { 0.f };

	_float					m_fMinRange = { 5.f };
	_float					m_fBulletSpeed = { 40.f };
	_float					m_fBulletRange = { 30.f };
	_float					m_fBulletRadius = { 0.3f };
	_float					m_fBulletDamage = { 10.f };

	_int					m_iMaxHit = { 2 };
	_int					m_iHitCount = { 0 };

	_bool					m_bDeathStarted = { false };

	CShooter* m_pShooter = { nullptr };
	_float3					m_vShooterOffset = { -2.f, 0.f, -1.8f };
	_float3					m_vShooterScale = { 1.f, 1.f, 1.f };

public:
	static CBalistar* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END
