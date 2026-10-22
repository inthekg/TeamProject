#pragma once

#include "Resource.h"
#include "Texture.h"

NS_BEGIN(Engine)

class ENGINE_DLL CMaterial final : public CResource
{
public:
	struct FieldSlot
	{
		_uint iOffset;
		_uint iSize;
	};
private:
	CMaterial(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CMaterial() = default;

private:
	static unordered_map<string, FieldSlot> g_MaterialBufferLayout;
	static _uint g_MaterialStride;
public:
	static void							Ready_MaterialLayout();
	static _uint						Get_Stride() { return g_MaterialStride; }

public:
	HRESULT								Initialize();

	vector<CTexture*>*					Get_Textures(TEXTURE_TYPE type)
	{
		if (type == TEXTURE_TYPE::END) return nullptr;
		return &m_Textures[ETOUI(type)];
	}

	ID3D11ShaderResourceView*			Get_SRV(TEXTURE_TYPE type, _uint index)
	{
		if ((_uint)m_Textures[ETOUI(type)].size() <= index || m_Textures[ETOUI(type)][index] == nullptr) return nullptr;
		return m_Textures[ETOUI(type)][index]->Get_SRV();
	}

	const MaterialData&					Get_MaterialData() const { return m_MaterialData; }
	const Material_Meta&				Get_Meta_Desc() const { return m_tMeta; }
	void								Set_Meta_Desc(const Material_Meta& tMeta);

	RENDERID							Get_RenderGroup() const { return m_eRenderGroup; }
	void								Set_RenderGroup(RENDERID eRenderGroup) { m_eRenderGroup = eRenderGroup; }

public:
	HRESULT								Input_Texture(TEXTURE_TYPE type, CTexture* pTexture	);
	void								Input_Scalar(const string& strName, _float fValue);
	void								Input_Color(const string& strName, _float4 vColor);

	void								Set_ReflectionInfo(const unordered_map<string, Shader_ReflectionData>& reflectionMap, _uint iBufferSize);
	void								Write_MaterialParams(void* pBuffer);
	bool								Has_MaterialBuffer() const { return m_iMaterialBufferSize > 0; }

	_int								Get_OpacityMaskSource() const;
	

private:
	vector<CTexture*>								m_Textures[TEXTURE_TYPE_MAX];

private:
	unordered_map<string, _float>					m_ScalarProperties;
	unordered_map<string, _float4>					m_ColorProperties;

	MaterialData									m_MaterialData = {};
	Material_Meta									m_tMeta = {};
	RENDERID										m_eRenderGroup = { RENDERID::NONBLEND };

	unordered_map<string, Shader_ReflectionData>	m_MaterialBuffer_Reflection;
	_uint											m_iMaterialBufferSize = { 0 };
	bool											m_bReflectionReady = { false };

public:
	static CMaterial*					Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void						Free() override;
};

NS_END
