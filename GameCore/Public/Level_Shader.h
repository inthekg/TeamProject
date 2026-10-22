#pragma once
#include "Core_Defines.h"
#include "Level.h"
#include "IListener.h"


NS_BEGIN(Engine)
class CMap_Manager;
class CMapController;
class CCustomStaticObject;
#ifdef _DEBUG
class CPXDebugRenderer;
#endif
NS_END


NS_BEGIN(GameCore)
class CChapterMapManager;
#ifdef _DEBUG
class CNavigationDebugRenderer;
#endif

class CLevel_Shader final : public CLevel, public Engine::IListener
{
private:
	CLevel_Shader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd);
	virtual ~CLevel_Shader() = default;

public:
	virtual HRESULT			Initialize() override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:	
	HRESULT					Rendy_Layer_Sky(const _wstring& strLayerTag);
	HRESULT					Ready_Layer_Character(const _wstring& strLayerTag);
	HRESULT					Ready_Layer_Monster(const _wstring& strLayerTag);
	HRESULT					Ready_Layer_Map(const _wstring& strLayerTag);
	HRESULT					Ready_Layer_Fog(const _wstring& strLayerTag);
	HRESULT					Ready_Effect();
	HRESULT					Ready_UI();

public :
	void					m_TriggerCallbackFunc(PxTriggerPair* pairs, PxU32 count);
	virtual void			OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;

public:
	virtual void			Level_Exit() override;

	//별바라기등 전환할떄 호출 할 함수 
	void					Request_ChangeChapter(_uint iChapterIndex, const Respawn_Point& SpawnPoint);
	void					Request_Respawn();

	void					Set_RespawnPoint(const Respawn_Point& Point) { m_RespawnPoint = Point; }
	_bool					Is_Transitioning() const { return CHAPTER_STATE::ACTIVE != m_eChapterState; }

private:
	void					Update_ChapterState(_float fTimeDelta);
	//각 상태별 기능 
	void					State_FadeOut(_float fTimeDelta);
	void					State_Unload();
	void					State_Load();
	void					State_LoadWait();
	void					State_Create();
	void					State_Resetting();
	void					State_FadeIn(_float fTimeDelta);

	_bool					Can_RequestTransition() const { return CHAPTER_STATE::ACTIVE == m_eChapterState; }
	void					Begin_FadeOut(TRANSITION_KIND eKind);
	void					Begin_FadeIn();

	void					Respawn_Player(const Respawn_Point& Point);
	void					ResetPlayer_ForChangeMap(const Respawn_Point& Point);

	void					Lock_PlayerInput(_bool bLock);

#ifdef _DEBUG
	//피직스 디버그 랜더러 
	HRESULT 				Ready_DebugRenderer();
	void					Render_DebugWindow();
	HRESULT					Ready_NavigationDebugRenderer();

private:
	CPXDebugRenderer*		m_pPXDebugRenderer = { nullptr };
	_bool					m_bRenderPXDebug = { true };

	CNavigationDebugRenderer* m_pNavigationDebugRenderer = { nullptr };
#endif 

	class CChapterMapManager* m_pChapterMapManager = { nullptr };

	CHAPTER_STATE			m_eChapterState = { CHAPTER_STATE::ACTIVE };
	TRANSITION_KIND			m_eTransitionKind = { TRANSITION_KIND::NONE };

	_uint					m_iCurrentChapter = { 0 };
	_uint					m_iPendingChapter = { UINT_MAX };
	Respawn_Point			m_PendingSpawnPoint = {};
	Respawn_Point			m_RespawnPoint = {};

	_float					m_fFadeTimer = { 0.f };
	static constexpr _float	FADE_DURATION = { 0.5f };

	class CLoader* m_pChapterLoader = { nullptr };


public:
	static CLevel_Shader*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd);
	virtual void			Free() override;
};

NS_END