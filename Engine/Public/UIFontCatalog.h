#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

// 에디터 TTF와 런타임 SpriteFont를 같은 FontAssetID로 연결
struct UI_FONT_CATALOG_ENTRY
{
    string strFontAssetID;

    // Resource 폴더 기준 상대 경로
    string strEditorTTFPath;
    string strRuntimeSpriteFontPath;

    // 현재 생성된 SpriteFont에 한글 Glyph가 포함됐는지
    _bool bSupportKorean = { false };
};

class ENGINE_DLL CUIFontCatalog final
{
public:
    // 프로젝트에서 선택 가능한 전체 폰트 목록
    static const vector<UI_FONT_CATALOG_ENTRY>& Get_Entries();

    // FontAssetID로 Catalog Entry 검색
    static const UI_FONT_CATALOG_ENTRY*         Find_Entry(const string& strFontAssetID);
};

NS_END