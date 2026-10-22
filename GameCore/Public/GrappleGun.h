#pragma once
#include "LegionArm.h"
#include "GrappleWire.h"

NS_BEGIN(GameCore)
class CAIObject;
class GAMECORE_DLL CGrappleGun final : public CLegionArm
{
	typedef struct tagShotPreview
	{
		_float3 vHitPos;
		CAIObject* pTarget;
		_bool	bHit;
		_bool	bStatic;
	} SHOT_RESULT;

protected:
	CGrappleGun(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CGrappleGun() = default;

public:
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;
	virtual void				Post_BoneUpdate(_float fTimeDelta) override;
public:
	GrappleState				Get_GrappleState() const { return m_eState; }
	const _float3				Get_HitPosition() const { return m_vHitPosition; }
	CAIObject*					Get_GrappleTarget() const { return m_pGrappleTarget; }
	_float						Get_MinDistToTarget(_float fGap) const;
	_bool						Is_MobilityUnlocked() const { return m_bMobilityUnlocked; }

public:
	virtual void				Use() override;
	virtual void				Use_Upgrade() override;
	virtual _bool				Is_Used() const override { return m_eState != GrappleState::Idle; }
	virtual void				Cancel() override;

	void						Unlock_Mobility() { m_bMobilityUnlocked = true; }
	virtual void				On_AnimEvent(LegionArmEvent eEvent) override;

private:
	void						Update_Wire(_float fTimeDelta);
	void						Update_YawFollow(_float fTimeDelta);
	void						Update_AimPose();
	void						Reset_Pending();

private:
	/* 발사 */
	// 즉시 발사 : 락온 대상 또는 화면 중앙으로 바로 발사
	void						Fire_QuickShot(_bool bUpgrade);
	// 조준 발사 : 조준 모드에서 화면 중앙으로 발사 
	void						Fire_AimedShot();

	SHOT_RESULT					Sweep_GrappleRay(_fvector vStart, _fvector vDir) const;
	_bool						Commit_Preview(const SHOT_RESULT& result);

private:
	//애니메이터 요청만 
	void						Clear_Grapple(_bool bRestoreInputLock);
	void						Begin_Grapple();
	void						End_Grapple();
	void						Play_GrappleAction();

private:
	void						Enter_Pull();
	void						Enter_Fail();
	void						Enter_Chase();
	void						Enter_Smash();
	void						Enter_Aim();
	void						Enter_Zip();
	void						Release_Zip();

private: 
	void						Smash_Attack(LegionArmEvent event);

private:
	void						Request_Aim();
	void						Update_Aim(_float fTimeDelta);
	void						Exit_Aim();
	void						Set_AimSlow(_bool bOn); //전역 슬로우 모드 
	void						Begin_ChangeView();

private:
	static constexpr _float		MAX_DISTANCE			= { 15.f };
	static constexpr _float		SWEEP_RADIUS			= { 0.5f };
	static constexpr _float4	SWEEP_OFFSET			= { 0.f, 1.2f, 0.f, 0.f };
	
	static constexpr const char* ROOT_BONE_NAME			= { "Bip001-Pelvis" };
	static constexpr const char* WIRE_HIT_EFFECT		= { "Player_GrappleArn_Wire_Hit" };
	static constexpr const char* CAM_SHAKE_PULLING		= { "BP_Shake_Wire_Pulling" };
	static constexpr const char* CAM_SHAKE_ZIP_RELEASE	= { "BP_Shake_Zip_Release" };

	//SpringArm 조정 데이터 
	static constexpr _float		CAM_BLEND_IN_TIME		= { 0.15f };
	static constexpr _float		CAM_BLEND_OUT_TIME		= { 0.25f };
	static constexpr _float		CAM_SWING_RIGHT_OFFSET	= { 0.75f };
	static constexpr _float		CAM_SWING_UP_OFFSET		= { -0.2f};
	static constexpr _float		CAM_ARM_LENGTH			= { 1.85f };

	static constexpr _float		WIRE_FIRE_TIME			= { 0.4f };
	static constexpr _float		WIRE_REWIND_TIME		= { 0.4f };
	static constexpr const char*WIRE_START_BONE			= { "PT_GrappleGun_Shot" };

	static constexpr _float		GRAPPLE_PULL_DELAY		= { 0.4f };
	static constexpr _float		GRAPPLE_PULL_TIME		= { 0.3f };
	static constexpr _float		GRAPPLE_PULL_DEST_GAP	= { 0.3f }; // 원본데이터 0.8f

	//추적 데이터 
	static constexpr _float		CHASE_SPEED				= { 18.f };
	static constexpr _float		CHASE_STOP_EXTRA		= { 0.2f };

	static constexpr _float		ZIP_STOP_EXTRA			= { 0.2f };

	//스매시 - 1타 STRIKE , 2타 내려찍기 JUMP
	static constexpr _float		SMASH_EXTRA				= { 1.0f };
	static constexpr _float		SMASH_STRIKE_DAMAGE		= { 20.f };
	static constexpr _float		SMASH_JUMP_DAMAGE		= { 85.f };
	static constexpr _float		SMASH_GROGGY			= { 50.f };

	//입체기동 목적지 높이 오프셋 
	static constexpr _float		RELEASE_CLEAR_HEIGHT	= { 1.3f };
	static constexpr _float		RELEASE_UP_MAX_SPEED	= { 15.f };

	static constexpr _float		AIM_SLOW_SCALE			= { 0.08f };
	static constexpr _float		AIM_YAW_FOLLOW_RATE		= { 18.f };
	static constexpr _float		AIM_CHAIN_MAX_ANGLE		= { 80.f };

private:
	GrappleState				m_eState				= { GrappleState::Idle };
	_float3						m_vHitPosition			= {};
	//Smash때만 추적 할 루트본 Y좌표
	_float						m_fSmashBaseBoneY		= {};
private:
	CGrappleWire*				m_pWire					= { nullptr };
	_float						m_fStateTime			= {};
	_float						m_fWireProgress		= {};
	_float3						m_vWireEndPos				= {};
	_bool						m_bWireLaunched			= {};
	_bool						m_bWireHited			= { false };
	CAIObject*					m_pGrappleTarget		= { nullptr };

	_bool						m_bPullStarted			= { false };
	_bool						m_bPullDone				= { false };
	_float3						m_vPullStartPos			= {};
	_float3						m_vPullDestPos			= {};
	_bool						m_bTargetGrabbed		= { false };
	const _float4x4*			m_pOwnerRootMat			= { nullptr };
	const _float4x4*			m_pMuzzleBoneMat		= { nullptr };

private:
	//조준 자세 프로필 번호
	_int						m_iAimProfile			= { -1 };
	_bool						m_bPendingTarget		= { false };
	_bool						m_bPendingUpgrade		= { false };
	_bool						m_bPendingTarget_Static	= { false };
	//입체기동 해금 플래그
	_bool						m_bMobilityUnlocked		= { true };

	_bool						m_bSlowActive			= { false };
	_bool						m_bResumeAim			= { false };

private:
	static constexpr CGrappleWire::WIRE_PROFILE WIRE_PROFILES[ETOUI(GrappleState::End)] =
	{
		//Slack		GravityScale    Damping		State
		{ 1.05f,		1.0f,		0.98f },	// Idle
		{ 1.15f,		1.0f,		0.98f },	// Fire
		{ 0.90f,		0.1f,		0.98f },	// Pull
		{ 1.05f,		1.0f,		0.88f },	// Fail
		{ 0.95f,		0.1f,		0.98f },	// Chase
		{ 1.00f,		1.0f,		0.98f },	// Smash
		{ 0.90f,		0.1f,		0.98f },	// Zip
		{ 1.00f,		1.0f,		0.98f },	// Aim
	};
public:
	static CGrappleGun*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg);
	virtual void				Free() override;
};

NS_END