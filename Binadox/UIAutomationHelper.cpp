#include "UIAutomationHelper.h"
#include <UIAutomation.h>
#include <thread>
#include <future>

IUIAutomation *g_pAutomation = NULL;

// CoInitialize must be called before calling this function, and the  
// caller must release the returned pointer when finished with it.
// 
HRESULT InitializeUIAutomation(IUIAutomation **ppAutomation)
{
	return CoCreateInstance(CLSID_CUIAutomation, NULL,
		CLSCTX_INPROC_SERVER, IID_IUIAutomation,
		reinterpret_cast<void**>(ppAutomation));
}

IUIAutomationElementArray* FindElemByControlType(IUIAutomationElement* pParent, const long controlTypeId)
{
	IUIAutomationCondition* pElemCondition = NULL;
	IUIAutomationElementArray* pFound = NULL;

	VARIANT varProp;
	varProp.vt = VT_I4;
	varProp.lVal = controlTypeId;

	g_pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, varProp, &pElemCondition);

	if (pElemCondition == NULL) {
		OutputDebugString(L"Failed to create PropertyCondition\n");
		return NULL;
	}

	pParent->FindAll(TreeScope_Descendants, pElemCondition, &pFound);

	if (pElemCondition != NULL) {
		pElemCondition->Release();
	}

	if (pFound == NULL) {
		OutputDebugString(L"Failed in FindAll()\n");
		return NULL;
	}

	return pFound;
}

IUIAutomationElement* FindFirstElemByControlType(IUIAutomationElement* pParent, const long controlTypeId)
{
	IUIAutomationCondition* pElemCondition = NULL;
	IUIAutomationElement* pFound = NULL;

	VARIANT varProp;
	varProp.vt = VT_I4;
	varProp.lVal = controlTypeId;

	g_pAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, varProp, &pElemCondition);

	if (pElemCondition == NULL) {
		OutputDebugString(L"Failed to create PropertyCondition\n");
		return NULL;
	}

	pParent->FindFirst(TreeScope_Descendants, pElemCondition, &pFound);
	
	if (pElemCondition != NULL) {
		pElemCondition->Release();
	}

	if (pFound == NULL) {
		OutputDebugString(L"Failed in FindFirst()\n");
		return NULL;
	}

	return pFound;
}

bool GetTabsListThread(std::vector<std::wstring>& tabsList)
{
	HWND hIEFrame = FindWindow(L"IEFrame", NULL);

	if (!hIEFrame) {
		OutputDebugString(L"IE window not found\n");
		return false;
	}

	if (!g_pAutomation) {

		//CoInitializeEx(NULL, COINIT_MULTITHREADED);

		if (S_OK != InitializeUIAutomation(&g_pAutomation)) {
			OutputDebugString(L"Failed to init UIAutomation\n");
			return false;
		}
	}

	IUIAutomationElement* pParent = NULL;

	if (S_OK != g_pAutomation->ElementFromHandle(hIEFrame, &pParent)) {
		OutputDebugString(L"Failed to get parent IUIAutomationElement\n");
		return false;
	}

	IUIAutomationElementArray* pFound = FindElemByControlType(pParent, UIA_TabControlTypeId);
	
	if (!pFound) {
		return false;
	}

	int len = 0;
	pFound->get_Length(&len);

	if (len != 1) {
		OutputDebugString(L"Bad search results\n");
		return false;
	}

	IUIAutomationElement* pTabsBar = NULL;

	pFound->GetElement(0, &pTabsBar);

	if (!pTabsBar) {
		OutputDebugString(L"GetElement() error\n");
		return false;
	}

	pFound = FindElemByControlType(pTabsBar, UIA_TabItemControlTypeId);

	if (!pFound) {
		return false;
	}

	len = 0;
	pFound->get_Length(&len);

	if (!len) {
		OutputDebugString(L"Bad search results #2\n");
		return false;
	}

	for (int i = 0; i < len; ++i) {

		IUIAutomationElement* pTab = NULL;

		pFound->GetElement(i, &pTab);

		if (!pTabsBar) {
			OutputDebugString(L"Wrong tab index\n");
			return false;
		}

		BSTR bstrName;
		pTab->get_CurrentName(&bstrName);

		tabsList.push_back(bstrName);
		OutputDebugString(bstrName);
		OutputDebugString(L"\n");

	}

	return true;
}

bool GetTabsList(std::vector<std::wstring>& tabsList)
{
	/*std::future<bool> ret = std::async(&GetTabsListThread, tabsList);
	return ret.get();*/

	std::thread getTabsListThread(GetTabsListThread, tabsList);
	getTabsListThread.detach();

	return true;
}




