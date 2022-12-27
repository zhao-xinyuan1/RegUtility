#define _CRT_SECURE_NO_WARNINGS
#include "Everything.h"

typedef struct RegAbsPath
{
	DWORD RootKeyIndex;
	LPTSTR pRelativePath;
} RegAbsPath;

BOOL DisplayPair(LPTSTR, DWORD, LPBYTE, DWORD, LPBOOL);
BOOL InputPair(LPTSTR, DWORD, LPBYTE, DWORD, LPBOOL);
RegAbsPath AbsolutePath(LPTSTR);
BOOL RegOpenNewKeyCheck(RegAbsPath, HKEY*, BOOL);
BOOL RegEnumDisplayPair(LPTSTR, HKEY*);
BOOL ConsoleResponse(LPTSTR, DWORD, BOOL);

HKEY preDefKeys[] = { HKEY_CLASSES_ROOT,
			   HKEY_CURRENT_CONFIG,
			   HKEY_CURRENT_USER,
			   HKEY_LOCAL_MACHINE };

LPCTSTR preDefKeyNames[] = { _T("HKEY_CLASSES_ROOT"),
							 _T("HKEY_CURRENT_CONFIG"),
							 _T("HKEY_CURRENT_USER"),
							 _T("HKEY_LOCAL_MACHINE"),
							 NULL };

int _tmain(int agrc, LPTSTR argv[])
{
	TCHAR Prompt[MAX_PATH];
	TCHAR Response[MAX_PATH];
	TCHAR absKeyName[MAX_PATH];
	LPTSTR pSpace, pCommand, pKey;
	HKEY curKey = 0;

	_tcscpy(Prompt, _T(":Computer>"));
	while (TRUE)
	{
		ConsolePrompt(Prompt, Response, MAX_PATH, TRUE);
		pCommand = Response;
		pSpace = _tcschr(Response, _T(' '));
		if (pSpace)
		{
			*pSpace = _T('\0');
			for (pSpace++; *pSpace == _T(' '); pSpace++);
			pKey = pSpace;
			if (!_tcscmp(pCommand, _T("dir")))
			{
				//List all subkeys and values
				if (!_tcscmp(Prompt, _T(":Computer>")))
				{
					if (!RegEnumDisplayPair(pKey, &curKey))
						_tprintf(_T("Fail to list the key\n"));
				}
				else {
					if (curKey != 0) RegCloseKey(curKey);
					size_t i;
					if (!RegEnumDisplayPair(pKey, &curKey))
					{
						for (i = 1; Prompt[i] != _T('>') && Prompt[i] != _T('\0'); i++)
							absKeyName[i - 1] = Prompt[i];
						absKeyName[i - 1] = _T('\0');
						if (i > 2 && absKeyName[i - 2] != _T('\\')) { _tcscat(absKeyName, _T("\\")); }
						_tcscat(absKeyName, pKey);
						if (!RegEnumDisplayPair(absKeyName, &curKey))
							_ftprintf(stderr, _T("Fail to list the key %s\n"), absKeyName);
						for (i = 1; Prompt[i] != _T('>') && Prompt[i] != _T('\0'); i++)
							absKeyName[i - 1] = Prompt[i];
						absKeyName[i - 1] = _T('\0');
						if (!RegOpenNewKeyCheck(AbsolutePath(absKeyName), &curKey, FALSE))
							_ftprintf(stderr, _T("Failed to restore the current key %s\n"), absKeyName);
					}
					else {
						for (i = 1; Prompt[i] != _T('>') && Prompt[i] != _T('\0'); i++)
							absKeyName[i - 1] = Prompt[i];
						absKeyName[i - 1] = _T('\0');
						if (!RegOpenNewKeyCheck(AbsolutePath(absKeyName), &curKey, FALSE))
							_ftprintf(stderr, _T("Failed to restore the current key %s\n"), absKeyName);
					}
				}
			}
			else if (!_tcscmp(pCommand, _T("cd")))
			{
				//Open the key(edit) and show the key
				if (!_tcscmp(Prompt, _T(":Computer>")))
				{
					if (RegOpenNewKeyCheck(AbsolutePath(pKey), &curKey, TRUE)) _stprintf(Prompt, _T(":%s>"), pKey);
				}
				else
				{
					if (curKey != 0)
						RegCloseKey(curKey);

					if (RegOpenNewKeyCheck(AbsolutePath(pKey), &curKey, FALSE))
					{
						_stprintf(Prompt, _T(":%s>"), pKey);
					}
					else
					{
						size_t i = 1;
						for (; Prompt[i] != _T('>') && Prompt[i] != _T('\0'); i++)
							absKeyName[i - 1] = Prompt[i];
						absKeyName[i - 1] = _T('\0');
						if (i > 2 && absKeyName[i - 2] != _T('\\')) { _tcscat(absKeyName, _T("\\")); }
						_tcscat(absKeyName, pKey);
						if (RegOpenNewKeyCheck(AbsolutePath(absKeyName), &curKey, FALSE))
						{
							_stprintf(Prompt, _T(":%s>"), absKeyName);
						}
						else
						{
							for (; Prompt[i] != _T('>') && Prompt[i] != _T('\0'); i++)
								absKeyName[i - 1] = Prompt[i];
							absKeyName[i - 1] = _T('\0');
							if (RegOpenNewKeyCheck(AbsolutePath(absKeyName), &curKey, FALSE))
								_ftprintf(stderr, _T("Failed to open the Key, Restore the current key\n"));
							else
								_ftprintf(stderr, _T("Failed to restore the current key %s\n"), absKeyName);
						}
					}
				}
			}
			else if (!_tcscmp(pCommand, _T("edit")))
			{
				if (!_tcscmp(Prompt, _T(":Computer>")))
				{
					_ftprintf(stderr, _T("No value in root key computer\n"));
				}
				else
				{
					if (curKey)
					{
						DWORD valueLen, maxValueLen, valueType;
						LPBYTE value;
						RegQueryInfoKey(curKey, NULL, NULL, NULL, NULL, NULL, NULL,
							NULL, NULL, &maxValueLen, NULL, NULL);
						value = malloc(maxValueLen);
						valueLen = maxValueLen;

						if (RegGetValue(curKey, NULL, pKey, RRF_RT_ANY, &valueType, value, &valueLen) == ERROR_MORE_DATA)
						{
							_tprintf(_T("Value has more data %d than %d\n"), valueLen, maxValueLen);
							free(value);
							value = malloc(valueLen);
						}
						if (RegGetValue(curKey, NULL, pKey, RRF_RT_ANY, &valueType, value, &valueLen) == ERROR_SUCCESS)
						{
							DisplayPair(pKey, valueType, value, valueLen, NULL);
							_tcscat(pKey, _T(":"));
							_tprintf(_T("\n"));
							InputPair(pKey, valueType, value, valueLen, NULL);
							pKey[_tcslen(pKey) - 1] = _T('\0');
							RegSetValueEx(curKey, pKey, 0, valueType, value, valueLen);
						}
						else
						{
							_tprintf(_T("Cannot edit value %s"), pKey);
						}
						_tprintf(_T("\n"));
					}
				}
			}
			else {
				_tprintf(_T("Enter dir/cd/edit key quit\n"));
			}
		}
		else {
			if (!_tcscmp(pCommand, _T("quit")))
				break;
			_tprintf(_T("Enter dir/cd/edit key quit\n"));
		}
	}
}

RegAbsPath AbsolutePath(LPTSTR pKey)
{
	LPTSTR pScan;
	RegAbsPath AbsolutePath;
	TCHAR KeyName[MAX_PATH] = { 0 };

	DWORD i = 0;
	for (pScan = pKey; *pScan != _T('\\') && *pScan != _T('\0'); pScan++, i++)
		KeyName[i] = *pScan;
	KeyName[i] = _T('\0');
	if (*pScan != _T('\0')) pScan++;
	for (i = 0; preDefKeyNames[i] != NULL && _tcscmp(preDefKeyNames[i], KeyName); i++);

	AbsolutePath.RootKeyIndex = i;
	AbsolutePath.pRelativePath = pScan;
	return AbsolutePath;
}

BOOL ConsoleResponse(LPTSTR pResponse, DWORD maxChar, BOOL echo)

/* Prompt the user at the console and get a response
	which can be up to maxChar generic characters.

	pPromptMessage:	Message displayed to user.
	pResponse:	Programmer-supplied buffer that receives the response.
	maxChar:	Maximum size of the user buffer, measured as generic characters.
	echo:		Do not display the user's response if this flag is FALSE. */
{
	HANDLE hIn;
	DWORD charIn, echoFlag;
	BOOL success;
	hIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	/* Should the input be echoed? */
	echoFlag = echo ? ENABLE_ECHO_INPUT : 0;

	success = SetConsoleMode(hIn, ENABLE_LINE_INPUT | echoFlag | ENABLE_PROCESSED_INPUT)
		&& ReadConsole(hIn, pResponse, maxChar - 2, &charIn, NULL);

	/* Replace the CR-LF by the null character. */
	if (success)
		pResponse[charIn - 2] = _T('\0');
	else
		ReportError(_T("ConsoleResponse failure."), 0, TRUE);

	CloseHandle(hIn);
	return success;
}

BOOL RegOpenNewKeyCheck(RegAbsPath AbsolutePath, HKEY* pCurKey, BOOL PrintError)
{
	HKEY curRootKey;
	LSTATUS resultOpenKey;
	LPTSTR pMsg;

	curRootKey = preDefKeys[AbsolutePath.RootKeyIndex];
	resultOpenKey = RegOpenKeyEx(curRootKey, AbsolutePath.pRelativePath, 0, KEY_READ | KEY_WRITE, pCurKey);
	if (resultOpenKey != ERROR_SUCCESS)
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, resultOpenKey,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pMsg, 0, NULL);
		if (_tcslen(pMsg) > 0 && PrintError)
			_ftprintf(stderr, _T("Failed to Open the Key %s"), pMsg);
		return FALSE;
	}
	return TRUE;
}

BOOL RegEnumDisplayPair(LPTSTR pKey, HKEY* pCurKey)
{
	TCHAR* subKeyName;
	TCHAR* valueName;
	DWORD numSubKeys, maxSubKeyNameLen, numValues, maxValueNameLen, maxValueLen;
	DWORD subKeyNameLen, valueNameLen, valueType, valueLen;
	LPBYTE value;

	if (RegOpenNewKeyCheck(AbsolutePath(pKey), pCurKey, FALSE))
	{
		RegQueryInfoKey(*pCurKey, NULL, NULL, NULL, &numSubKeys, &maxSubKeyNameLen, NULL,
			&numValues, &maxValueNameLen, &maxValueLen, NULL, NULL);
		subKeyName = malloc(sizeof(TCHAR) * (maxSubKeyNameLen + 1));
		valueName = malloc(sizeof(TCHAR) * (maxValueNameLen + 1));
		value = malloc(maxValueLen);

		DWORD i = 0;
		for (i = 0; i < numValues; i++)
		{
			valueNameLen = maxValueNameLen + 1;
			valueLen = maxValueLen;
			RegEnumValue(*pCurKey, i, valueName, &valueNameLen, NULL, &valueType, value, &valueLen);
			DisplayPair(valueName, valueType, value, valueLen, NULL);
		}
		for (i = 0; i < numSubKeys; i++)
		{
			subKeyNameLen = maxSubKeyNameLen + 1;
			RegEnumKeyEx(*pCurKey, i, subKeyName, &subKeyNameLen, NULL, NULL, NULL, NULL);
			_tprintf(_T("\n%s"), pKey);
			if (subKeyName != NULL && _tcslen(subKeyName) != 0)
				_tprintf(_T("\\%s"), subKeyName);
		}

		RegCloseKey(*pCurKey);
		free(subKeyName);
		free(valueName);
		free(value);

		_tprintf(_T("\n"));
		return TRUE;
	}
	return FALSE;
}

BOOL DisplayPair(LPTSTR valueName, DWORD valueType,
	LPBYTE value, DWORD valueLen,
	LPBOOL flags)
	/* Function to display key-value pairs. */

{

	LPBYTE pV = value;
	DWORD i;

	_tprintf(_T("\n%s = "), valueName);
	switch (valueType) {
	case REG_FULL_RESOURCE_DESCRIPTOR: /* 9: Resource list in the hardware description */
	case REG_BINARY: /*  3: Binary data in any form. */
		for (i = 0; i < valueLen; i++, pV++)
			_tprintf(_T(" %x"), *pV);
		break;

	case REG_DWORD: /* 4: A 32-bit number. */
		_tprintf(_T("%x"), (DWORD)*value);
		break;

	case REG_EXPAND_SZ: /* 2: null-terminated string with unexpanded references to environment variables (for example, “%PATH%”). */
	case REG_MULTI_SZ: /* 7: An array of null-terminated strings, terminated by two null characters. */
	case REG_SZ: /* 1: A null-terminated string. */
		_tprintf(_T("%s"), (LPTSTR)value);
		break;

	case REG_DWORD_BIG_ENDIAN: /* 5:  A 32-bit number in big-endian format. */
	case REG_LINK: /* 6: A Unicode symbolic link. */
	case REG_NONE: /* 0: No defined value type. */
	case REG_RESOURCE_LIST: /* 8: A device-driver resource list. */
	default: _tprintf(_T(" ** Cannot display value of type: %d. Exercise for reader\n"), valueType);
		break;
	}

	return TRUE;
}

BOOL InputPair(LPTSTR valueName, DWORD valueType,
	LPBYTE value, DWORD valueLen,
	LPBOOL flags)
	/* Function to display key-value pairs. */

{

	LPBYTE pV = value;
	DWORD i;

	_tprintf(_T("\n%s = "), valueName);
	switch (valueType) {
	case REG_FULL_RESOURCE_DESCRIPTOR: /* 9: Resource list in the hardware description */
	case REG_BINARY: /*  3: Binary data in any form. */
		for (i = 0; i < valueLen; i++, pV++)
			_tscanf(_T(" %x"), (DWORD*)pV);
		break;

	case REG_DWORD: /* 4: A 32-bit number. */
		_tscanf(_T("%x"), (DWORD*)value);
		break;

	case REG_EXPAND_SZ: /* 2: null-terminated string with unexpanded references to environment variables (for example, “%PATH%”). */
	case REG_MULTI_SZ: /* 7: An array of null-terminated strings, terminated by two null characters. */
	case REG_SZ: /* 1: A null-terminated string. */
		ConsoleResponse((LPTSTR)value, MAX_PATH, TRUE);
		break;

	case REG_DWORD_BIG_ENDIAN: /* 5:  A 32-bit number in big-endian format. */
	case REG_LINK: /* 6: A Unicode symbolic link. */
	case REG_NONE: /* 0: No defined value type. */
	case REG_RESOURCE_LIST: /* 8: A device-driver resource list. */
	default: _tprintf(_T(" ** Cannot display value of type: %d. Exercise for reader\n"), valueType);
		break;
	}

	return TRUE;
}