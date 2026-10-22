#pragma once
#include "PartObject.h"

NS_BEGIN(Engine)
class CModel;
class CAnimator;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CShooter final : public CPartObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CShooter)
#endif

public:
	typedef struct tagShooterDesc : public CPartObject::PARTOBJECT_DESC
	{
		_wstring	strModelProtoTag	= {};
		_wstring	strAnimFolderTag	= { TEXT("Shooter") };
		_uint		iModelLevel			= {};
		_float3		vOffset				= {};
		_float3		vScale				= { 1.f, 1.f, 1.f };
	} SHOOTER_DESC;

private:
	CShooter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CShooter(const CShooter& Prototype);
	virtual ~CShooter() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;
	virtual HRESULT			Render_Shadow() override;

private:
	HRESULT					Ready_Model(const SHOOTER_DESC& Desc);

private:
	CModel*					m_pModelCom			= { nullptr };
	CAnimator*				m_pAnimator			= { nullptr };
	_float3					m_vOffset			= {};
	_float3					m_vScale			= { 1.f, 1.f, 1.f };
	_float					m_fDbgTimer			= { 0.f };

public:
	static CShooter*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END
