#pragma once
#include "Base.h"
#include "Map.h"
#include "ModelDataStruct.h"

NS_BEGIN(Engine)

//추가 
struct MeshEntryBase
{
	MeshPassData passData{};
	_wstring		strMeshName = {};
	_wstring		strMaterialName = {};
	_wstring		strMaterialPath = {};
};

struct StaticEntry : public MeshEntryBase
{
	_float4x4		worldMatrix = {};
};

struct InstancedEntry : public MeshEntryBase
{
	vector<MapInstanceData> instances;
	_wstring		strInstanceBufferName = {};
};

//Cooked Stream을 담을 구조체 
struct CookedMeshData
{
    vector<uint8_t> streamData;
};

enum class MAP_PHYSICS_GEOMETRY_TYPE : _uint
{
	TRIANGLE_MESH,
	BOX,
	END
};

enum class MAP_PHYSICS_ROLE : _uint
{
	GROUND,
	OBJECT,
	END
};


struct MapPhysicsBoxData
{
	_float3 vCenter = {};
	_float3 vExtents = {};
};

struct MapPhysicsGroupData
{
	_bool bInstanced = false;
	_uint iGroupIndex = UINT_MAX;
	MAP_PHYSICS_GEOMETRY_TYPE eGeometryType = MAP_PHYSICS_GEOMETRY_TYPE::END;
	MAP_PHYSICS_ROLE eRole = MAP_PHYSICS_ROLE::OBJECT;
	wchar_t szCookedMeshPath[MAX_PATH] = {};
};

struct MapObjectSourceData
{
	_wstring strMapName = {};
	_bool bInstanced = false;
	_uint iGroupIndex = UINT_MAX;
	_uint iInstanceIndex = UINT_MAX;
};

struct MapPhysicsSettingData
{
	MapObjectSourceData Source = {};
	MAP_PHYSICS_GEOMETRY_TYPE eGeometryType = MAP_PHYSICS_GEOMETRY_TYPE::END;
	MAP_PHYSICS_ROLE eRole = MAP_PHYSICS_ROLE::OBJECT;
};

struct MapPhysicsBuildGroup
{
	_bool bInstanced = false;
	_uint iGroupIndex = UINT_MAX;
	MAP_PHYSICS_GEOMETRY_TYPE eGeometryType = MAP_PHYSICS_GEOMETRY_TYPE::END;
	MAP_PHYSICS_ROLE eRole = MAP_PHYSICS_ROLE::OBJECT;
	MapPhysicsBoxData BoxData = {};
	_wstring strCookedMeshPath = {};
	vector<_float4x4> WorldMatrices = {};
};


class ENGINE_DLL CMap_Manager : public CBase
{

private:
	CMap_Manager(ID3D11Device* m_pDevice, ID3D11DeviceContext* m_pContext);
	virtual ~CMap_Manager() = default;
public:
	HRESULT							Load_Map(_wstring strMapName, CMap** pOutMap, _float fMapScale=1.f, _bool bCreateEditorRealObjects= false);
	HRESULT							Load_MapPhysics(const _wstring& strMapName);

	HRESULT							Load_HLODMap(const _wstring& strOriginalMapName, CMap** pOutMap);

	HRESULT							Load_MapPhysicsSettings(const _wstring strMapName);
	HRESULT							Save_MapPhysicsSettings(const _wstring& strMapName);

	HRESULT							Build_MapPhysics(const _wstring& strMapName); 
	void							Unload_MapPhysics(const _wstring& strMapName);
	// 로드된 맵 피직스 Actor는 유지한 채 충돌/query shape만 켜거나 끈다.
	void							Set_MapPhysicsActive(const _wstring& strMapName, _bool bActive);
	CMap*							Get_Map(_wstring strMapName);
	_wstring						Get_MapFolderPath(const _wstring& strMapName) const;
	_wstring						Get_MapRuntimeRelativePath(const _wstring& strMapName, const _wstring& strFileName) const;

	HRESULT							Read_MapMeshGroup(const _wstring& strMapName, _bool bInstanced, _uint iGroupIndex, MeshPassData* pOutPassData) const;
	_float4x4						Remove_MapScale(const _float4x4& WorldMatrix, _float fMapScale) const;

	HRESULT							Append_ManualPhysicsBuildGroups(const _wstring& strMapName, vector<MapPhysicsBuildGroup>* pGroups) const;


public:
	MAP_PHYSICS_GEOMETRY_TYPE		Get_MapPhysicsType(const MapObjectSourceData& Source) const;
	void							Set_MapPhysicsType(const MapObjectSourceData& Source, MAP_PHYSICS_GEOMETRY_TYPE eGeometryType);
	MAP_PHYSICS_ROLE				Get_MapPhysicsRole(const MapObjectSourceData& Source) const;
	void							Set_MapPhysicsRole(const MapObjectSourceData& Source, MAP_PHYSICS_ROLE eRole);


#ifdef _DEBUG
	_bool Find_MapObjectSource(const CMapObject* pMapObject, MapObjectSourceData* pOutSource) const;

	void Set_MapPhysicsDebugVisible(const _wstring& strMapName, _bool bVisible);
	void Set_MapPhysicsDebugCategory(_bool bOn);
#endif

private:
	CMap* 							Create_Map_FromFile(const _wstring& MapName, const _wstring& MaterialDir, const _wstring& MeshFilePath, _float fMapScale, _bool bCreateEditorRealObjects);
	HRESULT							Create_Mesh(MeshPassData& passData, _wstring strMeshName);
	HRESULT							Create_Material(_wstring& strMaterialFilePath);
	HRESULT							Create_MapInstanceBuffer(MeshPassData& passData, const vector<MapInstanceData>& instances, const _wstring& strBufferName);



	// 맵 크기 변경
	_float4x4						Apply_MapScale(const _float4x4& WorldMatrix, _fvector vScale) const;



    //HRESULT						Ready_MapMesh(CMap* pMap, const _wstring& MeshFilePath);
    //HRESULT						Ready_CookedMesh(const string& PureMeshFilePath,const string& CookingMeshFilPath);
    //HRESULT						Ready_ColliderMesh(CMap* pMap, const string& CookingMeshFilPath);


private:
	ID3D11Device*					m_pDevice = { nullptr };
	ID3D11DeviceContext*			m_pContext = { nullptr };
	class CGameInstance*			m_pGameInstance = { nullptr };

private:
	unordered_map<_wstring, CMap*>	m_Maps;
	unordered_map<_wstring, _float> m_MapScales;
	unordered_map<_wstring, vector<MapPhysicsSettingData>> m_MapPhysicsSettings;
	unordered_map<_wstring, vector<PxRigidStatic*>> m_MapPhysicsActors;

#ifdef _DEBUG
	_bool							m_bDebugOn = { true };
	unordered_map<_wstring, _bool>	m_DebugSectorVisible;
#endif 
public:
	static CMap_Manager*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void					Free() override;
};

NS_END
