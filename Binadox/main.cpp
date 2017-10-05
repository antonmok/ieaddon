
#include "common.h"
#include <Olectl.h>
#include "ClassFactory.h"

// DLL-global reference count. This is incremented and decremented as objects are created and destroyed by the DLL.
volatile LONG DllRefCount=0;
// DLL's HINSTANCE
HINSTANCE hInstance=NULL;
// GUID
// {3543619C-D563-43f7-95EA-4DA7E1CC396A}
const CLSID CLSID_IEPlugin = { 0x3543619c, 0xd563, 0x43f7,{ 0x95, 0xea, 0x4d, 0xa7, 0xe1, 0xcc, 0x39, 0x6a } }; // The CLSID in binary format

// Called when the DLL is loaded into the process, attached or detached from a thread, and unloaded from the process
BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);
	TCHAR mainexe[1024];
	int len;

	if(fdwReason==DLL_PROCESS_ATTACH) { // we only care about when the DLL is loaded into the process
		hInstance=hinstDLL; // store our HINSTANCE
		DisableThreadLibraryCalls(hInstance); // Disable calls to DllMain on thread attach/detach. Increases performance since we don't need those notifications anyways.
		// Get the full path of the main executable module of the process that loaded us
		// Since explorer.exe sometimes also loads BHOs, we want to stop the DLL from loading if we are being loaded by explorer.exe
		// Note that we can't check if we are being loaded into iexplore.exe, because other processes can have legitimate reasons for loading us as well
		//  such as regsvr32.exe for registering and unregistering our COM class.
		GetModuleFileName(NULL,mainexe,1024);
		len=_tcsnlen(mainexe,1024);
		if(len>12 && _tcsicmp(mainexe+len-12,_T("explorer.exe"))==0) return FALSE;
	}
	return TRUE;
}

// Called by COM to get a reference to our CClassFactory object
STDAPI DllGetClassObject(REFIID rclsid,REFIID riid,LPVOID *ppv)
{
	HRESULT hr;

	// We only support one class, make sure rclsid matches CLSID_IEPlugin
	if(!IsEqualCLSID(rclsid,CLSID_IEPlugin)) return CLASS_E_CLASSNOTAVAILABLE;
	// Make sure the ppv pointer is valid
	if(IsBadWritePtr(ppv,sizeof(LPVOID))) return E_POINTER;
	// Set *ppv to NULL
	(*ppv)=NULL;
	// Create a new CClassFactory object
	CClassFactory *pFactory=new CClassFactory;
	// If we couldn't allocate the new object, return an out-of-memory error
	if(pFactory==NULL) return E_OUTOFMEMORY;
	// Query the pFactory object for the requested interface
	hr=pFactory->QueryInterface(riid,ppv);
	// If the requested interface isn't supported by pFactory, delete the newly created object
	if(FAILED(hr)) delete pFactory;
	// Return the same HRESULT as CClassFactory::QueryInterface
	return hr;
}

// This function is called by COM to determine if the DLL safe to unload.
// We return true if no objects from this DLL are being used and the DLL is unlocked.
STDAPI DllCanUnloadNow()
{
	if(DllRefCount>0) return S_FALSE;
	return S_OK;
}

// This function is called to register our DLL in the system, for example, by regsvr32.exe
// We register ourselves with both COM and Internet Explorer
STDAPI DllRegisterServer()
{
	HKEY hk;
	TCHAR dllpath[1024];
	DWORD n;

	// Get the full path to this DLL's file so we can register it
	GetModuleFileName(hInstance,dllpath,1024);
	// Create our key under HKCR\\CLSID
	if(RegCreateKeyEx(HKEY_CLASSES_ROOT,_T("CLSID\\") CLSID_IEPlugin_Str,0,NULL,0,KEY_ALL_ACCESS,NULL,&hk,NULL)!=ERROR_SUCCESS) return SELFREG_E_CLASS;
	// Set the name of our BHO
	RegSetValueEx(hk,NULL,0,REG_SZ,(const BYTE*)_T("Binadox license checker"),24*sizeof(TCHAR));
	RegCloseKey(hk);
	// Create the InProcServer32 key
	if(RegCreateKeyEx(HKEY_CLASSES_ROOT,_T("CLSID\\") CLSID_IEPlugin_Str _T("\\InProcServer32"),0,NULL,0,KEY_ALL_ACCESS,NULL,&hk,NULL)!=ERROR_SUCCESS) return SELFREG_E_CLASS;
	// Set the path to this DLL
	RegSetValueEx(hk,NULL,0,REG_SZ,(const BYTE*)dllpath,(_tcslen(dllpath)+1)*sizeof(TCHAR));
	// Set the ThreadingModel to Apartment
	RegSetValueEx(hk,_T("ThreadingModel"),0,REG_SZ,(const BYTE*)_T("Apartment"),10*sizeof(TCHAR));
	RegCloseKey(hk);
	// Now register the BHO with Internet Explorer
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\") CLSID_IEPlugin_Str,0,NULL,0,KEY_ALL_ACCESS,NULL,&hk,NULL)!=ERROR_SUCCESS) return SELFREG_E_CLASS;
	// I believe the following tells explorer.exe not to load our BHO
	n=1;
	RegSetValueEx(hk,_T("NoExplorer"),0,REG_DWORD,(const BYTE*)&n,sizeof(DWORD));
	RegCloseKey(hk);
	return S_OK;
}

// This function is called to unregister our DLL in the system, for example, by regsvr32.exe
// We remove our registration entries from both COM and Internet Explorer
STDAPI DllUnregisterServer()
{
	// Remove the Internet Explorer BHO registration
	RegDeleteKey(HKEY_LOCAL_MACHINE,_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\") CLSID_IEPlugin_Str);
	// Remove the COM registration, starting with the deeper key first since RegDeleteKey is not recursive
	RegDeleteKey(HKEY_CLASSES_ROOT,_T("CLSID\\") CLSID_IEPlugin_Str _T("\\InProcServer32"));
	RegDeleteKey(HKEY_CLASSES_ROOT,_T("CLSID\\") CLSID_IEPlugin_Str);
	return S_OK;
}
