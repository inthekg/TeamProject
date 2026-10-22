#include "UIFontRegistry.h"

CUIFontRegistry::~CUIFontRegistry()
{
	Clear();
}

// Registry가 Font 생성에 사용할 Device 연결
HRESULT CUIFontRegistry::Initialize(ID3D11Device* pDevice)
{
	if (pDevice == nullptr)
		return E_FAIL;

	m_pDevice = pDevice;

	return S_OK;
}

// FontAssetID와 spritefont 파일을 등록
HRESULT CUIFontRegistry::Register_Font(const string& strFontAssetID, const _wstring& strFontPath)
{
    if (m_pDevice == nullptr)
        return E_FAIL;

    if (strFontAssetID.empty() || strFontPath.empty())
        return E_FAIL;

    if (Contains_Font(strFontAssetID))
        return E_FAIL;

    // 경로가 잘못된 폰트는 Catalog 등록 단계에서 차단
    const DWORD iFileAttributes = GetFileAttributesW(strFontPath.c_str());

    if (iFileAttributes == INVALID_FILE_ATTRIBUTES || (iFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        return E_FAIL;
    }

    FONT_ENTRY Entry = {};

    Entry.strFontPath = strFontPath;
    Entry.pFont = nullptr;

    m_Fonts.emplace(strFontAssetID, Entry);

    /*
     * 여기서는 SpriteFont를 생성하지 않는다.
     * 실제 Text가 Find_Font를 호출할 때 생성
     */

    return S_OK;
}

// 등록된 SpriteFont 조회
SpriteFont* CUIFontRegistry::Find_Font(const string& strFontAssetID)
{
    auto Iter = m_Fonts.find(strFontAssetID);

    if (Iter == m_Fonts.end())
        return nullptr;

    FONT_ENTRY& Entry = Iter->second;

    // 이미 만들어진 폰트는 그대로 재사용
    if (Entry.pFont != nullptr)
        return Entry.pFont;

    try
    {
        Entry.pFont = new SpriteFont(m_pDevice, Entry.strFontPath.c_str());
    }
    catch (const exception& Exception)
    {
        Safe_Delete(Entry.pFont);

        const string strError = "[UI Font Registry] Failed to load font: " + strFontAssetID + " / " + Exception.what() + "\n";

        OutputDebugStringA(strError.c_str());

        return nullptr;
    }
    catch (...)
    {
        Safe_Delete(Entry.pFont);

        const string strError = "[UI Font Registry] Unknown load error: " + strFontAssetID + "\n";

        OutputDebugStringA(strError.c_str());

        return nullptr;
    }

    return Entry.pFont;
}

// FontAssetID 등록 여부 확인
_bool CUIFontRegistry::Contains_Font(const string& strFontAssetID) const
{
    return m_Fonts.find(strFontAssetID) != m_Fonts.end();
}

size_t CUIFontRegistry::Get_FontCount() const
{
    return m_Fonts.size();
}

void CUIFontRegistry::Clear()
{
    for (auto& Pair : m_Fonts)
    {
        Safe_Delete(Pair.second.pFont);
    }

    m_Fonts.clear();
}