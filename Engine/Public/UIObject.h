#pragma once
#include "Base.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIButton;
class CGameInstance;
class CUIAnimationPlayer;
struct UI_ANIMATION_CLIP_ASSET_DESC;

class ENGINE_DLL CUIObject : public CBase
{
protected:
    CUIObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CUIObject() = default;


public:
    _bool                                   Is_Visible() const;
    _uint                                   Get_SortOrder() const;
    CUIElement*                             Get_RootElement() const;
    const vector<CUIButton*>&               Get_Buttons() const;
    _bool                                   Is_RenderSuppressed() const;

    _bool                                   Has_Animation(const string& strClipName) const;
    _bool                                   Is_AnimationPlaying() const;
    _bool                                   Is_AnimationFinished() const;

public:
    void                                    Set_Visible(_bool bVisible);
    void                                    Set_SortOrder(_uint iSortOrder);
    HRESULT                                 Set_RootElement(CUIElement* pRootElement);
    void                                    Set_RenderSuppressed(_bool bSuppressed);
    // 에셋의 Animation Clip을 이 UIObject의 런타임 플레이어에 등록한다.
    HRESULT                                 Set_Animations(const vector<UI_ANIMATION_CLIP_ASSET_DESC>& Animations, const _float2& vRuntimeScale);

public:
    virtual HRESULT                         Initialize();

    virtual void                            Priority_Update(_float fTimeDelta);
    virtual void                            Update(_float fTimeDelta);
    virtual void                            Late_Update(_float fTimeDelta);

    virtual void                            Submit_RenderCommand();

public:
    HRESULT                                 Add_Element(const _wstring& strElementTag, CUIElement* pElement);
    CUIElement*                             Find_Element(const _wstring& strElementTag);
    CUIElement*                             Find_Element(_uint iElementID);

    // Screen 원본 에셋에 직접 속한 Element를 Animation 대상 ID로 등록한다.
    HRESULT                                 Register_AnimationTarget(CUIElement* pElement);

    HRESULT                                 Register_Button(CUIButton* pButton);

    void                                    Remove_Element(const _wstring& strElementTag);
    void                                    Clear_Elements();

    // Controller에서 이름으로 Animation Clip을 재생한다.
    _bool                                   Play_Animation(const string& strClipName, _bool bRestart = true);
    void                                    Stop_Animation();
    void                                    Pause_Animation();
    void                                    Resume_Animation();
    _bool                                   Set_AnimationTime(_float fTime);

    // Open/Close 애니메이션을 거쳐 렌더 억제 상태를 변경합니다.
    void                                    Request_RenderSuppressed(_bool bSuppressed); 


protected:
    ID3D11Device*                           m_pDevice = { nullptr };
    ID3D11DeviceContext*                    m_pContext = { nullptr };
    CGameInstance*                          m_pGameInstance = { nullptr };

    _bool                                   m_bVisible = { true };
    _uint                                   m_iSortOrder = {};

    CUIElement*                             m_pRootElement = { nullptr };
    unordered_map<_wstring, CUIElement*>    m_Elements;

    vector<CUIButton*>                      m_Buttons;      // 중앙 Input Router의 빠른 접근을 위한 비소유 캐시

    // Component 내부 ID 충돌을 피하기 위한 Screen 원본 Element ID 캐시
    unordered_map<_uint, CUIElement*>       m_AnimationTargets;

    CUIAnimationPlayer*                     m_pAnimationPlayer = { nullptr };

private:

     // 논리적인 Visible 상태는 유지하면서 렌더링만 일시적으로 차단
    _bool                                   m_bRenderSuppressed = { false };

private:
    _bool                                   m_bRenderSuppressionRequested = false; // 최종적으로 적용하려는 렌더 억제 상태
    _bool                                   m_bPendingRenderSuppression = false; // Close 애니메이션 종료를 기다리는 중인지 

public:
    static CUIObject* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END