#pragma once
#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_BlackBoard final : public CBase
{
private:
	CBT_BlackBoard();
	virtual ~CBT_BlackBoard() = default;

public:
	template<class T>
	unordered_map<_wstring, T>& Get_Map();
	template<class T>
	const unordered_map<_wstring, T>& Get_Map() const;

	/* Get_Map<T> 타입별 명시적 특수화 "선언" (정의는 .cpp)        */
	/* Set/Get 등에서 사용되기 전에 보여야 하므로 멤버 선언 직후 배치 */
	template<> unordered_map<_wstring, _bool>& Get_Map<_bool>();
	template<> unordered_map<_wstring, _int>& Get_Map<_int>();
	template<> unordered_map<_wstring, _float>& Get_Map<_float>();
	template<> unordered_map<_wstring, _float3>& Get_Map<_float3>();
	template<> unordered_map<_wstring, _float4>& Get_Map<_float4>();
	template<> unordered_map<_wstring, _float4x4>& Get_Map<_float4x4>();
	template<> unordered_map<_wstring, _wstring>& Get_Map<_wstring>();
	template<> unordered_map<_wstring, void*>& Get_Map<void*>();

	template<> const unordered_map<_wstring, _bool>& Get_Map<_bool>() const;
	template<> const unordered_map<_wstring, _int>& Get_Map<_int>() const;
	template<> const unordered_map<_wstring, _float>& Get_Map<_float>() const;
	template<> const unordered_map<_wstring, _float3>& Get_Map<_float3>() const;
	template<> const unordered_map<_wstring, _float4>& Get_Map<_float4>() const;
	template<> const unordered_map<_wstring, _float4x4>& Get_Map<_float4x4>() const;
	template<> const unordered_map<_wstring, _wstring>& Get_Map<_wstring>() const;
	template<> const unordered_map<_wstring, void*>& Get_Map<void*>() const;

public:
	template<class T>
	void Set(const _wstring& strKey, const T& value)
	{
		Get_Map<T>()[strKey] = value;
	}

	template<class T>
	T Get(const _wstring& strKey, const T& tDefault = T{}) const
	{
		const auto& Datas = Get_Map<T>();
		auto iter = Datas.find(strKey);
		return (iter != Datas.end()) ? iter->second : tDefault;
	}

	template<class T>
	_bool Has(const _wstring& strKey) const
	{
		const auto& Datas = Get_Map<T>();
		return Datas.find(strKey) != Datas.end();
	}

	template<class T>
	_bool Remove(const _wstring& strKey)
	{
		auto& Datas = Get_Map<T>();
		return Datas.erase(strKey) > 0;
	}

	template<class T>
	_bool TryGet(const _wstring& strKey, T& Out) const
	{
		const auto& Datas = Get_Map<T>();
		auto iter = Datas.find(strKey);
		if (iter == Datas.end())
			return false;

		Out = iter->second;
		return true;
	}

	template<class T>
	void Set_Object(const _wstring& strKey, T* pObject)
	{
		m_PtrDatas[strKey] = reinterpret_cast<void*>(pObject);
	}

	template<class T>
	T* Get_Object(const _wstring& strKey) const
	{
		auto it = m_PtrDatas.find(strKey);
		return (it != m_PtrDatas.end()) ? reinterpret_cast<T*>(it->second) : nullptr;
	}

	_bool Has_Object(const _wstring& strKey) const
	{
		return m_PtrDatas.find(strKey) != m_PtrDatas.end();
	}

	_bool Remove_Object(const _wstring& strKey)
	{
		return m_PtrDatas.erase(strKey) > 0;
	}

public:
	void Clear();

private:
	unordered_map<_wstring, _bool>		m_BoolDatas;
	unordered_map<_wstring, _int>		m_IntDatas;
	unordered_map<_wstring, _float>		m_FloatDatas;
	unordered_map<_wstring, _float3>	m_Float3Datas;
	unordered_map<_wstring, _float4>	m_Float4Datas;
	unordered_map<_wstring, _float4x4>	m_Float4x4Datas;
	unordered_map<_wstring, _wstring>	m_StringDatas;
	unordered_map<_wstring, void*>		m_PtrDatas;

public:
	static CBT_BlackBoard* Create();
	virtual void Free() override;
};

NS_END