#pragma once

#include "Core_Defines.h"


NS_BEGIN(GameCore)

// 섹터 간 연결이 전환용인지 렌더용인지 구분함
enum class SECTOR_LINK_TYPE
{
    TRANSITION_ONLY,
    RENDER_ONLY,
    TRANSITION_AND_RENDER
};


enum class SECTOR_ROLE : _uint
{
    PRIMARY,
    SUPPORT,
    END
};

struct SECTOR_DESC
{
    // 챕터 안에서 섹터 하나를 설명하는 기본 데이터임
    _uint ID = UINT_MAX;
    _wstring MapName;
    _float MapScale = 1.f;
    BoundingBox Bounds{};
    SECTOR_ROLE eRole = SECTOR_ROLE::PRIMARY;
    vector<_uint> OwnerSectorIDs;
};

struct SECTOR_LINK_DESC
{
    // 두 섹터를 잇는 링크와 포탈 렌더 조건을 묶어둔 데이터임
    _uint ID = UINT_MAX;
    _wstring Name;

    _uint SourceID = UINT_MAX;
    _uint TargetID = UINT_MAX;

    BoundingBox Bounds{};
    _float3 Rotation = { 0.f, 0.f, 0.f }; // Link volume rotation in degrees.
    _float3 Forward = { 0.f, 0.f, 1.f };
    _bool TwoWay = false;

    SECTOR_LINK_TYPE eType = SECTOR_LINK_TYPE::TRANSITION_AND_RENDER;
    _float RenderDepth = -1.f;

    _float RenderEnter = 100.f;
    _float RenderExit = 140.f;
    _float UpdateEnter = 60.f;
    _float UpdateExit = 100.f;
};

struct CHAPTER_DESC
{
    // 챕터를 구성하는 섹터 목록과 시작 지점을 들고 있음
    _uint Version = 1;
    _wstring Name;
    _uint StartSectorID = UINT_MAX;


    unordered_map<_uint, SECTOR_DESC> Sectors;
    vector<SECTOR_LINK_DESC> Links;
    vector<MONSTER_SPAWN_DATA> MonsterSpawns;
    vector<MAP_GAMEPLAY_SPAWN_DATA> MapGameplaySpawns;
    vector<MAP_TRIGGER_DATA> Triggers;
};


// 챕터 맵 데이터를 파일로 저장/로드하고 유효성 검사하는 유틸 클래스임
class GAMECORE_DLL CChapterData final
{
public:
    CChapterData();

public:
    static HRESULT Save(const _wstring& strPath, const CHAPTER_DESC& Chapter, _wstring* pError = nullptr);
    static HRESULT Load(const _wstring& strPath, CHAPTER_DESC* pChapter, _wstring* pError = nullptr);
    static HRESULT Check(const CHAPTER_DESC& Chapter, _wstring* pError = nullptr);

public:
    static constexpr _uint VERSION = 1;



};

NS_END
