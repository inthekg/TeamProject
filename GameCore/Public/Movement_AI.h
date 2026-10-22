#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CTransform;
class CNavigation;
NS_END

NS_BEGIN(GameCore)

class CAIObject;

class GAMECORE_DLL CMovement_AI : public CComponent
{
public:
	typedef struct tagMovementAIDesc
	{
		CAIObject* pOwner;
		CTransform* pTransform;
		CNavigation* pNavigation;
		_float			fRunSpeed;
		_float			fSprintSpeed;
		_float			fAcceleration;
		_float			fDeceleration;
		_float			fGroundStick;
		_float			fNaviRepathInterval;
		_float			fRotationPerSec;
		_float			fAgentRadius = { 0.5f };
	} MOVEMENT_DESC;

	typedef struct tagMovementAIResult
	{
		_float3			vMoveDir;
		_bool			bChangedQuat;
		_float4			vMoveQuat;
	} MOVEMENT_RESULT;

protected:
	CMovement_AI(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMovement_AI(const CMovement_AI& Prototype);
	virtual ~CMovement_AI() = default;

public:
	void					Set_MoveDir(_fvector vWorldDir);
	void					Set_MoveTarget(_fvector vWorldPos);
	void					Set_SpeedType(SPEED_TYPE eType) { m_eSpeedType = eType; }
	void					Set_Speed(SPEED_TYPE eType, _float fSpeed) { if (SPEED_TYPE::END != eType) m_Speed[ETOUI(eType)] = fSpeed; }
	void					Set_UseAccel(_bool bUse) { m_bUseAccel = bUse; }
	void					Set_Grounded(_bool bGrounded) { m_bGrounded = bGrounded; }
	void					Set_SideBlocked(_bool bBlocked) { m_bSideBlocked = bBlocked; }
	void					Set_Navigation(CNavigation* pNavigation) { m_pNavigation = pNavigation; }
	void					Set_HomingSpeed(_float fSpeed) { m_fHomingSpeed = fSpeed; }


	_float					Get_CurrentSpeed() const;
	_bool					Is_Reached(_fvector vWorldPos, _float fToL) const;
	_bool					Is_Launching() const { return m_bLaunching; }
	_bool					Is_MoveActive() const { return m_bMoveTo; }
	_bool					Is_MoveReached() const { return m_bMoveReached; }
	_float					Get_ReachRadius() const { return m_fAgentRadius + 0.3f; }

	_bool					Is_Grounded() const { return m_bGrounded; }
	_float					Get_FallVelocityY() const { return m_fFallVelocityY; }

#ifdef _DEBUG
	const vector<_float3>& Get_DebugNaviPath() const { return m_NaviPath; }
	_uint Get_DebugNaviPathIndex() const { return m_iNaviPathIndex; }
#endif // _DEBUG


public:
	virtual	HRESULT			Initialize_Prototype() override;
	virtual	HRESULT			Initialize(void* pArg) override;

public:
	MOVEMENT_RESULT			Compute_Movement(_float fTimeDelta);
	_bool					MoveTo_Navi(_fvector vWorldPos, _float fTimeDelta);
	void					TurnTo(_fvector vWorldPos, _float fTimeDelta);
	void					Strafe(_int iDir, _float fTimeDelta);
	void					Orbit(_int iDir, _fvector vTargetPos, _float fTimeDelta);
	void					Push_RootMotion(_bool bHasRootMotion, const TransformData& tDelta);
	void					Begin_MoveTo(_fvector vTargetPos, _fvector vTargetQuat, _float fDuration, EASE_TYPE eEaseType = EASE_TYPE::LINEAR);
	void					Begin_KnockBack(_fvector vDirection, _float fDistance, _float fDuration, EASE_TYPE eEaseType = EASE_TYPE::LINEAR);
	void					Stop_MoveTo();
	void					Reset_Motion();
	void					Begin_Launch(_fvector vDir, _float fHorizontalSpeed, _float fUpSpeed);

private:
	_float					Get_FallGravity() const;


private:
	const _float		REVERSE_MULT_ACCEL = { 5.f };
	const _float		MAX_FALL_SPEED = { -30.f };

private:
	CAIObject* m_pOwner = { nullptr };
	CTransform* m_pOwnerTransformCom = { nullptr };
	CNavigation* m_pNavigation = { nullptr };

private:
	MOVEMENT_RESULT		m_tMovementResult = {};
	_float3				m_vVelocity = {};
	_float4				m_vMoveDir = {};
	_bool				m_bHasSteerInput = {};
	_float				m_fHomingSpeed = { 0.f };
	_bool				m_bUseAccel = { false };

	SPEED_TYPE			m_eSpeedType = { SPEED_TYPE::END };
	_float				m_Speed[ETOUI(SPEED_TYPE::END)] = {};
	_float				m_fAcceleration = {};
	_float				m_fDeceleration = {};
	_float				m_fRotationPerSec = {};

	_float				m_fFallVelocityY = {};
	_float				m_fGroundStick = {};
	_bool				m_bGrounded = {};
	_bool				m_bSideBlocked = {};

	_float3				m_vLaunchVelXZ = {};
	_bool				m_bLaunching = {};

	_bool				m_bMoveTo = { false };
	_bool				m_bMoveReached = { false };
	_float4				m_vMoveStartPos = {};
	_float4				m_vMoveStartQuat = {};
	_float4				m_vMoveTargetPos = {};
	_float4				m_vMoveTargetQuat = {};
	_float				m_fMoveDuration = {};
	_float				m_fMoveElapsedTime = {};
	EASE_TYPE			m_eMoveEaseType = { EASE_TYPE::LINEAR };

	_bool				m_bPlayRootMotion = {};
	TransformData		m_tRootMotionTransform = {};
	_float4				m_vRootMotionQuat = {};

	vector<_float3>		m_NaviPath;
	_uint				m_iNaviPathIndex = {};
	_int				m_iNaviCurCellIndex = { -1 };
	_float				m_fNaviRepathAcc = {};
	_float				m_fNaviRepathInterval = { 0.3f };
	_float3				m_vLastNaviGoal = {};
	_float				m_fAgentRadius = { 0.5f };
	_float				m_fCornerInset = { 0.f };
	_float				m_fNaviPathHold = { 0.f };

public:
	static CMovement_AI* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent* Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END
