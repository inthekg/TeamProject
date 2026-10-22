#pragma once
#include "Base.h"
#include "ObjectPool.h"

#include <typeindex>

/*
	//사용법 예시
	//풀 생성 <타입 , 풀 객체 최대 개수 > . 한 타입 당 하나의 풀만 생성됨
	m_pGameInstance->Create_Pool<CGameObject, 100>();
	//풀에서 객체 얻기 size 표시 필요없음
	CGameObject* pObject = m_pGameInstance->Get_PoolObject<CGameObject>(pArg); // 오브젝트의 Activate함수 호출됨. 매개변수 필요하면 pArg로 전달 
	//풀 객체 반납은 다른 객체처럼 그냥 Safe_Release하면 됨. ref가 0이 되면 풀로 돌아감. PoolableObject Release 재정의 참고 
	Safe_Release(pObject); // 풀로 돌아갈때 InActivate 함수 호출됨 

*/

NS_BEGIN(Engine)

class IObjectPool;

class CPool_Manager : public CBase
{
private:
	CPool_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CPool_Manager() = default;

public:
	//typeIndex : type info값을 해싱이 가능한 형태로 변환한 값 
	template<typename T, size_t Size>
	HRESULT Create_Pool()
	{
		type_index typeIndex = typeid(T);
		if (m_Pools.find(typeIndex) != m_Pools.end()) return E_FAIL;

		//T타입 객체를 Size만큼 생성(Create)
		IObjectPool* pPool = CObjectPool<T, Size>::Create(m_pDevice, m_pContext);
		if (pPool == nullptr) return E_FAIL;	

		m_Pools[typeIndex] = pPool;

		return S_OK;
	}

	template<typename T>
	T* Get_Object(void* pArg = nullptr)
	{
		auto iter = m_Pools.find(typeid(T));
		if (iter == m_Pools.end()) return nullptr;

		ITypedObjectPool<T>* pTypedPool = static_cast<ITypedObjectPool<T>*>(iter->second);
		return pTypedPool->Get_Object(pArg);
	}

	template<typename T>
	void Remove_Pool()
	{
		auto iter = m_Pools.find(typeid(T));
		if (iter == m_Pools.end()) return;

		Safe_Release(iter->second);
		m_Pools.erase(iter);

	}

	void Remove_AllPools()
	{
		for (auto& Pair : m_Pools)
			Safe_Release(Pair.second);
		m_Pools.clear();
	}

private:
	ID3D11Device*							m_pDevice = { nullptr };
	ID3D11DeviceContext*					m_pContext = { nullptr };
	
	unordered_map<type_index, IObjectPool*> m_Pools;

public:
	static CPool_Manager*					Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void							Free();
};

NS_END