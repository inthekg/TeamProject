#include "Material.h"
#include "Shader_Resolver.h"

unordered_map<string, CMaterial::FieldSlot> CMaterial::g_MaterialBufferLayout;
_uint CMaterial::g_MaterialStride = {0};

CMaterial::CMaterial(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CResource{pDevice, pContext}
{

}
/* GPU의 Material 구조체가 달라졌다면 Mateiral Layout도 변경하기. 두 구조체가 완전 동일해야함 */
void CMaterial::Ready_MaterialLayout()
{
	if (!g_MaterialBufferLayout.empty()) return;

	g_MaterialStride = sizeof(MaterialData);

	g_MaterialBufferLayout["Roughness"]				= { offsetof(MaterialData, Roughness), sizeof(_float) };
	g_MaterialBufferLayout["Roughness_Factor"]		= { offsetof(MaterialData, Roughness_Factor), sizeof(_float) };
	g_MaterialBufferLayout["Emissive"]				= { offsetof(MaterialData, Emissive), sizeof(_float) };
	g_MaterialBufferLayout["Specular"]				= { offsetof(MaterialData, Specular), sizeof(_float) };
	g_MaterialBufferLayout["AO_Pow"]				= { offsetof(MaterialData, AO_Pow), sizeof(_float) };
	g_MaterialBufferLayout["Metallic_Factor"]		= { offsetof(MaterialData, Metallic_Factor), sizeof(_float) };
	g_MaterialBufferLayout["RefractionDepthBias"]	= { offsetof(MaterialData, RefractionDepthBias), sizeof(_float) };
	g_MaterialBufferLayout["EmissiveMode"]			= { offsetof(MaterialData, EmissiveMode), sizeof(_float) };

	g_MaterialBufferLayout["EmissiveColor"]			= { offsetof(MaterialData, EmissiveColor), sizeof(_float4) };
	g_MaterialBufferLayout["BaseColor_Tint"]		= { offsetof(MaterialData, BaseColor_Tint), sizeof(_float4) };
	g_MaterialBufferLayout["GlassColor"]			= { offsetof(MaterialData, GlassColor), sizeof(_float4) };
	g_MaterialBufferLayout["DirtyColor"]			= { offsetof(MaterialData, DirtyColor), sizeof(_float4) };
	g_MaterialBufferLayout["Emissive_Factor"]		= { offsetof(MaterialData, EmissiveColor), sizeof(_float4) };
	g_MaterialBufferLayout["EmissiveEyeColor"]		= { offsetof(MaterialData, EmissiveEyeColor), sizeof(_float4) };
	g_MaterialBufferLayout["BossPhase_Emis_Color_NeckOut"]	= { offsetof(MaterialData, EmissiveColor), sizeof(_float4) };
	g_MaterialBufferLayout["BossPhase_Emis_Color_EyeLight"] = { offsetof(MaterialData, EmissiveEyeColor), sizeof(_float4) };
}

HRESULT CMaterial::Initialize()
{
	return S_OK;
}


void CMaterial::Set_Meta_Desc(const Material_Meta& tMeta)
{
	m_tMeta = tMeta;
	m_eRenderGroup = CShader_Resolver::Resolve_Group(tMeta);
}

HRESULT CMaterial::Input_Texture(TEXTURE_TYPE type, CTexture* pTexture)
{
	if (type == TEXTURE_TYPE::END || pTexture == nullptr) return E_FAIL;

	auto& vecTextures = m_Textures[ETOUI(type)];
	if (std::find(vecTextures.begin(), vecTextures.end(), pTexture) != vecTextures.end())
		return S_OK;

	Safe_AddRef(pTexture);
	m_Textures[ETOUI(type)].emplace_back(pTexture);

	return S_OK;
}

void CMaterial::Input_Scalar(const string& strName, _float fValue)
{
	m_ScalarProperties.emplace(strName, fValue);	
	auto iter = g_MaterialBufferLayout.find(strName);
	if (iter == g_MaterialBufferLayout.end() || iter->second.iSize != sizeof(_float)) return;

	BYTE* pMaterialData = reinterpret_cast<BYTE*>(&m_MaterialData);
	*reinterpret_cast<_float*>(pMaterialData + iter->second.iOffset) = fValue;

}

void CMaterial::Input_Color(const string& strName, _float4 vColor)
{
	m_ColorProperties.emplace(strName, vColor);
	auto iter = g_MaterialBufferLayout.find(strName);
	if (iter == g_MaterialBufferLayout.end() || iter->second.iSize != sizeof(_float4)) return;

	BYTE* pMaterialData = reinterpret_cast<BYTE*>(&m_MaterialData);
	*reinterpret_cast<_float4*>(pMaterialData + iter->second.iOffset) = vColor;
}

void CMaterial::Set_ReflectionInfo(const unordered_map<string, Shader_ReflectionData>& reflectionMap, _uint iBufferSize)
{
	if (m_bReflectionReady) return;
	m_MaterialBuffer_Reflection = reflectionMap;
	m_iMaterialBufferSize = iBufferSize;

	if (iBufferSize > 0)
		m_bReflectionReady = true;
}

void CMaterial::Write_MaterialParams(void* pBuffer)
{
	BYTE* pByte = static_cast<BYTE*>(pBuffer);
	memset(pByte, 0, m_iMaterialBufferSize);

	for (auto iter = m_MaterialBuffer_Reflection.begin();
		iter != m_MaterialBuffer_Reflection.end(); ++iter)
	{
		const string& strName = iter->first;
		const Shader_ReflectionData& tRefl = iter->second;

		if (tRefl.eValueType == SHADER_VALUE_TYPE::SCALAR)
		{
			auto itScalar = m_ScalarProperties.find(strName);
			if (itScalar != m_ScalarProperties.end())
				*reinterpret_cast<_float*>(pByte + tRefl.iOffset) = itScalar->second;
		}
		else if (tRefl.eValueType == SHADER_VALUE_TYPE::COLOR)
		{
			auto itColor = m_ColorProperties.find(strName);
			if (itColor != m_ColorProperties.end())
				*reinterpret_cast<_float4*>(pByte + tRefl.iOffset) = itColor->second;
		}
	}
}

_int CMaterial::Get_OpacityMaskSource() const
{
	if (m_tMeta.eBlendMode != BLEND_MODE::BLEND_MASKED)
		return 0;
	if (false == m_Textures[ETOUI(TEXTURE_TYPE::MASK2)].empty())
		return 2;
	if (false == m_Textures[ETOUI(TEXTURE_TYPE::MASK1)].empty())
		return 1;
	return 0;
}

CMaterial* CMaterial::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMaterial* pInstance = new CMaterial(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CMaterial");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMaterial::Free()
{
	__super::Free();

	for (_uint i = 0; i < TEXTURE_TYPE_MAX; i++)
	{
		for (auto typeInfo : m_Textures[i])
			Safe_Release(typeInfo);
		m_Textures[i].clear();
	}
}
