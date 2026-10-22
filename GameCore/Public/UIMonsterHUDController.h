#pragma once
#include "UIControllerBase.h"
#include "IListener.h"
#include "UI_RuntimeData.h"

NS_BEGIN(Engine)
class CUIObject;
class CUIElement;
class CUIProgressBar;
class CUIText;
NS_END

NS_BEGIN(GameCore)

class CAIObject;
class CStatComponent_AI;
class CMonsterHUDViewModel;

class CUIMonsterHUDController final : public CUIControllerBase, public IListener
{
private:
    enum class MONSTER_FATAL_ANIM_STATE
    {
        NONE,
        ACTIVE,
        POSSIBLE
    };

public:
    typedef struct tagMonsterHUDDesc : public CUIControllerBase::UI_CONTROLLER_DESC
    {
        // 몬스터 별 뷰를 만들 수 있는 에셋
        string strAssetID = "MonsterHUD";

        // 각 몬스터HUD 인스턴스 태그를 만들 때 사용할 Prefix
        _wstring strInstancePrefix = L"GameHUD_MonsterHUD";

        _wstring strMonsterLayerTag = TEXT("Layer_Monster");

        _float2 vScreenOffset = _float2(0.f, 70.f);
        _float2 vFatalScreenOffset = _float2(0.f, 0.f);
        _float2 vGroggyScreenOffset = _float2(0.f, 0.f);
    } MONSTERHUD_DESC;

private:
    typedef struct tagMonsterHPEntry
    {
        CAIObject*          pMonster = nullptr;
        CStatComponent_AI*  pStatCom = nullptr;
        CMonsterHUDViewModel* pViewModel = nullptr;

        CUIObject*          pUIObject = nullptr;
        CUIProgressBar*     pHPBar = nullptr;
        CUIElement*         pFrame = nullptr;
        CUIElement*         pFatal = nullptr;
        CUIElement*         pGroggy = nullptr;

        _int                iFillSlotID = -1;
        UI_HANDLE           hUI = {};

        CUIText*            pDamageText = nullptr;
        _float2             vDamageTextOffset = {};

        MONSTER_FATAL_ANIM_STATE eFatalAnimState = MONSTER_FATAL_ANIM_STATE::NONE;
        _float4 vFatalBaseColor = { 1.f, 1.f, 1.f, 1.f };

    } MONSTER_HP_ENTRY;

    typedef struct tagMonsterPendingEntry
    {
        CAIObject* pMonster = nullptr;
        UI_HANDLE hUI = {};
    }MONSTER_PENDING_ENTRY;

private:
    CUIMonsterHUDController();
    virtual ~CUIMonsterHUDController() = default;

public:
    virtual HRESULT             Initialize(void* pArg) override;

public:
    //추가
	virtual void                OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;
protected:
    virtual HRESULT             Bind_UI() override;
    virtual void                Update_ViewModel(_float fTimeDelta) override;
    virtual void                Apply_ViewModel() override;

private:
    void                        Register_ExistingMonsters();            
    void                        Process_PendingSpawnActors();           
    void                        Queue_MonsterSpawn(CAIObject* pMonster);

    void                        Update_Entry(MONSTER_HP_ENTRY& Entry, CGameObject* pLockedTarget, CAIObject* pFatalPossibleTarget, _bool bFatalActionStarted, _float fTimeDelta);
    void                        Update_FatalAnimation(MONSTER_HP_ENTRY& Entry, MONSTER_FATAL_ANIM_STATE eNextState);

    _bool                       Is_Registered(CAIObject* pMonster) const;

    HRESULT                     Request_MonsterHUD(CAIObject* pMonster);
    HRESULT                     Complete_MonsterHUD(const MONSTER_PENDING_ENTRY& Pending);
    void                        Resolve_PendingUIBindings();
    _bool                       Is_PendingRegistration(CAIObject* pMonster) const;

private:
    vector<MONSTER_HP_ENTRY>    m_Entries;

    unordered_set<CAIObject*>   m_FailedRegistrations;
    unordered_set<CAIObject*>   m_PendingSpawnActors;       // 스폰 이벤트 안에서 UI를 바로 생성하지 않고 다음 컨트롤러 업데이트까지 보관

    vector<MONSTER_PENDING_ENTRY>   m_PendingUIBindings;

    string                      m_strAssetID = "MonsterHUD";

    _wstring                    m_strInstancePrefix = L"GameHUD_MonsterHUD";
    _wstring                    m_strMonsterLayerTag = TEXT("Layer_Monster");

    _wstring                    m_strHPBarElementTag = L"MonsterHPBar";
    _wstring                    m_strFrameElementTag = L"MonsterHPBar_Frame";
    _wstring                    m_strFillSlotName = L"Fill";

    _wstring                    m_strGroggyElementTag = L"MonsterGroggy";
    _float2                     m_vGroggyScreenOffset = _float2(0.f, 0.f);

    _float2                     m_vScreenOffset = _float2(0.f, 70.f);
    _uint                       m_iNextInstanceID = 0;

    _wstring                    m_strFatalElementTag = L"Fatal";
    _float2                     m_vFatalScreenOffset = _float2(0.f, 0.f);

    _wstring                    m_strDamageTextElementTag = L"MonsterDamageText";

public:
    static CUIMonsterHUDController* Create();
    virtual void Free() override;
};

NS_END