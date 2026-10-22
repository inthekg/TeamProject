#pragma once
#include "Engine_Defines.h"
#include <functional>

namespace Engine
{
	enum class EPropertyType { Float, Int, Bool, UInt, Float2, Float3, Float4, String, WString };

	struct FProperty
	{
		string			strPropertyName;
		EPropertyType	ePropertyType;
		size_t			iOffset;
		std::function<HRESULT(void*, const void*)>	pSetter = nullptr;
		bool			bIsColor = false;
	};

	struct FClassInfo
	{
		string				strClassName;
		string				strParentClassName;
		vector<FProperty>	Properties;
	};

	extern ENGINE_DLL std::unordered_map<std::string, FClassInfo>& Get_ReflectionMaps();
}

#define DECLARE_REFLECTION_BASE(ClassType) \
public: \
	virtual const string Get_ClassName() const { return StaticClassName(); } \
	virtual const string Get_ParentClassName() const { return StaticParentClassName(); } \
	static const string StaticClassName(); \
	static const string StaticParentClassName(); \
	static void RegisterReflection_Internal();

#define DECLARE_REFLECTION(ClassType) \
public: \
	virtual const string Get_ClassName() const override { return StaticClassName(); } \
	virtual const string Get_ParentClassName() const override { return StaticParentClassName(); } \
	static const string StaticClassName(); \
	static const string StaticParentClassName(); \
	static void RegisterReflection_Internal();

#define BEGIN_REFLECTION(ClassName, ParentClassName) \
const string ClassName::StaticClassName() { return #ClassName; } \
const string ClassName::StaticParentClassName() { return #ParentClassName; } \
void ClassName::RegisterReflection_Internal() { \
	FClassInfo info; \
	info.strClassName = #ClassName; \
	info.strParentClassName = #ParentClassName;

// 일반 프로퍼티 (setter 없음, 직접 대입)
#define ADD_PROPERTY(ClassName, MemberName, PropType) \
	info.Properties.push_back({#MemberName, EPropertyType::PropType, offsetof(ClassName, MemberName), nullptr});

// setter 연동 프로퍼티. SetterType은 setter 인자 타입(예: _wstring, _int, _float).
#define ADD_PROPERTY_SETTER(ClassName, MemberName, PropType, SetterFunc, SetterType) \
	info.Properties.push_back({#MemberName, EPropertyType::PropType, offsetof(ClassName, MemberName), \
		[](void* pObj, const void* pNew) -> HRESULT { \
			return static_cast<ClassName*>(pObj)->SetterFunc(*static_cast<const SetterType*>(pNew)); \
		}});
#define ADD_PROPERTY_COLOR(ClassName, MemberName, PropType) \
        info.Properties.push_back({#MemberName, EPropertyType::PropType, offsetof(ClassName, MemberName), nullptr, true});

#define END_REFLECTION(ClassName) \
	Get_ReflectionMaps()[#ClassName] = info; \
} \
struct ClassName##_AutoRegister { \
	ClassName##_AutoRegister() { ClassName::RegisterReflection_Internal(); } \
}; \
static ClassName##_AutoRegister s_##ClassName##_AutoRegister;