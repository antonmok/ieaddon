/*
 This file contains implementation of the DWebBrowserEvents2 dispatch interface.
*/

//#include <initguid.h>

#include "common.h"
#include "EventSink.h"
#include "HttpHandler.h"
#include "Helpers.h"
#include <Mshtml.h>
#include <Shobjidl.h>

#include <shlguid.h>
#include <atlbase.h>

#include "UIAutomationHelper.h"

// The single global object of CEventSink
CEventSink EventSink;

STDMETHODIMP CEventSink::QueryInterface(REFIID riid,void **ppvObject)
{
	// Check if ppvObject is a valid pointer
	if(IsBadWritePtr(ppvObject,sizeof(void*))) return E_POINTER;
	// Set *ppvObject to NULL
	(*ppvObject)=NULL;
	// See if the requested IID matches one that we support
	// If it doesn't return E_NOINTERFACE
	if(!IsEqualIID(riid,IID_IUnknown) && !IsEqualIID(riid,IID_IDispatch) && !IsEqualIID(riid,DIID_DWebBrowserEvents2)) return E_NOINTERFACE;
	// If it's a matching IID, set *ppvObject to point to the global EventSink object
	(*ppvObject)=(void*)&EventSink;

	return S_OK;
}

STDMETHODIMP_(ULONG) CEventSink::AddRef()
{
	return 1; // We always have just one static object
}

STDMETHODIMP_(ULONG) CEventSink::Release()
{
	return 1; // Ditto
}

// We don't need to implement the next three methods because we are just a pure event sink
// We only care about Invoke() which is what IE calls to notify us of events

STDMETHODIMP CEventSink::GetTypeInfoCount(UINT *pctinfo)
{
	UNREFERENCED_PARAMETER(pctinfo);

	return E_NOTIMPL;
}

STDMETHODIMP CEventSink::GetTypeInfo(UINT iTInfo,LCID lcid,ITypeInfo **ppTInfo)
{
	UNREFERENCED_PARAMETER(iTInfo);
	UNREFERENCED_PARAMETER(lcid);
	UNREFERENCED_PARAMETER(ppTInfo);

	return E_NOTIMPL;
}

STDMETHODIMP CEventSink::GetIDsOfNames(REFIID riid,LPOLESTR *rgszNames,UINT cNames,LCID lcid,DISPID *rgDispId)
{
	UNREFERENCED_PARAMETER(riid);
	UNREFERENCED_PARAMETER(rgszNames);
	UNREFERENCED_PARAMETER(cNames);
	UNREFERENCED_PARAMETER(lcid);
	UNREFERENCED_PARAMETER(rgDispId);

	return E_NOTIMPL;
}

// This is called by IE to notify us of events
// Full documentation about all the events supported by DWebBrowserEvents2 can be found at
//  http://msdn.microsoft.com/en-us/library/aa768283(VS.85).aspx
STDMETHODIMP CEventSink::Invoke(DISPID dispIdMember,REFIID riid,LCID lcid,WORD wFlags,DISPPARAMS *pDispParams,VARIANT *pVarResult,EXCEPINFO *pExcepInfo,UINT *puArgErr)
{
	UNREFERENCED_PARAMETER(lcid);
	UNREFERENCED_PARAMETER(wFlags);
	UNREFERENCED_PARAMETER(pVarResult);
	UNREFERENCED_PARAMETER(pExcepInfo);
	UNREFERENCED_PARAMETER(puArgErr);
	VARIANT v[5]; // Used to hold converted event parameters before passing them onto the event handling method
	int n;

	if(!IsEqualIID(riid,IID_NULL)) return DISP_E_UNKNOWNINTERFACE; // riid should always be IID_NULL

	/*if (&pDispParams->rgvarg == NULL) {
		return S_OK;
	}*/

	if (firstCall) {
		tabId = std::to_string(GetTabID());
		macId = std::to_string(GetVolumeHash());

		firstCall = false;
	}

	// Initialize the variants
	for (n=0;n<5;n++) VariantInit(&v[n]);

	switch (dispIdMember) {

		case DISPID_ONQUIT: {
			std::string data("OnQuit " + tabId);
			DoHttp(data);
			// generate new tab id
			tabId = std::to_string(GetTabID());

			break;
		}

		case DISPID_NEWWINDOW3: {
			OutputDebugString(L"NewWindow3\n");

			VariantChangeType(&v[0], &pDispParams->rgvarg[0], 0, VT_BSTR); // bstrUrl 
			VariantChangeType(&v[1], &pDispParams->rgvarg[1], 0, VT_BSTR); // bstrUrlContext 
			VariantChangeType(&v[2], &pDispParams->rgvarg[2], 0, VT_I4); // dwFlags 

			OnNewWindow3(v[0], v[1], v[2]);

			break;
		}

		case DISPID_WINDOWSTATECHANGED: {

			VariantChangeType(&v[0], &pDispParams->rgvarg[0], 0, VT_I4); // dwMask
			VariantChangeType(&v[1], &pDispParams->rgvarg[1], 0, VT_I4); // dwFlags 

			OnWindowStateChanged(v[1], v[0]);

			break;
		}

		case DISPID_DOCUMENTCOMPLETE: {
			
			VariantChangeType(&v[0], &pDispParams->rgvarg[0], 0, VT_BSTR); // bstrUrl 
			VariantChangeType(&v[1], &pDispParams->rgvarg[1], 0, VT_DISPATCH); // bstrUrlContext 
			IDispatch * pIDisp = v[1].pdispVal;

			OnDocumentComplete(v[1].pdispVal, v[0]);

			break;
		}
	}
	
	// Free the variants
	for (n = 0; n < 5; n++) {
		VariantClear(&v[n]);
	}

	return S_OK;
}

void CEventSink::OnDocumentComplete(IDispatch *pDispWebBrowser, VARIANT bstrUrl)
{
	std::wstring url(bstrUrl.bstrVal);

	if (url.substr(0, 6) == L"about:" || url.substr(0, 11) == L"javascript:" || url.size() < 3) {
		return;
	}

	OutputDebugString(L"COMPLETE\n");

	IDispatch* pDocDisp = NULL;

	if (((IWebBrowser2*)pDispWebBrowser)->get_Document(&pDocDisp) == S_OK) {
		IHTMLDocument2* pDoc = NULL;
		if (pDocDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc) == S_OK) {
			BSTR title;
			pDoc->get_title(&title);
			//MessageBox(0, title, NULL, MB_OK);
		}
	}
	
	std::string data;
	ws2s(url, data);
	data = "onComplete " + std::to_string((long)pDispWebBrowser) + " " + data + " " + tabId;
	DoHttp(data);
}

void CEventSink::OnNewWindow3(VARIANTARG& dstUrl, VARIANTARG& srcUrl, VARIANTARG& flags)
{
	std::wstring urlDstW(dstUrl.bstrVal);
	std::wstring urlSrcW(srcUrl.bstrVal);

	std::string urlDst;
	std::string urlSrc;

	/*DWORD dwFlags = flags.lVal;
	std::string strFlags;

	if (dwFlags & NWMF_UNLOADING) {
		strFlags.append("NWMF_UNLOADING ");
	}
	if (dwFlags & NWMF_USERINITED) {
		strFlags.append("NWMF_USERINITED ");
	}
	if (dwFlags & NWMF_FIRST) {
		strFlags.append("NWMF_FIRST ");
	}
	if (dwFlags & NWMF_OVERRIDEKEY) {
		strFlags.append("NWMF_OVERRIDEKEY ");
	}
	if (dwFlags & NWMF_SHOWHELP) {
		strFlags.append("NWMF_SHOWHELP ");
	}
	if (dwFlags & NWMF_HTMLDIALOG) {
		strFlags.append("NWMF_HTMLDIALOG ");
	}
	if (dwFlags & NWMF_FROMDIALOGCHILD) {
		strFlags.append("NWMF_FROMDIALOGCHILD ");
	}
	if (dwFlags & NWMF_USERREQUESTED) {
		strFlags.append("NWMF_USERREQUESTED ");
	}
	if (dwFlags & NWMF_USERALLOWED) {
		strFlags.append("NWMF_USERALLOWED ");
	}
	if (dwFlags & NWMF_FORCEWINDOW) {
		strFlags.append("NWMF_FORCEWINDOW ");
	}
	if (dwFlags & NWMF_FORCETAB) {
		strFlags.append("NWMF_FORCETAB ");
	}
	if (dwFlags & NWMF_SUGGESTWINDOW) {
		strFlags.append("NWMF_SUGGESTWINDOW ");
	}
	if (dwFlags & NWMF_SUGGESTTAB) {
		strFlags.append("NWMF_SUGGESTTAB ");
	}
	if (dwFlags & NWMF_INACTIVETAB) {
		strFlags.append("NWMF_INACTIVETAB ");
	}*/

	ws2s(urlDstW, urlDst);
	ws2s(urlSrcW, urlSrc);

	// generate new tab id
	//tabId = std::to_string(GetTabID());

	std::vector<std::wstring> tabs;

	bool res = GetTabsList(tabs);

	std::string data("NewWin: " + tabId + " url_from: " + urlSrc + " url_to: " + urlDst);
	DoHttp(data);
}

void CEventSink::OnWindowStateChanged(VARIANT flags, VARIANT validFlagsMask)
{
	DWORD dwMask = validFlagsMask.lVal;
	DWORD dwFlags = flags.lVal;

	// We only care about WINDOWSTATE_USERVISIBLE.
	if (dwMask & OLECMDIDF_WINDOWSTATE_USERVISIBLE) {
		if (dwFlags & OLECMDIDF_WINDOWSTATE_USERVISIBLE) {
			OutputDebugString(L"Visible\n");
			std::string data("Visible: " + tabId);
			DoHttp(data);
		}
	}
}