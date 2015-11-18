#pragma once
#ifndef PUBLIC_FUNCTION_H
#define PUBLIC_FUNCTION_H

#include <string>
#include <map>
#include <vector>
#include <stdio.h>
#include <io.h>
#include <atlstr.h>
#include "iconv.h"
using namespace std;

/************************************************************************
Function：    AnsiToUTF8
Description： 将AnsiString格式字符串转成UTF8格式	
Input：       strIn 需要转换的字符串     
Output：      strOut 转换后传出字符串	     
Return：     
Others：           
************************************************************************/
string AnsiToUTF8( const string& strIn, string& strOut );

/************************************************************************
Function：    AnsiToUtf8
Description： 将AnsiString格式字符串转成UTF8格式 
Input：       strIn 需要转换的字符串 
Output：      
Return：      转换后的UTF8格式的字符串 
Others：           
************************************************************************/
string AnsiToUtf8( const string& strIn);
void AnsiToUTF8( const char* _pstrIn,CString &_cstrOut);
/************************************************************************
Function：    UTF8ToAnsi
Description： 将UTF8格式的字符串转成AnsiString格式
Input：       strIn 需要转换字符串 
Output：      
Return：      转换后的AnsiString格式的字符串
Others：           
************************************************************************/
string UTF8ToAnsi( const string& strIn );
void UTF8ToAnsi( const char* _pstrIn,CString &_strOut);

unsigned Ucs2BeToUcs2Le(unsigned short *ucs2bige, unsigned int size);

unsigned int Ucs2ToUtf8(unsigned short *ucs2, unsigned int ucs2_size,
	unsigned char *utf8, unsigned int utf8_size);
/************************************************************************
Function：    ConvertStringToURLCoding
Description： 将字符转化为URL格式
Input：      
Output：      
Return：     
Others：           
************************************************************************/
BOOL ConvertStringToURLCoding(CString &strDest, const char* strUTF8, int iLength);

/************************************************************************
Function：    StdStrTrim
Description： 去掉std string 字符串左右空格
Input：       
Output：      
Return：     
Others：           
************************************************************************/
string StdStrTrim(const string _strSource);

/************************************************************************
Function：    IntToStr
Description： 将整型转化为字符串 
Input：       _iValue：需要转换的整型
Output：      
Return：      转化后的字符串  
Others：           
************************************************************************/
string IntToStr( int _iValue );

/************************************************************************
Function：     GetExName
Description：  获得应用程序名称
Input：       
Output：      
Return：       应用程序名称  
Others：           
************************************************************************/
string GetExName();

/************************************************************************
Function：    ExtractFilePath
Description： 获取全路径
Input：       
Output：      
Return：     
Others：           
************************************************************************/
string ExtractFilePath(string _strFullName);
/************************************************************************
Function：    GetText
Description： 通过资源ID，获取
Input：      
Output：      
Return：      返回资源描述     
Others：           
************************************************************************/
CString GetTextEx(UINT _uiID);

/************************************************************************
Function：    GetText
Description： 通过json文件获取
Input：      
Output：      
Return：      返回资源描述     
Others：           
************************************************************************/
CString GetText(string _strID);

/************************************************************************
Function：    Execute
Description： 双击使用windows自带查看器打开
Input：      _cstrFilePath 文件路径
Output：      
Return：      返回资源描述     
Others：           
************************************************************************/
HINSTANCE Execute(CString _cstrFilePath);

/************************************************************************
Function：    CreateMulityDir
Description： 创建文件夹
Input：       _strDir:文件路径
Output：      
Return：           
Others：           
************************************************************************/
BOOL CreateMulityDir(string _strDir);

/************************************************************************
Function：    TheContainSubStr
Description： 判断是否包含某个字符
Input：       _wstrSource：要判断的字符串
			  _iChnSin:非法字符
			  _iSize:非法字符个数
Output：      
Return：      -1:不含非法字符 ,其余：含有非法字符
Others：      
************************************************************************/
int __stdcall ContainSubStr(const wstring& _wstrSource, const int* _iChnSin, const int& _iSize);

/************************************************************************
Function：    IsIllCharInStrForCutShort
Description： 是否为非法字符（缩减版）
Input：       _strSource：要判断的字符串
Output：      
Return：      true:非法，false：合法
Others：      
************************************************************************/
bool IsIllCharInStrForCutShort(const std::string& _strSource);

/************************************************************************、、
Function：    IsIllCharInStr
Description： 是否为非法字符  除去：\ 空格 ： ! ()字符
Input：       _strSource：要判断的字符串
Output：      
Return：      true:非法，false：合法
Others：      
************************************************************************/
bool IsIllCharInStrForPath(const std::string& _strSource);

/************************************************************************
Function：    GetLocalFileSize
Description： 得到文件大小
Input：       _strPath 文件路径
Output：      
Return：      文件大小 
Others：      
************************************************************************/
__int64 GetLocalFileSize(const std::string& _strPath);
/************************************************************************
Function：    NewGUID
Description： 创建一个UUID
Input：       
Output：      
Return：      UUID
Others：      
************************************************************************/
string NewGUID();

/************************************************************************
Function：    SaveFile
Description： 判断保存文件是否成功
Input：       _strSavePath 文件保存路径
Output：      
Return：      成功;失败
Others：      
************************************************************************/
BOOL SaveFile(const std::string _strSavePath, bool _blFullPath = false);

int covert(char *, char *, char *, size_t, char *, size_t);

wchar_t* string_to_wstring(char* c);

#endif;
