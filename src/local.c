/*
 * local.c -- local commands
 *
 * Yet Another FTP Client
 * Copyright (C) 1998-2001, Martin Hedenfalk <mhe@stacken.kth.se>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See COPYING for more details.
 */

#include "syshdr.h"
#include "input.h"
#include "commands.h"
#include "gvars.h"
#include "strq.h"
#include "utils.h"

/* print local working directory */
void cmd_lpwd(int argc, char **argv)
{
	if(argv) {
		OPT_HELP_NEW(_("Print local working directory."), "lpwd [options]", NULL);
		maxargs(optind - 1);
	}

	char* tmp = getcwd(NULL, 0);
	if (tmp == NULL) {
		fprintf(stderr, _("Couldn't get local working directory...\n"));
		return;
	}
	printf(_("local working directory is '%s'\n"), tmp);
}

/* local change directory */
void cmd_lcd(int argc, char **argv)
{
	char *e = 0, *tmp;
	char *te;

	OPT_HELP_NEW(_("Change local working directory."), "lcd [options] [directory]",
	  _("if [directory] is '-', lcd changes to the previous working directory\n"
			"if omitted, changes to home directory\n"));

	maxargs(optind);

	if(argc < optind + 1)
		e = gvLocalHomeDir;
	else {
		e = argv[optind];
		if(strcmp(e, "-") == 0)
			e = gvLocalPrevDir;
	}
	if(!e)
		return;
	tmp = getcwd(NULL, 0);
	if (tmp == (char *)NULL) {
		fprintf(stderr, _("Couldn't change local working directory...\n"));
		return;
	}
	te = tilde_expand_home(e, gvLocalHomeDir);
	if(chdir(te) == -1)
		perror(te);
	else {
		free(gvLocalPrevDir);
		gvLocalPrevDir = xstrdup(tmp);
	}
	free(te);
	cmd_lpwd(0, 0);
}

/**
* Return a string for the replacement of a specific arg in the shell.
* Don't forget to free() when done.
*
* %h - hostname ("ftp.foo.bar")
* %p - port number
* %u - username
* %d - current directory
**/
char * shell_replace(const char arg)
{
	switch(arg) {
		case 'h':
			if (!ftp->connected) return NULL;
			return xstrdup(ftp->url->hostname);
		case 'p':
			if (!ftp->connected) return NULL;
			char buf[6] = { 0 };
			snprintf(buf, sizeof(buf), "%i", ftp->url->port);
			return xstrdup(buf);
		case 'u':
			if (!ftp->loggedin) return NULL;
			return xstrdup(ftp->url->username);
		case 'd':
			if (!ftp->loggedin) return NULL;
			return xstrdup(ftp->curdir);
		default:
			return NULL;
	}
}

/**
* Return the length of the replacement arg.
**/
size_t shell_replace_len(const char arg)
{
	switch(arg) {
		case 'h':
			if (!ftp->connected) return 0;
			return strlen(ftp->url->hostname);
		case 'p':
			if (!ftp->connected) return 0;
			return 5;
		case 'u':
			if (!ftp->loggedin) return 0;
			return strlen(ftp->url->username);
		case 'd':
			if (!ftp->loggedin) return 0;
			return strlen(ftp->curdir);
		default:
			return 0;
	}
}

/**
* Executes shell commands. Supports replacements for arguments (see func shell_replace)
**/
void cmd_shell(int argc, char **argv)
{
	char *e = 0;

	if(argc > 1) {
		int i;
		size_t s = 0;

		for(i=1; i<argc; i++) {
			s += strlen(argv[i]) + 1;
			if (argv[i][0] == '%') s += shell_replace_len(argv[i][1]);
		}

		e = (char *)xmalloc(s);

		for(i=1; i<argc; i++) {
			if (argv[i][0] == '%' && argv[i][1] != 0 && argv[i][2] == 0) {
				char *repl = shell_replace(argv[i][1]);
				if (repl != NULL) {
					strlcat(e, repl, s);
					free(repl);
				} else {
					strlcat(e, argv[i], s);
				}
			} else {
				strlcat(e, argv[i], s);
			}
			if(i+1 < argc)
				strlcat(e, " ", s);
		}
	}

  if (e)
    invoke_shell("%s", e);
  else
  	invoke_shell(NULL);

	free(e);
}
