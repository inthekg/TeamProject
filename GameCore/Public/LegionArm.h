#pragma once
#include "BodyPart.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CMasterModel;
class CSpringArm;
class CTransform;
NS_END

NS_BEGIN(GameCore)
class CStatusComponent;
class CAnimDriver;
class CBodyPart;
class CPlayer;
class CAimComponent;
class CAIObject;

class GAMECORE_DLL CLegionArm abstract : public CBodyPart
{
public:
	typedef struct tagLegionArmDesc : public BODYPART_DESC
	{
		CPlayer*				pOwner = { nullptr };
	}LEGIONARM_DESC;

protected:
	CLegionArm(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLegionArm() = default;

	CLegionArm(const CLegionArm& Prototype) = delete;
	CLegionArm& operator=(const CLegionArm& Prototype) = delete;

public:
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void 			Late_Update(_float fTimeDelta) override;
	virtual void			Post_BoneUpdate(_float fTimeDelta) {}

public:
	virtual void			On_AnimEvent(LegionArmEvent eEvent) {};

	LegionArmType			Get_LegionArmType() const { return m_eType; }
	_bool					Is_GaugeEmpty() { return m_fGauge <= 0.f; }
	void					Consume_Gauge(_float fAmount) { m_fGauge = max(0.f, m_fGauge - fAmount); }
	void					Regen_Gauge() { m_fGauge = min(m_fMaxGauge, m_fGauge + m_fGaugeRegenRate); }

public:
	//리전암 사용 시작
	virtual void			Use() {}
	virtual _bool			Is_Used() const { return false; }
	virtual void			Use_Upgrade() {};

	_bool					Is_Upgrade() const { return m_bUpgrade; }
	void					Upgrade() { m_bUpgrade = true; }

	virtual void			Cancel() {} 

protected:
	void					Compute_RayToTarget(CAIObject* pTarget, _fvector vStartOffset, _vector& vStart, _vector& vDir) const;
	void					Compute_CameraRay(_vector& vStart, _vector& vDir) const;
	void					Set_CrosshairVisible(_bool bVisible);

protected:
	CPlayer*				m_pOwner			= { nullptr };
	CAnimDriver*			m_pAnimDriver		= { nullptr };
	CSpringArm*				m_pSpringArm		= { nullptr };
	CStatusComponent*		m_pStatusCom		= { nullptr };
	CTransform*				m_pOwnerTransform	= { nullptr };
	CAimComponent*			m_pAimCom			= { nullptr };

	LegionArmType			m_eType				= { LegionArmType::End };

	_bool					m_bCrosshairVisible = { false };

	_float					m_fGaugeRegenRate	= { 5.f };
	_float					m_fGauge			= { 0.f };
	_float					m_fMaxGauge			= { 100.f };

	_bool					m_bAnimEntered		= {};
	_bool					m_bUpgrade			= {};
public:
	virtual CGameObject* Clone(void* pArg) { return nullptr; };
	virtual void Free() override;
};

NS_END