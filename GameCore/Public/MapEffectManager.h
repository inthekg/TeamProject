#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "IListener.h"

NS_BEGIN(Engine)

class CGameInstance;
class CEffect;

NS_END

NS_BEGIN(GameCore)

struct MAP_EFFECT_RUNTIME_ENTRY
{
	_uint iEffectID = { UINT_MAX };
	_uint iSectorID = { UINT_MAX };
	string strEffectName = {};
	_float3 vPosition = {};
	_bool bEnable = { true };
	_bool bSectorActive = { false };
	Engine::CEffect* pEffect = { nullptr };
};

class GAMECORE_DLL CMapEffectManager final : public CBase, public Engine::IListener
{
private:
	CMapEffectManager();
	virtual ~CMapEffectManager() = default;

public:
	HRESULT Initialize();
	HRESULT Load_ChapterEffects(const _wstring& strChapterPath);
	HRESULT Apply_EffectSectorCulling(const unordered_set<_uint>* pActiveSectorIDs);
	void Update();
	virtual void OnEvent(EVENT_TYPE etype, Event_Desc* pData) override;

private:
	_wstring Make_ChapterEffectPath(const _wstring& strChapterPath) const;
	HRESULT Start_Effect(MAP_EFFECT_RUNTIME_ENTRY& Entry);
	void Stop_Effect(MAP_EFFECT_RUNTIME_ENTRY& Entry);
	void Clear_ChapterEffects();

private:
	Engine::CGameInstance* m_pGameInstance = { nullptr };
	vector<MAP_EFFECT_RUNTIME_ENTRY> m_ChapterEffects = {};

public:
	static CMapEffectManager* Create();
	virtual void Free() override;
};

NS_END
