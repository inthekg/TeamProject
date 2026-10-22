#pragma once
#include "GameObject.h"

//맵툴에서 메시 정보를 가지고 생성할 오브젝트. 
NS_BEGIN(Engine)
class CShader;
class CMaterial;
class CMesh;

class ENGINE_DLL CMapObject : public CGameObject
{
public:
	//TODO : 0608 피킹용 ksb 
	struct MapPickedRenderInfo
	{
		_wstring strMaterialName;
		_wstring strShaderName;

		vector<pair<TEXTURE_TYPE, _wstring>> Textures;

		_int iBlendMode = -1; // 아직 CMaterial에 BlendMode 없음. 나중에 교체.
	};

protected:
	CMapObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContexts);
	CMapObject(const CMapObject& Prototype);
	virtual ~CMapObject() = default;

public:
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;


public:
	HRESULT					Set_Material(wstring MaterialName);
	HRESULT					Set_Mesh(wstring MeshName);
	HRESULT					Set_Transform(_fmatrix fModelMatrix);
	void					Set_LocalBounds(const BoundingBox& LocalBounds) { m_LocalBounds = LocalBounds; }
	const BoundingBox&		Get_LocalBounds() const { return m_LocalBounds; }
	virtual	BoundingBox		Get_WorldBounds() const;


	void					Set_MapObjectName(const _wstring& strName) { Set_Name(strName); }

#ifdef _DEBUG
	HRESULT					Add_Material(wstring MaterialName);
#endif

	//TODO : 0608 피킹시 머테리얼 shader 정보 가지고 오기 위해서 
public:
	CMaterial*						Get_Material() const { return m_pMaterialcom; }
	virtual MapPickedRenderInfo		Get_PickedRenderInfo() const;

#ifdef _DEBUG
	CMaterial* Get_DebugMaterial(_uint iMaterialIndex) const { return iMaterialIndex < m_DebugMaterialComs.size() ? m_DebugMaterialComs[iMaterialIndex] : nullptr; }
	_uint							Get_DebugMaterialCount() const { return static_cast<_uint>(m_DebugMaterialComs.size()); }
	virtual MapPickedRenderInfo		Get_PickedRenderInfo(_uint iMaterialIndex) const;
#endif

protected:
	virtual HRESULT			Ready_Components();

protected:
	CMaterial*				m_pMaterialcom	= { nullptr };
	CMesh*					m_pMeshCom		= { nullptr };
	CShader*				m_pShaderCom = { nullptr };
	BoundingBox             m_LocalBounds = {};

#ifdef _DEBUG
	vector<CMaterial*>		m_DebugMaterialComs = {};
#endif

public:
	static CMapObject*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	static CMapObject*		Create_Copy(const CMapObject& Source);
	virtual void			Free();
};

NS_END
