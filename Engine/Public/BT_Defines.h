#pragma once
#include "Base.h"

NS_BEGIN(Engine)

enum class BT_NodeType : unsigned int
{
	None = 0,
	// Composite
	Selector, Sequence, ReactiveSelector, ReactiveSequence, Parallel, RandomSelector,
	// Decorator
	Inverter, Repeat, Cooldown, ForceSuccess, ForceFailure, Service,
	// Leaf
	Task, Condition, Wait,
	End
};

enum class BT_NodeCategory
{
	Composite,
	Decorator,
	Leaf
};

inline BT_NodeCategory BTNode_Category(BT_NodeType eType)
{
	switch (eType)
	{
	case BT_NodeType::Selector:
	case BT_NodeType::Sequence:
	case BT_NodeType::ReactiveSelector:
	case BT_NodeType::ReactiveSequence:
	case BT_NodeType::Parallel:
	case BT_NodeType::RandomSelector:
		return BT_NodeCategory::Composite;

	case BT_NodeType::Inverter:
	case BT_NodeType::Repeat:
	case BT_NodeType::Cooldown:
	case BT_NodeType::ForceSuccess:
	case BT_NodeType::ForceFailure:
	case BT_NodeType::Service:
		return BT_NodeCategory::Decorator;

	case BT_NodeType::Task:
	case BT_NodeType::Condition:
	case BT_NodeType::Wait:
		return BT_NodeCategory::Leaf;

	default:
		// None/End 등 비정상 타입은 Leaf로 간주(자식 0).
		return BT_NodeCategory::Leaf;
	}
}

// 타입 → 표시 이름("Selector" 등). None/End는 빈 문자열.
inline const char* BTNode_TypeName(BT_NodeType eType)
{
	switch (eType)
	{
	case BT_NodeType::Selector:			return "Selector";
	case BT_NodeType::Sequence:			return "Sequence";
	case BT_NodeType::ReactiveSelector:	return "ReactiveSelector";
	case BT_NodeType::ReactiveSequence:	return "ReactiveSequence";
	case BT_NodeType::Parallel:			return "Parallel";
	case BT_NodeType::RandomSelector:	return "RandomSelector";
	case BT_NodeType::Inverter:			return "Inverter";
	case BT_NodeType::Repeat:			return "Repeat";
	case BT_NodeType::Cooldown:			return "Cooldown";
	case BT_NodeType::ForceSuccess:		return "ForceSuccess";
	case BT_NodeType::ForceFailure:		return "ForceFailure";
	case BT_NodeType::Service:			return "Service";
	case BT_NodeType::Task:				return "Task";
	case BT_NodeType::Condition:		return "Condition";
	case BT_NodeType::Wait:				return "Wait";
	default:							return ""; // None/End
	}
}


// 표시 이름 → 타입. 미일치 시 None.
inline BT_NodeType BTNode_TypeFromName(const string& strName)
{
	// None/End를 제외한 전 타입을 순회하며 이름 일치 검사.
	for (_uint i = ETOUI(BT_NodeType::None) + 1; i < ETOUI(BT_NodeType::End); ++i)
	{
		BT_NodeType eType = static_cast<BT_NodeType>(i);
		if (strName == BTNode_TypeName(eType))
			return eType;
	}
	return BT_NodeType::None;
}

// 레지스트리 함수 키가 필요한 타입인가(Task/Condition/Service만 true).
inline _bool BTNode_NeedsFuncKey(BT_NodeType eType)
{
	return (BT_NodeType::Task == eType ||
		BT_NodeType::Condition == eType ||
		BT_NodeType::Service == eType);
}

// 허용 자식 수. Leaf=0, Decorator=1, Composite=99(사실상 무제한).
inline _int BTNode_MaxChildren(BT_NodeType eType)
{
	switch (BTNode_Category(eType))
	{
	case BT_NodeCategory::Composite:	return 99; // 사실상 무제한
	case BT_NodeCategory::Decorator:	return 1;
	case BT_NodeCategory::Leaf:
	default:							return 0;
	}
}

NS_END