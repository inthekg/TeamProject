#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

// FontAssetID와 DirectXTK SpriteFont를 연결하고 공유 소유하는 저장소
class CUIFontRegistry final
{
private:
    struct FONT_ENTRY
    {
        _wstring strFontPath;

        // 처음 사용될 때 생성되며 Registry가 소유
        SpriteFont* pFont = { nullptr };
    };

public:
	CUIFontRegistry() = default;
	~CUIFontRegistry();

	CUIFontRegistry(const CUIFontRegistry&) = delete;
	CUIFontRegistry& operator=(const CUIFontRegistry&) = delete;


public:
    HRESULT                             Initialize(ID3D11Device* pDevice);

    HRESULT                             Register_Font(const string& strFontAssetID, const _wstring& strFontPath);

    SpriteFont*                         Find_Font(const string& strFontAssetID);
    
    _bool                               Contains_Font(const string& strFontAssetID) const;

    size_t                              Get_FontCount() const;

    void                                Clear();

private:
    // CUI_Manager가 Device 수명을 보장하므로 비소유
    ID3D11Device*                       m_pDevice = { nullptr };

    unordered_map<string, FONT_ENTRY>   m_Fonts;
};

NS_END