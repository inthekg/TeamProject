#pragma once

#include "Core_Defines.h"
#include "Base.h"

NS_BEGIN(Engine)

class CGameInstance;
class CMapController;

NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CMapLightManager final : public CBase
{
private:
    CMapLightManager();
    virtual ~CMapLightManager() = default;

public:
    HRESULT Initialize();
    HRESULT Load_ChapterLights(const _wstring& strChapterPath);
    HRESULT Register_Light(LIGHT eLightType, _uint iLightIndex, _uint iOwnerSectorID, const BoundingBox& WorldBounds, _bool bUseBounds);
    HRESULT Update(Engine::CMapController* pMapController);

private:
    void Release_RegisteredLights();

private:
    Engine::CGameInstance* m_pGameInstance = nullptr;
    vector<MAP_SECTOR_LIGHT_DATA> m_SectorLights{};

public:
    static CMapLightManager* Create();
    virtual void Free() override;
};

NS_END