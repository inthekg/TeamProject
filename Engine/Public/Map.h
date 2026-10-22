#pragma once
#include "Base.h"

// TODO : DLL노출 결정 보류. Map Add,Remove Object들을 manager통해 수행 + Instance Wrapper할거면 제거
// 실제로 사용해보고 결정

NS_BEGIN(Engine)

class CMapObject;
class ENGINE_DLL CMap : public CBase
{
private:
	CMap(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CMap() = default;

public:
	virtual HRESULT				Initialize();
	virtual void				Priority_Update(_float fTimeDelta) ;
	virtual void				Update(_float fTimeDelta) ;
	virtual void				Late_Update(_float fTimeDelta) ;
	//virtual HRESULT Render();

	//TODO 제거하기 디버깅 함수 
	_fvector					Get_MapObjectPos(_uint iIndex);
	const vector<CMapObject*>&	Get_Objects() const { return m_Objects; }

	void                      Set_SpatialDirty(_bool bDirty = true) { m_bSpatialDirty = bDirty; }
	_bool                     Is_SpatialDirty() const { return m_bSpatialDirty; }

public:
	//Object Manager 에 소유한 오브젝트 Add 등록
	void						Add_GameObjects_ToMgr(_uint iLayerLevelIndex, const _wstring& strLayerTag);
	////전체 objectManger에서 해제 
	//void						Remove_GameObjects_FromMgr(_uint iLayerLevelIndex, const _wstring& strLayerTag);


    void						Push_MapObject(CMapObject* pStaticObject);
	void						ReMove_MapObject(CMapObject* pMapObject);
    //void						Push_StaticCollider(CStaticCollider* pStaticCollider);



protected:
	ID3D11Device*				m_pDevice		= { nullptr };
	ID3D11DeviceContext*		m_pContext		= { nullptr };
	class CGameInstance*		m_pGameInstance = { nullptr };

protected:
	vector<CMapObject*>			m_Objects;
	_bool						m_bSpatialDirty = { true };
    //vector<CStaticCollider*>	m_Colliders;  

	_uint						m_iNumObjects	= {};
public:
	static CMap*				Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void				Free() override;
};

NS_END