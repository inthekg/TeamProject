#pragma once
#include "Component.h"

NS_BEGIN(Engine)
class CGameObject;

class ENGINE_DLL CSpringArm : public CComponent
{
public:
	typedef struct tagSpringArmDesc
	{
		CGameObject*	pOwner				= { nullptr };
		_float2			vArmRotLimits;
		_float			fTargetOffset;
		_float			fArmLength;
		_float			fMouseSensor;

	}SPRING_DESC;

	//상황별 카메라 보정 profile
	typedef struct tagSpringAdjustDesc
	{
		_float3			vLocalOffset		= {};
		_float3			vLookTargetPos		= {};
		_float			fPitchMult			= { 10.f };
		_float			fYawMult			= { 15.f };
		_float			fMaxAngle			= { XMConvertToRadians(20.f) };
		_float			fArmLength			= { -1.f };
		_float			fBlendInTime		= { 0.15f };
		_float			fBlendOutTime		= { 0.35f };
		_bool			bLookBias			= { false };

	} SPRING_ADJUST_DESC;

private:
	CSpringArm();
	CSpringArm(const CSpringArm& Prototype);
	virtual ~CSpringArm() = default;
public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	void 						Update(_long lVertical_MoveDelta, _long lHorizon_MoveDelta, _float fTimeDelta);
	void						Late_Update(_float fTimeDelta);

public:
	CameraPose					Get_FinalCameraPose() const { return m_FinalCameraPose; }
	_float						Get_ArmLength() const { return m_fArmLength; }
	_float						Get_HorizonValue() const { return m_fHorizonValue; }
	_float						Get_VerticalValue() const { return m_fVerticalValue; }

public:
	void						Set_PivotExtraY(_float fY) { m_fPivotExtraY = fY; }
	void						Set_LockOn(_bool bLockOn, _float3 vTargetPos);
	virtual void				Clear() override;
	//카메라 액션모드 종료 상태를 FOLLOW모드에서 유지하기 위해 마지막 상태와 동기화 함
	void						Sync_FromQuat(_fvector vCamQuat);

public:
	//카메라 조정(뷰 전환)
	void						Begin_Adjust(const SPRING_ADJUST_DESC& AdjustDesc);
	void						End_Adjust();
private:
	_vector						SmoothDamp(_float4 vCurrent, _float4 vTarget, _float4& vCurrentVelocity, _float fSmoothTime, _float fDeltaTime);

	void						Update_TargetPos(_float fTimeDelta);
	void						Update_Arm(_float fTimeDelta);
	void						Update_Collision(_float fTimeDelta, _fvector vCurPos, _vector& vFinalPos);

	_float						Approach_Angle(_float fCurrent, _float fTarget, _float fDeadZone,_float fMaxSpeed, _float fRate, _float fTimeDelta);
private:
	static constexpr _float		SMOOTH_TIME				= { 0.15f };
	static constexpr _float		CAM_LAG_MAX_DIST		= { 1.2f };
	static constexpr _float		CAM_RADIUS				= { 0.2f };
	
	static constexpr _float		LOCKON_SMOOTH_TIME		= { 0.2f };
	static constexpr _float		LOCKON_BLEND_RATE		= { 12.f };
	static constexpr _float		LOCKON_PITCH			= { XMConvertToRadians(20.f) };
	static constexpr _float		LOCKON_PITCH_MIN		= { XMConvertToRadians(-60.f) };
	static constexpr _float		LOCKON_PITCH_MAX		= { XMConvertToRadians(80.f) };
	static constexpr _float		LOCKON_PITCH_SPEED_MAX	= { XMConvertToRadians(80.f) };
	static constexpr _float		LOCKON_DEADZONE_YAW		= { XMConvertToRadians(6.f) };
	static constexpr _float		LOCKON_DEADZONE_PITCH	= { XMConvertToRadians(4.f) };
	static constexpr _float		LOCKON_YAW_SPEED_MAX	= { XMConvertToRadians(180.f) };

	static constexpr _float		COL_DIST_MIN			= { 0.3f };
	static constexpr _float		COL_RECOVER_RATE		= { 8.f };
private:
	_float						m_fTargetOffset			= {};
	_float						m_fArmLength			= {};
	_float2						m_vArmRotLimits			= {};

	_float						m_fVerticalValue		= {};
	_float						m_fHorizonValue			= {};
	_float						m_fMouseSensor			= {};

	CameraPose					m_FinalCameraPose		= {};
	//Damp용 
	_float4						m_vCurrentVelocity		= {};
	_float4						m_vCurrentPos			= {};
	class CTransform*			m_pOwnerTransform		= { nullptr };

	_bool						m_bLockOn				= { false };
	_float3						m_vLockOnTargetPos		= {};

	//루트모션 Y값이 트랜스폼에 반영이 되지않아 따로 추적값 
	_float						m_fPivotExtraY			= {};
	_float						m_fCollisionDist		= { -1.f };

private:
	//Adjust용
	SPRING_ADJUST_DESC			m_AdjustDesc			= {};
	_bool						m_bAdjusting			= { false };
	_float						m_fAdjustAlpha			= {};

public:
	static CSpringArm*			Create();
	CComponent*					Clone(void* pArg) override;
	virtual void				Free() override;

};

NS_END
