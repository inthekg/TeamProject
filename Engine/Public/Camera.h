#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCamera abstract : public CGameObject
{
public:
	typedef struct tagCameraDesc : public CGameObject::GAMEOBJECT_DESC
	{
		_float3			vEye, vAt;
		_float			fFovy, fNear, fFar;
	}CAMERA_DESC;

protected:
	CCamera(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCamera(const CCamera& Prototype);
	virtual ~CCamera() = default;

public:
	virtual HRESULT			Initialize_Prototype();
	virtual HRESULT			Initialize(void* pArg);
	virtual void			Priority_Update(_float fTimeDelta);
	virtual void			Update(_float fTimeDelta);
	virtual void			Late_Update(_float fTimeDelta);
	virtual HRESULT			Render();

public:
	_float					Get_Fovy() const { return m_fFovy; }
	void					Set_Fovy(_float fFovy);
protected:
	virtual void			Update_PipeLine();

protected:
	_float					m_fFovy, m_fAspect, m_fNear, m_fFar;
	_float4x4				m_ProjMatrix = {};
	class CPipeLine*		m_pPipeLine = { nullptr };


public:
	virtual CGameObject*	Clone(void* pArg) = 0;
	virtual void			Free() override;
};

NS_END