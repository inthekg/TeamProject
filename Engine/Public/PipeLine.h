#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CPipeLine final : public CBase
{
private:
	CPipeLine();
	virtual ~CPipeLine() = default;

public:
	const _float4x4* Get_Transform(D3DTS eState) const {
		return &m_TransformStateMatrices[ETOUI(eState)];
	}

	const _float4x4* Get_Transform_Inverse(D3DTS eState) const {
		return &m_TransformStateInverseMatrices[ETOUI(eState)];
	}

	const _float4* Get_CamPosition() const {
		return &m_vCamPosition;
	}

	const _float4* Get_CamRight() const
	{
		return &m_vCamRight;
	}

	const _float4* Get_CamLook() const
	{
		return &m_vCamLook;
	}

public:
	void Set_Transform(D3DTS eState, _fmatrix StateMatrix) 
	{
		XMStoreFloat4x4(&m_TransformStateMatrices[ETOUI(eState)], StateMatrix);
	}


public:		
	void Update();
	//셰이더 버퍼 업데이트용 
	void Fill_CameraData(PerCameraData& cameraData);

private:
	_float4x4				m_TransformStateMatrices[ETOUI(D3DTS::END)] = {};
	_float4x4				m_TransformStateInverseMatrices[ETOUI(D3DTS::END)] = {};

	_float4					m_vCamPosition = {};
	_float4					m_vCamRight = {};
	_float4					m_vCamLook = {};

public:
	static CPipeLine*		Create();
	virtual void			Free() override;
};

NS_END