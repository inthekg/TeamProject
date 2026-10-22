#include "BT_TreeCompiler.h"

#include "BT_NodeModel.h"
#include "BT_FunctionRegistry.h"

#include "BT_Node.h"
#include "BT_Defines.h"        

#include "BT_Selector.h"
#include "BT_Sequence.h"
#include "BT_ReactiveSelector.h"
#include "BT_ReactiveSequence.h"
#include "BT_Parallel.h"
#include "BT_RandomSelector.h"

#include "BT_Inverter.h"
#include "BT_Repeat.h"
#include "BT_Cooldown.h"
#include "BT_ForceSuccess.h"
#include "BT_ForceFailure.h"
#include "BT_Service.h"

#include "BT_Task.h"
#include "BT_Condition.h"
#include "BT_Wait.h"

#include "Engine_Utility.h"



CBT_Node* CBT_TreeCompiler::Compile(const BT_NODE_MODEL* pModel, CBT_FunctionRegistry* pRegistry)
{
	if (nullptr == pModel)
		return nullptr;

	const _wstring strName = StringToWString(pModel->strName);

	switch (pModel->eType)
	{
	case BT_NodeType::Selector:
	{
		CBT_Selector* pNode = CBT_Selector::Create(strName);
		if (nullptr == pNode)
			return nullptr;

		for (auto* pChildModel : pModel->Children)
		{
			CBT_Node* pChild = Compile(pChildModel, pRegistry);
			if (nullptr != pChild)         
				pNode->Add_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::Sequence:
	{
		CBT_Sequence* pNode = CBT_Sequence::Create(strName);
		if (nullptr == pNode)
			return nullptr;

		for (auto* pChildModel : pModel->Children)
		{
			CBT_Node* pChild = Compile(pChildModel, pRegistry);
			if (nullptr != pChild)
				pNode->Add_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::ReactiveSelector:
	{
		CBT_ReactiveSelector* pNode = CBT_ReactiveSelector::Create(strName);
		if (nullptr == pNode)
			return nullptr;

		for (auto* pChildModel : pModel->Children)
		{
			CBT_Node* pChild = Compile(pChildModel, pRegistry);
			if (nullptr != pChild)
				pNode->Add_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::ReactiveSequence:
	{
		CBT_ReactiveSequence* pNode = CBT_ReactiveSequence::Create(strName);
		if (nullptr == pNode)
			return nullptr;

		for (auto* pChildModel : pModel->Children)
		{
			CBT_Node* pChild = Compile(pChildModel, pRegistry);
			if (nullptr != pChild)
				pNode->Add_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::Parallel:
	{
		CBT_Parallel* pNode = CBT_Parallel::Create(
			strName,
			static_cast<CBT_Parallel::Parallel_Policy>(pModel->iSuccessPolicy),
			static_cast<CBT_Parallel::Parallel_Policy>(pModel->iFailurePolicy));
		if (nullptr == pNode)
			return nullptr;

		for (auto* pChildModel : pModel->Children)
		{
			CBT_Node* pChild = Compile(pChildModel, pRegistry);
			if (nullptr != pChild)
				pNode->Add_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::RandomSelector:
	{
		CBT_RandomSelector* pNode = CBT_RandomSelector::Create(strName);
		if (nullptr == pNode)
			return nullptr;

		for (auto* pChildModel : pModel->Children)
		{
			CBT_Node* pChild = Compile(pChildModel, pRegistry);
			if (nullptr != pChild)
				pNode->Add_Child(pChild);
		}

		if (!pModel->Weights.empty() &&
			pModel->Weights.size() == pModel->Children.size())
		{
			pNode->Set_Weight(pModel->Weights);
		}
		return pNode;
	}

	case BT_NodeType::Inverter:
	{
		CBT_Inverter* pNode = CBT_Inverter::Create(strName);
		if (nullptr == pNode)
			return nullptr;

		if (!pModel->Children.empty())
		{
			CBT_Node* pChild = Compile(pModel->Children[0], pRegistry);
			if (nullptr != pChild)
				pNode->Set_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::Repeat:
	{
		CBT_Repeat* pNode = CBT_Repeat::Create(strName, nullptr, pModel->iRepeatCount);
		if (nullptr == pNode)
			return nullptr;

		if (!pModel->Children.empty())
		{
			CBT_Node* pChild = Compile(pModel->Children[0], pRegistry);
			if (nullptr != pChild)
				pNode->Set_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::Cooldown:
	{
		CBT_Cooldown* pNode = CBT_Cooldown::Create(strName, nullptr, pModel->fSeconds);
		if (nullptr == pNode)
			return nullptr;

		if (!pModel->Children.empty())
		{
			CBT_Node* pChild = Compile(pModel->Children[0], pRegistry);
			if (nullptr != pChild)
				pNode->Set_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::ForceSuccess:
	{
		CBT_ForceSuccess* pNode = CBT_ForceSuccess::Create(strName);
		if (nullptr == pNode)
			return nullptr;

		if (!pModel->Children.empty())
		{
			CBT_Node* pChild = Compile(pModel->Children[0], pRegistry);
			if (nullptr != pChild)
				pNode->Set_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::ForceFailure:	{
		CBT_ForceFailure* pNode = CBT_ForceFailure::Create(strName);
		if (nullptr == pNode)
			return nullptr;

		if (!pModel->Children.empty())
		{
			CBT_Node* pChild = Compile(pModel->Children[0], pRegistry);
			if (nullptr != pChild)
				pNode->Set_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::Service:
	{		
		CBT_FunctionRegistry::ServiceFunc regFn =
			pRegistry ? pRegistry->Find_Service(pModel->strFuncKey) : CBT_FunctionRegistry::ServiceFunc{};
		BT_PARAMS params = pModel->Params;
		CBT_Service::ServiceFunc func = [regFn, params](CBT_BlackBoard* pBB)->void {
			if (regFn) regFn(pBB, params); };

		CBT_Service* pNode = CBT_Service::Create(strName, nullptr, func, pModel->fInterval);
		if (nullptr == pNode)
			return nullptr;

		if (!pModel->Children.empty())
		{
			CBT_Node* pChild = Compile(pModel->Children[0], pRegistry);
			if (nullptr != pChild)
				pNode->Set_Child(pChild);
		}
		return pNode;
	}

	case BT_NodeType::Task:
	{		
		CBT_FunctionRegistry::TaskFunc regFn =
			pRegistry ? pRegistry->Find_Task(pModel->strFuncKey) : CBT_FunctionRegistry::TaskFunc{};
		BT_PARAMS params = pModel->Params;
		CBT_Task::TaskFunc func = [regFn, params](CBT_BlackBoard* pBB)->CBT_Node::Status {
			return regFn ? regFn(pBB, params) : CBT_Node::Status::Failure; };

		return CBT_Task::Create(strName, func);
	}

	case BT_NodeType::Condition:
	{
		
		CBT_FunctionRegistry::ConditionFunc regFn =
			pRegistry ? pRegistry->Find_Condition(pModel->strFuncKey) : CBT_FunctionRegistry::ConditionFunc{};
		BT_PARAMS params = pModel->Params;
		CBT_Condition::ConditionFunc func = [regFn, params](CBT_BlackBoard* pBB)->_bool {
			return regFn ? regFn(pBB, params) : false; };

		return CBT_Condition::Create(strName, func);
	}

	case BT_NodeType::Wait:
	{
		return CBT_Wait::Create(strName, pModel->fSeconds);
	}

	default:
		MSG_BOX("CBT_TreeCompiler::Compile - Unknown BT_NodeType");
		return nullptr;
	}
}
