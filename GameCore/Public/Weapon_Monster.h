#pragma once
#include "Core_Defines.h"
#include "PartObject.h"

NS_BEGIN(Engine)
class CModel;
class CEffect;
NS_END

NS_BEGIN(GameCore)

class CAIObject;

class GAMECORE_DLL CWeapon_Monster abstract : public CPartObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CWeapon_Monster)
#endif // _DEBUG

public:
	typedef struct tagWeaponDesc : public CPartObject::PARTOBJECT_DESC
	{
		CAIObject* pOwner = { nullptr };
		const _float4x4* pSocketBoneMatrix = { nullptr };
		_wstring		 strModelTag = {};
		_uint			 iLevelID = {};
		_float			 fDamage = { 10.f };
		_bool			 bRender = { true };
	}WEAPON_DESC;

protected:
	CWeapon_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CWeapon_Monster(const CWeapon_Monster& Prototype);
	virtual ~CWeapon_Monster() = default;

public:
	void					Set_Render(_bool bRender) { m_bRender = bRender; }
	void					Set_Dissolve(_float fAmount, const _float4& vColor) { m_fDissolveAmount = fAmount; m_vDissolveColor = vColor; }

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;
	virtual HRESULT			Render_Shadow() override;


public:
	virtual	void			Attack_Begin(_float fDamage = -1.f, HIT_TYPE eType = HIT_TYPE::Normal) {}
	virtual void			Attack_End() {}
	virtual void			Fire(_fvector vOrigin, _fvector vDir, _float fDamage = 0.f , HIT_TYPE eType = HIT_TYPE::Normal) {}
	virtual void			Update_Attack(_float fTimeDelta) {}

public:
	CAIObject* Get_Owner() const { return m_pOwner; }
	_bool					Is_HitSuccess() const { return m_bHitSuccess; }
	void					Clear_HitSuccess() { m_bHitSuccess = false; }
	CModel* Get_Model() const { return m_pModelCom; }

public:
	void					Begin_ParryFlash();
	_bool					Get_BoneWorld(const string& strBoneName, _matrix& OutMatrix);
	HRESULT					Get_BoneWorldMatrix(_int iBoneIndex, _matrix& OutMatrix);

protected:
	HRESULT					Ready_Components(void* pArg);
	void					Update_GroundSpark(_float fTimeDelta, _fvector vRayOrigin, _fvector vRayDir, _float fRayLength, _bool bSweepTouch);
	void					Stop_GroundSpark();
	void					Update_ParryFlash(_float fTimeDelta);

protected:
	static constexpr _int	PARRY_RIM_PROFILE = { 4 };

protected:
	CAIObject* m_pOwner = { nullptr };
	CModel* m_pModelCom = { nullptr };
	const _float4x4* m_pSocketBoneMatrix = { nullptr };
	POSE					m_CurrentPose;

	_float					m_fDamage = { 10.f };
	FILTER_GROUP			m_eTargetGroup = { FILTER_GROUP::PLAYER};
	_bool					m_bRender = { true };
	HIT_TYPE				m_eHitType = { HIT_TYPE::Normal};

	_bool					m_bHitSuccess = { false };

	CEffect* m_pSparkFx = { nullptr };
	string					m_strSparkFx = {};
	_float4x4				m_matSparkRoot = {};
	_float					m_fSparkGrace = { 0.f };
	_float					m_fSparkGraceTime = { 0.15f };
	_float					m_fSparkRayExtend = { 1.f };

	_float					m_fParryFlashTimer = { 0.f };
	_float					m_fParryFlashTime = { 1.f };
	_float					m_fParryFlashPeak = { 1.2f };

	_float					m_fDissolveAmount = { 0.f };
	_float4					m_vDissolveColor = { 0.f, 0.f, 0.f, 0.f };

public:
	virtual CGameObject* Clone(void* pArg) PURE;
	virtual void			Free() override;

#ifdef _DEBUG
public:
	HRESULT					Set_SparkGraceTime(_float fSeconds) { m_fSparkGraceTime = fSeconds; return S_OK; }
	HRESULT					Set_SparkRayExtend(_float fLength) { m_fSparkRayExtend = fLength; return S_OK; }
	HRESULT					Set_ParryFlashTime(_float fSeconds) { m_fParryFlashTime = fSeconds; return S_OK; }
	HRESULT					Set_ParryFlashPeak(_float fPeak) { m_fParryFlashPeak = fPeak; return S_OK; }
	void					Debug_GroundSparkLog(_bool bRayHit, _bool bSweepTouch, _fvector vRayOrigin, _fvector vRayDir, _float fRayLength);
#endif

};

NS_END
