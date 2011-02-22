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

#include "unzip32.h"

#include <glib/gi18n-lib.h>

typedef DWORD (*GETVERSIONPROC)(void);
typedef BOOL (*CHECKARCHIVE)(LPCSTR szFilename, const int iMode);
typedef int (*GETARCHIVETYPE)(LPCSTR szFilename);
typedef HARC (*OPENARCHIVE)(const HWND hWnd, LPCSTR szFilename, const DWORD dwMode);


int main(int argc, char *argv[])
{
  HANDLE hDLL = LoadLibrary(L"7-zip32");
  DWORD dwVersion;
  if (hDLL != NULL){
      GETVERSIONPROC hfunc = (GETVERSIONPROC)GetProcAddress(hDLL, "SevenZipGetVersion");
      if (!hfunc){
      }else{
          dwVersion = hfunc();
          /* 256*3+16*9+8 = 920 -> 9.20 */
          DWORD dwHi = dwVersion & 0x0000ffff;
          g_print("%08lx\n", dwHi);
          DWORD dwLow = ((dwVersion & 0xffff0000) >>16);
          g_print("%08lx\n", dwLow);
      }
      CHECKARCHIVE hChkFunc = (CHECKARCHIVE)GetProcAddress(hDLL, "SevenZipCheckArchive");
      if (!hChkFunc){
          g_print("404 SevenZipCheckArchive\n");
      }else{
          BOOL bCheck = hChkFunc("passwd.7z", CHECKARCHIVE_BASIC);
          if (bCheck!=TRUE){
              g_print("invalid\n");
          }
      }
      GETARCHIVETYPE hArchFunc = (GETARCHIVETYPE)GetProcAddress(hDLL, "SevenZipGetArchiveType");
      if (!hArchFunc){
          g_print("404 SevenZipGetArchiveType\n");
      }else{
          int nType = hArchFunc("passwd.zip");
          switch(nType){
          case 0:
              g_print("unknown archive\n");
              break;
          case 1:
              g_print("zip archive\n");
              break;
          case 2:
              g_print("7zip archive\n");
              break;
          default:
              g_print("invalid archive\n");
              break;
          }
      }
      OPENARCHIVE hOpenFunc = (OPENARCHIVE)GetProcAddress(hDLL, "SevenZipOpenArchive");
      if (!hOpenFunc){
          g_print("404 SevenZipOpenArchive\n");
      }else{
          HARC hArch = hOpenFunc(NULL, "passwd.zip", M_ERROR_MESSAGE_ON);
      }
  }
  return 0;
}
