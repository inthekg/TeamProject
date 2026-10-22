#pragma once
#include "LegionArm.h"

NS_BEGIN(GameCore)
class GAMECORE_DLL CChargeGun final : public CLegionArm
{
protected:
	CChargeGun(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CChargeGun() = default;

public:
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;

public:
	virtual void				On_AnimEvent(LegionArmEvent eEvent) override;
	virtual void				Use() override;
	virtual void				Use_Upgrade() override;
	virtual _bool				Is_Used() const override { return m_eState != ChargeGunState::Idle; }

public:
	ChargeGunState				Get_ChargeGunState() const { return m_eState; }
	_bool						Is_Aiming() const { return m_eState == ChargeGunState::Aim; }

	void						Request_Cancel();
	virtual void				Cancel() override;

private:
	void						Request_ChargeGun();
	void						Begin_ChargeGun();
	void						End_ChargeGun();

	void						Use_Internal(_bool bUgrade);
private:
	void						Enter_Aim();
	void						Enter_Fire();
	void						Enter_Exit();

	void						Fire_Bullet();
	//카메라 수평 회전만 따라감 
	void						Update_YawFollow(_float fTimeDelta);

private:
	static constexpr _float		CAM_BLEND_IN_TIME		= { 0.15f };
	static constexpr _float		CAM_BLEND_OUT_TIME		= { 0.35f };
	static constexpr _float		CAM_AIM_RIGHT_OFFSET	= { -0.5f };
	static constexpr _float		CAM_AIM_UP_OFFSET		= { -0.0f };
	static constexpr _float		CAM_ARM_LENGTH			= { 2.2f };

	static constexpr _float		FIRE_RANGE				= { 15.f };
	static constexpr _float		SWEEP_RADIUS			= { 0.2f };

	static constexpr _float		STATE_TIMEOUT			= { 1.5f };
	//에이밍 중 카메라 Yaw를 몸이 따라가는 속도
	static constexpr _float		AIM_YAW_FOLLOW_RATE		= { 18.f };
	static constexpr _float		AIM_CHAIN_MAX_ANGLE		= { 75.f };

	static constexpr _float		BOMB_DELAY				= { 2.f };
	static constexpr _float		BOMB_LINGER				= { 0.5f };
	static constexpr _float		BOMB_RADIUS				= { 2.f };
	static constexpr _float		BOMB_DAMAGE				= { 1000.f };

	//Effect_Common_Fire
	static constexpr const char* MUZZLE_EFFECT_NAME		= { "Effect_Shotgun_Muzzle" }; //Effect_Shotgun_Muzzle
	static constexpr const char* MUZZLE_OWNER_BONE_NAME = { "BN_Flam_Cannon" }; // 아니면 더 앞쪽 +Z으로 BN_SniperCannon01~03 본 

private:
	ChargeGunState				m_eState				= { ChargeGunState::Idle };
	_float						m_fStateTime			= { 0.f };
	_bool						m_bFirePending			= { false };
	_bool						m_bUpgradeShot			= { false };

	_int						m_iAimProfile			= { -1 };
public:
	static CChargeGun*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg);
	virtual void				Free() override;
};

NS_END