#pragma once

#include "Camera.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CCutScene_Camera final : public CCamera
{
private:
	CCutScene_Camera(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCutScene_Camera(const CCutScene_Camera& Prototype);
	virtual ~CCutScene_Camera() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Priority_Update(_float fTimeDelta) override;
	virtual void Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

public:
	void Set_CameraPose(_fvector vPosition, _fvector vQuaternion, _float fFovy);

protected:
	// 가상 카메라는 직접 View/Projection을 출력하지 않는다.
	virtual void Update_PipeLine() override;

public:
	static CCutScene_Camera* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END