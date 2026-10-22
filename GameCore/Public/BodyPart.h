#pragma once
#include "Core_Defines.h"
#include "SkeletalObject.h"
#include "IItemable.h"

NS_BEGIN(Engine)
class CMasterModel;
NS_END

NS_BEGIN(GameCore)
class GAMECORE_DLL CBodyPart : public CSkeletalObject, public IItemable
{
public:
	typedef struct tagBodyPartDesc 
	{
		CMasterModel*		pMasterModel;
		const _float4x4*	pParentMatrix;
		_uint				iLevelIndex;
		_wstring			strModelTag;
		_bool				bAlphaClip = { false };
	}BODYPART_DESC;

protected:
	CBodyPart(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBodyPart(const CBodyPart& Prototype);
	virtual ~CBodyPart() = default;	

protected:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;

protected:
	virtual HRESULT			Ready_Components() override;
	
public:
	virtual HRESULT			Render() override;
	virtual HRESULT			Render_Shadow() override;
	void                    Set_Dissolve(_float fAmount, const _float4& vColor);

protected:
	_bool					m_bVisible = { true };
	_bool					m_bAlphaClip = { false };
	class CMasterModel*		m_pMasterModel = {};
	const _float4x4*		m_pParentMatrix = {};
	_uint					m_iLevelIndex = {};
	_wstring				m_strModelTag = {};
	_float                  m_fDissolveAmount = { 0.f };
	_float4                 m_vDissolveColor = { 0.f, 0.f, 0.f, 0.f };

public:
	static CBodyPart*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END