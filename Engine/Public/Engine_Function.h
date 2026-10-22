#ifndef Engine_Function_h__
#define Engine_Function_h__

namespace Engine
{
	// 템플릿은 기능의 정해져있으나 자료형은 정해져있지 않은 것
	// 기능을 인스턴스화 하기 위하여 만들어두는 틀

	template<typename T>
	void	Safe_Delete(T& Pointer)
	{
		if (nullptr != Pointer)
		{
			delete Pointer;
			Pointer = nullptr;
		}
	}

	template<typename T>
	void	Safe_Delete_Array(T& Pointer)
	{
		if (nullptr != Pointer)
		{
			delete [] Pointer;
			Pointer = nullptr;
		}
	}

	template<typename T>
	unsigned int Safe_Release(T& Instance)
	{
		unsigned int		iRefCnt = { 0 };

		if (nullptr != Instance)
		{
			iRefCnt = Instance->Release();

			if (0 == iRefCnt)
				Instance = nullptr;
		}

		return iRefCnt;
	}

	template<typename T>
	unsigned int Safe_AddRef(T& Instance)
	{
		unsigned int		iRefCnt = { 0 };

		if (nullptr != Instance)	
			iRefCnt = Instance->AddRef();		

		return iRefCnt;
	}

	// 상위 16비트 : 이벤트 카테고리 , 하위 16비트 : 이벤트 ID
	constexpr _uint Make_EventKey(EVENT_CATEGORY eCategory, _uint iEventID)
	{
		return (static_cast<_uint>(eCategory) << 16) | (iEventID & 0x0000FFFF);
	}

	constexpr _uint To_EventKey(EVENT_TYPE eType)
	{
		return Make_EventKey(EVENT_CATEGORY::COMMON, ETOUI(eType));
	}

	constexpr _uint To_EventKey(EVENT_TYPE_MAP_OBJECT eType)
	{
		return Make_EventKey(EVENT_CATEGORY::MAP_OBJECT, ETOUI(eType));
	}

	constexpr _uint To_EventKey(EVENT_TYPE_UI eType)
	{
		return Make_EventKey(EVENT_CATEGORY::UI, ETOUI(eType));
	}

	//TODO : 이벤트 카테고리 추가되면 To_EventKey 오버로딩 추가하기 
}

#endif // Engine_Function_h__
