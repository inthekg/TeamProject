#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)
class CShader;
class CMaterial;
class CVIBuffer_Rect;

class ENGINE_DLL CSprite : public CGameObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CSprite)
#endif

public:
	typedef struct tagSpriteDesc : public CGameObject::GAMEOBJECT_DESC
	{
		_wstring strMaterialName;
	}SPRITE_DESC;

protected:
	CSprite(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CSprite(const CSprite& Prototype);
	virtual ~CSprite() = default;

public:
	virtual HRESULT         Initialize_Prototype() override;
	virtual HRESULT         Initialize(void* pArg) override;
	virtual void            Priority_Update(_float fTimeDelta) override;
	virtual void            Update(_float fTimeDelta) override;
	virtual void            Physical_Update(_float fFixedTime) override;
	virtual void            Late_Update(_float fTimeDelta) override;
	virtual HRESULT         Render();

public:
	HRESULT					Set_Material(_wstring strMaterialName);
protected:
	HRESULT					Ready_Components();

protected:
	CVIBuffer_Rect*			m_pVIBufferCom = { nullptr };
	CShader*				m_pShaderCom = { nullptr };
	CMaterial*				m_pMaterialCom = { nullptr };

#ifdef _DEBUG
protected :
	_wstring				m_strMaterialName = { };
#endif

public:
	static CSprite*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg);
	virtual void            Free();

};

NS_END