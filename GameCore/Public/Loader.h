#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "ThreadPool.h"
#include "AnimationData.h"

NS_BEGIN(Engine)
class CGameInstance;
class CTexture; 
NS_END

NS_BEGIN(GameCore)

/* Loader에서는 효율이 좋은 Model , Map의 입부만 병렬로 돌림. heap할당만 하는것은 효율X */
// GamePlay의 Bullet처럼, Loader내에서 모델 클론이 필요한경우(Bullet의 경우는 Create Pool때문, 다른 예시라면 객체 원형이 모델 클론을 할 경우) 
// 이 경우, Job에 넣지않고 기존 코드처럼 바로 생성+ AddPrototype 하기

class CLoader final : public CBase
{
private:
	//Model Load용 Job , 캐시 라인 최적화 
	struct alignas(64) ModelJob
	{
		_uint				iLevel		= {};
		_wstring			strPrototypeTag;
		function<CBase* ()> CreateFunc	= { nullptr };
		CBase*				pResult		= { nullptr };
	};

private:
	CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd);
	virtual ~CLoader() = default;

public:
	//레벨 로딩
	HRESULT					Initialize(LEVEL eNextLevelID);
	//인게임 챕터 로딩 
	HRESULT					Initialize(_uint iChapterIndex);
	HRESULT					Loading();

	static _bool			Is_ChapterCached(_uint iChapterIndex);
	static void				Clear_CachedChapters();

	// memory_order_acquire : true를 반환하는 순간 Store(true,release)호출하기 전의 모든 작업이 동기화 보장된다. 
	// 즉, true가 뜬 이후부터 워커스레드가 만들어둔 리소스에 접근해도 경쟁이 일어나지않음 
	_bool isFinished() const {
		return m_isFinished.load(memory_order_acquire);
	}

	//isFinished 가 true인 뒤에만 유효함. isFinished()가 호출되면서 동기화가 이루어진 뒤 
	_bool isFailed() const
	{
		return m_isFailed.load(memory_order_relaxed);
	}

	_float					Get_LoadingProgress() const;	// 진행률 표시용

//#ifdef _DEBUG
public:
	void Show();
//#endif
private:
	HRESULT					Begin_Thread();

	HRESULT					Ready_Resources_For_Logo();
	HRESULT					Ready_Resources_For_GamePlay();
	//HRESULT					Ready_Resources_For_Shader();

	//챕터전용 리소스 
	//HRESULT				Ready_ChapterMap_For_GamePlay(); 함수를 다음 함수로 일반화만 시킴 
	HRESULT					Ready_ChapterMap(const _wstring& strChapterPath);

	HRESULT					Ready_Resources_For_Chapter(_uint iChapterIndex);

	HRESULT					Ready_Resources_For_Chapter01();  // 기차역 
	HRESULT					Ready_Resources_For_Chapter02();  // 호텔
	HRESULT					Ready_Resources_For_Chapter03();  // 락사시아
	HRESULT					Ready_Resources_For_ChapterTest();  // 테스트맵 
	HRESULT					Ready_Resources_For_ChapterTestObj();  // 테스트오브젝트맵 
	HRESULT					Ready_Resources_For_ChapterBossMap();  // 락사시아 보스맵 


private:
	void					Enqueue_AnimationJob(const _wstring& strFolderName, const _wstring& strBaseName, _bool bStatic, ANIMATION_TIME_UNIT eUnit = ANIMATION_TIME_UNIT::MILLISEC);
	HRESULT					Join_And_Register_Models();
	void					Clear_ModelJobs();
private:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	CGameInstance*			m_pGameInstance = { nullptr };

	HANDLE					m_hThread = { };
	CRITICAL_SECTION		m_CriticalSection = { };

private:
	//멀티스레딩용 
	vector<ModelJob>		m_GamePlay_ModelJobs;
	vector<ModelJob>		m_Chapter01_ModelJobs;
	vector<ModelJob>		m_Chapter02_ModelJobs;
	vector<ModelJob>		m_Chapter03_ModelJobs;
	vector<ModelJob>		m_TestMap_ModelJobs;
	vector<ModelJob>		m_TestObjMap_ModelJobs;
	vector<ModelJob>		m_BossMap_ModelJobs;

	//모델 생성 Batch 
	CLoadBatch				m_ModelBatch;	
	//애니메이션 I/O Batch
	CLoadBatch				m_AnimBatch; 
private:
	LEVEL					m_eNextLevelID = { LEVEL::END };
	_uint					m_iChapterIndex = { UINT_MAX };

	_tchar					m_szLoadingText[MAX_PATH] = {};

	atomic<_bool>			m_isFinished = { false };
	atomic<_bool>			m_isFailed = { false };

	//이미 로딩된 챕터 목록
	static unordered_set<_uint> g_LoadedChapters;

	HWND					m_hWnd = { nullptr };
	mutable atomic<_float>	m_fHighestLoadingProgress = { 0.f }; // 작업이 추가되어 분모가 늘어나도 UI 진행률이 역행하지 않게 한다.

public:
	static CLoader*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID, HWND hWnd);
	static CLoader*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _uint iChapterIndex, HWND hWnd);
	virtual void			Free() override;
};

NS_END
