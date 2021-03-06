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
typedef int (*WINAPI_UNLHA)(HWND _hwnd, LPCTSTR _szCmdLine, LPTSTR _szOutput,const DWORD _dwSize);

#define GETPROCADDRESS(type, var, fname)            \
    type var = (type)GetProcAddress(hDLL, fname);   \
        if (!var){                                  \
        g_print("404 %s", fname);                   \
        return 0; \
        }

int main(int argc, char*[] argv){
{
  HANDLE hDLL = LoadLibrary(L"unlha32");
  if (hDLL != NULL){
      GETPROCADDRESS(WINAPI_UNLHA, hUnlha, "Unlha");
      TCHAR buf[1024];
      DWORD dwSize;
      hUnlha(NULL, "x ", buf, dwSize);
  }
  return 0;
}
