#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "ChapterMapData.h"
#include "StargazerTravelData.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CStargazerManager final : public CBase
{
private:
	CStargazerManager() = default;
	virtual ~CStargazerManager() = default;

public:
	// Chapter의 모든 Stargazer Spawn을 Runtime Vector에 등록한다.
	HRESULT Register_Chapter(_uint iChapterIndex, const CHAPTER_DESC& ChapterDesc);

	// Broken Stargazer 복구 완료 시 활성화, 방문, 안전 위치를 함께 기록한다.
	HRESULT Activate_Stargazer(_uint64 iStargazerID, const Respawn_Point& RespawnPoint);

	// Normal Stargazer와 다시 상호작용한 안전 위치를 갱신한다.
	HRESULT Update_RespawnPoint(_uint64 iStargazerID, const Respawn_Point& RespawnPoint);

	// 실제 Player가 들어온 Sector와 처음부터 Normal인 Stargazer의 안전 위치를 기록한다.
	void Visit_Sector(_uint iChapterIndex, _uint iSectorID, const Respawn_Point& RespawnPoint);

	_bool Is_Activated(_uint64 iStargazerID) const;
	HRESULT Build_UIOpenDesc(_uint64 iSourceStargazerID, STARGAZER_UI_OPEN_DESC* pOutDesc) const;
	HRESULT Get_RespawnPoint(_uint64 iStargazerID, Respawn_Point* pOutPoint) const;
	HRESULT Resolve_Travel(_uint64 iTargetStargazerID, _uint iCurrentChapterIndex, _uint iCurrentSectorID, Respawn_Point* pOutPoint) const;

	static _uint64 Make_StargazerID(_uint iChapterIndex, _uint iSpawnID);

private:
	STARGAZER_RUNTIME_DATA* Find_RuntimeData(_uint64 iStargazerID);
	const STARGAZER_RUNTIME_DATA* Find_RuntimeData(_uint64 iStargazerID) const;

private:
	vector<STARGAZER_RUNTIME_DATA> m_RuntimeData = {};
	unordered_set<_uint> m_RegisteredChapterIndices = {};

public:
	static CStargazerManager* Create();
	virtual void Free() override;
};

NS_END
