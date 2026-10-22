#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(GameCore)

class CAIObject; 

class CStatusComponent : public CComponent
{
protected:
	CStatusComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CStatusComponent(const CStatusComponent& Prototype);
	virtual ~CStatusComponent() = default;

public:
	//Status 설정 세트 -> 현재 사용하지않음. 애니메이션 데이터 외의 Profile이 있으면 다시 사용 
	static StatusProfile g_Profile_Free; // 초기화
	static StatusProfile g_Profile_Attack;
	static StatusProfile g_Profile_Armored;
	static StatusProfile g_Profile_Invincible;
	static StatusProfile g_Profile_Interacting;
	static StatusProfile g_Profile_Stunned;

public:
	//지정한 프로타입에 맞게 적용
	void				Apply_Profile(const StatusProfile& profile);
	//일시적 Status(직전 점프 체크 등) 초기화
	void				Reset_FrameStatus();

public:
	/* Get */
	_bool				CanInput() const { return !m_State.m_bInputLocked; }
	_bool				CanMove() const { return !m_State.m_bMovementLocked; }
	_bool				CanRotate() const { return !m_State.m_bRotationLocked; }
	_bool				CanMoveCam() const { return !m_State.m_bCameraLocked; }
	_bool				CanCancel() const { return m_State.m_bCanCancel; }
	_bool 				CanJump() const { return !m_State.m_bJumpLocked; }

public:
	_bool				Is_FatalLastHit() const { return m_State.m_bFatalLastHit; }
	_bool				Is_JustJumped() const { return m_State.m_bJustJumped; }
	_bool				Is_JustLanded() const { return m_State.m_bJustLanded; }
	_bool				Is_Falling() const { return m_State.m_bFalling; }
	_bool				Is_Grounded() const { return m_State.m_bGrounded; }
	_bool				Is_SuperArmor() const { return m_State.m_bSuperArmor; }
	_bool				Is_Invincible() const { return m_State.m_Invincible; }
	_bool 				Is_GuardActive() const { return m_State.m_bGuardActive; }
	_bool 				Is_ParryActive() const { return m_State.m_bParryActive; }
	_bool 				Is_HitActive() const { return m_State.m_bHitActive; }
	_bool 				Is_CanTargetTracking() const { return m_State.m_bCanTargetTracking; }
	_bool				Is_LockedOn() const;
	_bool				Is_ActionPlaying() const { return m_State.m_bActionPlaying; }
	_bool				Is_TakeDamage() const { return m_State.m_bTakeDamage; }
	_bool				Is_TakeGuardHit() const { return m_State.m_bTakeGuardHit; }
	_bool				Is_GravityDisabled() const { return m_State.m_bGravityDisabled; }

public:
	CAIObject*			Get_LockedTarget() const;
	HitInfo				Get_PendingHitResult() const { return m_State.m_Pending_HitInfo; }
	void				Clear_TakeDamage() { m_State.m_bTakeDamage = false; }
	/* SET */
	void				Set_FatalLastHit(_bool bFatal) { m_State.m_bFatalLastHit = bFatal; }
	void				Set_JumpLock(bool bLock) { m_State.m_bJumpLocked = bLock; }
	void				Set_InputLock(bool bLock) { m_State.m_bInputLocked = bLock; }
	void				Set_MovementLock(bool bLock) { m_State.m_bMovementLocked = bLock; }
	void				Set_RotationLock(bool bLock) { m_State.m_bRotationLocked = bLock; }
	void				Set_CamMoveLock(bool bCamLock) { m_State.m_bCameraLocked = bCamLock; }
	void				Set_JustLanded(_bool bState) { m_State.m_bJustLanded = bState; }
	void				Set_Falling(_bool bState) { m_State.m_bFalling = bState; }
	void				Set_Grounded(_bool bState) { m_State.m_bGrounded = bState; }
	void				Set_JustJumped(_bool bState) { m_State.m_bJustJumped = bState; }

	void				Set_SuperArmor(_bool bSuperArmor) { m_State.m_bSuperArmor = bSuperArmor; }
	void 				Set_Invincible(_bool bInvincible) { m_State.m_Invincible = bInvincible; }
	void				Set_GuardActive(_bool bGuardActive) { m_State.m_bGuardActive = bGuardActive; }
	void				Set_ParryActive(_bool bParryActive) { m_State.m_bParryActive = bParryActive; }
	void				Set_HitActive(_bool bHitActive) { m_State.m_bHitActive = bHitActive; }
	void				Set_CanTargetTracking(_bool bCanTrack) { m_State.m_bCanTargetTracking = bCanTrack; }
	void 				Set_LockedTarget(CAIObject* pTarget) { m_State.m_pLockedTarget = pTarget; }
	
	//TODO : 플래그만 받도록 변경하기 
	void 				Set_TakeDamage(_bool bTakeDamage, HitInfo info ) 
	{ 
		m_State.m_bTakeDamage = bTakeDamage; 
		m_State.m_Pending_HitInfo = info;
	}

	void 				Set_TakeGuardHit(_bool bTakeGuardHit, HitInfo info)
	{
		m_State.m_bTakeGuardHit = bTakeGuardHit;
		m_State.m_Pending_HitInfo = info;
	}

	void				Set_GravityDisabled(_bool bLocked) { m_State.m_bGravityDisabled = bLocked; }
	void				Set_CanCancel(_bool bCanCancel) { m_State.m_bCanCancel = bCanCancel; }
	
	void				Set_ActionPlaying(_bool bPlaying) { m_State.m_bActionPlaying = bPlaying; }
	void				Set_RootMotion(_bool bUseRootMotion) { m_State.m_bUseRootMotion = bUseRootMotion; }

	void				Set_PendingInteraction(const ActionData* data) { m_State.m_pPending_Interact = data; }
	const ActionData*	Get_PendingInteraction() const { return m_State.m_pPending_Interact; }

	virtual void		Clear() override;
private:
	struct STATUS_STATE
	{
		_bool				m_bCanCancel = { false };
		_bool				m_bSuperArmor = { false };			//데미지는 받지만 모션캔슬 X
		_bool				m_Invincible = { false };			//데미지 받지 않음
		_bool				m_bCanTargetTracking = { true };	//타겟 추적 가능 여부(Animator 속성) 
		_bool				m_bHitActive = { false };			//공격 판정 활성 여부 
		_bool				m_bGuardActive = { false };
		_bool				m_bParryActive = { false };

		//권한 제어
		_bool				m_bGravityDisabled = { false };
		_bool				m_bCameraLocked = { false };
		_bool				m_bMovementLocked = { false };
		_bool				m_bRotationLocked = { false };
		_bool				m_bInputLocked = { false };
		_bool				m_bJumpLocked = { false };

		CAIObject*			m_pLockedTarget = { nullptr };


		_bool				m_bFalling = { false };
		_bool				m_bGrounded = { false };
		_bool				m_bActionPlaying = { false };

		//일시적 이벤트 용
		_bool				m_bJustLanded = { false };
		_bool				m_bJustJumped = { false };
		_bool				m_bTakeDamage = { false };
		_bool				m_bTakeGuardHit = { false };
		_bool				m_bUseRootMotion = { false };

		_bool				m_bFatalLastHit = { false };

		const ActionData*	m_pPending_Interact = { nullptr };
		HitInfo				m_Pending_HitInfo = {};
	};
	STATUS_STATE m_State = {};

public:
	static CStatusComponent* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent* Clone(void* pArg);
	virtual void Free() override;
};


NS_END