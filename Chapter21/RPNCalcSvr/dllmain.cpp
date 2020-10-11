// dllmain.cpp : Defines the entry point for the DLL application.

#include "pch.h"
#include "RPNCalculatorFactory.h"
#include "RPNCalcInterfaces.h"

#pragma comment(lib, "ktmw32")

HMODULE g_hInstDll;

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
	if (rclsid == __uuidof(RPNCalculator)) {
		static RPNCalculatorFactory factory;
		return factory.QueryInterface(riid, ppv);
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			g_hInstDll = hModule;
			DisableThreadLibraryCalls(hModule);
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

HANDLE CreateSimpleTransaction() {
	return ::CreateTransaction(nullptr, nullptr, TRANSACTION_DO_NOT_PROMOTE, 0, 0, INFINITE, nullptr);
}

STDAPI DllRegisterServer() {
	HANDLE hTransaction = CreateSimpleTransaction();
	if(hTransaction == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(::GetLastError());

	HKEY hKey;
	auto error = ::RegCreateKeyTransacted(HKEY_CLASSES_ROOT,
		L"CLSID\\{FA523D4E-DB35-4D0B-BD0A-002281FE3F31}",
		0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, 
		nullptr, &hKey, nullptr, hTransaction, nullptr);
	if (error != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(error);

	WCHAR value[] = L"RPNCalculator";
	error = ::RegSetValueEx(hKey, L"", 0, REG_SZ, (const BYTE*)value, sizeof(value));
	if (error != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(error);

	HKEY hInProcKey;
	error = ::RegCreateKeyTransacted(hKey, L"InProcServer32", 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hInProcKey, nullptr,
		hTransaction, nullptr);
	if (error != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(error);

	WCHAR path[MAX_PATH];
	::GetModuleFileName(g_hInstDll, path, _countof(path));
	error = ::RegSetValueEx(hInProcKey, L"", 0, REG_SZ, (const BYTE*)path, DWORD(::wcslen(path) + 1) * sizeof(WCHAR));
	if (error != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(error);

	auto ok = ::CommitTransaction(hTransaction);
	auto hr = ok ? S_OK : HRESULT_FROM_WIN32(::GetLastError());

	::RegCloseKey(hInProcKey);
	::RegCloseKey(hKey);
	::CloseHandle(hTransaction);

	return hr;
}

STDAPI DllUnregisterServer() {
	HKEY hKey;
	auto error = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, L"CLSID\\{FA523D4E-DB35-4D0B-BD0A-002281FE3F31}", 0,
		DELETE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey);
	if (error != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(error);

	error = ::RegDeleteTree(hKey, nullptr);
	::RegCloseKey(hKey);

	return HRESULT_FROM_WIN32(error);
}
