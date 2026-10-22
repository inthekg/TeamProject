#pragma once
#include "UIElement.h"

NS_BEGIN(Engine)

// 마우스 입력에 따른 배타적 상태
enum class UI_BUTTON_POINTER_STATE
{
    NORMAL,
    HOVERED,
    PRESSED,
};

// 에셋의 상태별 Override 분류
enum class UI_BUTTON_VISUAL_STATE
{
    HOVERED,
    PRESSED,
    SELECTED,
    FOCUSED,
    DISABLED,
    END,
};

class ENGINE_DLL CUIButton : public CUIElement
{
public:
    // UI Manager가 Asset Desc를 런타임 데이터로 변환해 전달
    struct UI_BUTTON_VISUAL_OVERRIDE
    {
        CUIElement* pTarget = { nullptr };

        _bool		bOverrideVisible = { false };
        _bool		bVisible = { true };

        _bool		bOverrideColor = { false };
        _float4		vColor = { 1.f, 1.f, 1.f, 1.f };

        _bool		bOverrideScale = { false };
        _float2		vScale = { 1.f, 1.f };
    };

private:
    // 상태 적용 전 자식의 원래 값 또는 계산된 목표값
    struct UI_BUTTON_VISUAL_VALUE
    {
        CUIElement* pTarget = { nullptr };

        _bool		bVisible = { true };
        _float4		vColor = { 1.f, 1.f, 1.f, 1.f };
        _float2		vScale = { 1.f, 1.f };
    };

    // 실제 상태 Override는 Target 포인터 대신 Visual 배열 인덱스를 사용
    struct UI_BUTTON_RESOLVED_OVERRIDE
    {
        _uint		iVisualIndex = { UINT_MAX };

        _bool		bOverrideVisible = { false };
        _bool		bVisible = { true };

        _bool		bOverrideColor = { false };
        _float4		vColor = { 1.f, 1.f, 1.f, 1.f };

        _bool		bOverrideScale = { false };
        _float2		vScale = { 1.f, 1.f };
    };

protected:
    CUIButton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CUIButton() = default;

public:
    void	                                    Set_CommandKey(const string& strCommandKey);
    void                                        Set_FocusOnHover(_bool bFocusOnHover);
    void                                        Set_SelectOnFocus(_bool bSelectOnFocus);
    void                                        Set_CommandOnPress(_bool bCommandOnPress);
    void	                                    Set_FocusGroup(const string& strFocusGroup);
    void	                                    Set_FocusOrder(_int iFocusOrder);

    void	                                    Set_Interactable(_bool bInteractable);
    void	                                    Set_Selected(_bool bSelected);
    void	                                    Set_Focused(_bool bFocused);

    void	                                    Set_PointerState(UI_BUTTON_POINTER_STATE ePointerState);

public:
    const string&                               Get_CommandKey() const;
    const string&                               Get_FocusGroup() const;
    _int			                            Get_FocusOrder() const;

    _bool			                            Is_Interactable() const;
    _bool			                            Is_Selected() const;
    _bool			                            Is_Focused() const;
    _bool                                       Is_PointInside(const _float2& vUIPoint) const;
    _bool                                       Is_InputAvailable() const;
    _bool                                       Should_FocusOnHover() const;
    _bool                                       Should_SelectOnFocus() const;
    _bool                                       Should_CommandOnPress() const;

    UI_BUTTON_POINTER_STATE                     Get_PointerState() const;

public:
    HRESULT                                     Add_VisualOverride(UI_BUTTON_VISUAL_STATE eState, const UI_BUTTON_VISUAL_OVERRIDE& Override);
    void	                                    Refresh_VisualState();

public:
    virtual HRESULT			                    Initialize() override;
    virtual void			                    Priority_Update(_float fTimeDelta) override;
    virtual void			                    Update(_float fTimeDelta) override;
    virtual void			                    Late_Update(_float fTimeDelta) override;
    virtual void			                    Submit_RenderCommand() override;
    virtual void			                    Submit_RenderCommand(_uint iObjectSortOrder) override;

private:
    _uint	                                    Find_OrAddBaseVisual(CUIElement* pTarget);

    void	                                    Resolve_TargetVisuals();

    void	                                    Apply_ResolvedState(UI_BUTTON_VISUAL_STATE eState);
    void	                                    Apply_TargetVisualsImmediately();

private:
    string	                                    m_strCommandKey;
    string	                                    m_strFocusGroup = "Default";
    _int	                                    m_iFocusOrder = {};

    _bool	                                    m_bInteractable = { true };
    _bool	                                    m_bSelected = { false };
    _bool	                                    m_bFocused = { false };
    _bool                                       m_bFocusOnHover = { false };
    _bool                                       m_bSelectOnFocus = { false };
    _bool                                       m_bCommandOnPress = { false };

    UI_BUTTON_POINTER_STATE                     m_ePointerState = UI_BUTTON_POINTER_STATE::NORMAL;

    // 에디터에서 저장한 자식의 원래 런타임 값
    vector<UI_BUTTON_VISUAL_VALUE>              m_BaseVisuals;

    // 현재 상태를 반영해 계산한 목표값
    vector<UI_BUTTON_VISUAL_VALUE>              m_TargetVisuals;

    vector<UI_BUTTON_RESOLVED_OVERRIDE>         m_StateOverrides[static_cast<_uint>(UI_BUTTON_VISUAL_STATE::END)];

public:
    static CUIButton* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END