/*
 * test7zip32.c
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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sylmain.h"
#include "plugin.h"
#include "procmsg.h"
#include "procmime.h"
#include "utils.h"
#include "alertpanel.h"
#include "compose.h"

#include "unlha32.h"
#include "unzip32.h"

#include <glib/gi18n-lib.h>

/* LHA.DLL compatible API */
typedef int WINAPI (*WINAPI_SEVENZIP)(const HWND _hwnd, LPCSTR _szCmdLine, LPSTR _szOutput, const DWORD _dwSize);
typedef WORD  WINAPI (*WINAPI_SEVENZIPGETVERSION)(void);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETCURSORMODE)(void);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPSETCURSORMODE)(const BOOL _CursorMode);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETBACKGROUNDMODE)(void);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPSETBACKGROUNDMODE)(const BOOL _BackGroundMode);
typedef WORD  WINAPI (*WINAPI_SEVENZIPGETCURSORINTERVAL)(void);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPSETCURSORINTERVAL)(const WORD _Interval);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETRUNNING)(void);

typedef BOOL  WINAPI (*WINAPI_SEVENZIPCONFIGDIALOG)(const HWND _hwnd, LPSTR _szOptionBuffer, const int _iMode);
typedef BOOL WINAPI (*WINAPI_SEVENZIPCHECKARCHIVE)(LPCSTR _szFileName, const int _iMode);
typedef int   WINAPI (*WINAPI_SEVENZIPGETFILECOUNT)(LPCSTR _szArcFile);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPQUERYFUNCTIONLIST)(const int _iFunction);

typedef HARC  WINAPI (*WINAPI_SEVENZIPOPENARCHIVE)(const HWND _hwnd, LPCSTR _szFileName, const DWORD _dwMode);
typedef int   WINAPI (*WINAPI_SEVENZIPCLOSEARCHIVE)(HARC _harc);
typedef int   WINAPI (*WINAPI_SEVENZIPFINDFIRST)(HARC _harc, LPCSTR _szWildName, INDIVIDUALINFO *_lpSubInfo);
typedef int   WINAPI (*WINAPI_SEVENZIPFINDNEXT)(HARC _harc, INDIVIDUALINFO *_lpSubInfo);
typedef int   WINAPI (*WINAPI_SEVENZIPGETARCFILENAME)(HARC _harc, LPSTR _lpBuffer, const int _nSize);
typedef DWORD WINAPI (*WINAPI_SEVENZIPGETARCFILESIZE)(HARC _harc);
typedef DWORD WINAPI (*WINAPI_SEVENZIPGETARCORIGINALSIZE)(HARC _harc);
typedef DWORD WINAPI (*WINAPI_SEVENZIPGETARCCOMPRESSEDSIZE)(HARC _harc);
typedef WORD  WINAPI (*WINAPI_SEVENZIPGETARCRATIO)(HARC _harc);
typedef WORD  WINAPI (*WINAPI_SEVENZIPGETARCDATE)(HARC _harc);
typedef WORD  WINAPI (*WINAPI_SEVENZIPGETARCTIME)(HARC _harc);
typedef UINT  WINAPI (*WINAPI_SEVENZIPGETARCOSTYPE)(HARC _harc);
typedef int   WINAPI (*WINAPI_SEVENZIPISSFXFILE)(HARC _harc);
typedef int   WINAPI (*WINAPI_SEVENZIPGETFILENAME)(HARC _harc, LPSTR _lpBuffer, const int _nSize);
typedef DWORD WINAPI (*WINAPI_SEVENZIPGETORIGINALSIZE)(HARC _harc);
typedef DWORD WINAPI (*WINAPI_SEVENZIPGETCOMPRESSEDSIZE)(HARC _harc);
typedef WORD  WINAPI (*WINAPI_SEVENZIPGETRATIO)(HARC _harc);
typedef WORD  WINAPI (*WINAPI_SEVENZIPGETDATE)(HARC _harc);
typedef	WORD  WINAPI (*WINAPI_SEVENZIPGETTIME)(HARC _harc);
typedef DWORD WINAPI (*WINAPI_SEVENZIPGETCRC)(HARC _harc);
typedef int   WINAPI (*WINAPI_SEVENZIPGETATTRIBUTE)(HARC _harc);
typedef UINT  WINAPI (*WINAPI_SEVENZIPGETOSTYPE)(HARC _harc);
typedef int   WINAPI (*WINAPI_SEVENZIPGETMETHOD)(HARC _harc, LPSTR _lpBuffer, const int _nSize);
typedef DWORD WINAPI (*WINAPI_SEVENZIPGETWRITETIME)(HARC _harc);
typedef	DWORD WINAPI (*WINAPI_SEVENZIPGETCREATETIME)(HARC _harc);
typedef DWORD WINAPI (*WINAPI_SEVENZIPGETACCESSTIME)(HARC _harc);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETWRITETIMEEX)(HARC _harc, FILETIME *_lpftLastWriteTime);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETCREATETIMEEX)(HARC _harc, FILETIME *_lpftLastWriteTime);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETACCESSTIMEEX)(HARC _harc, FILETIME *_lpftLastWriteTime);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETARCCREATETIMEEX)(HARC _harc, FILETIME *_lpftCreationTime);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETARCACCESSTIMEEX)(HARC _harc, FILETIME *_lpftLastAccessTime);
typedef	BOOL  WINAPI (*WINAPI_SEVENZIPGETARCWRITETIMEEX)(HARC _harc, FILETIME *_lpftLastWriteTime);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETARCFILESIZEEX)(HARC _harc, ULHA_INT64 *_lpllSize);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETARCORIGINALSIZEEX)(HARC _harc, ULHA_INT64 *_lpllSize);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETARCCOMPRESSEDSIZEEX)(HARC _harc, ULHA_INT64 *_lpllSize);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETORIGINALSIZEEX)(HARC _harc, ULHA_INT64 *_lpllSize);
typedef BOOL  WINAPI (*WINAPI_SEVENZIPGETCOMPRESSEDSIZEEX)(HARC _harc, ULHA_INT64 *_lpllSize);

typedef BOOL WINAPI (*WINAPI_SEVENZIPSETOWNERWINDOW)(HWND _hwnd);
typedef BOOL WINAPI (*WINAPI_SEVENZIPCLEAROWNERWINDOW)(void);
typedef BOOL WINAPI (*WINAPI_SEVENZIPSETOWNERWINDOWEX)(HWND _hwnd, LPARCHIVERPROC _lpArcProc);
typedef BOOL WINAPI (*WINAPI_SEVENZIPKILLOWNERWINDOWEX)(HWND _hwnd);
typedef BOOL WINAPI (*WINAPI_SEVENZIPSETOWNERWINDOWEX64)(HWND _hwnd, LPARCHIVERPROC _lpArcProc, DWORD _dwStructSize);
typedef BOOL WINAPI (*WINAPI_SEVENZIPKILLOWNERWINDOWEX64)(HWND _hwnd);

typedef WORD WINAPI (*WINAPI_SEVENZIPGETSUBVERSION)();
typedef int  WINAPI (*WINAPI_SEVENZIPGETARCHIVETYPE)(LPCSTR _szFileName);
typedef BOOL WINAPI (*WINAPI_SEVENZIPSETUNICODEMODE)(BOOL _bUnicode);
typedef int WINAPI (*WINAPI_SEVENZIPSETDEFAULTPASSWORD)(HARC _harc, LPCSTR _szPassword);
typedef DWORD WINAPI (*WINAPI_SEVENZIPGETDEFAULTPASSWORD)(HARC _harc, LPSTR _szPassword, DWORD _dwSize);
typedef int WINAPI (*WINAPI_SEVENZIPPASSWORDDIALOG)(HWND _hwnd, LPSTR _szBuffer, DWORD _dwSize);
typedef BOOL WINAPI (*WINAPI_SEVENZIPSETPRIORITY)(const int _nPriority);

#define GETPROCADDRESS(type, var, fname)            \
    type var = (type)GetProcAddress(hDLL, fname);   \
        if (!var){                                  \
        g_print("404 %s", fname);                   \
        return 0; \
        }

int main(int argc, char *argv[])
{
  HANDLE hDLL = LoadLibrary(L"7-zip32");
  if (hDLL != NULL){
      GETPROCADDRESS(WINAPI_SEVENZIP, hZip, "SevenZip");
      GETPROCADDRESS(WINAPI_SEVENZIPGETVERSION, hVersion, "SevenZipGetVersion");
      GETPROCADDRESS(WINAPI_SEVENZIPGETSUBVERSION, hSubVersion, "SevenZipGetSubVersion");
      GETPROCADDRESS(WINAPI_SEVENZIPCHECKARCHIVE, hChkFunc, "SevenZipCheckArchive");
      GETPROCADDRESS(WINAPI_SEVENZIPGETARCHIVETYPE, hArchTypeFunc, "SevenZipGetArchiveType");
      GETPROCADDRESS(WINAPI_SEVENZIPOPENARCHIVE, hOpenFunc, "SevenZipOpenArchive");
      GETPROCADDRESS(WINAPI_SEVENZIPCLOSEARCHIVE, hCloseFunc, "SevenZipCloseArchive");
      GETPROCADDRESS(WINAPI_SEVENZIPSETDEFAULTPASSWORD, hSetDefPasswd, "SevenZipSetDefaultPassword");
      GETPROCADDRESS(WINAPI_SEVENZIPGETFILECOUNT, hFileCount, "SevenZipGetFileCount");
      GETPROCADDRESS(WINAPI_SEVENZIPGETATTRIBUTE, hAttribute, "SevenZipGetAttribute");
      GETPROCADDRESS(WINAPI_SEVENZIPQUERYFUNCTIONLIST, hQueryFunc, "SevenZipQueryFunctionList");

      DWORD dwVersion;
      dwVersion = hVersion();
      DWORD dwSubVersion;
      dwSubVersion = hSubVersion();
      /* 256*3+16*9+8 = 920 -> 9.20 */
      DWORD dwHi = dwVersion & 0x0000ffff;
      g_print("%08lx\n", dwHi);
      DWORD dwLow = (dwSubVersion & 0x0000ffff);
      g_print("%08lx\n", dwLow);

      BOOL bCheck;
      int nType;
      HARC hArch;
      int nAttr;
      int nResult;
      bCheck = hQueryFunc(ISARC_GET_ATTRIBUTE);
      g_print("bCheck:%08x\n", bCheck);
      char buf[1024];
      DWORD dwSize = 0;
      {
          nResult = hZip(NULL, "x hpasswd.7z -aoa -p\"foo\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("hpasswd.7z invalid password nResult:%08x\n", nResult);
      }
      {
          nResult = hZip(NULL, "x hpasswd.7z -aoa -p\"\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("hpasswd.7z blank password nResult:%08x\n", nResult);
      }
      {
          nResult = hZip(NULL, "x hpasswd.7z -aoa -p\"test\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("hpasswd.7z test password nResult:%08x\n", nResult);
      }

      {
          nResult = hZip(NULL, "x passwd.7z -aoa -p\"foo\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("passwd.7z invalid password passwd.7z nResult:%08x\n", nResult);

          nResult = hZip(NULL, "x passwd.7z -aoa -p\"test\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("passwd.7z test password passwd.7z nResult:%08x\n", nResult);

          nResult = hZip(NULL, "x passwd.7z -aoa -p\"\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("passwd.7z blank password passwd.7z nResult:%08x\n", nResult);
      }

      {
          nResult = hZip(NULL, "x passwd.zip -aoa -p\"\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("passwd.zip blank password nResult:%08x\n", nResult);

          nResult = hZip(NULL, "x passwd.zip -aoa -p\"test\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("passwd.zip test password nResult:%08x\n", nResult);
          nResult = hZip(NULL, "x passwd.zip -aoa -p\"foo\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("passwd.zip invalid password nResult:%08x\n", nResult);
      }

      {
          nResult = hZip(NULL, "x nopasswd.zip -aoa -p\"\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("nopasswd.zip blank password nResult:%08x\n", nResult);

          nResult = hZip(NULL, "x nopasswd.zip -aoa -p\"foo\" -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("nopasswd.zip invalid password nResult:%08x\n", nResult);

          nResult = hZip(NULL, "x nopasswd.zip -aoa -hide -oc:\\Temp\\7zip -r", buf, dwSize);
          g_print("nopasswd.zip no password nResult:%08x\n", nResult);
      }

      {
          nResult = hZip(NULL, "x passwd2.zip -aoa -p\"\" -hide -oc:\\Temp\\7zip -r-", buf, dwSize);
          g_print("passwd2.zip blank password nResult:%08x\n", nResult);

          nResult = hZip(NULL, "x passwd2.zip -aoa -p\"foo\" -hide -oc:\\Temp\\7zip -r-", buf, dwSize);
          g_print("passwd2.zip invalid password nResult:%08x\n", nResult);

          nResult = hZip(NULL, "x passwd2.zip -aoa -p\"test\" -hide -oc:\\Temp\\7zip -r-", buf, dwSize);
          g_print("passwd2.zip no password nResult:%08x\n", nResult);
      }

      return 0;
      {
          /*
           * check hpasswd.7z
           */
          nResult = hSetDefPasswd(NULL, "test");
          hArch = hOpenFunc(NULL, "hpasswd.7z", M_ERROR_MESSAGE_OFF|M_BAR_WINDOW_OFF);
          if (hArch != NULL){
              bCheck = hChkFunc("hpasswd.7z", CHECKARCHIVE_BASIC);
              nType = hArchTypeFunc("hpasswd.7z");
              nAttr = hAttribute(hArch);
              /*hArch = hOpenFunc(NULL, "hpasswd.7z", 0);*/
              g_print("nResult:%08x\n", nResult);
              g_print("bCheck:%08x\n", bCheck);
              g_print("nType:%08x\n", nType);
              g_print("nAttr:%08x\n", nAttr);
              if (hArch != NULL && (nAttr & 0x40)){
                  g_print("may be header encrypted\n");
              }
              hCloseFunc(hArch);
          }else{
              g_print("?\n");
          }
      }
      {
          /*
           * check passwd.7z
           */
          bCheck = hChkFunc("passwd.7z", CHECKARCHIVE_BASIC);
          nType = hArchTypeFunc("passwd.7z");
          hArch = hOpenFunc(NULL, "passwd.7z", 0);
          nAttr = hAttribute(hArch);
          g_print("nAttr:%08x\n", nAttr);
          if (bCheck == TRUE && nType == 2 && hArch != NULL && nAttr & 0x40){
              g_print("passwd.7z password encrypted\n");
          }
          hCloseFunc(hArch);
      }
      {
          /*
           * check passwd.zip
           */
          nResult = hSetDefPasswd(NULL, "foo");
          bCheck = hChkFunc("passwd.zip", CHECKARCHIVE_BASIC);
          nType = hArchTypeFunc("passwd.zip");
          hArch = hOpenFunc(NULL, "passwd.zip", 0);
          g_print("hArch:%p\n", hArch);
          nAttr = hAttribute(hArch);
          g_print("bCheck:%08x\n", bCheck);
          g_print("nType:%08x\n", nType);
          g_print("nAttr:%08x\n", nAttr);
          if (bCheck == TRUE && nType == 1 && hArch != NULL && nAttr & 0x40){
              g_print("passwd.zip password encrypted\n");
          }
          hCloseFunc(hArch);
      }
      {
          /*
           * check nopasswd.zip
           */
          nResult = hSetDefPasswd(NULL, "foo");
          bCheck = hChkFunc("nopasswd.zip", CHECKARCHIVE_BASIC);
          nType = hArchTypeFunc("nopasswd.zip");
          hArch = hOpenFunc(NULL, "nopasswd.zip", 0);
          g_print("hArch:%p\n", hArch);
          nAttr = hAttribute(hArch);
          g_print("bCheck:%08x\n", bCheck);
          g_print("nType:%08x\n", nType);
          g_print("nAttr:%08x\n", nAttr);
          if (bCheck == TRUE && nType == 1 && hArch != NULL && nAttr & 0x40){
              g_print("nopasswd.zip password encrypted\n");
          }
      }
      return 0;
  }
  return 0;
}
