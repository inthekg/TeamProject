#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

class CVIBuffer_DebugCapsule;
class CShader;
class CGameInstance;

class ENGINE_DLL CDebugCapsule : public CGameObject
{
public:
	typedef struct tagDebugCapsuleObjectDesc
	{
		_float	fRadius;
		_float	fHalfHeight;
		_float4	vColor;
		_uint	iSegment;
	} DEBUGCAPSULE_OBJ_DESC;
private:
	CDebugCapsule(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CDebugCapsule() = default;

public:
	virtual HRESULT			Initialize_Prototype(const DEBUGCAPSULE_OBJ_DESC& OBJ_DESC);
	virtual HRESULT			Render() override;
	virtual void			Late_Update(_float fTimeDelta) override;
	void					Update_Pose(_fvector vPos, _fvector vQuat);

private:
	CVIBuffer_DebugCapsule*	m_pVIBufferCom = { nullptr };
	CShader*				m_pShaderCom = { nullptr };

	_float					m_fRadius = {};
	_float					m_fHeight = {};
	CGameInstance*			m_pGameInstance = { nullptr };

public:
	static CDebugCapsule* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const DEBUGCAPSULE_OBJ_DESC& OBJ_DESC);
	virtual void Free() override;

};

NS_END