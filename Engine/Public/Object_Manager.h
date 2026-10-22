#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CObject_Manager final : public CBase
{
private:
	CObject_Manager();
	virtual ~CObject_Manager() = default;

public:
	class CLayer* Get_Layer(_uint iLayerLevelIndex, const _wstring& strLayerTag)
	{
		return	Find_Layer(iLayerLevelIndex, strLayerTag);
	}

	class CComponent*		Get_Component(_uint iLevelIndex, const _wstring& strLayerTag, const _wstring& strComponentTag, _uint iIndex);
	class CComponent*		Get_Component(_uint iLevelIndex, const _wstring& strLayerTag, const _wstring& strPartTag, const _wstring& strComponentTag, _uint iIndex);

#ifdef _DEBUG
	// IMGUI 용 전체 레이어 
	const map<_wstring, class CLayer*>& Get_Layers(_uint iLevelIndex) const
	{
		static const map<_wstring, class CLayer*> s_EmptyLayers;
		if (iLevelIndex >= m_iNumLevels || nullptr == m_pLayers)
			return s_EmptyLayers;
		return m_pLayers[iLevelIndex];
	}
#endif // _DEBUG


public:
	//TODO : 프레임워크 수정 사항. Clone이 아닌 원본 오브젝트를 삽입하는 함수 오버로딩 추가 
	HRESULT					Add_GameObject(_uint iLayerLevelIndex, const _wstring& strLayerTag, class CGameObject* pGameObject);
	HRESULT					Add_GameObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, _uint iLayerLevelIndex, const _wstring& strLayerTag, void* pArg);

public:
	HRESULT					Initialize(_uint iNumLevels);
	void					Priority_Update(_float fTimeDelta);
	void					Update(_float fTimeDelta);
	void					Physical_Update(_float fFixedTime);
	void					Late_Update(_float fTimeDelta);
	void					Clear(_uint iLevelIndex);
	void					Clear_Layer(_uint iLevelIndex, const _wstring& strLayerTag); 

private:
	class CLayer*			Find_Layer(_uint iLayerLevelIndex, const _wstring& strLayerTag);

private:
	//이름으로 업데이트 순서 제어하기 위해 map으로 설정
	map<_wstring, class CLayer*>*			m_pLayers = { nullptr };
	typedef map<_wstring, class CLayer*>	LAYERS;

	class CGameInstance*	m_pGameInstance = { nullptr };
	size_t					m_iNumLevels = {};


public:
	static CObject_Manager* Create(_uint iNumLevels);
	virtual void			Free() override;

};

NS_END