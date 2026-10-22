#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "IListener.h"

NS_BEGIN(Engine)
class CGameInstance;
class CFogVolume;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CMapFogManager final : public CBase, public Engine::IListener
{
private:
    struct SECTOR_EXPOSURE
    {
        _uint  iSectorID = UINT_MAX;
        _float fScale = 1.f;
        _float fBlendTime = 0.8f;
    };

private:
    CMapFogManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CMapFogManager() = default;

public:
    HRESULT Initialize();
    HRESULT Load_ChapterFogs(const _wstring& strChapterPath, _uint iLayerLevelIndex, const _wstring& strLayerTag);
    HRESULT Apply_FogSectorCulling(const unordered_set<_uint>* pActiveRenderSectorIDs);
    HRESULT Apply_SectorExposure(_uint iCurrentSectorID);
    void    Update_ExposureTuning();
    virtual void OnEvent(EVENT_TYPE etype, Event_Desc* pData) override;

private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;
    Engine::CGameInstance* m_pGameInstance = nullptr;

private:
    vector<pair<_uint, Engine::CFogVolume*>> m_FogVolumes = {};
    vector<SECTOR_EXPOSURE>                 m_SectorExposures = {};
    _float                                  m_fDefaultBlendTime = { 0.8f };
    _uint                                   m_iCurrentSectorID = { UINT_MAX };
    _float                                  m_fExposureTuneStep = { 0.05f };

public:
    static CMapFogManager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END
