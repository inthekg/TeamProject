#pragma once
#include "BT_NodeModel.h"
#include "BT_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_BinarySerializer final
{
private :
	CBT_BinarySerializer() = delete;
	~CBT_BinarySerializer() = delete;
	CBT_BinarySerializer(const CBT_BinarySerializer&) = delete;
	CBT_BinarySerializer& operator=(const CBT_BinarySerializer&) = delete;

public:
	// 루트 모델을 바이너리 파일로 기록. 성공 시 true.
	static _bool			Save(const BT_NODE_MODEL* pRoot, const string& strPath);
	// 바이너리 파일을 읽어 새 모델 트리를 생성(호출자 소유). 실패 시 false, *ppOutRoot=nullptr.
	static _bool			Load(const string& strPath, BT_NODE_MODEL** ppOutRoot);

	// JSON(그룹 C CBT_JsonSerializer::Load)으로 로드 후 바이너리로 변환 저장. 성공 시 true.
	static _bool			ConvertJsonToBinary(const string& strJsonPath, const string& strBinPath);

private:
	// 모델 노드 → 스트림(재귀 기록).
	static void				Write(const BT_NODE_MODEL* pNode, std::ofstream& os);
	// 스트림 → 모델 노드(재귀, 새로 할당). 실패 시 nullptr.
	static BT_NODE_MODEL* Read(std::ifstream& is);
};

NS_END