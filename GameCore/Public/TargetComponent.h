#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CGameObject;
class CTransform;
NS_END

NS_BEGIN(GameCore)
class CAIObject;

class GAMECORE_DLL CTargetComponent : public CComponent
{
public:
	typedef struct tagTargetDesc
	{
		CGameObject*	pOwner				= { nullptr };
		FILTER_GROUP	iMyFilter			= { FILTER_GROUP::END };
		FILTER_GROUP	iTargetFilter		= { FILTER_GROUP::END };
		_float			fFOV				= { XMConvertToRadians(160.f) };
		_float			fSearchRange		= { 0.f };
	} TARGET_DESC;
	
private:
	CTargetComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTargetComponent(const CTargetComponent& Prototype);
	virtual ~CTargetComponent() = default;	
public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta);	

public:
	CAIObject*					Find_LockOnTarget();
	CAIObject*					Find_FatalTarget();
	CAIObject*					Get_CurrentTarget() const { return m_pCurrentTarget; }
	_bool						Has_Target() const { return (m_pCurrentTarget != nullptr); }
	void						Clear_Target() { m_pCurrentTarget = nullptr; }

	void						Set_SearchRange(_float fRange) { m_fSearchRange = fRange; }

private:
	_bool						Has_LineOfSight(_fvector vFrom , _fvector vTo);
private:
	const _float				FATAL_DETECT_RANGE = { 2.5f };
	const _float				LOS_HEIGHT_OFFSET = { 1.2f };

private:
	CGameObject*				m_pOwner = { nullptr };
	CTransform*					m_pOwnerTransform = { nullptr };
	CAIObject*					m_pCurrentTarget = { nullptr };

	_float						m_fSearchRange = { 0.f };
	_float						m_fFOV = { 0.f };
	
	FILTER_GROUP				m_iMyFilterGroup = { FILTER_GROUP::END };
	FILTER_GROUP				m_iTargetFilterGroup = { FILTER_GROUP::END };

public:
	static CTargetComponent*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CComponent*					Clone(void* pArg) override;
	virtual void				Free() override;

};

NS_END