/*********************************************************************
 * gncmod-app-file.c
 * module definition/initialization app-level file interface 
 * 
 * Copyright (c) 2001 Linux Developers Group, Inc. 
 *********************************************************************/

#include <stdio.h>
#include <glib.h>
#include <libguile.h>
#include "guile-mappings.h"

#include "gnc-file-p.h"
#include "gnc-file-history.h"
#include "gnc-file-dialog.h"
#include "gnc-mdi-utils.h"
#include "gnc-module.h"
#include "gnc-module-api.h"

/* version of the gnc module system interface we require */
int libgncmod_app_file_LTX_gnc_module_system_interface = 0;

/* module versioning uses libtool semantics. */
int libgncmod_app_file_LTX_gnc_module_current  = 0;
int libgncmod_app_file_LTX_gnc_module_revision = 0;
int libgncmod_app_file_LTX_gnc_module_age      = 0;

/* forward references */
char *libgncmod_app_file_LTX_gnc_module_path(void);
char *libgncmod_app_file_LTX_gnc_module_description(void);
int libgncmod_app_file_LTX_gnc_module_init(int refcount);
int libgncmod_app_file_LTX_gnc_module_end(int refcount);


char *
libgncmod_app_file_LTX_gnc_module_path(void) 
{
  return g_strdup("gnucash/app-file");
}

char * 
libgncmod_app_file_LTX_gnc_module_description(void) 
{
  return g_strdup("Application level file interface");
}

static void
lmod(char * mn) 
{
  char * form = g_strdup_printf("(use-modules %s)\n", mn);
  scm_c_eval_string(form);
  g_free(form);
}

int
libgncmod_app_file_LTX_gnc_module_init(int refcount) 
{
  /* load the engine (we depend on it) */
  if(!gnc_module_load("gnucash/engine", 0)) {
    return FALSE;
  }

  /* load the calculation module (we depend on it) */
  if(!gnc_module_load("gnucash/app-utils", 0)) {
    return FALSE;
  }

  lmod ("(g-wrapped gw-app-file)");

  if (refcount == 0)
  {
    gnc_file_set_handlers (gnc_history_add_file,
                           gnc_history_get_last,
                           gnc_file_dialog);    

    gnc_file_set_pct_handler (gnc_mdi_show_progress);

    gnc_file_init ();
  }

  return TRUE;
}

int
libgncmod_app_file_LTX_gnc_module_end(int refcount) 
{
  if(refcount == 0) 
  {
    gnc_file_set_handlers (NULL, NULL, NULL);
    gnc_file_set_pct_handler (NULL);
  }
  return TRUE;
}