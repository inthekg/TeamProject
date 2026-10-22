#pragma once
#include "UIElement.h"

NS_BEGIN(Engine)

enum class UI_TEXT_HORIZONTAL_ALIGN
{
    LEFT,
    CENTER,
    RIGHT,
};

enum class UI_TEXT_VERTICAL_ALIGN
{
    TOP,
    CENTER,
    BOTTOM,
};

class ENGINE_DLL CUIText : public CUIElement
{
private:
    // Canvas와 동일하게 한 줄씩 배치하기 위한 런타임 캐시
    struct TEXT_LINE_LAYOUT
    {
        _wstring strText;
        _float fWidth = { 0.f };
    };


protected:
    CUIText(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CUIText() = default;

public:
    void                        Set_Text(const _wstring& strText);
    void                        Set_Font(const SpriteFont* pFont);
    void                        Set_FontSize(_float fFontSize);
    void                        Set_HorizontalAlign(UI_TEXT_HORIZONTAL_ALIGN eAlign);
    void                        Set_VerticalAlign(UI_TEXT_VERTICAL_ALIGN eAlign);
    void                        Set_LineSpacing(_float fLineSpacing);
    void                        Set_Wrap(_bool bWrap);

public:
    const _wstring&             Get_Text() const;
    // 현재 FontSize가 적용된 문자열 너비 측정
    _float                      Measure_TextWidth(const _wstring& strText) const;      

public:
    virtual HRESULT			    Initialize() override;
    virtual void			    Priority_Update(_float fTimeDelta) override;
    virtual void			    Update(_float fTimeDelta) override;
    virtual void			    Late_Update(_float fTimeDelta) override;
    virtual void			    Submit_RenderCommand() override;
    virtual void			    Submit_RenderCommand(_uint iObjectSortOrder) override;

private:
    _bool                       Rebuild_TextLayout();

    void                        Add_LayoutLine(const _wstring& strText);
    
    void                        Wrap_Paragraph(const _wstring& strParagraph, _float fMaxWidth);

private:
    _wstring                    m_strText;

    // CUIFontRegistry가 소유하며 CUIText는 삭제하지 않음
    const SpriteFont*           m_pFont = { nullptr };

    _float                      m_fFontSize = { 32.f };
    _float                      m_fLineSpacing = { 1.f };

    UI_TEXT_HORIZONTAL_ALIGN    m_eHorizontalAlign = UI_TEXT_HORIZONTAL_ALIGN::LEFT;
    UI_TEXT_VERTICAL_ALIGN      m_eVerticalAlign = UI_TEXT_VERTICAL_ALIGN::TOP;

    _bool                       m_bWrap = { false };

    // 줄별 문자열과 측정 너비를 저장
    vector<TEXT_LINE_LAYOUT>    m_TextLines;

    // 첫 줄 높이와 줄 간격을 포함한 전체 Text 높이
    _float                      m_fTextBlockHeight = { 0.f };

    // 영역 크기가 바뀌었는지 확인하기 위한 이전 값
    _float2                     m_vCachedLayoutSize = { -1.f, -1.f };

    // 문자열, 폰트, 크기 등이 바뀌면 true
    _bool                       m_bLayoutDirty = { true };

public:
    static CUIText* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END