#include "stdafx.h"
#include "Public_Function.h"
#include <shlwapi.h>


//---------------------------------------------------------------------------
string AnsiToUtf8( const string& strIn)
{
	string strOut;
	return AnsiToUTF8(strIn,strOut);
}
//---------------------------------------------------------------------------
string AnsiToUTF8( const string& strIn, string& strOut )
{
	WCHAR* strSrc    = NULL;
	TCHAR* szRes    = NULL;

	int len = MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strIn.c_str(), -1, NULL,0);

	unsigned short* wszUtf8 = new unsigned short[len+1];
	if (wszUtf8 == NULL){
		return "";
	}
	memset(wszUtf8, 0, len * 2 + 2);
	MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strIn.c_str(), -1, (LPWSTR)wszUtf8, len);

	len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, NULL, 0, NULL, NULL);

	char* szUtf8 = new char[len + 1];
	if (szUtf8 == NULL){
		delete[] wszUtf8;
		return "";
	}
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte (CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, szUtf8, len, NULL,NULL);

	strOut = szUtf8;

	delete[] szUtf8;
	delete[] wszUtf8;

	return strOut;
}

//---------------------------------------------------------------------------
string UTF8ToAnsi( const string& strIn )
{
	WCHAR* strSrc    = NULL;
	TCHAR* szRes    = NULL;
	string strOut = "";

	int i = MultiByteToWideChar(CP_UTF8, 0, strIn.c_str(), -1, NULL, 0);

	strSrc = new WCHAR[i+1];
	if (strSrc == NULL){
		return "";
	}
	MultiByteToWideChar(CP_UTF8, 0, strIn.c_str(), -1, strSrc, i);

	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);

	szRes = new TCHAR[i+1];
	if (szRes == NULL){
		delete[] strSrc;
		return "";
	}
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	strOut = szRes;

	delete[] strSrc;
	delete[] szRes;

	return strOut;
}

void AnsiToUTF8( const char* _pstrIn,CString &_cstrOut) 
{
	WCHAR* strSrc    = NULL;
	TCHAR* szRes    = NULL;

	int len = MultiByteToWideChar(CP_ACP, 0, _pstrIn, -1, NULL,0);

	unsigned short* wszUtf8 = new unsigned short[len+1];
	if (wszUtf8 == NULL){
		return;
	}
	memset(wszUtf8, 0, len * 2 + 2);
	MultiByteToWideChar(CP_ACP, 0, _pstrIn, -1, (LPWSTR)wszUtf8, len);

	len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, NULL, 0, NULL, NULL);

	char* szUtf8 = new char[len + 1];
	if (szUtf8 == NULL){
		delete[] wszUtf8;
		return;
	}
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte (CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, szUtf8, len, NULL,NULL);

	_cstrOut.Format("%s",szUtf8);

	delete[] szUtf8;
	delete[] wszUtf8;

	return;
}

void UTF8ToAnsi( const char* _pstrIn,CString &_strOut)
{
	WCHAR* strSrc    = NULL;
	TCHAR* szRes    = NULL;

	int i = MultiByteToWideChar(CP_UTF8, 0,_pstrIn, -1, NULL, 0);

	strSrc = new WCHAR[i+1];
	if (strSrc == NULL){
		return;
	}
	MultiByteToWideChar(CP_UTF8, 0,_pstrIn, -1, strSrc, i);

	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);

	szRes = new TCHAR[i+1];
	if (szRes == NULL){
		delete[] strSrc;
		return;
	}
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	_strOut = szRes;

	delete[] strSrc;
	delete[] szRes;

}

BOOL ConvertStringToURLCoding(CString &strDest, const char* strUTF8, int iLength)
{
	strDest.Empty();
	CString strTemp;
	int i = 0;
	while(i < iLength)
	{
		if ((unsigned)strUTF8[i] <= (unsigned char)0x7f)
		{ 

			if ((strUTF8[i] >= '0' && strUTF8[i] <= '9') || 
				(strUTF8[i] >= 'A' && strUTF8[i] <= 'Z') ||
				(strUTF8[i] >= 'a' && strUTF8[i] <= 'z')||
				(strUTF8[i]>=8 && strUTF8[i]<=14)||
				strUTF8[i]==42||
				strUTF8[i] =='@'||
				strUTF8[i] =='_')
			{
				strDest += (TCHAR)strUTF8[i];
			}
			else if (strUTF8[i] == ' ')
			{
				strDest += _T('+');
			}
			else 
			{
				strTemp.Format(_T("%%%02X"), (unsigned char)strUTF8[i]);
				strDest += strTemp;
			}
			i++;
		}
		else
		{  
			strTemp.Format(_T("%%%02X%%%02X%%%02X"), (unsigned char)strUTF8[i], 
				(unsigned char)strUTF8[i + 1], (unsigned char)strUTF8[i + 2]);
			strDest += strTemp;
			i += 3;
		}
	}
	if (i == 0)
	{
		return FALSE;
	}

	i = 0;
	std::string temp = strDest.GetBuffer();
	iLength = temp.length();
	const char* strUTF8Temp = temp.c_str();
	strDest= _T("");
	strDest.Empty();
	while(i < iLength)
	{
		if ((unsigned)strUTF8Temp[i] <= (unsigned char)0x7f)
		{ 
			if ((strUTF8Temp[i] >= '0' && strUTF8Temp[i] <= '9') || 
				(strUTF8Temp[i] >= 'A' && strUTF8Temp[i] <= 'Z') ||
				(strUTF8Temp[i] >= 'a' && strUTF8Temp[i] <= 'z')||
				(strUTF8Temp[i]>=8 && strUTF8Temp[i]<=14)||
				strUTF8Temp[i]==42||
				strUTF8Temp[i] =='@'||
				strUTF8Temp[i] =='_')
			{
				strDest += (TCHAR)strUTF8Temp[i];
			}
			else if (strUTF8Temp[i] == ' ')
			{
				strDest += _T('+');
			}
			else
			{
				strTemp.Format(_T("%%%02X"), (unsigned char)strUTF8Temp[i]);
				strDest += strTemp;
			}
			i++;
		}
		else
		{  
			strTemp.Format(_T("%%%02X%%%02X%%%02X"), (unsigned char)strUTF8Temp[i], 
				(unsigned char)strUTF8Temp[i + 1], (unsigned char)strUTF8Temp[i + 2]);
			strDest += strTemp;
			i += 3;
		}
	}
	if (i == 0)
	{
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
string IntToStr( int _iValue )
{
	char pcValue[32] = {0};
	sprintf_s(pcValue,"%d",_iValue);
	string strTemp = pcValue;
	return strTemp;
}

//---------------------------------------------------------------------------
string StdStrTrim(const string _strSource)
{
	string strRet = _strSource;
	strRet.erase(strRet.find_last_not_of(' ')+1);
	strRet.erase(0,strRet.find_first_not_of(' '));
	return strRet;
}

string GetExName()
{
	char strFilePath[2048];
	memset(strFilePath, 0, 2048);
	::GetModuleFileName(GetModuleHandle(0), strFilePath, 2048);
	string strRet(strFilePath);
	return strRet;
}
//---------------------------------------------------------------------------
string ExtractFilePath(string _strFullName)
{
	string strRet= "";
	string strFileName(_strFullName);

	string::size_type iIndex = strFileName.find_last_of("\\");
	if (iIndex != std::string::npos)
	{
		strRet = strFileName.substr(0, iIndex + 1);	
	}
	return strRet;
}

std::map<string, string> m_mapLang;

CString GetText(string _strID)
{
	std::map<string, string>::iterator it = m_mapLang.find(_strID.c_str());
	if (it == m_mapLang.end())
	{
		return "";
	}
	string strStr = it->second;
	return strStr.c_str();
}

CString GetTextEx(UINT _uiID)
{
	CString strBuffer;
	strBuffer.LoadString(_uiID);
	return strBuffer;
}

HINSTANCE Execute(CString _cstrFilePath)
{
	CString strSystemDir;
	::GetSystemDirectory(strSystemDir.GetBuffer(256), 256);
	strSystemDir.ReleaseBuffer();
	CString strRundll;
	strRundll = strSystemDir + "\\rundll32.exe";
	CString strParm;
	strParm.Format("%s\\shimgvw.dll,ImageView_Fullscreen %s", strSystemDir, _cstrFilePath);
	HINSTANCE hinStance = ShellExecute(NULL, "Open", strRundll, strParm, NULL,  SW_SHOWNORMAL);
	return hinStance;
}
BOOL CreateMulityDir(string _strDir)
{
	if (_access(_strDir.c_str(), 0) != 0)
	{
		std::string::size_type iIndex = _strDir.find('\\', 0);
		while(iIndex != std::string::npos)
		{
			std::string strTmp = _strDir.substr(0, iIndex + 1);
			if(_access(strTmp.c_str(), 0) != 0)
			{
				if(!CreateDirectory(strTmp.c_str(), NULL))
				{
					return FALSE;
				}
			}
			iIndex = _strDir.find('\\', iIndex + 1);
		}
	}
	return TRUE;
}

string strIllChar = "~$!@#￥%^&*()+=[]{};:'\"|/?><`\\ \0";
CONST int m_iChnSin[31] = {126,36,33,64,35,65509,37,94,38,42,40,41,43,61,91,93,123,125,59,58,39,34,124,47,63,62,60,96,92,32};
int __stdcall ContainSubStr(const wstring& _wstrSource, const int* _iChnSin, const int& _iSize)
{
	int iLen = _wstrSource.length();
	for (int i=0; i<iLen; ++i)
	{
		wchar_t wcData = _wstrSource[i];
		for(int j = 0; j < _iSize; ++j)
		{
			if (int(wcData) == _iChnSin[j])
			{
				return i;
			}
		}
	}
	return -1;
}

bool IsIllCharInStrForCutShort(const std::string& _strSource)
{
	USES_CONVERSION;
	wstring wstrSource = A2W(_strSource.c_str());
	int iSubStr = ContainSubStr(wstrSource, m_iChnSin, _countof(m_iChnSin));
	if (-1 != iSubStr)
	{
		return true;
	}
	return false;
}

//能够对路径进行判断
string strIllCharForPath = "~$@#￥%^&*+=[]{};'\"|/?><`";
CONST int m_iChnSinForPath[31] = {126,36,64,35,65509,37,94,38,42,43,61,91,93,123,125,59,39,34,124,63,62,60,96};
bool IsIllCharInStrForPath(const std::string& _strSource)
{
	USES_CONVERSION;
	wstring wstrSource = A2W(_strSource.c_str());
	int iSubStr = ContainSubStr(wstrSource, m_iChnSinForPath, _countof(m_iChnSinForPath));
	if (-1 != iSubStr)
	{
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------
__int64 GetLocalFileSize(const std::string& _strPath)
{
	if(!PathFileExists(_strPath.c_str()))
	{
		return -1;

	}

	FILE* file = NULL;
	__int64 ifileSizeBytes = 0;
	fopen_s(&file, _strPath.c_str(),"r");
	if(file != NULL && file != INVALID_HANDLE_VALUE)
	{
		fseek(file, 0, SEEK_END);
		ifileSizeBytes = _ftelli64(file);
		fseek(file, 0, SEEK_SET);
		fclose(file);
	}
	else
	{
		OutputDebugString("file != NULL && file != INVALID_HANDLE_VALUE");
		return -1;
	}

	return ifileSizeBytes;

}

string NewGUID()
{
	char buf[64] = {0};
	GUID guid;
	CoInitialize(NULL);
	if (S_OK == ::CoCreateGuid(&guid))
	{
		_snprintf_s(buf, sizeof(buf), 60, 
			"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			guid.Data1,
			guid.Data2,
			guid.Data3,
			guid.Data4[0], guid.Data4[1],
			guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]);
	}
	CoUninitialize();
	return buf;
}

BOOL SaveFile(const std::string _strSavePath, bool _blFullPath)
{
	CString cstrSavePath = "";
	if (!_blFullPath){
		cstrSavePath.Format("%s%s",_strSavePath.c_str(), "\\123.txt");
		HANDLE hFile = NULL;
		hFile = CreateFile(cstrSavePath,GENERIC_WRITE,FILE_SHARE_READ,NULL,
			OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (GetLastError() == 5) {
			OutputDebugString("SaveFile err, savepath(%s) not have write permission error is 5");
			return FALSE;
		}
		CloseHandle(hFile);
	}

	int ret = _access(_strSavePath.c_str(), 2);
	if(ret == -1){
		OutputDebugString("SaveFile err, savepath(%s) not have write permission");
		DeleteFile(cstrSavePath);
		return FALSE;
	}
	DeleteFile(cstrSavePath);
	return TRUE;
}

int covert(char *desc, char *src, char *input, size_t ilen, char *output, size_t olen)
{
	const char **pin = (const char**)&input;
	char **pout = &output;
	iconv_t cd = iconv_open(desc, src);
	if (cd == (iconv_t)-1)
	{
		return -1;
	}
	memset(output, 0, olen);
	if (iconv(cd, pin, &ilen, pout, &olen)) return -1;
	iconv_close(cd);
	return 0;
}

wchar_t* string_to_wstring(char* c)
{
	wchar_t* strDes = NULL;
	int len = 0;
	len = strlen(c) + 1;
	int unicodeLen = ::MultiByteToWideChar(CP_ACP,
		0,
		c,
		-1,
		NULL,
		0);
	wchar_t * pUnicode;
	pUnicode = new wchar_t[unicodeLen + 1];
	memset(pUnicode, 0, (unicodeLen + 1)*sizeof(wchar_t));
	::MultiByteToWideChar(CP_ACP,
		0,
		c,
		-1,
		(LPWSTR)pUnicode,
		unicodeLen);

	return pUnicode;
}