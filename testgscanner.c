
#include "defs.h"

#include <glib.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <sys/stat.h>

#include <glib/gi18n-lib.h>
#include <locale.h>

int main(int argc, char*argv[])
{
  GScanner *gscan = g_scanner_new(NULL);
  gscan->config->scan_identifier_1char=TRUE;
  
  const gchar *text="testパスワードはpasswdです\t\r\nnopass いろはにほへと";
  
  g_scanner_input_text(gscan, text, strlen(text));

  GTokenValue gvalue;
  int index=0;
  while( g_scanner_eof(gscan) != TRUE){
    GTokenType gtoken = g_scanner_get_next_token (gscan);
    switch (gtoken){
    case G_TOKEN_CHAR:
      gvalue = g_scanner_cur_value(gscan);
      g_print("char:%s\n", gvalue.v_identifier);
      break;
    case G_TOKEN_IDENTIFIER:
      gvalue = g_scanner_cur_value(gscan);
      for (index = 0; index<strlen(gvalue.v_identifier);index++){
        if (index == strlen(gvalue.v_identifier)-1 &&
            gvalue.v_identifier[index] & 0xe308){
          g_print("0xe308 found:%d\n", index);
          gvalue.v_identifier[index]= '\0';
        }else{
          g_print("char :%0x08\n", gvalue.v_identifier[index]);
        }
      }
      g_print("identifier:%s\n", gvalue.v_identifier);
      break;
    default:
      break;
    }
  }
    
  return 0;
}
