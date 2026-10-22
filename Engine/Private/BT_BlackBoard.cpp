#include "BT_BlackBoard.h"

CBT_BlackBoard::CBT_BlackBoard()
{
}

/* Get_Map<T> 타입별 명시적 특수화 "정의" (non-const) */
template<> unordered_map<_wstring, _bool>& CBT_BlackBoard::Get_Map<_bool>() { return m_BoolDatas; }
template<> unordered_map<_wstring, _int>& CBT_BlackBoard::Get_Map<_int>() { return m_IntDatas; }
template<> unordered_map<_wstring, _float>& CBT_BlackBoard::Get_Map<_float>() { return m_FloatDatas; }
template<> unordered_map<_wstring, _float3>& CBT_BlackBoard::Get_Map<_float3>() { return m_Float3Datas; }
template<> unordered_map<_wstring, _float4>& CBT_BlackBoard::Get_Map<_float4>() { return m_Float4Datas; }
template<> unordered_map<_wstring, _float4x4>& CBT_BlackBoard::Get_Map<_float4x4>() { return m_Float4x4Datas; }
template<> unordered_map<_wstring, _wstring>& CBT_BlackBoard::Get_Map<_wstring>() { return m_StringDatas; }
template<> unordered_map<_wstring, void*>& CBT_BlackBoard::Get_Map<void*>() { return m_PtrDatas; }

/* Get_Map<T> 타입별 명시적 특수화 "정의" (const) */
template<> const unordered_map<_wstring, _bool>& CBT_BlackBoard::Get_Map<_bool>()     const { return m_BoolDatas; }
template<> const unordered_map<_wstring, _int>& CBT_BlackBoard::Get_Map<_int>()      const { return m_IntDatas; }
template<> const unordered_map<_wstring, _float>& CBT_BlackBoard::Get_Map<_float>()    const { return m_FloatDatas; }
template<> const unordered_map<_wstring, _float3>& CBT_BlackBoard::Get_Map<_float3>()   const { return m_Float3Datas; }
template<> const unordered_map<_wstring, _float4>& CBT_BlackBoard::Get_Map<_float4>()   const { return m_Float4Datas; }
template<> const unordered_map<_wstring, _float4x4>& CBT_BlackBoard::Get_Map<_float4x4>() const { return m_Float4x4Datas; }
template<> const unordered_map<_wstring, _wstring>& CBT_BlackBoard::Get_Map<_wstring>()  const { return m_StringDatas; }
template<> const unordered_map<_wstring, void*>& CBT_BlackBoard::Get_Map<void*>()     const { return m_PtrDatas; }

void CBT_BlackBoard::Clear()
{
	m_BoolDatas.clear();
	m_IntDatas.clear();
	m_FloatDatas.clear();
	m_Float3Datas.clear();
	m_Float4Datas.clear();
	m_Float4x4Datas.clear();
	m_StringDatas.clear();
	m_PtrDatas.clear();
}

CBT_BlackBoard* CBT_BlackBoard::Create()
{
	CBT_BlackBoard* pInstance = new CBT_BlackBoard();
	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_BlackBoard");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CBT_BlackBoard::Free()
{
	Clear();
	__super::Free();
}