#pragma once
#include "UIViewModelBase.h"

NS_BEGIN(GameCore)

enum class EQUIPMENT_CATEGORY
{
    EQUIPMENT,
    USE_ITEM,
    COSTUME,
    END
};

enum class EQUIPMENT_DETAIL_MODE
{
    NONE,               // 상세 패널을 표시하지 않음
    ITEM,               // 사용 아이템 상세 정보
    WEAPON_ASSEMBLED,   // 날 + 손잡이 조합형 무기
    WEAPON_SINGLE,      // 단일 이미지 무기
    END
};

enum class COSTUME_CATEGORY
{
    ACCESSORY,
    MASK,
    BODY,
    END
};

struct EQUIPMENT_DETAIL_DATA
{
    EQUIPMENT_DETAIL_MODE eMode = { EQUIPMENT_DETAIL_MODE::NONE };

    _wstring strTitle;
    _wstring strCategory;
    _wstring strDescription;
    _wstring strWeaponTitle;

    string strIconTexturePath;
    string strSubIconTexturePath;
};

struct COSTUME_SLOT_DATA
{
    _uint64 iItemID = {};                    // 인벤토리 아이템 ID
    _bool bVisible = { false };              // 현재 슬롯 표시 여부
    _bool bEquipped = { false };             // 현재 장착 중인지 여부
    _wstring strName;                        // 디테일 이름
    _wstring strCategory;                    // 디테일 카테고리
    _wstring strDescription;                 // 디테일 설명
    string strIconTexturePath;               // 슬롯 및 디테일 아이콘 경로
};

class CEquipmentViewModel final : public CUIViewModelBase
{
private:
    CEquipmentViewModel() = default;
    virtual ~CEquipmentViewModel() = default;

public:
    void Open_Equipment();
    void Close_Equipment();
    void Set_Category(EQUIPMENT_CATEGORY eCategory);
    void Move_Category(_int iDirection);

public:
    _bool                           Is_Visible() const { return m_bVisible; }
    EQUIPMENT_CATEGORY              Get_Category() const { return m_eCategory; }
    const EQUIPMENT_DETAIL_DATA&    Get_Detail() const { return m_Detail; }
    _uint                           Get_CurrentErgo() const { return m_iCurrentErgo; }

    void                            Set_ItemDetail(const _wstring& strTitle, const _wstring& strCategory, const _wstring& strDescription, const string& strIconTexturePath);
    void                            Set_WeaponDetail(EQUIPMENT_DETAIL_MODE eMode, const _wstring& strTitle, const _wstring& strWeaponTitle, const _wstring& strDescription, const string& strPrimaryIconTexturePath, const string& strSecondaryIconTexturePath = "");
    void                            Set_SecondaryWeaponOwned(_bool bOwned);
    _bool                           Has_SecondaryWeapon() const { return m_bSecondaryWeaponOwned; }
    void                            Set_CurrentErgo(_uint iCurrentErgo);

    void                            Clear_Detail();


public:
    void                            Open_CostumeSelect(COSTUME_CATEGORY eCategory);      // 코스튬 선택 페이지를 열고 카테고리를 초기화한다.
    void                            Close_CostumeSelect();                               // 코스튬 선택 페이지를 닫는다.
    void                            Set_CostumeCategory(COSTUME_CATEGORY eCategory);     // 카테고리를 변경하고 슬롯 선택을 해제한다.
    void                            Move_CostumeCategory(_int iDirection);               // Q/E 입력에 따라 이전 또는 다음 카테고리로 이동한다.
    void                            Clear_CostumeSlots();                                // 새 카테고리를 채우기 전 기존 슬롯 데이터를 초기화한다.
    void                            Set_CostumeSlotData(_uint iIndex, const COSTUME_SLOT_DATA& Data); // 지정 슬롯의 표시 데이터를 설정한다.
    void                            Select_CostumeSlot(_int iIndex);                     // 클릭된 슬롯을 실제 선택 상태로 지정한다.

    _bool                           Is_CostumeSelectVisible() const { return m_bCostumeSelectVisible; }
    COSTUME_CATEGORY                Get_CostumeCategory() const { return m_eCostumeCategory; }
    _int                            Get_SelectedCostumeSlot() const { return m_iSelectedCostumeSlot; }

    const COSTUME_SLOT_DATA&        Get_CostumeSlotData(_uint iIndex) const;

private:
    // 장비 UI 전체의 Open/Close 상태
    _bool                           m_bVisible = { false };

    // 현재 표시 중인 장비/사용 아이템/코스튬 페이지
    EQUIPMENT_CATEGORY              m_eCategory = { EQUIPMENT_CATEGORY::EQUIPMENT };
    // 현재 포커스된 슬롯의 오른쪽 상세 패널 표시 데이터
    EQUIPMENT_DETAIL_DATA           m_Detail = {};

    _bool                           m_bSecondaryWeaponOwned = { false };

    _bool                           m_bCostumeSelectVisible = { false };                    // 코스튬 선택 페이지 표시 여부
    COSTUME_CATEGORY                m_eCostumeCategory = { COSTUME_CATEGORY::BODY };        // 현재 코스튬 카테고리
    _int                            m_iSelectedCostumeSlot = { -1 };                        // 실제 클릭하여 선택한 슬롯
    COSTUME_SLOT_DATA               m_CostumeSlots[3] = {};                                // 현재 카테고리에 표시할 최대 3개 슬롯

    _uint                           m_iCurrentErgo = { 0 };

public:
    static CEquipmentViewModel* Create();
    virtual void Free() override;
};

NS_END