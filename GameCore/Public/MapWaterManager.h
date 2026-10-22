#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "IListener.h"

NS_BEGIN(Engine)
class CGameInstance;
class CMap;
NS_END

NS_BEGIN(GameCore)

class CMapWorld;

class GAMECORE_DLL CMapWaterManager final : public CBase, public Engine::IListener
{
private:
    struct WATER_PLANE
    {
        _uint         iSectorID = UINT_MAX;
        _float        fHeight = 0.f;
        _float        fMaxDistance = 100.f;
        vector<_uint> ReflectSectors;
    };

private:
    CMapWaterManager();
    virtual ~CMapWaterManager() = default;

public:
    HRESULT Initialize();
    HRESULT Load_ChapterWaters(const _wstring& strChapterPath);
    void    Update_Reflection(CMapWorld* pMapWorld);
    virtual void OnEvent(EVENT_TYPE etype, Event_Desc* pData) override;

private:
    const WATER_PLANE* Find_ActivePlane(const unordered_set<_uint>& ActiveSectorIDs) const;

private:
    Engine::CGameInstance* m_pGameInstance = nullptr;
    vector<WATER_PLANE>     m_WaterPlanes = {};
    _uint                   m_iCurrentSectorID = { UINT_MAX };

    _bool                   m_bReflectionActive = { false };
    _float                  m_fEnterHeight = { -1.0f };
    _float                  m_fExitHeight = { -2.0f };
    _float                  m_fExitDistanceScale = { 1.2f };

public:
    static CMapWaterManager* Create();
    virtual void Free() override;
};

NS_END
