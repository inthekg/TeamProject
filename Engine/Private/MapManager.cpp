#include "MapManager.h"
#include "GameInstance.h"
#include "ModelDataStruct.h"
#include "MapObject.h"
#include "MapInstanceObject.h"
#include "VIBuffer_MapInstance.h"
#include "Engine_Utility.h"
#include "Material_Parser.h"
#include "ThreadPool.h"


static _wstring Find_MapGroupFolder(const _wstring& strMapName)
{
    if (0 == strMapName.find(L"LV_Hotel_"))
        return L"Hotel";

    if (0 == strMapName.find(L"LV_Krat_Outer_") || 0 == strMapName.find(L"LV_Outer_"))
        return L"Chapter01";

    if (0 == strMapName.find(L"LV_Monastery_"))
        return L"Chapter02";

    if (0 == strMapName.find(L"Test_object"))
        return L"Test_object";

    if (0 == strMapName.find(L"Test"))
        return L"Test";

    return L"";
}

static _wstring Normalize_MapRelativePath(const _wstring& strMapName)
{
    _wstring strResult = strMapName;
    replace(strResult.begin(), strResult.end(), L'/', L'\\');

    while (false == strResult.empty() && L'\\' == strResult.front())
        strResult.erase(strResult.begin());

    while (false == strResult.empty() && L'\\' == strResult.back())
        strResult.pop_back();

    return strResult;
}

static _bool Starts_WithMapGroupFolder(const _wstring& strMapName, const _wstring& strGroupFolder)
{
    return false == strGroupFolder.empty() && strMapName.size() > strGroupFolder.size() && 0 == strMapName.find(strGroupFolder) && L'\\' == strMapName[strGroupFolder.size()];
}

static _wstring Get_MapLeafName(const _wstring& strMapName)
{
    const size_t iSlash = strMapName.find_last_of(L'\\');
    return _wstring::npos == iSlash ? strMapName : strMapName.substr(iSlash + 1);
}

static BoundingBox Make_LocalBounds_FromMeshPassData(const MeshPassData& passData)
{
    BoundingBox localBounds{};

    if (passData.NoAnimVertices.empty())
        return localBounds;

    _vector vMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 1.f);
    _vector vMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 1.f);

    for (const VTXMESH& vertex : passData.NoAnimVertices)
    {
        _vector vPosition = XMLoadFloat3(&vertex.vPosition);

        vMin = XMVectorMin(vMin, vPosition);
        vMax = XMVectorMax(vMax, vPosition);
    }

    BoundingBox::CreateFromPoints(localBounds, vMin, vMax);
    return localBounds;
}

CMap_Manager::CMap_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:	m_pDevice(pDevice), m_pContext(pContext), m_pGameInstance{CGameInstance::Get_Instance()}
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);

}

_wstring CMap_Manager::Get_MapFolderPath(const _wstring& strMapName) const
{
    const _wstring strNormalizedMapName = Normalize_MapRelativePath(strMapName);
    const _wstring strGroupFolder = Find_MapGroupFolder(Get_MapLeafName(strNormalizedMapName));

    if (strGroupFolder.empty())
        return m_pGameInstance->Get_ResourcePath() + L"\\Map\\" + strNormalizedMapName;

    if (Starts_WithMapGroupFolder(strNormalizedMapName, strGroupFolder))
        return m_pGameInstance->Get_ResourcePath() + L"\\Map\\" + strNormalizedMapName;

    return m_pGameInstance->Get_ResourcePath() + L"\\Map\\" + strGroupFolder + L"\\" + strNormalizedMapName;
}

_wstring CMap_Manager::Get_MapRuntimeRelativePath(const _wstring& strMapName, const _wstring& strFileName) const
{
    const _wstring strNormalizedMapName = Normalize_MapRelativePath(strMapName);
    const _wstring strGroupFolder = Find_MapGroupFolder(Get_MapLeafName(strNormalizedMapName));

    if (strGroupFolder.empty())
        return L"Map\\" + strNormalizedMapName + L"\\" + strFileName;

    if (Starts_WithMapGroupFolder(strNormalizedMapName, strGroupFolder))
        return L"Map\\" + strNormalizedMapName + L"\\" + strFileName;

    return L"Map\\" + strGroupFolder + L"\\" + strNormalizedMapName + L"\\" + strFileName;
}


HRESULT CMap_Manager::Load_Map(_wstring strMapName, CMap** pOutMap, _float fMapScale, _bool bCreateEditorRealObjects )
{
    if (nullptr == pOutMap)
        return E_FAIL;

    auto it = m_Maps.find(strMapName);
    if (it != m_Maps.end())
    {
        *pOutMap = it->second;
        return S_OK;
    }

    const _wstring strResourceDir = Get_MapFolderPath(strMapName);
    const _wstring strMeshPath = strResourceDir + L"\\" + strMapName + GlobalFileRoute::g_strMeshFileTag;
    const _wstring strMaterialDir = strResourceDir + L"\\" + GlobalFileRoute::g_strMaterialFolder;

    CMap * pMap = Create_Map_FromFile(strMapName, strMaterialDir, strMeshPath,fMapScale, bCreateEditorRealObjects);

    if (pMap == nullptr)
    {
        if (false == m_pGameInstance->Is_LoadingCancled())
            MSG_BOX("MapManager : Create Map Failed");
        return E_FAIL;
    }

    m_Maps.emplace(strMapName, pMap);
    m_MapScales.emplace(strMapName, fMapScale);
    *pOutMap = pMap;
	return S_OK;
}

HRESULT CMap_Manager::Load_MapPhysics(const _wstring& strMapName)
{
    if (strMapName.empty() || nullptr == m_pGameInstance || nullptr == m_pGameInstance->Get_CurrentPxScene())
        return E_FAIL;

    if (m_MapPhysicsActors.end() != m_MapPhysicsActors.find(strMapName))
        return S_OK;

    const _wstring strMapFolder = Get_MapFolderPath(strMapName);
    const _wstring strFilePath = strMapFolder + L"\\" + strMapName + L"_MapPhysics.bin";
    ifstream File(strFilePath, ios::binary);

    if (false == File.is_open())
        return S_FALSE;

    auto ReadFile = [&File](void* pData, streamsize iSize) -> _bool { if (0 == iSize) return true; File.read(reinterpret_cast<char*>(pData), iSize); return File.gcount() == iSize; };

    vector<PxRigidStatic*> Actors{};

    auto ReleaseActors = [&Actors]() { for (PxRigidStatic* pActor : Actors) if (nullptr != pActor) pActor->release(); Actors.clear(); };

    auto LoadFailed = [&ReleaseActors]() -> HRESULT { MSG_BOX("Map Physics Binary Load Failed"); ReleaseActors(); return E_FAIL; };

    _uint iGroupCount = 0;

    if (false == ReadFile(&iGroupCount, sizeof(_uint)))
        return LoadFailed();

    const auto ScaleIter = m_MapScales.find(strMapName);
    const _float fMapScale = m_MapScales.end() != ScaleIter ? ScaleIter->second : 1.f;

    for (_uint iGroup = 0; iGroup < iGroupCount; ++iGroup)
    {
        MapPhysicsGroupData GroupData{};
        MapPhysicsBoxData BoxData{};
        _uint iInstanceCount = 0;

        if (false == ReadFile(&GroupData, sizeof(MapPhysicsGroupData)))
            return LoadFailed();

        if (MAP_PHYSICS_GEOMETRY_TYPE::BOX == GroupData.eGeometryType && false == ReadFile(&BoxData, sizeof(MapPhysicsBoxData)))
            return LoadFailed();

        if (false == ReadFile(&iInstanceCount, sizeof(_uint)))
            return LoadFailed();

        vector<_float4x4> WorldMatrices(iInstanceCount);

        if (false == ReadFile(WorldMatrices.data(), sizeof(_float4x4) * iInstanceCount))
            return LoadFailed();

        PxTriangleMesh* pTriangleMesh = nullptr;

        if (MAP_PHYSICS_GEOMETRY_TYPE::TRIANGLE_MESH == GroupData.eGeometryType)
        {
            GroupData.szCookedMeshPath[MAX_PATH - 1] = L'\0';

            const _wstring strCookedMeshPath = GroupData.szCookedMeshPath;

            if (strCookedMeshPath.empty())
                return LoadFailed();

            const _wstring strCookedPath = m_pGameInstance->Get_ResourcePath() + L"\\" + strCookedMeshPath;
            ifstream CookedFile(strCookedPath, ios::binary | ios::ate);

            if (false == CookedFile.is_open())
                return LoadFailed();

            const streamsize iFileSize = CookedFile.tellg();

            if (iFileSize <= 0)
                return LoadFailed();

            CookedFile.seekg(0, ios::beg);

            CookedMeshData CookedData{};
            CookedData.streamData.resize(static_cast<size_t>(iFileSize));
            CookedFile.read(reinterpret_cast<char*>(CookedData.streamData.data()), iFileSize);

            if (CookedFile.gcount() != iFileSize)
                return LoadFailed();

            PxDefaultMemoryInputData InputData(CookedData.streamData.data(), static_cast<PxU32>(CookedData.streamData.size()));
            pTriangleMesh = m_pGameInstance->Create_TriangleMesh(InputData);

            if (nullptr == pTriangleMesh)
                return LoadFailed();
        }
        else if (MAP_PHYSICS_GEOMETRY_TYPE::BOX != GroupData.eGeometryType)
        {
            return LoadFailed();
        }

        for (const _float4x4& StoredWorldMatrix : WorldMatrices)
        {
            _float4x4 WorldMatrix = Apply_MapScale(StoredWorldMatrix, XMVectorSet(fMapScale, fMapScale, fMapScale, 0.f));
            _vector vScale{}, vRotation{}, vTranslation{};

            if (false == XMMatrixDecompose(&vScale, &vRotation, &vTranslation, XMLoadFloat4x4(&WorldMatrix)))
            {
                if (nullptr != pTriangleMesh)
                    pTriangleMesh->release();

                return LoadFailed();
            }

            _float3 Scale{};
            XMStoreFloat3(&Scale, vScale);

            PxShape* pShape = nullptr;

            if (MAP_PHYSICS_GEOMETRY_TYPE::BOX == GroupData.eGeometryType)
            {
                PxBoxGeometry Geometry(fabsf(BoxData.vExtents.x * Scale.x), fabsf(BoxData.vExtents.y * Scale.y), fabsf(BoxData.vExtents.z * Scale.z));

                if (false == Geometry.isValid())
                    return LoadFailed();

                pShape = m_pGameInstance->Create_PxShape(Geometry, m_pGameInstance->Get_Default_Material(), true);

                if (nullptr != pShape)
                    pShape->setLocalPose(PxTransform(PxVec3(BoxData.vCenter.x * Scale.x, BoxData.vCenter.y * Scale.y, BoxData.vCenter.z * Scale.z)));
            }
            else
            {
                PxTriangleMeshGeometry Geometry(pTriangleMesh, PxMeshScale(PxVec3(Scale.x, Scale.y, Scale.z)));

                if (false == Geometry.isValid())
                {
                    pTriangleMesh->release();
                    return LoadFailed();
                }

                pShape = m_pGameInstance->Create_PxShape(Geometry, m_pGameInstance->Get_Default_Material(), true);
            }

            PxRigidStatic* pActor = m_pGameInstance->Create_RigidStatic(vTranslation, vRotation);

            if (nullptr == pShape || nullptr == pActor)
            {
                if (nullptr != pShape)
                    pShape->release();

                if (nullptr != pActor)
                    pActor->release();

                if (nullptr != pTriangleMesh)
                    pTriangleMesh->release();

                return LoadFailed();
            }

            //추가 
            PxFilterData FilterData;
            FilterData.word0 = MAP_PHYSICS_ROLE::OBJECT == GroupData.eRole ? ETOUI(FILTER_GROUP::OBJECT) : ETOUI(FILTER_GROUP::GROUND);  // 지형 그룹
            FilterData.word1 = ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER); // 지형 그룹
            pShape->setSimulationFilterData(FilterData);
            pShape->setQueryFilterData(FilterData);


            if (false == pActor->attachShape(*pShape))
            {
                pShape->release();
                pActor->release();

                if (nullptr != pTriangleMesh)
                    pTriangleMesh->release();

                return LoadFailed();
            }

            pShape->release();
            Actors.push_back(pActor);
        }

        if (nullptr != pTriangleMesh)
            pTriangleMesh->release();
    }

    // 로더 스레드에서는 PxScene에 쓰기 작업을 하지 않고 Actor만 보관한다.
    m_MapPhysicsActors.emplace(strMapName, move(Actors));
    return S_OK;
}

HRESULT CMap_Manager::Load_HLODMap(const _wstring& strOriginalMapName, CMap** pOutMap)
{
    if (strOriginalMapName.empty() || nullptr == pOutMap)
        return E_FAIL;

    *pOutMap = nullptr;

    // 현재 HLOD 리소스 규칙은 Original Map 이름 뒤에 _HLOD를 붙이는 방식임
    const _wstring strHLODMapName = strOriginalMapName + L"_HLOD";

    // 이미 같은 HLOD가 로드되어 있으면 기존 CMap을 반환함
    auto MapIter = m_Maps.find(strHLODMapName);

    if (m_Maps.end() != MapIter)
    {
        *pOutMap = MapIter->second;
        return S_OK;
    }

    // 기존 Map 이름 규칙을 그대로 사용해서 Chapter 폴더를 결정함
    const _wstring strMapGroupFolder = Find_MapGroupFolder(strOriginalMapName);

    // 현재 그룹 규칙으로 분류되지 않는 Map은 HLOD 미지원으로 처리함
    if (strMapGroupFolder.empty())
        return S_FALSE;

    const _wstring strHLODResourceDir = m_pGameInstance->Get_ResourcePath() + L"\\Map\\" + strMapGroupFolder + L"\\HLOD\\" + strHLODMapName;
    const _wstring strHLODMeshPath = strHLODResourceDir + L"\\" + strHLODMapName + GlobalFileRoute::g_strMeshFileTag;
    const _wstring strHLODMaterialDir = strHLODResourceDir + L"\\" + GlobalFileRoute::g_strMaterialFolder;

    // HLOD Mesh가 없으면 오류가 아니라 해당 Map의 HLOD가 없는 정상 상태임
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(strHLODMeshPath.c_str()))
        return S_FALSE;

    // 현재 HLOD 월드 행렬은 Scale 1에서 Original 월드 위치와 일치함
    CMap* pHLODMap = Create_Map_FromFile(strHLODMapName, strHLODMaterialDir, strHLODMeshPath, 1.f, false);

    // 파일이 존재하는데 CMap 생성이 실패했다면 실제 리소스 로드 오류임
    if (nullptr == pHLODMap)
    {
        if (false == m_pGameInstance->Is_LoadingCancled())
            MSG_BOX("MapManager : Create HLOD Map Failed");

        return E_FAIL;
    }

    // HLOD CMap도 기존 MapManager가 소유하고 Free에서 함께 해제함
    auto InsertResult = m_Maps.emplace(strHLODMapName, pHLODMap);

    if (false == InsertResult.second)
    {
        // 예상하지 못한 중복이 발생했다면 새 CMap을 정리하고 기존 CMap을 사용함
        Safe_Release(pHLODMap);
        *pOutMap = InsertResult.first->second;
        return S_OK;
    }

    // HLOD에는 Physics Scale이 필요 없으므로 m_MapScales에는 추가하지 않음
    *pOutMap = pHLODMap;

    return S_OK;
}

HRESULT CMap_Manager::Load_MapPhysicsSettings(const _wstring strMapName)
{
    CMap* pMap = Get_Map(strMapName);

    if (strMapName.empty() || nullptr == pMap)
        return E_FAIL;

    vector<MapPhysicsSettingData>& Settings = m_MapPhysicsSettings[strMapName];
    Settings.clear();

    const _wstring strMapFolder = Get_MapFolderPath(strMapName);
    const _wstring strFilePath = strMapFolder + L"\\" + strMapName + L"_MapPhysicsEditor.json";
    ifstream File(strFilePath);

    if (false == File.is_open())
        return S_FALSE;

    json Root{};

    try
    {
        File >> Root;
    }
    catch (const json::exception&)
    {
        MSG_BOX("Map Physics Settings Read Failed");
        return E_FAIL;
    }

    if (false == Root.contains("Objects") || false == Root["Objects"].is_array())
    {
        MSG_BOX("Invalid Map Physics Settings Format");
        return E_FAIL;
    }

    const vector<CMapObject*>& Objects = pMap->Get_Objects();

    for (const json& ObjectData : Root["Objects"])
    {
        try
        {
            if (false == ObjectData.is_object())
                continue;

            if (false == ObjectData.contains("Instanced") || false == ObjectData["Instanced"].is_boolean())
                continue;

            if (false == ObjectData.contains("GroupIndex") || false == ObjectData["GroupIndex"].is_number_unsigned())
                continue;

            if (false == ObjectData.contains("GeometryType") || false == ObjectData["GeometryType"].is_number_unsigned())
                continue;

            MapPhysicsSettingData Setting{};
            Setting.Source.strMapName = strMapName;
            Setting.Source.bInstanced = ObjectData["Instanced"].get<_bool>();
            Setting.Source.iGroupIndex = ObjectData["GroupIndex"].get<_uint>();
            Setting.Source.iInstanceIndex = UINT_MAX;

            if (Setting.Source.bInstanced)
            {
                if (false == ObjectData.contains("InstanceIndex") || false == ObjectData["InstanceIndex"].is_number_unsigned())
                    continue;

                Setting.Source.iInstanceIndex = ObjectData["InstanceIndex"].get<_uint>();
            }

            const _uint iGeometryType = ObjectData["GeometryType"].get<_uint>();

            if (iGeometryType >= static_cast<_uint>(MAP_PHYSICS_GEOMETRY_TYPE::END))
                continue;

            Setting.eGeometryType = static_cast<MAP_PHYSICS_GEOMETRY_TYPE>(iGeometryType);
            Setting.eRole = MAP_PHYSICS_ROLE::OBJECT;

            if (ObjectData.contains("PhysicsRole") && ObjectData["PhysicsRole"].is_number_unsigned())
            {
                const _uint iPhysicsRole = ObjectData["PhysicsRole"].get<_uint>();

                if (iPhysicsRole < static_cast<_uint>(MAP_PHYSICS_ROLE::END))
                    Setting.eRole = static_cast<MAP_PHYSICS_ROLE>(iPhysicsRole);
            }

            CMapObject* pMapObject = nullptr;
            _uint iStaticGroupIndex = 0;
            _uint iInstanceGroupIndex = 0;

            for (CMapObject* pObject : Objects)
            {
                if (nullptr == pObject)
                    continue;

                CMapInstanceObject* pInstanceOwner = dynamic_cast<CMapInstanceObject*>(pObject);

                if (nullptr == pInstanceOwner)
                {
                    if (false == Setting.Source.bInstanced && iStaticGroupIndex == Setting.Source.iGroupIndex)
                    {
                        pMapObject = pObject;
                        break;
                    }

                    ++iStaticGroupIndex;
                    continue;
                }

                if (Setting.Source.bInstanced && iInstanceGroupIndex == Setting.Source.iGroupIndex)
                {
                    if (pInstanceOwner->Is_InActive())
                        break;

                    pMapObject = pInstanceOwner->Get_InstanceObject(Setting.Source.iInstanceIndex);
                    break;
                }

                ++iInstanceGroupIndex;
            }

            if (nullptr == pMapObject || pMapObject->Is_InActive())
                continue;

            Settings.push_back(move(Setting));
        }
        catch (const json::exception&)
        {
            continue;
        }
    }

    return S_OK;
}

HRESULT CMap_Manager::Save_MapPhysicsSettings(const _wstring& strMapName)
{
    CMap* pMap = Get_Map(strMapName);

    if (strMapName.empty() || nullptr == pMap)
        return E_FAIL;

    json Root{};
    Root["Objects"] = json::array();

    auto SettingsIter = m_MapPhysicsSettings.find(strMapName);

    if (m_MapPhysicsSettings.end() != SettingsIter)
    {
        const vector<CMapObject*>& Objects = pMap->Get_Objects();

        for (const MapPhysicsSettingData& Setting : SettingsIter->second)
        {
            CMapObject* pMapObject = nullptr;

            if (false == Setting.Source.bInstanced)
            {
                _uint iStaticGroupIndex = 0;

                for (CMapObject* pObject : Objects)
                {
                    if (nullptr == pObject || nullptr != dynamic_cast<CMapInstanceObject*>(pObject))
                        continue;

                    if (iStaticGroupIndex == Setting.Source.iGroupIndex)
                    {
                        pMapObject = pObject;
                        break;
                    }

                    ++iStaticGroupIndex;
                }
            }
            else
            {
                _uint iInstanceGroupIndex = 0;

                for (CMapObject* pObject : Objects)
                {
                    CMapInstanceObject* pInstanceOwner = dynamic_cast<CMapInstanceObject*>(pObject);

                    if (nullptr == pInstanceOwner)
                        continue;

                    if (iInstanceGroupIndex == Setting.Source.iGroupIndex)
                    {
                        if (pInstanceOwner->Is_InActive())
                            break;

                        pMapObject = pInstanceOwner->Get_InstanceObject(Setting.Source.iInstanceIndex);
                        break;
                    }

                    ++iInstanceGroupIndex;
                }
            }

            if (nullptr == pMapObject || pMapObject->Is_InActive())
                continue;

            json ObjectData{};
            ObjectData["Instanced"] = Setting.Source.bInstanced;
            ObjectData["GroupIndex"] = Setting.Source.iGroupIndex;
            ObjectData["InstanceIndex"] = Setting.Source.bInstanced ? static_cast<_int>(Setting.Source.iInstanceIndex) : -1;
            ObjectData["GeometryType"] = static_cast<_uint>(Setting.eGeometryType);
            ObjectData["PhysicsRole"] = static_cast<_uint>(Setting.eRole); // 분기 정보 
            Root["Objects"].push_back(move(ObjectData));
        }
    }
    const _wstring strMapFolder = Get_MapFolderPath(strMapName);
    const _wstring strFilePath = strMapFolder + L"\\" + strMapName + L"_MapPhysicsEditor.json";
    ofstream File(strFilePath, ios::trunc);

    if (false == File.is_open())
        return E_FAIL;

    File << Root.dump(2);
    File.close();

    if (File.fail())
        return E_FAIL;

    return S_OK;

}

HRESULT CMap_Manager::Build_MapPhysics(const _wstring& strMapName)
{
    CMap* pMap = Get_Map(strMapName);

    if (strMapName.empty() || nullptr == pMap)
        return E_FAIL;

    if (FAILED(Save_MapPhysicsSettings(strMapName)))
        return E_FAIL;

    const vector<CMapObject*>& Objects = pMap->Get_Objects();
    vector<MapPhysicsBuildGroup> Groups{};
    auto SettingsIter = m_MapPhysicsSettings.find(strMapName);
    const _float fMapScale = m_MapScales.end() != m_MapScales.find(strMapName) ? m_MapScales.at(strMapName) : 1.f;

    if (m_MapPhysicsSettings.end() != SettingsIter)
    {
        for (const MapPhysicsSettingData& Setting : SettingsIter->second)
        {
            CMapObject* pMapObject = nullptr;

            if (false == Setting.Source.bInstanced)
            {
                _uint iStaticGroupIndex = 0;

                for (CMapObject* pObject : Objects)
                {
                    if (nullptr == pObject || nullptr != dynamic_cast<CMapInstanceObject*>(pObject))
                        continue;

                    if (iStaticGroupIndex == Setting.Source.iGroupIndex)
                    {
                        pMapObject = pObject;
                        break;
                    }

                    ++iStaticGroupIndex;
                }
            }
            else
            {
                _uint iInstanceGroupIndex = 0;

                for (CMapObject* pObject : Objects)
                {
                    CMapInstanceObject* pInstanceOwner = dynamic_cast<CMapInstanceObject*>(pObject);

                    if (nullptr == pInstanceOwner)
                        continue;

                    if (iInstanceGroupIndex == Setting.Source.iGroupIndex)
                    {
                        if (pInstanceOwner->Is_InActive())
                            break;

                        pMapObject = pInstanceOwner->Get_InstanceObject(Setting.Source.iInstanceIndex);
                        break;
                    }
                    ++iInstanceGroupIndex;
                }
            }

            if (nullptr == pMapObject || pMapObject->Is_InActive())
                continue;

            auto GroupIter = find_if(Groups.begin(), Groups.end(), [&Setting](const MapPhysicsBuildGroup& Group)
                {
                    return  Group.bInstanced == Setting.Source.bInstanced &&
                        Group.iGroupIndex == Setting.Source.iGroupIndex &&
                        Group.eGeometryType == Setting.eGeometryType &&
                        Group.eRole == Setting.eRole;
                });

            if (Groups.end() == GroupIter)
            {
                MapPhysicsBuildGroup Group{};
                Group.bInstanced = Setting.Source.bInstanced;
                Group.iGroupIndex = Setting.Source.iGroupIndex;
                Group.eGeometryType = Setting.eGeometryType;
                Group.eRole = Setting.eRole;

                if (MAP_PHYSICS_GEOMETRY_TYPE::BOX == Setting.eGeometryType)
                {
                    const BoundingBox& Bounds = pMapObject->Get_LocalBounds();
                    Group.BoxData.vCenter = Bounds.Center;
                    Group.BoxData.vExtents = Bounds.Extents;
                }

                Groups.push_back(move(Group));
                GroupIter = Groups.end() - 1;
            }

            const _float4x4* pWorldMatrix = pMapObject->Get_WorldTransform();

            if (nullptr == pWorldMatrix)
                return E_FAIL;

            GroupIter->WorldMatrices.push_back(Remove_MapScale(*pWorldMatrix, fMapScale));
        }

    }

    if (FAILED(Append_ManualPhysicsBuildGroups(strMapName, &Groups)))
        return E_FAIL;

    const _wstring strMapFolder = Get_MapFolderPath(strMapName);
    const _wstring strPhysicsPath = strMapFolder + L"\\" + strMapName + L"_MapPhysics.bin";

    if (Groups.empty())
    {
        if (false == DeleteFileW(strPhysicsPath.c_str()))
        {
            const DWORD dwError = GetLastError();

            if (ERROR_FILE_NOT_FOUND != dwError && ERROR_PATH_NOT_FOUND != dwError)
                return E_FAIL;
        }

        WIN32_FIND_DATAW FindData{};
        HANDLE hFind = FindFirstFileW((strMapFolder + L"\\" + strMapName + L"_MapPhysics_*_CookingMesh.data").c_str(), &FindData);

        if (INVALID_HANDLE_VALUE != hFind)
        {
            do
            {
                const _wstring strCookedPath = strMapFolder + L"\\" + FindData.cFileName;

                if (false == DeleteFileW(strCookedPath.c_str()))
                {
                    FindClose(hFind);
                    return E_FAIL;
                }
            } while (FindNextFileW(hFind, &FindData));

            const DWORD dwError = GetLastError();
            FindClose(hFind);

            if (ERROR_NO_MORE_FILES != dwError)
                return E_FAIL;
        }
        else
        {
            const DWORD dwError = GetLastError();

            if (ERROR_FILE_NOT_FOUND != dwError && ERROR_PATH_NOT_FOUND != dwError)
                return E_FAIL;
        }

        return S_OK;
    }

    vector<_wstring> UsedCookedPaths{};

    for (MapPhysicsBuildGroup& Group : Groups)
    {
        if (MAP_PHYSICS_GEOMETRY_TYPE::TRIANGLE_MESH != Group.eGeometryType)
            continue;

        MeshPassData PassData{};

        if (FAILED(Read_MapMeshGroup(strMapName, Group.bInstanced, Group.iGroupIndex, &PassData)))
            return E_FAIL;

        vector<PxVec3> Positions{};
        Positions.reserve(PassData.NoAnimVertices.size());

        for (const VTXMESH& Vertex : PassData.NoAnimVertices)
            Positions.emplace_back(Vertex.vPosition.x, Vertex.vPosition.y, Vertex.vPosition.z);

        PxTriangleMeshDesc MeshDesc{};
        MeshDesc.points.count = static_cast<PxU32>(Positions.size());
        MeshDesc.points.stride = sizeof(PxVec3);
        MeshDesc.points.data = Positions.data();
        MeshDesc.triangles.count = static_cast<PxU32>(PassData.Indices.size() / 3);
        MeshDesc.triangles.stride = sizeof(_uint) * 3;
        MeshDesc.triangles.data = PassData.Indices.data();

        if (false == MeshDesc.isValid())
            return E_FAIL;

        PxDefaultMemoryOutputStream WriteBuffer{};

        if (false == m_pGameInstance->Cook_TriangleMesh(MeshDesc, WriteBuffer))
            return E_FAIL;

        const _wstring strGroupType = Group.bInstanced ? L"Instance" : L"Static";
        const _wstring strCookedName = strMapName + L"_MapPhysics_" + strGroupType + L"_" + to_wstring(Group.iGroupIndex) + L"_CookingMesh.data";
        const _wstring strCookedPath = strMapFolder + L"\\" + strCookedName;

        ofstream CookedFile(strCookedPath, ios::binary | ios::trunc);

        if (false == CookedFile.is_open())
            return E_FAIL;

        CookedFile.write(reinterpret_cast<const char*>(WriteBuffer.getData()), WriteBuffer.getSize());
        CookedFile.close();

        if (CookedFile.fail())
            return E_FAIL;

        Group.strCookedMeshPath = Get_MapRuntimeRelativePath(strMapName, strCookedName);
        UsedCookedPaths.push_back(strCookedPath);
    }

    ofstream PhysicsFile(strPhysicsPath, ios::binary | ios::trunc);

    if (false == PhysicsFile.is_open())
        return E_FAIL;

    const _uint iGroupCount = static_cast<_uint>(Groups.size());
    PhysicsFile.write(reinterpret_cast<const char*>(&iGroupCount), sizeof(_uint));

    for (const MapPhysicsBuildGroup& Group : Groups)
    {
        MapPhysicsGroupData GroupData{};
        GroupData.bInstanced = Group.bInstanced;
        GroupData.iGroupIndex = Group.iGroupIndex;
        GroupData.eGeometryType = Group.eGeometryType;
        GroupData.eRole = Group.eRole;

        if (false == Group.strCookedMeshPath.empty())
            wcscpy_s(GroupData.szCookedMeshPath, MAX_PATH, Group.strCookedMeshPath.c_str());

        PhysicsFile.write(reinterpret_cast<const char*>(&GroupData), sizeof(MapPhysicsGroupData));

        if (MAP_PHYSICS_GEOMETRY_TYPE::BOX == Group.eGeometryType)
            PhysicsFile.write(reinterpret_cast<const char*>(&Group.BoxData), sizeof(MapPhysicsBoxData));

        const _uint iInstanceCount = static_cast<_uint>(Group.WorldMatrices.size());
        PhysicsFile.write(reinterpret_cast<const char*>(&iInstanceCount), sizeof(_uint));

        for (const _float4x4& WorldMatrix : Group.WorldMatrices)
            PhysicsFile.write(reinterpret_cast<const char*>(&WorldMatrix), sizeof(_float4x4));
    }

    PhysicsFile.close();

    if (PhysicsFile.fail())
        return E_FAIL;

    WIN32_FIND_DATAW FindData{};
    HANDLE hFind = FindFirstFileW((strMapFolder + L"\\" + strMapName + L"_MapPhysics_*_CookingMesh.data").c_str(), &FindData);

    if (INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            const _wstring strCookedPath = strMapFolder + L"\\" + FindData.cFileName;

            if (UsedCookedPaths.end() != find(UsedCookedPaths.begin(), UsedCookedPaths.end(), strCookedPath))
                continue;

            if (false == DeleteFileW(strCookedPath.c_str()))
            {
                FindClose(hFind);
                return E_FAIL;
            }
        } while (FindNextFileW(hFind, &FindData));

        const DWORD dwError = GetLastError();
        FindClose(hFind);

        if (ERROR_NO_MORE_FILES != dwError)
            return E_FAIL;
    }
    else
    {
        const DWORD dwError = GetLastError();

        if (ERROR_FILE_NOT_FOUND != dwError && ERROR_PATH_NOT_FOUND != dwError)
            return E_FAIL;
    }

    return S_OK;
}

void CMap_Manager::Unload_MapPhysics(const _wstring& strMapName)
{
    auto Iter = m_MapPhysicsActors.find(strMapName);

    if (m_MapPhysicsActors.end() == Iter)
        return;

    for (PxRigidStatic* pActor : Iter->second)
    {
        if (nullptr == pActor)
            continue;

        if (nullptr != pActor->getScene())
            m_pGameInstance->Remove_ActorFromScene(pActor);

        pActor->release();
    }

    m_MapPhysicsActors.erase(Iter);
}

void CMap_Manager::Set_MapPhysicsActive(const _wstring& strMapName, _bool bActive)
{
    auto Iter = m_MapPhysicsActors.find(strMapName);

    if (m_MapPhysicsActors.end() == Iter)
        return;

    for (PxRigidStatic* pActor : Iter->second)
    {
        if (nullptr == pActor)
            continue;

        const PxU32 iShapeCount = pActor->getNbShapes();

        if (0 != iShapeCount)
        {
            vector<PxShape*> Shapes(iShapeCount);
            const PxU32 iReadCount = pActor->getShapes(Shapes.data(), iShapeCount);

            for (PxU32 i = 0; i < iReadCount; ++i)
            {
                if (nullptr == Shapes[i])
                    continue;

                // Actor를 다시 Scene에 넣을 때 shape 상태도 같이 복구되도록 flag를 맞춰 둔다.
                Shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, bActive);
                Shapes[i]->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, bActive);
                Shapes[i]->setFlag(PxShapeFlag::eVISUALIZATION, bActive);
            }
        }

        const _bool bActorInScene = nullptr != pActor->getScene();

        if (bActive && false == bActorInScene)
        {
            // 활성 섹터 피직스만 PhysX Scene에 넣어 디버그 렌더와 충돌 대상에 포함한다.
            m_pGameInstance->Add_ActorToScene(pActor);
        }
        else if (false == bActive && bActorInScene)
        {
            // 비활성 섹터 피직스는 Actor 자체를 Scene에서 빼서 디버그/충돌/쿼리 대상에서 모두 제외한다.
            m_pGameInstance->Remove_ActorFromScene(pActor);
        }
    }
}

CMap* CMap_Manager::Get_Map(_wstring strMapName)
{
    auto iter = m_Maps.find(strMapName);
    if (iter != m_Maps.end()) return iter->second;
    return nullptr;
}

//기존 로직은 그대로. 리소스 로딩만 멀티스레딩 되게 수정함 
CMap* CMap_Manager::Create_Map_FromFile(const _wstring& MapName, const _wstring& MaterialDir, const _wstring& MeshFilePath, _float fMapScale, _bool bCreateEditorRealObjects)
{
    // 맵 크기 조절
    const _bool bApplyMapScale = fabsf(fMapScale - 1.f) > FLT_EPSILON;
    const _vector vMapScale = XMVectorSet(fMapScale, fMapScale, fMapScale, 0.f);
    CMap* pMap = CMap::Create(m_pDevice, m_pContext);

    ifstream fin(MeshFilePath, ios::binary);
    if (!fin.is_open())
    {
        MSG_BOX("Read Mesh Bone Failed");
        Safe_Release(pMap);
        Safe_Release(pMap);
        return nullptr;
    }

    auto Read_File = [&fin](void* pData, streamsize iSize) -> _bool
        {
            if (0 == iSize)
                return true;

            fin.read(reinterpret_cast<char*>(pData), iSize);
            return fin.gcount() == iSize;
        };

    /* 1. Parse : 파일을 읽어서 생성없이 CPU 데이터만 적재함 ------------------------------------------------ */

    if (m_pGameInstance->Is_LoadingCancled())
    {
        Safe_Release(pMap);
        return nullptr;
    }
    
    _uint staticMeshCount = {};
    if (false == Read_File(&staticMeshCount, sizeof(_uint)))
    {
        MSG_BOX("MapManager : Static Mesh Count Read Failed");
        Safe_Release(pMap);
        return nullptr;
    }

    //크기만큼 세팅함. 이후 push_back 절대 금지
    vector<StaticEntry> StaticEntries(staticMeshCount);

    // Static Mesh 개수만큼 읽기
    for (_uint i = 0; i < staticMeshCount; ++i)
    {
        StaticEntry& entry = StaticEntries[i];

        // Mesh Header 읽기
        MeshData header{};
        fin.read(reinterpret_cast<char*>(&header), sizeof(MeshData));

		//Entry 세팅
        strcpy_s(entry.passData.name, header.meshName);
        strcpy_s(entry.passData.materialName, header.materialName);
        entry.passData.hasAnimation = false;
        entry.passData.vertexCount = header.vertexCount;
        entry.passData.indexCount = header.indexCount;
        entry.passData.vertexStride = header.vertexStride;
        entry.passData.boneCount = header.boneCount;

        // Vertex 데이터 읽기
        entry.passData.NoAnimVertices.resize(header.vertexCount);
        fin.read(reinterpret_cast<char*>(entry.passData.NoAnimVertices.data()), sizeof(VTXMESH) * header.vertexCount);

        // Index 데이터 읽기
        entry.passData.Indices.resize(header.indexCount);
        fin.read(reinterpret_cast<char*>(entry.passData.Indices.data()), sizeof(_uint) * header.indexCount);

        // Mesh 월드 행렬 읽기
        fin.read(reinterpret_cast<char*>(&entry.worldMatrix), sizeof(_float4x4));

        // Mesh 이름 / Material 이름 / Material 경로 생성
        entry.strMeshName = CharArrayToWstring(header.meshName);
        entry.strMaterialName = CharArrayToWstring(header.materialName);
        entry.strMaterialPath = MaterialDir + entry.strMaterialName + GlobalFileRoute::g_strMaterialFileTag;

    }

    _uint instancedMeshGroupCount = {};
    if (false == Read_File(&instancedMeshGroupCount, sizeof(_uint)))
    {
        MSG_BOX("MapManager : Instanced Mesh Group Count Read Failed");
        Safe_Release(pMap);
        return nullptr;
    }

    vector<InstancedEntry> InstancedEntries(instancedMeshGroupCount);

    // Instanced Mesh Group 개수만큼 읽기
    for (_uint i = 0; i < instancedMeshGroupCount; ++i)
    {
        InstancedEntry& entry = InstancedEntries[i];

        // 인스턴스 메시 헤더를 읽는다.
        MeshData header{};
        fin.read(reinterpret_cast<char*>(&header), sizeof(MeshData));

        // 인스턴스 메시 데이터를 설정한다.
        strcpy_s(entry.passData.name, header.meshName);
        strcpy_s(entry.passData.materialName, header.materialName);
        entry.passData.hasAnimation = false;
        entry.passData.vertexCount = header.vertexCount;
        entry.passData.indexCount = header.indexCount;
        entry.passData.vertexStride = header.vertexStride;
        entry.passData.boneCount = header.boneCount;

        // 정점 데이터를 읽는다.
        entry.passData.NoAnimVertices.resize(header.vertexCount);
        fin.read(reinterpret_cast<char*>(entry.passData.NoAnimVertices.data()), sizeof(VTXMESH) * header.vertexCount);

        // 인덱스 데이터를 읽는다.
        entry.passData.Indices.resize(header.indexCount);
        fin.read(reinterpret_cast<char*>(entry.passData.Indices.data()), sizeof(_uint) * header.indexCount);

        // 인스턴스 개수를 읽는다.
        _uint instanceCount = {};
        fin.read(reinterpret_cast<char*>(&instanceCount), sizeof(_uint));

        // 개별 인스턴스의 이름과 월드 행렬을 읽는다.
        entry.instances.resize(instanceCount);
        fin.read(reinterpret_cast<char*>(entry.instances.data()), sizeof(MapInstanceData) * instanceCount);

        if (true == bApplyMapScale)
        {
            // GPU 버퍼와 컬링 캐시가 같은 스케일 행렬을 사용하도록 미리 적용한다.
            for (MapInstanceData& Instance : entry.instances)
                Instance.worldMatrix = Apply_MapScale(Instance.worldMatrix, vMapScale);
        }

        // 메시 및 재질 이름을 설정한다.
        entry.strMeshName = CharArrayToWstring(header.meshName);
        entry.strMaterialName = CharArrayToWstring(header.materialName);
        entry.strMaterialPath = MaterialDir + entry.strMaterialName + GlobalFileRoute::g_strMaterialFileTag;

        // 그룹마다 서로 다른 인스턴스 버퍼 이름을 생성한다.
        entry.strInstanceBufferName = MapName + L"_" + entry.strMeshName + L"_" + entry.strMaterialName + L"_MapInst_" + to_wstring(i);
    }
    fin.close();


    
    // IO 완료
	//고유 리소스 수집. 고유 이름당 Job 1개씩만 생성하도록 함

    unordered_map<_wstring, MeshPassData*> UniqueMeshes;
    unordered_map<_wstring, _wstring> UniqueMaterials;

    for (StaticEntry& entry : StaticEntries)
    {
        UniqueMeshes.try_emplace(entry.strMeshName, &entry.passData);
        UniqueMaterials.try_emplace(entry.strMaterialName, entry.strMaterialPath);
    }
    for (InstancedEntry& entry : InstancedEntries)
    {
        UniqueMeshes.try_emplace(entry.strMeshName, &entry.passData);
        UniqueMaterials.try_emplace(entry.strMaterialName, entry.strMaterialPath);
    }


	// 2. Create : 고유 리소스만 생성함. 오브젝트는 아직 생성 X. 비중이 큰 이 구간만 threadPool사용 -----------------------------------

    if (m_pGameInstance->Is_LoadingCancled())
    {
        Safe_Release(pMap);
        return nullptr;
    }

    CThreadPool* pPool = m_pGameInstance->Get_ThreadPool();
    if(pPool == nullptr)
    {
        MSG_BOX("Thread Pool is nullptr");
        Safe_Release(pMap);
        return nullptr;
	}

    CLoadBatch Batch;

    for (auto& Pair : UniqueMaterials)
    {
        _wstring strPath = Pair.second;
        pPool->Enqueue(&Batch, [this, strPath]() ->HRESULT
            {
				_wstring strMaterialPath = strPath;
                return Create_Material(strMaterialPath);
			});
    }

    for (auto& Pair : UniqueMeshes)
    {
        MeshPassData* pData = Pair.second;
        _wstring strMeshName = Pair.first;
        pPool->Enqueue(&Batch, [this, pData, strMeshName]() ->HRESULT
            {
                return Create_Mesh(*pData, strMeshName);
            });
    }

    for (InstancedEntry& entry : InstancedEntries)
    {
        InstancedEntry* pEntry = &entry;
        pPool->Enqueue(&Batch, [this, pEntry]() ->HRESULT
            {
                return Create_MapInstanceBuffer(pEntry->passData, pEntry->instances, pEntry->strInstanceBufferName);
            });
    }

    //로더스레드 대기 
    Batch.Wait();

    if (Batch.IsFailed())
    {
        MSG_BOX("MapManager : Failed to create map resources");
        Safe_Release(pMap);
		return nullptr;
    }

	// 3. Create MapObjects : 오브젝트 생성 -----------------------------------

    if (m_pGameInstance->Is_LoadingCancled())
    {
        Safe_Release(pMap);
        return nullptr;
    }

	//Static MapObjects
    for (_uint i = 0; i < staticMeshCount; ++i)
    {
        StaticEntry& entry = StaticEntries[i];

        // MapObject 생성
        CMapObject* pMapObject = CMapObject::Create(m_pDevice, m_pContext);
        if (nullptr == pMapObject)
        {
            Safe_Release(pMap);
            return nullptr;
        }

        // Mesh / Material / Transform / 바운딩 박스 세팅
        pMapObject->Set_MapObjectName(entry.strMeshName + L"_" + L"_InstGroup_" + to_wstring(i));
        pMapObject->Set_Mesh(entry.strMeshName);
        pMapObject->Set_Material(entry.strMaterialName);
        entry.worldMatrix = Apply_MapScale(entry.worldMatrix, vMapScale);
        pMapObject->Set_Transform(XMLoadFloat4x4(&entry.worldMatrix));
        pMapObject->Set_LocalBounds(Make_LocalBounds_FromMeshPassData(entry.passData));

        // Map에 Static MapObject 추가
        pMap->Push_MapObject(pMapObject);
    }

    // Instanced MapObject
    for (_uint i = 0; i < instancedMeshGroupCount; ++i)
    {
        InstancedEntry& entry = InstancedEntries[i];

        // 인스턴스 그룹을 대표하는 오너 객체 하나만 생성한다.
        CMapInstanceObject* pInstanceObject = CMapInstanceObject::Create(m_pDevice, m_pContext);

        if (nullptr == pInstanceObject)
        {
            Safe_Release(pMap);
            return nullptr;
        }

        const BoundingBox LocalBounds = Make_LocalBounds_FromMeshPassData(entry.passData);

        pInstanceObject->Set_MapObjectName(entry.strMeshName + L"_" + L"_InstGroup_" + to_wstring(i));
        pInstanceObject->Set_InstanceBuffer(entry.strInstanceBufferName);
        pInstanceObject->Set_Material(entry.strMaterialName);
        pInstanceObject->Set_LocalBounds(LocalBounds);

        // 맵 파일의 인스턴스 행렬로 런타임 렌더링 및 컬링 캐시를 구성한다.
        if (FAILED(pInstanceObject->Initialize_InstanceData(entry.instances)))
        {
            Safe_Release(pInstanceObject);
            Safe_Release(pMap);
            return nullptr;
        }

        // Editor_Map이 명시적으로 요청한 경우에만 개별 편집용 실객체를 생성한다.
        if (true == bCreateEditorRealObjects)
        {
            for (_uint iInstance = 0; iInstance < static_cast<_uint>(entry.instances.size()); ++iInstance)
            {
                const MapInstanceData& Instance = entry.instances[iInstance];
                CMapObject* pRealObject = CMapObject::Create(m_pDevice, m_pContext);

                if (nullptr == pRealObject)
                {
                    Safe_Release(pInstanceObject);
                    Safe_Release(pMap);
                    return nullptr;
                }

                // 에디터 선택, Transform 편집 및 물리 설정 저장에 필요한 실객체를 구성한다.
                pRealObject->Set_MapObjectName(entry.strMeshName + L"_" + L"_InstGroup_" + to_wstring(i));
                pRealObject->Set_Mesh(entry.strMeshName);
                pRealObject->Set_Material(entry.strMaterialName);
                pRealObject->Set_Transform(XMLoadFloat4x4(&Instance.worldMatrix));
                pRealObject->Set_LocalBounds(LocalBounds);

                if (FAILED(pInstanceObject->Add_RealObject(pRealObject)))
                {
                    Safe_Release(pRealObject);
                    Safe_Release(pInstanceObject);
                    Safe_Release(pMap);
                    return nullptr;
                }
            }
        }

        // Map에는 인스턴스 그룹 오너만 등록한다.
        pMap->Push_MapObject(pInstanceObject);
    }
    return pMap;
}



HRESULT CMap_Manager::Create_Mesh(MeshPassData& passData, _wstring strMeshName)
{
    CVIBuffer* pBuffer = { nullptr };
    m_pGameInstance->Get_VIBuffer(strMeshName, &pBuffer);

    if (pBuffer == nullptr)
    {
        //없으면 생성 
        CMesh* pMesh = CMesh::Create(m_pDevice, m_pContext, passData);
        if (pMesh == nullptr)
        {
            MSG_BOX("Mesh Create Fail!");
            return E_FAIL;
        }
        pMesh->Set_Name(strMeshName);
        m_pGameInstance->Register_VIBuffer(pMesh);
    }
    else Safe_Release(pBuffer);

    return S_OK;
}


HRESULT CMap_Manager::Create_Material(_wstring& strMaterialFilePath)
{
    _wstring MaterialName = Get_FileNameFromPath(strMaterialFilePath);
    wchar_t szMaterialDrive[MAX_PATH] = {};
    wchar_t szMaterialDir[MAX_PATH] = {};
    _wsplitpath_s(strMaterialFilePath.c_str(), szMaterialDrive, MAX_PATH, szMaterialDir, MAX_PATH, nullptr, 0, nullptr, 0);
    const _wstring strMaterialDirectory = _wstring(szMaterialDrive) + _wstring(szMaterialDir);

    CMaterial* pMaterial = { nullptr };
    m_pGameInstance->Get_Material(MaterialName,&pMaterial);
    if (pMaterial != nullptr)
    {
        Safe_Release(pMaterial);
        return S_OK;
    }

    ifstream file(strMaterialFilePath);
    if (!file.is_open())
    {
        MSG_BOX("Material File Open Fail");
        return E_FAIL;
    }

    json matJson;
    file >> matJson;



    pMaterial = CMaterial::Create(m_pDevice, m_pContext);
    if (pMaterial == nullptr) return E_FAIL;

    pMaterial->Set_Name(MaterialName);

    if (matJson.contains("Textures"))
    {
        for (auto& item : matJson["Textures"].items())
        {
            //TEXTURE_TYPE type = MapKeyToTextureType(item.key());
            if (!item.value().is_string()) continue;
            TEXTURE_TYPE type = MapKeyToTextureType(item.key(), item.value().get<string>());

            wstring strTextureOriginPath = StringToWString(item.value().get<string>());
            wstring strTextureName = Get_FileNameFromPath(strTextureOriginPath);
            //TODO : 모든 텍스쳐는 DDS라고 가정함
            wstring strTexturePath = m_pGameInstance->Get_ResourcePath() + L"/Textures/" + strTextureName + L".dds";

            if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(strTexturePath.c_str()))
            {
                const _wstring strLocalDDSPath = strMaterialDirectory + strTextureName + L".dds";
                const _wstring strLocalPNGPath = strMaterialDirectory + strTextureName + L".png";

                if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(strLocalDDSPath.c_str()))
                    strTexturePath = strLocalDDSPath;
                else if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(strLocalPNGPath.c_str()))
                    strTexturePath = strLocalPNGPath;
            }

            CTexture* pTexture = nullptr;

            m_pGameInstance->Get_Texture(strTextureName, &pTexture);

            if (pTexture == nullptr)
            {
                pTexture = CTexture::Create(m_pDevice, m_pContext, &strTextureName, &strTexturePath);
                if (pTexture == nullptr) continue;

                if (FAILED(m_pGameInstance->Register_Texture(pTexture)))
                {
                    Safe_Release(pTexture);

                    //만약 다른 스레드에서 먼저 등록했다면 가져오기 시도 
                    m_pGameInstance->Get_Texture(strTextureName, &pTexture);
                    if (pTexture == nullptr) continue;

                    pMaterial->Input_Texture(type, pTexture);
                    //Get으로 증가된 Texture Ref 감소 
                    Safe_Release(pTexture);
                    continue;
                }
                pMaterial->Input_Texture(type, pTexture);
            }
            else
            {
                pMaterial->Input_Texture(type, pTexture);
                //Get으로 증가된 Texture Ref 감소 
                Safe_Release(pTexture);
            }
        }
    }

    if (matJson.contains("Parameters"))
    {
        auto& params = matJson["Parameters"];

        if (params.contains("Scalars"))
        {
            for (auto& item : params["Scalars"].items())
            {
                string key = item.key();
                _float value = item.value().get<float>();
                pMaterial->Input_Scalar(key, value);
            }
        }

        if (params.contains("Colors"))
        {
            for (auto& item : params["Colors"].items())
            {
                std::string key = item.key();
                auto& colorObj = item.value();

                _float4 colorValue;
                colorValue.x = colorObj.value("R", 0.0f);
                colorValue.y = colorObj.value("G", 0.0f);
                colorValue.z = colorObj.value("B", 0.0f);
                colorValue.w = colorObj.value("A", 1.0f);

                pMaterial->Input_Color(key, colorValue);
            }
        }
    }
    pMaterial->Set_Meta_Desc(Build_MaterialMeta(matJson));
    m_pGameInstance->Register_Material(pMaterial);
    file.close();
    return S_OK;
}

HRESULT CMap_Manager::Create_MapInstanceBuffer(MeshPassData& passData, const vector<MapInstanceData>& instances, const _wstring& strBufferName)
{
    CVIBuffer* pBuffer = nullptr;
    m_pGameInstance->Get_VIBuffer(strBufferName, &pBuffer);

    if (nullptr != pBuffer)
    {
        Safe_Release(pBuffer);
        return S_OK;
    }

    CVIBuffer_MapInstance* pInstanceBuffer =
        CVIBuffer_MapInstance::Create(m_pDevice, m_pContext, passData, instances);

    if (nullptr == pInstanceBuffer)
    {
        MSG_BOX("Map Instance Buffer Create Fail!");
        return E_FAIL;
    }

    pInstanceBuffer->Set_Name(strBufferName);
    m_pGameInstance->Register_VIBuffer(pInstanceBuffer);

    return S_OK;
}

_float4x4 CMap_Manager::Apply_MapScale(const _float4x4& WorldMatrix, _fvector vScale) const
{
  
    _matrix OrignalWorldMatrix = XMLoadFloat4x4(&WorldMatrix);

    _matrix ScaleMatrix = XMMatrixScalingFromVector(vScale);

    _float4x4 ScaledWorldMatrix{};

    XMStoreFloat4x4(&ScaledWorldMatrix, OrignalWorldMatrix*ScaleMatrix );

    return ScaledWorldMatrix;
}

HRESULT CMap_Manager::Read_MapMeshGroup(const _wstring& strMapName, _bool bInstanced, _uint iGroupIndex, MeshPassData* pOutPassData) const
{
    if (nullptr == pOutPassData)
        return E_FAIL;

    const _wstring strMapFolder = Get_MapFolderPath(strMapName);
    const _wstring strFilePath = strMapFolder + L"\\" + strMapName + GlobalFileRoute::g_strMeshFileTag;
    ifstream File(strFilePath, ios::binary);

    if (false == File.is_open())
        return E_FAIL;

    auto ReadFile = [&File](void* pData, streamsize iSize) -> _bool { if (0 == iSize) return true; File.read(reinterpret_cast<char*>(pData), iSize); return File.gcount() == iSize; };

    _uint iStaticCount = 0;

    if (false == ReadFile(&iStaticCount, sizeof(_uint)))
        return E_FAIL;

    for (_uint iGroup = 0; iGroup < iStaticCount; ++iGroup)
    {
        MeshData Header{};
        MeshPassData PassData{};
        _float4x4 WorldMatrix{};

        if (false == ReadFile(&Header, sizeof(MeshData)))
            return E_FAIL;

        // 복사용 MeshPassData에도 원본 메시/머테리얼 헤더를 보존한다.
        strcpy_s(PassData.name, Header.meshName);
        strcpy_s(PassData.materialName, Header.materialName);
        PassData.hasAnimation = Header.HasAnimation;
        PassData.vertexCount = Header.vertexCount;
        PassData.indexCount = Header.indexCount;
        PassData.vertexStride = Header.vertexStride;
        PassData.boneCount = Header.boneCount;
        PassData.NoAnimVertices.resize(Header.vertexCount);
        PassData.Indices.resize(Header.indexCount);

        if (false == ReadFile(PassData.NoAnimVertices.data(), sizeof(VTXMESH) * Header.vertexCount) || false == ReadFile(PassData.Indices.data(), sizeof(_uint) * Header.indexCount) || false == ReadFile(&WorldMatrix, sizeof(_float4x4)))
            return E_FAIL;

        if (false == bInstanced && iGroup == iGroupIndex)
        {
            *pOutPassData = move(PassData);
            return S_OK;
        }
    }

    _uint iInstanceGroupCount = 0;

    if (false == ReadFile(&iInstanceGroupCount, sizeof(_uint)))
        return E_FAIL;

    for (_uint iGroup = 0; iGroup < iInstanceGroupCount; ++iGroup)
    {
        MeshData Header{};
        MeshPassData PassData{};
        _uint iInstanceCount = 0;

        if (false == ReadFile(&Header, sizeof(MeshData)))
            return E_FAIL;

        // Instance 복제품 Export에서도 원본 메시/머테리얼 헤더가 유지되어야 한다.
        strcpy_s(PassData.name, Header.meshName);
        strcpy_s(PassData.materialName, Header.materialName);
        PassData.hasAnimation = Header.HasAnimation;
        PassData.vertexCount = Header.vertexCount;
        PassData.indexCount = Header.indexCount;
        PassData.vertexStride = Header.vertexStride;
        PassData.boneCount = Header.boneCount;
        PassData.NoAnimVertices.resize(Header.vertexCount);
        PassData.Indices.resize(Header.indexCount);

        if (false == ReadFile(PassData.NoAnimVertices.data(), sizeof(VTXMESH) * Header.vertexCount) || false == ReadFile(PassData.Indices.data(), sizeof(_uint) * Header.indexCount) || false == ReadFile(&iInstanceCount, sizeof(_uint)))
            return E_FAIL;

        File.seekg(sizeof(MapInstanceData) * iInstanceCount, ios::cur);

        if (File.fail())
            return E_FAIL;

        if (bInstanced && iGroup == iGroupIndex)
        {
            *pOutPassData = move(PassData);
            return S_OK;
        }
    }

    return E_FAIL;
}

_float4x4 CMap_Manager::Remove_MapScale(const _float4x4& WorldMatrix, _float fMapScale) const
{
    if (fabsf(fMapScale) <= FLT_EPSILON || fabsf(fMapScale - 1.f) <= FLT_EPSILON)
        return WorldMatrix;

    _float4x4 Result{};
    XMStoreFloat4x4(&Result, XMLoadFloat4x4(&WorldMatrix) * XMMatrixScaling(1.f / fMapScale, 1.f / fMapScale, 1.f / fMapScale));
    return Result;
}

HRESULT CMap_Manager::Append_ManualPhysicsBuildGroups(const _wstring& strMapName, vector<MapPhysicsBuildGroup>* pGroups) const
{
    if (strMapName.empty() || nullptr == pGroups || nullptr == m_pGameInstance)
        return E_FAIL;

    const _wstring strMapFolder = Get_MapFolderPath(strMapName);
    const _wstring strFilePath = strMapFolder + L"\\" + strMapName + L"_ManualPhysics.json";
    ifstream File(strFilePath);

    if (false == File.is_open())
        return S_OK;

    json Root = {};

    try
    {
        File >> Root;
    }
    catch (const json::exception&)
    {
        MSG_BOX("Manual Physics Read Failed");
        return E_FAIL;
    }

    if (false == Root.contains("ManualPhysicsBoxes") || false == Root["ManualPhysicsBoxes"].is_array())
        return E_FAIL;

    const _float fMapScale = m_MapScales.end() != m_MapScales.find(strMapName) ? m_MapScales.at(strMapName) : 1.f;

    for (const json& BoxJson : Root["ManualPhysicsBoxes"])
    {
        if (false == BoxJson.is_object())
            continue;

        const json& Position = BoxJson["Position"];
        const json& Rotation = BoxJson["Rotation"];
        const json& Scale = BoxJson["Scale"];
        const json& Extents = BoxJson["Extents"];

        if (false == Position.is_array() || false == Rotation.is_array() || false == Scale.is_array() || false == Extents.is_array())
            continue;

        if (Position.size() < 3 || Rotation.size() < 3 || Scale.size() < 3 || Extents.size() < 3)
            continue;

        const string strRole = BoxJson.value("Role", string("OBJECT"));

        MapPhysicsBuildGroup Group = {};
        Group.bInstanced = false;
        Group.iGroupIndex = BoxJson.value("BoxID", UINT_MAX);
        Group.eGeometryType = MAP_PHYSICS_GEOMETRY_TYPE::BOX;
        Group.eRole = "GROUND" == strRole ? MAP_PHYSICS_ROLE::GROUND : MAP_PHYSICS_ROLE::OBJECT;
        Group.BoxData.vCenter = _float3(0.f, 0.f, 0.f);
        Group.BoxData.vExtents = _float3(Extents[0].get<_float>(), Extents[1].get<_float>(), Extents[2].get<_float>());

        const _float3 vPosition = _float3(Position[0].get<_float>(), Position[1].get<_float>(), Position[2].get<_float>());
        const _float3 vRotation = _float3(Rotation[0].get<_float>(), Rotation[1].get<_float>(), Rotation[2].get<_float>());
        const _float3 vScale = _float3(Scale[0].get<_float>(), Scale[1].get<_float>(), Scale[2].get<_float>());

        _float4x4 WorldMatrix = {};
        XMStoreFloat4x4(&WorldMatrix, XMMatrixScaling(vScale.x, vScale.y, vScale.z) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(vRotation.x), XMConvertToRadians(vRotation.y), XMConvertToRadians(vRotation.z)) * XMMatrixTranslation(vPosition.x, vPosition.y, vPosition.z));

        Group.WorldMatrices.push_back(Remove_MapScale(WorldMatrix, fMapScale));
        pGroups->push_back(move(Group));
    }

    return S_OK;
}

#ifdef _DEBUG
_bool CMap_Manager::Find_MapObjectSource(const CMapObject* pMapObject, MapObjectSourceData* pOutSource) const
{
    if (nullptr == pMapObject || nullptr == pOutSource)
        return false;

    for (const auto& MapPair : m_Maps)
    {
        CMap* pMap = MapPair.second;

        if (nullptr == pMap)
            continue;

        const vector<CMapObject*>& Objects = pMap->Get_Objects();
        _uint iStaticGroupIndex = 0;
        _uint iInstanceGroupIndex = 0;

        for (CMapObject* pObject : Objects)
        {
            CMapInstanceObject* pInstanceOwner = dynamic_cast<CMapInstanceObject*>(pObject);

            if (nullptr == pInstanceOwner)
            {
                if (pObject == pMapObject)
                {
                    pOutSource->strMapName = MapPair.first;
                    pOutSource->bInstanced = false;
                    pOutSource->iGroupIndex = iStaticGroupIndex;
                    pOutSource->iInstanceIndex = UINT_MAX;
                    return true;
                }

                ++iStaticGroupIndex;
                continue;
            }

            const _int iInstanceIndex = pInstanceOwner->Find_InstanceIndex(pMapObject);

            if (0 <= iInstanceIndex)
            {
                pOutSource->strMapName = MapPair.first;
                pOutSource->bInstanced = true;
                pOutSource->iGroupIndex = iInstanceGroupIndex;
                pOutSource->iInstanceIndex = static_cast<_uint>(iInstanceIndex);
                return true;
            }


            ++iInstanceGroupIndex;
        }
    }

    return false;
}

void CMap_Manager::Set_MapPhysicsDebugVisible(const _wstring& strMapName, _bool bVisible)
{
	m_DebugSectorVisible[strMapName] = bVisible;

    auto Iter = m_MapPhysicsActors.find(strMapName);

    if (m_MapPhysicsActors.end() == Iter)
        return;

    for (PxRigidStatic* pActor : Iter->second)
    {
        if (nullptr == pActor)
            continue;

        // 충돌 및 Scene 등록 상태는 건드리지 않고 해당 Actor의 디버그 표시만 제어한다.
        pActor->setActorFlag(PxActorFlag::eVISUALIZATION, bVisible);
    }
}

void CMap_Manager::Set_MapPhysicsDebugCategory(_bool bOn)
{
	m_bDebugOn = bOn;

    for (auto& Pair : m_MapPhysicsActors)
    {
		auto debugIter = m_DebugSectorVisible.find(Pair.first);
		
        _bool bSectorVisible = (debugIter == m_DebugSectorVisible.end()) ? true : debugIter->second;
		_bool bVisible = m_bDebugOn && bSectorVisible;

        for (PxRigidStatic* pActor : Pair.second)
        {
            if (nullptr == pActor)
                continue;

            pActor->setActorFlag(PxActorFlag::eVISUALIZATION, bVisible);
        }
    }
}

#endif // _DEBUG


MAP_PHYSICS_GEOMETRY_TYPE CMap_Manager::Get_MapPhysicsType(const MapObjectSourceData& Source) const
{
    auto MapIter = m_MapPhysicsSettings.find(Source.strMapName);

    if (m_MapPhysicsSettings.end() == MapIter)
        return MAP_PHYSICS_GEOMETRY_TYPE::END;

    for (const MapPhysicsSettingData& Setting : MapIter->second)
    {
        if (Setting.Source.bInstanced == Source.bInstanced && Setting.Source.iGroupIndex == Source.iGroupIndex && Setting.Source.iInstanceIndex == Source.iInstanceIndex)
            return Setting.eGeometryType;
    }

    return MAP_PHYSICS_GEOMETRY_TYPE::END;
}

void CMap_Manager::Set_MapPhysicsType(const MapObjectSourceData& Source, MAP_PHYSICS_GEOMETRY_TYPE eGeometryType)
{
    vector<MapPhysicsSettingData>& Settings = m_MapPhysicsSettings[Source.strMapName];

    for (auto Iter = Settings.begin(); Iter != Settings.end(); ++Iter)
    {
        if (Iter->Source.bInstanced != Source.bInstanced || Iter->Source.iGroupIndex != Source.iGroupIndex || Iter->Source.iInstanceIndex != Source.iInstanceIndex)
            continue;

        if (MAP_PHYSICS_GEOMETRY_TYPE::END == eGeometryType)
            Settings.erase(Iter);
        else
            Iter->eGeometryType = eGeometryType;

        return;
    }

    if (MAP_PHYSICS_GEOMETRY_TYPE::END != eGeometryType)
        Settings.push_back({ Source, eGeometryType });
}

MAP_PHYSICS_ROLE CMap_Manager::Get_MapPhysicsRole(const MapObjectSourceData& Source) const
{
    auto MapIter = m_MapPhysicsSettings.find(Source.strMapName);

    if (m_MapPhysicsSettings.end() == MapIter)
        return MAP_PHYSICS_ROLE::OBJECT;

    for (const MapPhysicsSettingData& Setting : MapIter->second)
    {
        if (Setting.Source.bInstanced == Source.bInstanced && Setting.Source.iGroupIndex == Source.iGroupIndex && Setting.Source.iInstanceIndex == Source.iInstanceIndex)
            return Setting.eRole;
    }

    return MAP_PHYSICS_ROLE::OBJECT;
}

void CMap_Manager::Set_MapPhysicsRole(const MapObjectSourceData& Source, MAP_PHYSICS_ROLE eRole)
{
    vector<MapPhysicsSettingData>& Settings = m_MapPhysicsSettings[Source.strMapName];

    for (MapPhysicsSettingData& Setting : Settings)
    {
        if (Setting.Source.bInstanced != Source.bInstanced || Setting.Source.iGroupIndex != Source.iGroupIndex || Setting.Source.iInstanceIndex != Source.iInstanceIndex)
            continue;

        if (MAP_PHYSICS_ROLE::END != eRole)
            Setting.eRole = eRole; // 기존 피직스 설정의 GROUND / OBJECT 역할만 갱신한다.

        return;
    }
}

CMap_Manager* CMap_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMap_Manager* pInstance = new CMap_Manager(pDevice, pContext);
    if (pInstance == nullptr)
    {
        MSG_BOX("MapManager : create fail");
        return nullptr;
    }
    return pInstance;
}

void CMap_Manager::Free()
{
    for (auto& Pair : m_MapPhysicsActors)
    {
        for (PxRigidStatic* pActor : Pair.second)
        {
            if (nullptr == pActor)
                continue;

            if (nullptr != pActor->getScene())
                m_pGameInstance->Remove_ActorFromScene(pActor);

            pActor->release();
        }
    }

    m_MapPhysicsActors.clear();
    m_MapPhysicsSettings.clear();
    m_MapScales.clear();

    for (auto& Pair : m_Maps)
        Safe_Release(Pair.second);

    m_Maps.clear();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}
