/*
 * testunzip32.c -in -- forward functionality plug-in for Sylpheed
 * Copyright (C) 2011 HAYASHI Kentaro <kenhys@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "defs.h"

#include <glib.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <sys/stat.h>

#include "sylmain.h"
#include "plugin.h"
#include "procmsg.h"
#include "procmime.h"
#include "utils.h"
#include "alertpanel.h"
#include "compose.h"

#include "unzip32.h"

#include <glib/gi18n-lib.h>

typedef int (*WINAPI_UNZIP)(const HWND hWnd,LPCSTR szCmdLine,LPSTR szOutput,const DWORD wSize);
typedef WORD (*WINAPI_UNZIPGETVERSION)(VOID);
typedef WORD (*WINAPI_UNZIPGETSUBVERSION)(VOID);
typedef BOOL (*WINAPI_UNZIPGETRUNNING)(VOID);
typedef BOOL (*WINAPI_UNZIPGETBACKGROUNDMODE)(VOID);
typedef BOOL (*WINAPI_UNZIPSETBACKGROUNDMODE)(const BOOL bBackGroundMode);
typedef BOOL (*WINAPI_UNZIPGETCURSORMODE)(VOID);
typedef BOOL (*WINAPI_UNZIPSETCURSORMODE)(const BOOL bCursorMode);
typedef WORD (*WINAPI_UNZIPGETCURSORINTERVAL)(VOID);
typedef BOOL (*WINAPI_UNZIPSETCURSORINTERVAL)(const WORD wInterval);

typedef int (*WINAPI_UNZIPCOMMANDLINE)(HWND hWnd,HINSTANCE hInst,LPCSTR szCmdLine,DWORD nCmdShow);

typedef BOOL (*WINAPI_UNZIPCHECKARCHIVE)(LPCSTR szFileName,const int iMode);
typedef BOOL (*WINAPI_UnZipConfigDialog)(const HWND hwnd,LPSTR szOption,const int iMode);
typedef BOOL (*WINAPI_UnZipQueryFunctionList)(const int iFunction);
typedef int (*WINAPI_UnZipGetFileCount)(LPCSTR szArcFile);
typedef HARC (*WINAPI_UNZIPOPENARCHIVE)(const HWND hWnd,LPCSTR szFileName,const DWORD dwMode);
typedef int (*WINAPI_UNZIPCLOSEARCHIVE)(HARC harc);
typedef int (*WINAPI_UnZipFindFirst)(HARC harc,LPCSTR szWildName,LPINDIVIDUALINFO lpSubInfo);
typedef int (*WINAPI_UnZipFindNext)(HARC harc,LPINDIVIDUALINFO lpSubInfo);
typedef int (*WINAPI_UnZipExtract)(HARC harc,LPCSTR szFileName,LPCSTR szDirName,DWORD dwMode);
typedef int (*WINAPI_UnZipAdd)(HARC harc,LPSTR szFileName,DWORD dwMode);
typedef int (*WINAPI_UnZipMove)(HARC harc,LPSTR szFileName,DWORD dwMode);
typedef int (*WINAPI_UnZipDelete)(HARC harc,LPSTR szFileName,DWORD dwMode);
typedef int (*WINAPI_UnZipGetArcFileName)(HARC harc,LPSTR lpBuffer,const int nSize);
typedef DWORD (*WINAPI_UnZipGetArcFileSize)(HARC harc);
typedef DWORD (*WINAPI_UnZipGetArcOriginalSize)(HARC harc);
typedef DWORD (*WINAPI_UnZipGetArcCompressedSize)(HARC harc);
typedef WORD (*WINAPI_UnZipGetArcRatio)(HARC harc);
typedef WORD (*WINAPI_UnZipGetArcDate)(HARC harc);
typedef WORD (*WINAPI_UnZipGetArcTime)(HARC harc);
typedef UINT (*WINAPI_UnZipGetArcOSType)(HARC harc);
typedef int (*WINAPI_UnZipGetFileName)(HARC harc,LPSTR lpBuffer,const int nSize);
typedef int (*WINAPI_UnZipGetMethod)(HARC harc,LPSTR lpBuffer,const int nSize);
typedef DWORD (*WINAPI_UnZipGetOriginalSize)(HARC harc);
typedef DWORD (*WINAPI_UnZipGetCompressedSize)(HARC harc);
typedef WORD (*WINAPI_UnZipGetRatio)(HARC harc);
typedef WORD (*WINAPI_UnZipGetDate)(HARC harc);
typedef WORD (*WINAPI_UnZipGetTime)(HARC harc);

typedef DWORD (*WINAPI_UnZipGetWriteTime)(HARC harc);
typedef DWORD (*WINAPI_UnZipGetAccessTime)(HARC harc);
typedef DWORD (*WINAPI_UnZipGetCreateTime)(HARC harc);

typedef DWORD (*WINAPI_UnZipGetCRC)(HARC harc);
typedef int (*WINAPI_UNZIPGETATTRIBUTE)(HARC harc);
UINT (*WINAPI_UnZipGetOSType)(HARC harc);

int (*WINAPI_UnZipIsSFXFile)(HARC harc);

BOOL (*WINAPI_UnZipSetOwnerWindow)(const HWND hwnd);
BOOL (*WINAPI_UnZipClearOwnerWindow)(VOID);

/*typedef BOOL CALLBACK ARCHIVERPROC(HWND hWnd,UINT uMsg,UINT nState,LPEXTRACTINGINFOEX lpEis);
  typedef ARCHIVERPROC *LPARCHIVERPROC;*/

typedef BOOL (*WINAPI_UnZipSetOwnerWindowEx)(HWND hWnd,LPARCHIVERPROC lpArcProc);
typedef BOOL (*WINAPI_UnZipKillOwnerWindowEx)(HWND hWnd);

typedef WORD (*WINAPI_MicUnZipGetVersion)(VOID);

#define GETPROCADDRESS(type, var, fname)            \
    type var = (type)GetProcAddress(hDLL, fname);   \
        if (!var){                                  \
        g_print("404 %s", fname);                   \
        return 0; \
        }

const char *GetUnzipError(int nCode)
{
    const char *p;
    switch (nCode){
    case ERROR_DIRECTORY:
        p = g_strdup("ERROR_DIRECTORY");
    case ERROR_CANNOT_WRITE:
        p = g_strdup("ERROR_CANNOT_WRITE");
        break;
    case ERROR_HUFFMAN_CODE:
        p = g_strdup("ERROR_HUFFMAN_CODE");
        break;
    case ERROR_COMMENT_HEADER:
        p =  g_strdup("ERROR_COMMENT_HEADER");
        break;
    case ERROR_HEADER_CRC:
        p = g_strdup("ERROR_HEADER_CRC");
        break;
    case ERROR_HEADER_BROKEN:
        p =g_strdup("ERROR_HEADER_BROKEN");
        break;
#if 0
        ERROR_ARCHIVE_FILE_OPEN
ERROR_NOT_ARCHIVE_FILE
ERROR_CANNOT_READ
ERROR_FILE_STYLE
ERROR_COMMAND_NAME
ERROR_MORE_HEAP_MEMORY
ERROR_ENOUGH_MEMORY
ERROR_ALREADY_RUNNING
#endif
    }
    return p;
}

int main(int argc, char* argv[])
{
  HANDLE hDLL = LoadLibrary(L"unzip32");
  if (hDLL != NULL){
      GETPROCADDRESS(WINAPI_UNZIPGETVERSION, hVersion, "UnZipGetVersion");
      GETPROCADDRESS(WINAPI_UNZIPGETSUBVERSION, hSubVersion, "UnZipGetSubVersion");
      GETPROCADDRESS(WINAPI_UNZIPCHECKARCHIVE, hChkArch, "UnZipCheckArchive");
      GETPROCADDRESS(WINAPI_UNZIPOPENARCHIVE, hOpenArch, "UnZipOpenArchive");
      GETPROCADDRESS(WINAPI_UNZIPCLOSEARCHIVE, hCloseArch, "UnZipCloseArchive");
      GETPROCADDRESS(WINAPI_UNZIPGETATTRIBUTE, hAttr, "UnZipGetAttribute");

      WORD dwVersion = hVersion();
      /* 16*16*2+16+14 = 542 */
      WORD dwSubVersion = hSubVersion();

      g_print("%08x\n", dwVersion);
      g_print("%08x\n", dwSubVersion);
      BOOL bChk = hChkArch("passwd.zip", 1);
      g_print("%08x\n", bChk);
      bChk = hChkArch("nopasswd.zip", 1);
      g_print("%08x\n", bChk);
      HARC hArch = hOpenArch(NULL, "passwd.zip", 0);
      if (hArch!=NULL){
          g_print("open harc %p\n", hArch);
      }
      int nResult = hAttr(hArch);
      g_print("attr arch %08x \n", nResult);

      nResult = hCloseArch(hArch);
      g_print("close arch %08x \n", nResult);/*, GetUnzipError(nResult));*/
      
  }
  return 0;
}
