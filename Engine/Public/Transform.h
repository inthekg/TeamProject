#pragma once

#include "Component.h"

#ifdef _DEBUG
#include "Engine_Reflection.h"
#endif

NS_BEGIN(Engine)

class ENGINE_DLL CTransform final : public CComponent
{
#ifdef _DEBUG
	DECLARE_REFLECTION_BASE(CTransform)
#endif
public:
	typedef struct tagTransformDesc
	{
		_float3		vScale = {1.f,1.f,1.f };
		_float4		vTranslate = {0.f,0.f ,0.f ,1.f};
		_float3		vRotationDegree = {0.f, 0.f, 0.f};

		_float		fSpeedPerSec = {};
		_float		fRotationPerSec = {};
	}TRANSFORM_DESC;

protected:
	CTransform(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTransform(const CTransform& Prototype);
	virtual ~CTransform() = default;

public:
	_vector	Get_State(STATE eState) 
	{
		return XMLoadFloat4(reinterpret_cast<_float4*>(&m_WorldMatrix.m[ETOUI(eState)]));
	}

	_float3 Get_Scaled() { return m_vScale; }

	void	Set_State(STATE eState, _fvector vState) 
	{
		//POSITION일때 vState의 마지막 속성이 0인 경우를 대비함 
		if (eState == STATE::POSITION)
		{
			_vector vTrans = XMVectorSetW(vState, 1.f);
			XMStoreFloat4(reinterpret_cast<_float4*>(&m_WorldMatrix.m[ETOUI(eState)]), vTrans);
		}
		else
		{
			XMStoreFloat4(reinterpret_cast<_float4*>(&m_WorldMatrix.m[ETOUI(eState)]), vState);
		}
	}

	const _float4x4* Get_WorldMatrixPtr() const 
	{
		return &m_WorldMatrix;
	}

	_vector	Get_Quat()
	{
		return XMLoadFloat4(&m_vQuat);
	}
	
	_float Get_RotationPerSec()
	{
		return m_fRotationPerSec;
	}

	void Set_RotationPerSec(_float fRad) { m_fRotationPerSec = fRad; }
public:
	virtual HRESULT			Initialize_Prototype();
	virtual HRESULT			Initialize(void* pArg);

public:

	void					Set_Quat(_fvector vQuat);
	void					Set_Scale(_float fScaleX = 1.f, _float fScaleY = 1.f, _float fScaleZ = 1.f);
	void					Set_Scale(const _float3& vScale);

#ifdef _DEBUG
	HRESULT					Setting_Scale(const _float3& vScale);
#endif // _DEBUG

	//디버깅 카메라에서만 사용 
	void					Go_Straight(_float fTimeDelta);
	void					Go_Backward(_float fTimeDelta);
	void					Go_Left(_float fTimeDelta);
	void					Go_Right(_float fTimeDelta);

public:
	void					Translate(_fvector vPosition);
	void					Scaling(_float fScaleX = 1.f, _float fScaleY = 1.f, _float fScaleZ = 1.f);
	//즉시 회전 
	void					Rotation(_fvector vAxis, _float fRadian);
	void					Rotation(_float fPitch, _float fYaw, _float fRoll);
	void					LookAt(_fvector vAt);

	//기존 회전상태에 누적해 회전 
	void					Turn(_fvector vAxis, _float fTimeDelta);
	void					Turn(_vector vNextQuat);
	//현재값 기준으로 남은 거리의 fRatio만큼 회전
	void					Smooth_Turn(_vector vNextQuat, _float fRatio);
	//매 프레임 회전속도 * fTimeDelta 만큼 회전
	void					LookAt(_fvector vAt, _float fTimeDelta);

private:
	_float4x4				m_WorldMatrix = {};
	_float					m_fSpeedPerSec = {};
	_float					m_fRotationPerSec = {};
private:
	//최종 Transform은 WorldMatrix. 계산의 편의성을 위해 현재 Quat, Scale값을 따로 보관함 
	_float4					m_vQuat = {};
	_float3					m_vScale = { 1.f, 1.f, 1.f };

public:
	virtual CComponent*		Clone(void* pArg) override;
	static CTransform*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void			Free() override;
};

NS_END