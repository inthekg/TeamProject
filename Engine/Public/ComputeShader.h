#pragma once
#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CComputeShader : public CResource
{
public:
	typedef struct tagComputeShaderDesc
	{
		_wstring	strShaderName;
		_wstring	strCSFilePath;
		string		strCSFunName = "CS_Main";
	} COMPUTE_SHADER_DESC;

private:
	CComputeShader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CComputeShader() = default;

public:
	virtual HRESULT Initialize_Prototype(COMPUTE_SHADER_DESC& Desc);

public:
	// 바인딩 헬퍼 함수
	void			Bind_ConstantBuffer(_uint iSlot, ID3D11Buffer* pCB);
	void			Bind_SRV(_uint iSlot, ID3D11ShaderResourceView* pSRV);
	void			Bind_Sampler(_uint iSlot, ID3D11SamplerState* pSampler);
	void			Bind_UAV(_uint iSlot, ID3D11UnorderedAccessView* pUAV, _uint iInitialCount = -1);

	// 디스패치 (내부에서 CSSetShader 호출)
	void			Dispatch(_uint iGroupX, _uint iGroupY = 1, _uint iGroupZ = 1);
	// Dispatch는 CPU에서 그룹 수를 GPU에 전달할 때 사용한다.
	// 처리량이 고정 or CPU가 개수를 알고 있는 경우

	void			DispatchIndirect(ID3D11Buffer* pArgsBuffer, _uint iAlignedOffset = 0);
	// DispatchIndirect는 GPU가 GPU 버퍼에서 알아서 그룹 수를 읽어올 때 사용한다.
	// 처리량이 GPU에서 동적으로 정해질 경우 ( 스폰 수, 살아있는 파티클 수 등)

	// 언바인드 (렌더 SRV로 다시 쓰기 전에 필수로 수행해줘야 함)
	void			Unbind_UAV(_uint iSlot, _uint iCount = 1);
	void			Unbind_SRV(_uint iSlot, _uint iCount = 1);

private:
	ID3D11ComputeShader* m_pComputeShader = { nullptr };

public:
	static CComputeShader*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, COMPUTE_SHADER_DESC& Desc);
	virtual void			Free() override;
};

NS_END