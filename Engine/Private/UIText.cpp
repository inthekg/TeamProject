#include "UIText.h"
#include "GameInstance.h"
#include <cwctype>

CUIText::CUIText(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUIElement{ pDevice, pContext }
{
}

void CUIText::Set_Text(const _wstring& strText)
{
    if (m_strText == strText)
        return;

	m_strText = strText;
    m_bLayoutDirty = true;
}

void CUIText::Set_Font(const SpriteFont* pFont)
{
    if (m_pFont == pFont)
        return;

    m_pFont = pFont;
    m_bLayoutDirty = true;
}

void CUIText::Set_FontSize(_float fFontSize)
{
    const _float fNewFontSize = max(1.f, fFontSize);

    if (fabsf(m_fFontSize - fNewFontSize) <= FLT_EPSILON)
        return;

    m_fFontSize = fNewFontSize;
    m_bLayoutDirty = true;
}

void CUIText::Set_HorizontalAlign(UI_TEXT_HORIZONTAL_ALIGN eAlign)
{
    if (m_eHorizontalAlign == eAlign)
        return;

    m_eHorizontalAlign = eAlign;
    m_bLayoutDirty = true;
}

void CUIText::Set_VerticalAlign(UI_TEXT_VERTICAL_ALIGN eAlign)
{
    if (m_eVerticalAlign == eAlign)
        return;

    m_eVerticalAlign = eAlign;
    m_bLayoutDirty = true;
}

void CUIText::Set_LineSpacing(_float fLineSpacing)
{
    const _float fNewLineSpacing = max(0.1f, fLineSpacing);

    if (fabsf(m_fLineSpacing - fNewLineSpacing) <= FLT_EPSILON)
        return;

    m_fLineSpacing = fNewLineSpacing;
    m_bLayoutDirty = true;
}

void CUIText::Set_Wrap(_bool bWrap)
{
    if (m_bWrap == bWrap)
        return;

    m_bWrap = bWrap;
    m_bLayoutDirty = true;
}

const _wstring& CUIText::Get_Text() const
{
	return m_strText;
}

HRESULT CUIText::Initialize()
{
	return S_OK;
}

void CUIText::Priority_Update(_float fTimeDelta)
{
}

void CUIText::Update(_float fTimeDelta)
{
}

void CUIText::Late_Update(_float fTimeDelta)
{
}

void CUIText::Submit_RenderCommand()
{
    Submit_RenderCommand(0);
}

void CUIText::Submit_RenderCommand(_uint iObjectSortOrder)
{
    if (!Is_HierarchyVisible())
        return;

    if (m_pFont == nullptr || m_strText.empty())
        return;

    // Set_Size는 CUIElement 함수이므로 CUIText Setter를 통하지 않는다. 따라서 이전 영역 크기와 직접 비교한다.

    if (fabsf(m_vCachedLayoutSize.x - m_tTransform.vSize.x) > FLT_EPSILON ||
        fabsf(m_vCachedLayoutSize.y - m_tTransform.vSize.y) > FLT_EPSILON)
    {
        m_bLayoutDirty = true;
    }

    if (m_bLayoutDirty)
    {
        if (!Rebuild_TextLayout())
            return;
    }

    const _float fBaseLineSpacing = m_pFont->GetLineSpacing();

    if (fBaseLineSpacing <= FLT_EPSILON)
        return;

    const _float fFontScale = m_fFontSize / fBaseLineSpacing;

    _float fStartY = 0.f;

    switch (m_eVerticalAlign)
    {
    case UI_TEXT_VERTICAL_ALIGN::TOP:
        fStartY = 0.f;
        break;

    case UI_TEXT_VERTICAL_ALIGN::CENTER:
        fStartY = (m_tTransform.vSize.y - m_fTextBlockHeight) * 0.5f;
        break;

    case UI_TEXT_VERTICAL_ALIGN::BOTTOM:
        fStartY = m_tTransform.vSize.y - m_fTextBlockHeight;
        break;
    }

    const _float2 vAnchoredPosition = Get_AnchoredPosition();

    const _matrix LocalTextMatrix =
        XMMatrixScaling(1.f, -1.f, 1.f) *
        XMMatrixTranslation(-m_tTransform.vPivot.x * m_tTransform.vSize.x, m_tTransform.vPivot.y * m_tTransform.vSize.y, 0.f) *
        XMMatrixScaling(m_tTransform.vScale.x, m_tTransform.vScale.y, 1.f) *
        XMMatrixRotationZ(m_tTransform.fRotation) *
        XMMatrixTranslation(vAnchoredPosition.x, vAnchoredPosition.y, 0.f)*
        Get_ParentWorldLayoutMatrix();

    const _float2 vViewport = m_pGameInstance->Get_ViewportDesc();

    if (vViewport.x <= 0.f || vViewport.y <= 0.f)
        return;

    const _matrix UIScreenMatrix =
        XMMatrixScaling(1.f, -1.f, 1.f) *
        XMMatrixTranslation(vViewport.x * 0.5f, vViewport.y * 0.5f, 0.f);

    const _matrix TextScreenMatrix = LocalTextMatrix * UIScreenMatrix;
    const _uint iFinalSortOrder = iObjectSortOrder * UI_ELEMENT_SORT_RANGE + m_tRenderData.iSortOrder;
    const _float fLineAdvance = m_fFontSize * m_fLineSpacing;

    for (size_t iLine = 0; iLine < m_TextLines.size(); ++iLine)
    {
        const TEXT_LINE_LAYOUT& Line = m_TextLines[iLine];

        // 빈 줄은 그리지 않지만 세로 배치 간격은 유지
        if (Line.strText.empty())
            continue;

        _float fLineX = 0.f;

        switch (m_eHorizontalAlign)
        {
        case UI_TEXT_HORIZONTAL_ALIGN::LEFT:
            fLineX = 0.f;
            break;

        case UI_TEXT_HORIZONTAL_ALIGN::CENTER:
            fLineX = (m_tTransform.vSize.x - Line.fWidth) * 0.5f;
            break;

        case UI_TEXT_HORIZONTAL_ALIGN::RIGHT:
            fLineX = m_tTransform.vSize.x - Line.fWidth;
            break;
        }

        const _float fLineY = fStartY + fLineAdvance * static_cast<_float>(iLine);

        UITextCommand Command = {};

        Command.header.etype = RENDER_TYPE::UI_TEXT;
        Command.header.iCommandSize = sizeof(UITextCommand);
        Command.header.iSortKey = iFinalSortOrder;

        XMStoreFloat4x4(&Command.worldMatrix, TextScreenMatrix);

        Command.pFont = m_pFont;
        Command.pText = Line.strText.c_str();

        Command.vPosition = _float2(fLineX, fLineY);
        Command.vOrigin = _float2(0.f, 0.f);
        Command.vScale = _float2(fFontScale, fFontScale);

        Command.vColor = m_tRenderData.vColor;

        // 텍스트 자신의 RGB는 유지하면서 루트까지의 Alpha를 반영한다.
        Command.vColor.w = Get_HierarchyAlpha();
        Command.fRotation = 0.f;
        Command.fLayerDepth = 0.f;

        m_pGameInstance->Submit_UIText(Command);
    }
}

// Text 또는 영역이 바뀌었을 때 줄 배치를 다시 계산
_bool CUIText::Rebuild_TextLayout()
{
    m_TextLines.clear();
    m_fTextBlockHeight = 0.f;

    m_vCachedLayoutSize = m_tTransform.vSize;

    m_bLayoutDirty = false;

    if (m_pFont == nullptr || m_strText.empty())
        return false;

    size_t iParagraphBegin = 0;

    while (iParagraphBegin <= m_strText.size())
    {
        const size_t iParagraphEnd = m_strText.find_first_of(L"\r\n", iParagraphBegin);
        const size_t iActualParagraphEnd = iParagraphEnd == _wstring::npos ? m_strText.size() : iParagraphEnd;

        const _wstring strParagraph = m_strText.substr(iParagraphBegin, iActualParagraphEnd - iParagraphBegin);

        if (m_bWrap)
        {
            Wrap_Paragraph(strParagraph, m_tTransform.vSize.x);
        }
        else
        {
            Add_LayoutLine(strParagraph);
        }

        if (iParagraphEnd == _wstring::npos)
            break;

        // Windows 개행 \r\n은 문자 두 개지만 하나의 개행으로 처리한다.

        if (m_strText[iParagraphEnd] == L'\r' &&
            iParagraphEnd + 1 < m_strText.size() &&
            m_strText[iParagraphEnd + 1] == L'\n')
        {
            iParagraphBegin = iParagraphEnd + 2;
        }
        else
        {
            iParagraphBegin = iParagraphEnd + 1;
        }

        // 문자열 마지막의 개행도 빈 줄로 보존
        if (iParagraphBegin == m_strText.size())
        {
            Add_LayoutLine(L"");
            break;
        }
    }

    if (m_TextLines.empty())
        return false;

    const _float fLineAdvance = m_fFontSize * m_fLineSpacing;

    m_fTextBlockHeight = m_fFontSize + fLineAdvance * static_cast<_float>(m_TextLines.size() - 1);

    return true;
}

_float CUIText::Measure_TextWidth(const _wstring& strText) const
{
    if (m_pFont == nullptr || strText.empty())
        return 0.f;

    const _float fBaseLineSpacing = m_pFont->GetLineSpacing();
    if (fBaseLineSpacing <= FLT_EPSILON)
        return 0.f;

    const _float fFontScale = m_fFontSize / fBaseLineSpacing;

    _float2 vMeasuredSize = {};
    XMStoreFloat2(&vMeasuredSize, m_pFont->MeasureString(strText.c_str(), false));

    return vMeasuredSize.x * fFontScale;
}

// 계산된 한 줄을 Layout Cache에 추가
void CUIText::Add_LayoutLine(const _wstring& strText)
{
    TEXT_LINE_LAYOUT Line = {};

    Line.strText = strText;
    Line.fWidth = Measure_TextWidth(strText);

    m_TextLines.push_back(Line);
}

// 한 문단을 Transform.vSize.x에 맞춰 여러 줄로 분리
void CUIText::Wrap_Paragraph(const _wstring& strParagraph, _float fMaxWidth)
{ 
    // 빈 문단도 하나의 줄로 보존
    if (strParagraph.empty())
    {
        Add_LayoutLine(L"");
        return;
    }

    if (fMaxWidth <= FLT_EPSILON)
    {
        Add_LayoutLine(strParagraph);
        return;
    }

    _wstring strRemaining = strParagraph;

    while (!strRemaining.empty())
    {
        size_t iFitCount = 0;
        size_t iLastWhitespace = _wstring::npos;

        /*
         * 앞에서부터 한 글자씩 늘려가며
         * 영역 안에 들어가는 마지막 위치를 찾는다.
         */
        for (size_t i = 0; i < strRemaining.size(); ++i)
        {
            const _wstring strCandidate = strRemaining.substr(0, i + 1);
            const _float fCandidateWidth = Measure_TextWidth(strCandidate);

            if (fCandidateWidth > fMaxWidth + FLT_EPSILON)
                break;

            iFitCount = i + 1;

            if (iswspace(strRemaining[i]) != 0)
                iLastWhitespace = i;
        }

        // 남은 문자열 전체가 한 줄에 들어감
        if (iFitCount >= strRemaining.size())
        {
            Add_LayoutLine(strRemaining);
            break;
        }

        /*
         * 한 글자도 들어가지 않을 정도로 영역이 작아도
         * 무한 반복되지 않도록 최소 한 글자는 배치
         */
        if (iFitCount == 0)
            iFitCount = 1;

        size_t iLineCharacterCount = iFitCount;
        size_t iConsumedCharacterCount = iFitCount;

         // 영역 안에 들어온 구간에 공백이 있다면 단어 중간보다 마지막 공백에서 먼저 줄바꿈한다.
        if (iLastWhitespace != _wstring::npos && iLastWhitespace < iFitCount)
        {
            iLineCharacterCount = iLastWhitespace;

            iConsumedCharacterCount = iLastWhitespace + 1;
        }

        _wstring strLine = strRemaining.substr(0, iLineCharacterCount);

        // 줄 끝의 띄어쓰기 제거
        while (!strLine.empty() && iswspace(strLine.back()) != 0)
        {
            strLine.pop_back();
        }

        if (!strLine.empty())
            Add_LayoutLine(strLine);

        strRemaining.erase(0, iConsumedCharacterCount);

        // 다음 줄 시작의 띄어쓰기 제거
        while (!strRemaining.empty() && iswspace(strRemaining.front()) != 0)
        {
            strRemaining.erase(strRemaining.begin());
        }
    }
}


CUIText* CUIText::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUIText* pInstance = new CUIText(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CUIText");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUIText::Free()
{
	__super::Free();

    m_TextLines.clear();

    m_pFont = nullptr;
}
