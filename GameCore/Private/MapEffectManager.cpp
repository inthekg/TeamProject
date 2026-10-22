#include "MapEffectManager.h"

#include "GameInstance.h"
#include "Effect.h"

CMapEffectManager::CMapEffectManager()
	: m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CMapEffectManager::Initialize()
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	m_pGameInstance->Subscribe(EVENT_TYPE::SECTOR_CHANGED, this);

	return S_OK;
}

HRESULT CMapEffectManager::Load_ChapterEffects(const _wstring& strChapterPath)
{
	if (nullptr == m_pGameInstance || true == strChapterPath.empty())
		return E_FAIL;

	// 같은 매니저에서 다시 로드될 경우 기존 챕터 이펙트를 먼저 정리한다.
	Clear_ChapterEffects();

	const _wstring strEffectPath = Make_ChapterEffectPath(strChapterPath);

	if (true == strEffectPath.empty())
		return E_FAIL;

	ifstream File(strEffectPath, ios::binary);

	// 챕터 이펙트 배치 파일은 선택 파일이므로 없으면 정상 처리한다.
	if (false == File.is_open())
		return S_OK;

	json Root = {};

	try
	{
		File >> Root;

		if (false == Root.contains("Effects") || false == Root["Effects"].is_array())
			return S_OK;

		_uint iEffectSerial = 0;

		for (const json& EffectJson : Root["Effects"])
		{
			if (false == EffectJson.is_object())
				continue;

			MAP_EFFECT_RUNTIME_ENTRY Entry = {};
			Entry.iEffectID = EffectJson.value("EffectID", iEffectSerial);
			Entry.iSectorID = EffectJson.value("SectorID", UINT_MAX);
			Entry.strEffectName = EffectJson.value("EffectName", string{});
			Entry.bEnable = EffectJson.value("Enable", true);
			++iEffectSerial;

			if (true == Entry.strEffectName.empty())
				continue;

			if (false == EffectJson.contains("Position") || false == EffectJson["Position"].is_array() || EffectJson["Position"].size() < 3)
				continue;

			const json& PositionJson = EffectJson["Position"];
			Entry.vPosition = _float3(PositionJson[0].get<_float>(), PositionJson[1].get<_float>(), PositionJson[2].get<_float>());

			// 배치 정보는 유지하고 실제 CEffect 수명만 섹터 컬링 단계에서 관리한다.
			m_ChapterEffects.push_back(Entry);
		}
	}
	catch (const std::exception&)
	{
		Clear_ChapterEffects();
		MSG_BOX("Chapter Effect Read Failed");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CMapEffectManager::Apply_EffectSectorCulling(const unordered_set<_uint>* pActiveSectorIDs)
{
	if (nullptr == m_pGameInstance || nullptr == pActiveSectorIDs)
		return E_FAIL;

	for (MAP_EFFECT_RUNTIME_ENTRY& Entry : m_ChapterEffects)
	{
		Entry.bSectorActive = UINT_MAX == Entry.iSectorID || pActiveSectorIDs->end() != pActiveSectorIDs->find(Entry.iSectorID);

		if (false == Entry.bEnable || false == Entry.bSectorActive)
		{
			Stop_Effect(Entry);
			continue;
		}

		if (nullptr != Entry.pEffect && false == Entry.pEffect->Is_Finished())
			continue;

		if (nullptr != Entry.pEffect)
		{
			// EffectManager 참조가 남아 있어도 배치 항목의 소유 포인터는 먼저 분리한다.
			CEffect* pFinishedEffect = Entry.pEffect;
			Entry.pEffect = nullptr;
			Safe_Release(pFinishedEffect);
		}

		if (FAILED(Start_Effect(Entry)))
			return E_FAIL;
	}

	return S_OK;
}

void CMapEffectManager::Update()
{
	for (MAP_EFFECT_RUNTIME_ENTRY& Entry : m_ChapterEffects)
	{
		if (false == Entry.bEnable || false == Entry.bSectorActive)
		{
			Stop_Effect(Entry); // 루프 이펙트는 자연 종료를 기다리지 않고 Stop_Immediate 경로로 즉시 끈다.
			continue;
		}

		if (nullptr != Entry.pEffect)
		{
			if (false == Entry.pEffect->Is_Finished())
				continue;

			// 배치 정보는 남기되 소유 포인터는 먼저 분리하여 중복 Release를 막는다.
			CEffect* pFinishedEffect = Entry.pEffect;
			Entry.pEffect = nullptr;
			Safe_Release(pFinishedEffect);
		}

		if (true == Entry.bEnable && true == Entry.bSectorActive)
			Start_Effect(Entry); // Pool이 찬 경우에도 다음 Update에서 재시도한다.
	}
}

void CMapEffectManager::OnEvent(EVENT_TYPE etype, Event_Desc* pData)
{
	if (EVENT_TYPE::SECTOR_CHANGED != etype || nullptr == pData)
		return;

	SECTOR_CHANGED_EVENT_DESC* pSectorChangedDesc = static_cast<SECTOR_CHANGED_EVENT_DESC*>(pData);

	// MapWorld가 계산한 렌더 활성 섹터를 기준으로 환경 이펙트를 생성하거나 정지한다.
	Apply_EffectSectorCulling(pSectorChangedDesc->pActiveRenderSectorIDs);
}

_wstring CMapEffectManager::Make_ChapterEffectPath(const _wstring& strChapterPath) const
{
	if (true == strChapterPath.empty())
		return L"";

	const size_t iSlashPosition = strChapterPath.find_last_of(L"\\/");
	const _wstring strFolderPath = _wstring::npos == iSlashPosition ? L"" : strChapterPath.substr(0, iSlashPosition + 1);
	_wstring strFileName = _wstring::npos == iSlashPosition ? strChapterPath : strChapterPath.substr(iSlashPosition + 1);
	const _wstring strChapterSuffix = L".chapter.json";
	const size_t iChapterSuffixPosition = strFileName.rfind(strChapterSuffix);

	if (_wstring::npos != iChapterSuffixPosition)
	{
		strFileName = strFileName.substr(0, iChapterSuffixPosition);
	}
	else
	{
		const size_t iExtensionPosition = strFileName.find_last_of(L'.');

		if (_wstring::npos != iExtensionPosition)
			strFileName = strFileName.substr(0, iExtensionPosition);
	}

	return strFolderPath + strFileName + L".Effect.json";
}

HRESULT CMapEffectManager::Start_Effect(MAP_EFFECT_RUNTIME_ENTRY& Entry)
{
	if (nullptr == m_pGameInstance || nullptr != Entry.pEffect || true == Entry.strEffectName.empty())
		return E_FAIL;

	Entry.pEffect = m_pGameInstance->Play_Effect(Entry.strEffectName, Entry.vPosition);

	if (nullptr == Entry.pEffect)
		return E_FAIL;

	// EffectManager와 MapEffectManager가 각각 하나의 참조를 보유한다.
	Safe_AddRef(Entry.pEffect);
	return S_OK;
}

void CMapEffectManager::Stop_Effect(MAP_EFFECT_RUNTIME_ENTRY& Entry)
{
	if (nullptr == Entry.pEffect)
		return;

	// 배치 항목에서 먼저 분리하여 반복 섹터 이벤트의 중복 Release를 막는다.
	CEffect* pEffect = Entry.pEffect;
	Entry.pEffect = nullptr;

	// 다른 시스템의 이펙트에는 영향 없이 이 배치가 소유한 재생 인스턴스만 종료한다.
	pEffect->Stop_Immediate();
	Safe_Release(pEffect);
}

void CMapEffectManager::Clear_ChapterEffects()
{
	for (MAP_EFFECT_RUNTIME_ENTRY& Entry : m_ChapterEffects)
		Stop_Effect(Entry);

	m_ChapterEffects.clear();
}

CMapEffectManager* CMapEffectManager::Create()
{
	CMapEffectManager* pInstance = new CMapEffectManager();

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CMapEffectManager");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMapEffectManager::Free()
{
	if (nullptr != m_pGameInstance)
		m_pGameInstance->UnSubscribe_AllEvent(this);

	Clear_ChapterEffects();
	__super::Free();
}
