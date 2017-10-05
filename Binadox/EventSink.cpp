/*
 This file contains implementation of the DWebBrowserEvents2 dispatch interface.
*/

#include "common.h"
#include "EventSink.h"
#include "HttpHandler.h"
#include "Helpers.h"

#include "Shobjidl.h"

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
	bool b;
	PVOID pv;
	LONG lbound,ubound,sz;

	if(!IsEqualIID(riid,IID_NULL)) return DISP_E_UNKNOWNINTERFACE; // riid should always be IID_NULL

	// Initialize the variants
	for (n=0;n<5;n++) VariantInit(&v[n]);

	switch (dispIdMember) { // Handle the BeforeNavigate2 event

		case DISPID_BEFORENAVIGATE2: {
			VariantChangeType(&v[0],&pDispParams->rgvarg[5],0,VT_BSTR); // url
			VariantChangeType(&v[1],&pDispParams->rgvarg[4],0,VT_I4); // Flags
			VariantChangeType(&v[2],&pDispParams->rgvarg[3],0,VT_BSTR); // TargetFrameName
			VariantChangeType(&v[3],&pDispParams->rgvarg[2],0,VT_UI1|VT_ARRAY); // PostData
			VariantChangeType(&v[4],&pDispParams->rgvarg[1],0,VT_BSTR); // Headers

			if (v[3].vt!=VT_EMPTY) {
				SafeArrayGetLBound(v[3].parray,0,&lbound);
				SafeArrayGetUBound(v[3].parray,0,&ubound);
				sz = ubound-lbound+1;
				SafeArrayAccessData(v[3].parray,&pv);
			} else {
				sz=0;
				pv=NULL;
			}

			b = Event_BeforeNavigate2((LPOLESTR)v[0].bstrVal, v[1].lVal, (LPOLESTR)v[2].bstrVal, (PUCHAR)pv, sz, (LPOLESTR)v[4].bstrVal, ((*(pDispParams->rgvarg[0].pboolVal)) != VARIANT_FALSE));

			if (v[3].vt != VT_EMPTY) {
				SafeArrayUnaccessData(v[3].parray);
			}

			if (b) {
				*(pDispParams->rgvarg[0].pboolVal) = VARIANT_TRUE;
			} else {
				*(pDispParams->rgvarg[0].pboolVal) = VARIANT_FALSE;
			}
		}

		case DISPID_NEWWINDOW3: {
			OutputDebugString(L"NewWindow\n");
			
			std::wstring urlDstW(pDispParams->rgvarg[0].bstrVal);
			std::wstring urlSrcW(pDispParams->rgvarg[1].bstrVal);
			std::string urlDst;
			std::string urlSrc;

			DWORD dwFlags = pDispParams->rgvarg[2].lVal;
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
			}

			ws2s(urlDstW, urlDst);
			ws2s(urlSrcW, urlSrc);

			std::string data("NewWin: " + std::to_string(GetTabID()) + " flags: " + strFlags + " url_from: " + urlSrc + " url_to: " + urlDst);
			DoHttp(data);
		}

		case DISPID_WINDOWSTATECHANGED: {

			DWORD dwMask = pDispParams->rgvarg[0].lVal;
			DWORD dwFlags = pDispParams->rgvarg[1].lVal;

			// We only care about WINDOWSTATE_USERVISIBLE.
			if (dwMask & OLECMDIDF_WINDOWSTATE_USERVISIBLE) {
				if (dwFlags & OLECMDIDF_WINDOWSTATE_USERVISIBLE) {
					OutputDebugString(L"Visible\n");
					std::string data("Visible: " + std::to_string(GetTabID()));
					DoHttp(data);
				} else {
					OutputDebugString(L"Hidden\n");
					std::string data("Hidden: " + std::to_string(GetTabID()) + " flags: " + std::to_string(dwFlags) + " mask: " + std::to_string(dwMask));
					DoHttp(data);
				}
			}
		}
	}
	
	// Free the variants
	for (n = 0; n < 5; n++) {
		VariantClear(&v[n]);
	}

	return S_OK;
}

// Return true to prevent the url from being opened
bool CEventSink::Event_BeforeNavigate2(LPOLESTR url,LONG Flags,LPOLESTR TargetFrameName,PUCHAR PostData,LONG PostDataSize,LPOLESTR Headers,bool Cancel)
{
	TCHAR urlStr[1024];
	int len = wcslen(url);
	
	if (len > 1023) {
		len = 1023;
	}

	//wsprintf(msg,_T("url=%ls\nFlags=0x%08X\nTargetFrameName=%ls\nPostData=%hs\nPostDataSize=%d\nHeaders=%ls\nCancel=%s"),url,Flags,TargetFrameName,(char*)PostData,PostDataSize,Headers,((Cancel)?(_T("true")):(_T("false"))));
	wcsncpy_s(urlStr, url, len);
	OutputDebugString(L"BeforeNavigate2 URL: ");
	OutputDebugString(urlStr);
	OutputDebugString(L"\n");

	std::string data("Hello World!");

	DoHttp(data);

	return Cancel;
}
