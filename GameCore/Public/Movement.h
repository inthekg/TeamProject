#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CTransform;
NS_END

NS_BEGIN(GameCore)
class CStatusComponent;
class CInputComponent;
class CPlayer;

//이번 프레임의 움직임을 계산만 하는 컴포넌트. 반영은 Player가 함
class GAMECORE_DLL CMovement : public CComponent
{
public:
	typedef struct tagMovementDesc
	{
		CPlayer*	pOwner;
		_float		fRunSpeed;
		_float		fSprintSpeed;
		_float		fAcceleration;
		_float		fDeceleration;
		//회전 따라가는 속도
		_float		fTurnRate;
	} MOVEMENT_DESC;

	typedef struct tagMovementResult
	{
		//이번 프레임 이동량
		_float3		vMoveDelta;
		_bool		bChangedQuat;
		_float4		vMoveQuat;
	} MOVEMENT_RESULT;

protected:
	CMovement(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMovement(const CMovement& Prototype);
	virtual ~CMovement() = default;
public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	void						Update_RootMotion(_bool bHasRootMotion, const TransformData& DeltaTransform);
	//입력 읽기부터 이번 프레임 이동량/회전 계산까지 한 번에
	MOVEMENT_RESULT				Compute_Movement(_float fTimeDelta);

public:
	_float						Get_CurrentSpeed() const;
	//이 캐릭터가 실제로 받는 낙하 가속도 = 캐릭터 공통 중력 × 무게
	_float						Get_FallGravity() const;
	void						Set_SideBlocked(_bool bBlocked) { m_bSideBlocked = bBlocked; }

	//루트모션 방향 강제 갱신
	void						Set_RootMotionQuat(_fvector vQuat) { XMStoreFloat4(&m_vRootMotionQuat, vQuat); }
	void						Restart_RootMotion() { m_bRootMotionRestart = true; }
	void						Set_RootMotionScale(_float fScale) { m_fRootMotionScale = fScale; }
	void						Clear_RootMotionScale() { m_fRootMotionScale = 1.f; }
public:
	void						Begin_MoveTo(_fvector vTargetPos, _fvector vTargetQuat,_float fDuration, EASE_TYPE eEaseType = EASE_TYPE::LINEAR, _bool b3DMove = false);
	void						Begin_KnockBack(_fvector vDirection, _float fDistance, _float fDuration, EASE_TYPE eEaseType = EASE_TYPE::LINEAR);

	void						Stop_MoveTo();
	_bool						Is_MoveActive() const { return m_bMoveTo; }
	_bool						Is_MoveReached() const { return m_bMoveReached; }
	_bool						Is_Sprinting() const { return m_eSpeedType == SPEED_TYPE::SPRINT; }

	virtual void				Clear() override;
	void						Set_AirTimeScale(_float fScale) { m_fAirTimeScale = fScale; }
public:
	void						Launch(_fvector vVelocity);
	_bool						Is_AirActive() const { return m_bAirActive; }

private:
	/* Compute_Movement 단계 */
	void						Update_Input();
	void						Handle_RootMotionStart();
	void						Update_AirState(_float fTimeDelta, _bool bGrounded, _bool bGravityDisabled);
	void						Compute_MoveTo(_float fTimeDelta);
	void						Compute_RootMotion(_float fTimeDelta);
	void						Compute_AirControl(_float fTimeDelta);
	void						Compute_GroundMove(_float fTimeDelta);
	void						Compute_Facing(_float fTimeDelta);
	void						Apply_Gravity(_float fTimeDelta, _bool bGrounded, _bool bGravityDisabled);

	//락온 타겟까지의 수평 방향. 락온 아님/타겟 없음/같은 위치면 false
	_bool						Get_LockOnDir(_vector& OutDir) const;
	static _vector				Get_Yaw_Quat(_fvector vDir);

private:
	//캐릭터 공통 중력 
	static constexpr _float		CHARACTER_GRAVITY		= { PhysX::g_fGravity * PhysX::g_fGravityScale };

	static constexpr _float		GUARD_MOVE_RATE			= { 0.75f };
	static constexpr _float		REVERSE_MULT_ACCEL		= { 5.f };

	static constexpr _float     MAX_FALL_SPEED			= { -20.f };
	static constexpr _float		FALL_VALUE				= { -2.f };

	static constexpr _float		DELAY_TIME				= { 0.15f };

	//공중에서 입력 관성 오프셋
	static constexpr _float		AIR_CONTROL_SPEED		= { 3.5f };
	//공중 보간 속도
	static constexpr _float		AIR_CONTROL_RATE		= { 8.f };
	//이보다 짧은 입력 방향은 입력 없음으로 봄
	static constexpr _float		MIN_INPUT_LENGTH		= { 0.001f };

private:
	_bool						m_bHasMoveInput			= {};
	_bool						m_bPlayRootMotion		= {};
	TransformData				m_tRootMotionTransform	= {};

	//Y축 이동 포함 여부
	_bool						m_bMoveTo3D = { false };
	_float3						m_vVelocity				= {};
	//입력축을 카메라 기준으로 바꾼 이동 방향 (월드 XZ, 길이 1, 입력 없으면 0)
	_float4						m_vCamInputDir			= {};
	//공중 이동,낙하에만 거는 시간 배율
	_float						m_fAirTimeScale			= { 1.f };

	_float						m_Speed[ETOUI(SPEED_TYPE::END)] = {};
	SPEED_TYPE					m_eSpeedType			= { SPEED_TYPE::IDLE };
	_float						m_fAcceleration			= {};
	_float						m_fDeceleration			= {};
	_float						m_fTurnRate				= {};

	_float						m_fFallVelocityY		= {};

	_bool						m_bSideBlocked			= {};
	//MoveTo(자동 이동) 데이터
	_bool						m_bMoveTo				= { false };
	_bool						m_bMoveReached			= { false };

	//플레이어 시작 행렬 데이터
	_float4						m_vMoveStartPos			= {};
	_float4						m_vMoveStartQuat		= {};

	_float4						m_vMoveTargetPos		= {};
	_float4						m_vMoveTargetQuat		= {};

	_float						m_fMoveDuration			= {};
	_float						m_fMoveElapsedTime		= {};
	EASE_TYPE					m_eMoveEaseType			= { EASE_TYPE::LINEAR };

	MOVEMENT_RESULT				m_tMovementResult		= {};

	_bool						m_bAirActive			= { false };
	_float						m_fAirTime				= {};
	_float2						m_vAirVelocityXZ		= {};
	_float2						m_vLastGroundedXZ		= {};

private:
	//루트모션 애니메이션 플래그 (1 프레임 소모)
	_bool						m_bRootMotionStarted	= { false };
	_bool						m_bRootMotionRestart	= { false };
	_float4						m_vRootMotionQuat		= {};
	//노티파이로 루트모션 이동량 스케일링하는 값
	_float						m_fRootMotionScale		= { 1.f };
private:
	CPlayer*					m_pOwner				= { nullptr };
	CTransform*					m_pOwnerTransformCom	= { nullptr };
	CInputComponent*			m_pOwnerInputCom		= { nullptr };
	CStatusComponent*			m_pOwnerStatusCom		= { nullptr };

public:
	static CMovement*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CComponent*					Clone(void* pArg) override;
	virtual void				Free() override;

};
NS_END
