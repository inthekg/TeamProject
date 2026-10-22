#pragma once
#include "Core_Defines.h"

NS_BEGIN(GameCore)

// 확인창을 어떤 기능에서 요청했는지 구분
enum class CONFIRMATION_ACTION_TYPE
{
    SHOP_PURCHASE,                   // 상점 상품 구매
    LEGION_ARM_UPGRADE,              // 리전 암 강화
    STARGAZER_TRAVEL,                // 별바라기 이동 확인
    END
};


// 확인창을 열 때 요청자가 전달하는 데이터
struct CONFIRMATION_REQUEST_DESC final : public Engine::Event_Desc
{
    CONFIRMATION_ACTION_TYPE eActionType = { CONFIRMATION_ACTION_TYPE::END };

    _wstring strTitle;               // ConfirmTitle: 구매 확인, 강화 확인
    _wstring strGoodsName;           // GoodsText: 상품명 또는 강화 능력명
    _wstring strConfirmText;         // ConfirmText: 구매하시겠습니까?
    _wstring strConfirmActionText = { L"확인" };
    _wstring strCancelActionText = { L"취소" };
    _wstring strPriceLabel = { L"가격" };

    _uint iCurrentErgo = { 0 };       // 현재 보유 에르고
    _uint iPrice = { 0 };             // 구매 또는 강화 비용
    _bool bShowPrice = { true };      // 가격 그룹 표시 여부

    _uint64 iItemID = { 0 };          // SHOP_PURCHASE에서 사용하는 상품 ID
    _uint iArmIndex = { UINT_MAX };   // LEGION_ARM_UPGRADE에서 사용하는 리전 암 번호
    _uint iUpgradeStage = { UINT_MAX }; // 강화하려는 단계

    _uint64 iTargetStargazerID = static_cast<_uint64>(-1);  // 이동할 별바라기 ID
};

// 사용자가 확인 또는 취소를 선택했을 때 요청자에게 돌려주는 데이터
struct CONFIRMATION_RESULT_DESC final : public Engine::Event_Desc
{
    CONFIRMATION_ACTION_TYPE eActionType = { CONFIRMATION_ACTION_TYPE::END };
    _bool bConfirmed = { false };

    _uint64 iItemID = { 0 };
    _uint iArmIndex = { UINT_MAX };
    _uint iUpgradeStage = { UINT_MAX };

    _uint64 iTargetStargazerID = static_cast<_uint64>(-1);  // 확인 결과와 함께 반환
};

NS_END