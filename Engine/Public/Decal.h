#pragma once
#include "PoolableObject.h"
#include "Effect_Define.h"

NS_BEGIN(Engine)

class CVIBuffer_Cube;
struct EMITTER_SHADER_BUNDLE;
class CMaterial;
class CTexture;

class ENGINE_DLL CDecal final : public CPoolableObject
{
private:
	CDecal(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CDecal() = default;

public:
	virtual HRESULT	Initialize(void* pArg) override;
	virtual void	Activate(void* pArg = nullptr) override;
	virtual void	InActivate() override;
	virtual void	Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

public:
	void		Bind_SharedShaders(const EMITTER_SHADER_BUNDLE* pBundle) { m_pBundle = pBundle; }
	void		Set_Desc(const DecalDesc& tDesc);
	void		Set_World(const _float4x4& matWorld) { m_matWorld = matWorld; }
	void		Set_GlobalAlpha(_float fAlpha) { m_fGlobalAlpha = fAlpha; }
	_int		Get_SortPriority() const { return m_tDesc.iSortPriority; }

private:
	void		Update_Texture(const string& strPath, string& strCache, CTexture** ppOutTex);
	CTexture*	Load_Cached_Texture(const wstring& strWPath);

private:
	CVIBuffer_Cube*					m_pBox = { nullptr };
	const EMITTER_SHADER_BUNDLE*	m_pBundle = { nullptr };
	ID3D11Buffer*					m_pDecalCB = { nullptr };

	_float4x4						m_matWorld;
	_float							m_fGlobalAlpha = { 1.f };
	DecalDesc						m_tDesc;

	CTexture*						m_pAlbedoTex = { nullptr };
	CTexture*						m_pNormalTex = { nullptr };
	CTexture*						m_pORMSTex = { nullptr };
	CTexture*						m_pEmissiveTex = { nullptr };
	CTexture*						m_pMaskTex = { nullptr };

	string							m_strAlbedoPath;
	string							m_strNormalPath;
	string							m_strORMSPath;
	string							m_strEmissivePath;
	string							m_strMaskPath;

public:
	static CDecal*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override { return nullptr; }
	virtual void			Free() override;
};

NS_END