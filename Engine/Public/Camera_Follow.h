#pragma once
#include "Camera.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCamera_Follow final : public CCamera
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CCamera_Follow)
#endif

public:
	typedef struct tagCameraFollowDesc final : public CCamera::CAMERA_DESC
	{		
		_float3		vOffset = { 0.6f, 1.7f, 3.f };
		_float		fLookAhead = { 3.f };
	}CAMERA_FOLLOW_DESC;

protected:
	CCamera_Follow(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCamera_Follow(const CCamera_Follow& Prototype);
	virtual ~CCamera_Follow() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public :
	void					Set_Target(class CGameObject* pTarget) { m_pTarget = pTarget; }
	HRESULT					Setting_Fovy(const _float& fFovy);

private :
	void					Follow_Target();

private :
	class CGameObject*		m_pTarget = { nullptr };
	_float3					m_vOffset = { 0.6f, 1.7f, 3.f };
	_float					m_fLookAhead = { 3.f };

public:
	static CCamera_Follow*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END