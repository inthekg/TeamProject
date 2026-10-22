#include "UIFontCatalog.h"

const vector<UI_FONT_CATALOG_ENTRY>& CUIFontCatalog::Get_Entries()
{
    // Ko 파일은 기존 JSON 호환을 위해 DefaultUI와 TitleUI라는 안정적인 ID를 사용한다.

    static const vector<UI_FONT_CATALOG_ENTRY> Entries =
    {
        {
            "DefaultUI",
            "Font/UIFF_Info_Midium_Ko.ttf",
            "Font/UIFF_Info_Midium_Ko.spritefont",
            true
        },
        {
            "TitleUI",
            "Font/UIFF_Title_Bold_Ko.ttf",
            "Font/UIFF_Title_Bold_Ko.spritefont",
            true
        },
        {
            "NoiseUI",
            "Font/UIFF_Noise.ttf",
            "Font/UIFF_Noise.spritefont",
            false
        },
        {
            "DefaultUI_Small",
            "Font/UIFF_Info_Midium_Ko.ttf",
            "Font/UIFF_Info_Midium_Ko_20.spritefont",
            true
        },
        {
            "TitleUI_Small",
            "Font/UIFF_Title_Bold_Ko.ttf",
            "Font/UIFF_Title_Bold_Ko_20.spritefont",
            true
        },
        {
            "NoiseUI_Small",
            "Font/UIFF_Noise.ttf",
            "Font/UIFF_Noise_20.spritefont",
            false
        },

        {
            "Info_Arabic",
            "Font/UIFF_Info_Midium_Arabic.ttf",
            "Font/UIFF_Info_Midium_Arabic.spritefont",
            false
        },
        {
            "Info_Bengali",
            "Font/UIFF_Info_Midium_Bengali.ttf",
            "Font/UIFF_Info_Midium_Bengali.spritefont",
            false
        },
        {
            "Info_Devanagari",
            "Font/UIFF_Info_Midium_Devanagari.ttf",
            "Font/UIFF_Info_Midium_Devanagari.spritefont",
            false
        },
        {
            "Info_English",
            "Font/UIFF_Info_Midium_En.ttf",
            "Font/UIFF_Info_Midium_En.spritefont",
            false
        },
        {
            "Info_Hebrew",
            "Font/UIFF_Info_Midium_Hebrew.ttf",
            "Font/UIFF_Info_Midium_Hebrew.spritefont",
            false
        },
        {
            "Info_Japanese",
            "Font/UIFF_Info_Midium_Jp.ttf",
            "Font/UIFF_Info_Midium_Jp.spritefont",
            true
        },
        {
            "Info_Sub",
            "Font/UIFF_Info_Midium_Sub.ttf",
            "Font/UIFF_Info_Midium_Sub.spritefont",
            false
        },
        {
            "Info_Thai",
            "Font/UIFF_Info_Midium_Thai.ttf",
            "Font/UIFF_Info_Midium_Thai.spritefont",
            false
        },
        {
            "Info_ChineseSimplified",
            "Font/UIFF_Info_Midium_zh_Hans.ttf",
            "Font/UIFF_Info_Midium_zh_Hans.spritefont",
            true
        },
        {
            "Info_ChineseTraditional",
            "Font/UIFF_Info_Midium_zh_Hant.ttf",
            "Font/UIFF_Info_Midium_zh_Hant.spritefont",
            true
        },

        {
            "Title_Arabic",
            "Font/UIFF_Title_Bold_Arabic.ttf",
            "Font/UIFF_Title_Bold_Arabic.spritefont",
            false
        },
        {
            "Title_Bengali",
            "Font/UIFF_Title_Bold_Bengali.ttf",
            "Font/UIFF_Title_Bold_Bengali.spritefont",
            false
        },
        {
            "Title_Devanagari",
            "Font/UIFF_Title_Bold_Devanagari.ttf",
            "Font/UIFF_Title_Bold_Devanagari.spritefont",
            false
        },
        {
            "Title_English",
            "Font/UIFF_Title_Bold_En.ttf",
            "Font/UIFF_Title_Bold_En.spritefont",
            false
        },
        {
            "Title_Hebrew",
            "Font/UIFF_Title_Bold_Hebrew.ttf",
            "Font/UIFF_Title_Bold_Hebrew.spritefont",
            false
        },
        {
            "Title_Japanese",
            "Font/UIFF_Title_Bold_Jp.ttf",
            "Font/UIFF_Title_Bold_Jp.spritefont",
            true
        },
        {
            "Title_Sub",
            "Font/UIFF_Title_Bold_Sub.ttf",
            "Font/UIFF_Title_Bold_Sub.spritefont",
            false
        },
        {
            "Title_Thai",
            "Font/UIFF_Title_Bold_Thai.ttf",
            "Font/UIFF_Title_Bold_Thai.spritefont",
            false
        },
        {
            "Title_ChineseSimplified",
            "Font/UIFF_Title_Bold_zh_Hans.ttf",
            "Font/UIFF_Title_Bold_zh_Hans.spritefont",
            true
        },
        {
            "Title_ChineseTraditional",
            "Font/UIFF_Title_Bold_zh_Hant.ttf",
            "Font/UIFF_Title_Bold_zh_Hant.spritefont",
            true
        },
    };

    return Entries;
}

const UI_FONT_CATALOG_ENTRY* CUIFontCatalog::Find_Entry(const string& strFontAssetID)
{
    const vector<UI_FONT_CATALOG_ENTRY>& Entries = Get_Entries();

    for (const UI_FONT_CATALOG_ENTRY& Entry : Entries)
    {
        if (Entry.strFontAssetID == strFontAssetID)
            return &Entry;
    }

    return nullptr;
}
