/* 
 * $Id: mailinglist.c,v 1.10 2004-02-07 09:22:36 rwidmer Exp $
 * Copyright (C) 1999-2002 Inter7 Internet Technologies, Inc. 
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include "config.h"
#include "qmailadmin.h"
#include "qmailadminx.h"
#include <errno.h>

char dotqmail_name[MAX_FILE_NAME];
char replyto_addr[256];
int replyto;
int dotnum;
int checkopt[256];    /* used to display mailing list options */

#define REPLYTO_SENDER 1
#define REPLYTO_LIST 2
#define REPLYTO_ADDRESS 3

void set_options();
void default_options();

int show_mailing_lists(char *user, char *dom, time_t mytime)
{

 fprintf(stderr, "show_mailing_lists was called\n");
  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  /* see if there's anything to display */
   if ( CurMailingLists == 0 ) {
    sprintf(StatusMessage,"%s", get_html_text(231));
    show_menu();
    return(231);
  }

   if ( MaxMailingLists == 0 ) {
    return(0);
  }
  send_template( "show_mailinglist.html" );
}

int show_mailing_list_line(char *user, char* dom, time_t mytime, char *dir)
{
  DIR *mydir;
  struct dirent *mydirent;
  FILE *fs;
  char *addr;
  char testfn[MAX_FILE_NAME];
  char Buffer[MAX_BUFF];
  int i,j;

  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }
   if ( MaxMailingLists == 0 ) {
    return(0);
  }

  if ( (mydir = opendir(".")) == NULL ) {
    fprintf(stderr,"QmailAdmin: %s %s", 
            get_html_text(143), "domain directory");
    return(143);
  }

  sort_init();

  /* Now, display each list */
  while( (mydirent=readdir(mydir)) != NULL ) {
    if ( strncmp(".qmail-", mydirent->d_name, 7) == 0 ) {
      if ( (fs=fopen(mydirent->d_name,"r"))==NULL) {
#ifdef EZMLMIDX
        strcpy(uBufA, "12");
#else
        strcpy(uBufA, "5");
#endif
        sprintf(uBufB, "SMLL %s %s\n", get_html_text(144), mydirent->d_name);
        send_template("show_error_line.html");
        continue;
      }
      fgets(Buffer, sizeof(Buffer), fs);
      fclose(fs);
      if ( strstr( Buffer, "ezmlm-reject") != 0 ) {
        sort_add_entry (&mydirent->d_name[7], 0);
      }
    }
  }
  closedir(mydir);
  sort_dosort();

  for (i = 0; addr = sort_get_entry(i); ++i) {
    sprintf (testfn, ".qmail-%s-digest-owner", addr);
    /* convert ':' in addr to '.' */
    str_replace (addr, ':', '.');

    qmail_button(uBufA, "delmailinglist", addr, "trash.png");

#ifdef EZMLMIDX
    qmail_button(uBufB, "modmailinglist", addr, "modify.png");
#endif
    sprintf(uBufC,"%s\n", addr); 

    qmail_button(uBufD, "addlistuser", addr, "delete.png");
    qmail_button(uBufE, "dellistuser", addr, "delete.png");
    qmail_button(uBufF, "showlistusers", addr, "delete.png");

#ifdef EZMLMIDX
    qmail_button(uBufG, "addlistmod", addr, "delete.png");
    qmail_button(uBufH, "dellistmod", addr, "delete.png");
    qmail_button(uBufI, "showlistmod", addr, "delete.png");

    /* Is it a digest list? */
    if ( (fs=fopen(testfn,"r"))==NULL) {
      /* not a digest list */
      fprintf (actout, "<TD COLSPAN=3> </TD>");
    } else {
      qmail_button(uBufJ, "addlistdig", addr, "delete.png");
      qmail_button(uBufK, "dellistdig", addr, "delete.png");
      qmail_button(uBufL, "showlistdig", addr, "delete.png");
      fclose(fs);
    }
#endif
  send_template("show_mailinglist_line.html");
  }
  sort_cleanup();
}

int is_mailing_list(FILE *fs)
{
  char Buffer[MAX_BUFF];

       while (!feof(fs)) {
               fgets( Buffer, sizeof(Buffer), fs);
               if ( strstr( Buffer, "ezmlm-reject") != 0 ||
                    strstr( Buffer, "ezmlm-send")   != 0 )
                       return -1;
       }
       return 0;
}

/* mailing list lines on the add user page */
int show_mailing_list_line2(char *user, char *dom, time_t mytime, char *dir)
{
 DIR *mydir;
 struct dirent *mydirent;
 FILE *fs;
 char *addr;
 int i,j;
 int listcount;
 char Buffer[MAX_BUFF];

  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  if (*EZMLMDIR == 'n' || MaxMailingLists == 0 ) {
    return(0);
  }

  if ( (mydir = opendir(".")) == NULL ) {
    fprintf(stderr,"%s %s\n", get_html_text(143), "domain directory");
    return(143);
  }

  listcount = 0;
  sort_init();

  while( (mydirent=readdir(mydir)) != NULL ) {
    if ( strncmp(".qmail-", mydirent->d_name, 7) == 0 ) {
      if ( (fs=fopen(mydirent->d_name,"r"))==NULL) {
        fprintf(stderr,"SMLL3 %s %s\n", get_html_text(144), mydirent->d_name);
        continue;
      }
      fgets( Buffer, sizeof(Buffer), fs);
      fclose(fs);
      if ( strstr( Buffer, "ezmlm-reject") != 0 ) {
        sort_add_entry (&mydirent->d_name[7], 0);
        listcount++;
      }
    }
  }
  closedir(mydir);

  /* if there aren't any lists, don't display anything */
  if (listcount == 0) {
    sort_cleanup();
    return 0;
  }

  sort_dosort();

  fprintf(actout, "<INPUT NAME=number_of_mailinglist TYPE=hidden VALUE=%d>\n", listcount);
  for (i = 0; i < listcount; ++i) {
    addr = sort_get_entry(i);
    str_replace (addr, ':', '.');
    sprintf(uBufA,"%d", i);
    sprintf(uBufB,"%s", addr);
    sprintf(uBufC,"%s@%s", addr, Domain);
    send_template("show_mailinglist_line2.html");
  }

  sort_cleanup();
}


int addmailinglist(void)
{

  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  if ( MaxMailingLists != -1 && CurMailingLists >= MaxMailingLists ) {
    fprintf(actout, "%s %d\n", get_html_text(184), 
      MaxMailingLists);
    show_menu();
    return(184);
  }
  
  /* set up default options for new list */
  default_options();

#ifdef EZMLMIDX
  send_template( "add_mailinglist-idx.html" );
#else
  send_template( "add_mailinglist-no-idx.html" );
#endif

}

int delmailinglist(void)
{
  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  send_template( "del_mailinglist_confirm.html" );
}

int delmailinglistnow(void)
{
 int pid;
 DIR *mydir;
 struct dirent *mydirent;
 char Buffer1[MAX_BUFF];
 char Buffer2[MAX_BUFF];

  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  if ( (mydir = opendir(".")) == NULL ) {
    fprintf(actout,"%s %d<BR>\n", get_html_text(143), 1);
    fprintf(actout,"</table>");
    return 0;
  }
 
  /* make dotqmail name */
  strcpy(dotqmail_name, ActionUser);
  for(dotnum=0;dotqmail_name[dotnum]!='\0';dotnum++) {
    if(dotqmail_name[dotnum]=='.') dotqmail_name[dotnum] = ':';
  }

  sprintf(Buffer2, ".qmail-%s", dotqmail_name);
  sprintf(Buffer1, ".qmail-%s-", dotqmail_name);
  while( (mydirent=readdir(mydir)) != NULL ) {

    /* delete the main .qmail-"list" file */
    if ( strcmp(Buffer2, mydirent->d_name) == 0 ) {
      if ( unlink(mydirent->d_name) != 0 ) {
        ack(get_html_text(185), Buffer2);
      }

    /* delete secondary .qmail-"list"-* files */
    } else if ( strncmp(Buffer1, mydirent->d_name, strlen(Buffer1)) == 0 ) {
      if ( unlink(mydirent->d_name) != 0 ) {
        ack(get_html_text(185), Buffer2);
      }
    }
  }
  closedir(mydir);


  sprintf(Buffer2, "%s/%s", RealDir, ActionUser);
  vdelfiles(Buffer2);

  CurMailingLists--;

  sprintf(StatusMessage, "%s %s\n", get_html_text(186), ActionUser);
    if ( CurMailingLists == 0 ) {
        show_menu();
    } else {
    show_mailing_lists(Username, Domain, Mytime);
  }

}

/* sets the Reply-To header in header* files based on form fields
 * designed to be called by ezmlm_make() (after calling ezmlm-make)
 * Replaces the "Reply-To" line in <filename> with <newtext>.
 */

void ezmlm_setreplyto (char *filename, char *newtext)
{
  FILE *headerfile, *temp;
  char realfn[256];
  char tempfn[256];
  char buf[256];

  sprintf (realfn, "%s/%s/%s", RealDir, ActionUser, filename);
  sprintf (tempfn, "%s.tmp", realfn);

  headerfile = fopen(realfn, "r");
  if (!headerfile) return;
  temp = fopen(tempfn, "w");
  if (!temp) { fclose (headerfile); return; }

  /* copy contents to new file, except for Reply-To header */
  while (fgets (buf, sizeof(buf), headerfile) != NULL) {
    if (strncasecmp ("Reply-To", buf, 8) != 0) {
      fputs (buf, temp);
    }
  }

  fputs (newtext, temp);

  fclose (headerfile);
  fclose (temp);
  unlink (realfn);
  rename (tempfn, realfn);
}

ezmlm_make (int newlist)
{
 FILE * file;
 int pid;
 char Buffer1[MAX_BUFF];
 char Buffer2[MAX_BUFF];
 char Buffer3[MAX_BUFF];
 char Buffer4[MAX_BUFF];

#ifdef EZMLMIDX
  char list_owner[MAX_BUFF];
  char owneremail[MAX_BUFF+5];  
#endif
  char options[MAX_BUFF];
  char *arguments[MAX_BUFF];
  int argc;
  int i=0;
  char tmp[64];
  char *tmpstr;
  char loop_ch[64];
  int  loop;
  
  /* Initialize listopt to be a string of the characters A-Z, with each one
   * set to the correct case (e.g., A or a) to match the expected behavior
   * of not checking any checkboxes.  Leave other letters blank.
   * NOTE: Leave F out, since we handle it manually.
   */
  char listopt[] = "A  D   hIj L N pQRST      ";
  
  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  if ( fixup_local_name(ActionUser) ) {
    sprintf(StatusMessage, "%s %s\n", get_html_text(188), ActionUser);
    addmailinglist();
    return(188);
  }

  /* update listopt based on user selections */
  for (loop = 0; loop < 20; loop++) {
    sprintf(tmp, "opt%d=", loop);
    GetValue(TmpCGI, loop_ch, tmp, sizeof(loop_ch));
    for (tmpstr = loop_ch; *tmpstr; tmpstr++)
    {
      if ((*tmpstr >= 'A') && (*tmpstr <= 'Z')) {
        listopt[*tmpstr-'A'] = *tmpstr;
      } else if ((*tmpstr >= 'a') && (*tmpstr <= 'z')) {
        listopt[*tmpstr-'a'] = *tmpstr;
      }
    }
  }
  
  /* don't allow option c, force option e if modifying existing list */
  listopt[2] = ' ';
  listopt[4] = newlist ? ' ' : 'e';
  
  argc=0;
  arguments[argc++] = "ezmlm-make";

#ifdef EZMLMIDX
  /* check the list owner entry */
  GetValue(TmpCGI, list_owner, "listowner=", sizeof(list_owner)); // Get the listowner
  if ( strlen(list_owner) > 0 ) {
    sprintf (owneremail, "-5%s", list_owner);
    arguments[argc++] = owneremail;
  }
#endif

  /* build the option string */
  tmpstr = options;
  arguments[argc++] = tmpstr;
  *tmpstr++ = '-';
#ifndef EZMLMIDX
  /* non idx list, only allows options A and P */
  *tmpstr++ = listopt[0];        /* a or A */
  *tmpstr++ = listopt['p'-'a'];  /* p or P */
  *tmpstr++ = 0;   /* add NULL terminator */
#else
  /* ignore options v-z, but test a-u */
  for (i = 0; i <= ('u'-'a'); i++)
  {
    if (listopt[i] != ' ') {
      *tmpstr++ = listopt[i];
    }
  }
  *tmpstr++ = 0;   /* add NULL terminator */

  /* check for sql support */
  GetValue(TmpCGI, tmp, "sqlsupport=", sizeof(tmp));
  if( strlen(tmp) > 0 ) {
    arguments[argc++] = tmpstr;
    tmpstr += sprintf (tmpstr, "%s", tmp) + 1;
    arguments[argc++] = tmpstr;
    for(loop = 1; loop <= NUM_SQL_OPTIONS; loop++) {  
      sprintf(tmp, "sql%d=", loop);
      GetValue(TmpCGI, loop_ch, tmp, sizeof(loop_ch));
      tmpstr += sprintf (tmpstr, "%s:", loop_ch);
    }
    /* remove trailing : */
    tmpstr--;
    *tmpstr++ = 0;
  }
#endif

  /* make dotqmail name */
  strcpy(dotqmail_name, ActionUser);
  for(dotnum=0;dotqmail_name[dotnum]!='\0';dotnum++) {
    if(dotqmail_name[dotnum]=='.') dotqmail_name[dotnum] = ':';
  }
  pid=fork();
  if (pid==0) {
    sprintf(Buffer1, "%s/ezmlm-make", EZMLMDIR);
    sprintf(Buffer2, "%s/%s", RealDir, ActionUser);
    sprintf(Buffer3, "%s/.qmail-%s", RealDir, dotqmail_name);

    arguments[argc++]=Buffer2;
    arguments[argc++]=Buffer3;
    arguments[argc++]=ActionUser;
    arguments[argc++]=Domain;
    arguments[argc]=NULL;

    execv(Buffer1, arguments);
    exit(127);
  } else {
    wait(&pid);
  }

  /* 
   * ezmlm-make -e leaves .qmail-listname-(accept||reject) links for some reason.
   * (causing file permission errors in "show mailing lists") Also, it doesn't 
   * delete dir/digest/ when turning off digests.  This section cleans up...
   */
  if(listopt['M'-'A'] == 'M') { /* moderation off */
    sprintf(tmp, "%s/.qmail-%s-accept-default", RealDir, dotqmail_name);
    unlink (tmp);
    sprintf(tmp, "%s/.qmail-%s-reject-default", RealDir, dotqmail_name);
    unlink (tmp);
  }
  if(listopt['D'-'A'] == 'D') { /* digest off */
    sprintf(tmp, "%s/.qmail-%s-digest-return-default", RealDir, dotqmail_name);
    unlink (tmp);
    sprintf(tmp, "%s/.qmail-%s-digest-owner", RealDir, dotqmail_name);
    unlink (tmp);

    /* delete the digest directory */
    sprintf(tmp, "%s/%s/digest", RealDir, ActionUser);
    vdelfiles(tmp);
    chdir(RealDir);
  }

  /* Check for prefix setting */
  GetValue(TmpCGI, tmp, "prefix=", sizeof(tmp));
  
  /* strip leading '[' and trailing ']' from tmp */
  tmpstr = strchr (tmp, ']');
  if (tmpstr != NULL) *tmpstr = '\0';
  tmpstr = tmp;
  while (*tmpstr == '[') tmpstr++;

  /* Create (or delete) the file as appropriate */
  sprintf(Buffer4, "%s/%s/prefix", RealDir, ActionUser);
  if (strlen(tmp) > 0)
  {
    file=fopen(Buffer4 , "w");
    if (file)
    {
      fprintf(file, "[%s]", tmpstr);
      fclose(file);
    }
  }
  else
  {
    unlink (Buffer4);
  }

  /* set Reply-To header */
  GetValue (TmpCGI, Buffer4, "replyto=", sizeof(Buffer4));
  replyto = atoi(Buffer4);
  if (replyto == REPLYTO_SENDER) {
    /* ezmlm shouldn't remove/add Reply-To header */
    ezmlm_setreplyto ("headeradd", "");
    ezmlm_setreplyto ("headerremove", "");
  } else {
    if (replyto == REPLYTO_ADDRESS) {
      GetValue (TmpCGI, replyto_addr, "replyaddr=", sizeof(replyto_addr));
      sprintf (Buffer4, "Reply-To: %s\n", replyto_addr);
    } else {  /* REPLYTO_LIST */
      strcpy (Buffer4, "Reply-To: <#l#>@<#h#>\n");
    }
    ezmlm_setreplyto ("headeradd", Buffer4);
    ezmlm_setreplyto ("headerremove", "Reply-To");
  }

  /* update inlocal file */
  sprintf(Buffer4, "%s/%s/inlocal", RealDir, ActionUser);
  if (file=fopen(Buffer4, "w")) {
    fprintf(file, "%s-%s", Domain, ActionUser);
    fclose(file);
  }
}

int addmailinglistnow(void)
{
  if ( MaxMailingLists != -1 && CurMailingLists >= MaxMailingLists ) {
    fprintf(actout, "%s %d\n", get_html_text(184),
      MaxMailingLists);
    show_menu();
    return(184);
  }

  if ( check_local_user(ActionUser) ) {
    sprintf(StatusMessage, "%s %s\n", get_html_text(175), ActionUser);
    addmailinglist();
    return(175);
  }

  ezmlm_make(1);
  CurMailingLists++;

  sprintf(StatusMessage, "%s %s@%s\n", get_html_text(187),
          ActionUser, Domain);
  show_mailing_lists(Username, Domain, Mytime);
}

int show_list_group_now(int mod)
{
 char Buffer1[MAX_BUFF];
 char Buffer2[MAX_BUFF];
 char Buffer3[MAX_BUFF];
 int buff1;
 int buff2;
 int buff3;

  /* mod = 0 for subscribers, 1 for moderators, 2 for digest users */
  
 FILE *fs;
 int i,handles[2],pid,a,x,y,z = 0,z1=1,subuser_count = 0; 
 char buf[256];
 char *addr;

  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  lowerit(ActionUser);
  pipe(handles);

  pid=fork();
  if (pid==0) {
    close(handles[0]);
    dup2(handles[1],fileno(stdout));
    sprintf(Buffer1, "%s/ezmlm-list", EZMLMDIR);
    if(mod == 1) {
        sprintf(Buffer2, "%s/%s/mod", RealDir, ActionUser);
    } else if(mod == 2) {
        sprintf(Buffer2, "%s/%s/digest", RealDir, ActionUser);
    } else {
        sprintf(Buffer2, "%s/%s/", RealDir, ActionUser);
    }
    execl(Buffer1, "ezmlm-list", Buffer2, NULL);
    exit(127);
  } else {
    close(handles[1]);
    fs = fdopen(handles[0],"r");

    /* Load subscriber/moderator list */

    sort_init();
    while( (fgets(buf, sizeof(buf), fs)!= NULL)) {
      sort_add_entry (buf, '\n');   /* don't copy newline */
      subuser_count++;
    }

    sort_dosort();

    /* Display subscriber/moderator/digest list, along with delete button */
    if(mod == 1) {
        buff1 = 220;
        buff3 = 228; 
    } else if(mod == 2) {
        buff1 = 246;
        buff3 = 244;
    } else {
        buff3 = 230; 
        buff1 = 222;
    }
    buff2 = 072;
    fprintf(actout,"<TABLE border=0 width=\"100%%\">\n");
    fprintf(actout," <TR>\n");
    fprintf(actout,"  <TH align=left COLSPAN=4><B>%s</B> %d<BR><BR></TH>\n", get_html_text(buff3), subuser_count);
    fprintf(actout," </TR>\n");
    fprintf(actout," <TR align=center bgcolor=\"#cccccc\">\n");
    fprintf(actout,"  <TH align=center><b><font size=2>%s</font></b></TH>\n", get_html_text(buff2));
    fprintf(actout,"  <TH align=center><b><font size=2>%s</font></b></TH>\n", get_html_text(buff1));
    fprintf(actout,"  <TH align=center><b><font size=2>%s</font></b></TH>\n", get_html_text(buff2));
    fprintf(actout,"  <TH align=center><b><font size=2>%s</font></b></TH>\n", get_html_text(buff1));
    fprintf(actout," </TR>\n");

    if(mod == 1) {
        strcpy(Buffer3, "dellistmodnow");
    } else if(mod == 2) {
        strcpy(Buffer3, "dellistdignow");
    } else {
        strcpy(Buffer3, "dellistusernow");
    }
    for(z = 0; addr = sort_get_entry(z); ++z) {
      fprintf(actout," <TR align=center>");
      fprintf(actout,"  <TD align=right><A href=\"%s/com/%s?modu=%s&newu=%s&dom=%s&user=%s&time=%d\"><IMG src=\"%s/trash.png\" border=0></A></TD>\n",
        CGIPATH, Buffer3, ActionUser, addr, Domain, Username, Mytime, IMAGEURL);
      fprintf(actout,"  <TD align=left>%s</TD>\n", addr);
      ++z;
      if(addr = sort_get_entry(z)) {
        fprintf(actout,"  <TD align=right><A href=\"%s/com/%s?modu=%s&newu=%s&dom=%s&user=%s&time=%d\"><IMG src=\"%s/trash.png\" border=0></A></TD>\n",
          CGIPATH, Buffer3, ActionUser, addr, Domain, Username, Mytime, IMAGEURL);
      fprintf(actout,"  <TD align=left>%s</TD>\n", addr);
      } else {
        fprintf(actout,"  <TD COLSPAN=2> </TD>");
      }
      fprintf(actout," </TR>");
    }

    sort_cleanup();

    fprintf(actout,"</TABLE>");
    fclose(fs); close(handles[0]);
    wait(&pid);
    sprintf(StatusMessage, "%s\n", get_html_text(190));
    fprintf(actout, get_html_text(1));

  }
}

/*
int show_list_users_now(void) { return show_list_group_now(0); }
int show_list_moderators_now(void) { return show_list_group_now(1); }
int show_list_digest_users_now(void) { return show_list_group_now(2); }
*/

int show_list_group(char *template)
{
  if (AdminType != DOMAIN_ADMIN) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }
  
  if (MaxMailingLists == 0) {
    return 0;
  }
  
  send_template(template);
}

/*
int show_list_users(void) { return show_list_group("show_subscribers.html"); }
int show_list_digest_users(void) { return show_list_group("show_digest_subscribers.html"); }
int show_list_moderators(void) { return show_list_group("show_moderators.html"); }
*/

addlistgroup (char *template)
{
  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  send_template(template);
}

addlistuser() { addlistgroup( "add_listuser.html" ); }
addlistmod() { addlistgroup( "add_listmod.html" ); }
addlistdig() { addlistgroup( "add_listdig.html" ); }

addlistgroupnow (int mod)
{
char Buffer1[MAX_BUFF];
char Buffer2[MAX_BUFF];

  // mod = 0 for subscribers, 1 for moderators, 2 for digest subscribers

 int i, result;
 int pid;

  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  lowerit(ActionUser);

  if ( check_email_addr(Newu) ) {
    sprintf(StatusMessage, "%s %s\n", get_html_text(148), Newu);
    if (mod == 1) {
      addlistmod();
    } else if (mod == 2) {
      addlistdig();
    } else {
      addlistuser();
    }
    return(148);
  }

  pid=fork();
  if (pid==0) {
    sprintf(Buffer1, "%s/ezmlm-sub", EZMLMDIR);
    if(mod == 1) {
        sprintf(Buffer2, "%s/%s/mod", RealDir, ActionUser);
    } else if(mod == 2) {
        sprintf(Buffer2, "%s/%s/digest", RealDir, ActionUser);
    } else {
        sprintf(Buffer2, "%s/%s/", RealDir, ActionUser);
    }
    execl(Buffer1, "ezmlm-sub", Buffer2, Newu, NULL);
    exit(127);
  } else wait(&pid);

  if(mod == 1 ) {
    sprintf(StatusMessage, "%s %s %s@%s\n", Newu, 
        get_html_text(194), ActionUser, Domain);
    send_template( "add_listmod.html" );
  } else if(mod == 2) {
    sprintf(StatusMessage, "%s %s %s@%s\n", Newu, 
        get_html_text(240), ActionUser, Domain);
    send_template( "add_listdig.html" );
  } else {
    sprintf(StatusMessage, "%s %s %s@%s\n", Newu, 
        get_html_text(193), ActionUser, Domain);
    send_template( "add_listuser.html" );
  }
  return(999);
}

/*
addlistusernow() { addlistgroupnow(0); }
addlistmodnow() { addlistgroupnow(1); }
addlistdignow() { addlistgroupnow(2); }
*/

dellistgroup(char *template)
{
  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  send_template(template);
}

dellistuser() { dellistgroup ( "del_listuser.html" ); }
dellistmod() { dellistgroup ( "del_listmod.html" ); }
dellistdig() { dellistgroup ( "del_listdig.html" ); }


dellistgroupnow(int mod)
{
 int i;
 int pid;
 char Buffer1[MAX_BUFF];
 char Buffer2[MAX_BUFF];

  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  lowerit(Newu);

  pid=fork();
  if (pid==0) {
    sprintf(Buffer1, "%s/ezmlm-unsub", EZMLMDIR);
    if(mod == 1) {
        sprintf(Buffer2, "%s/%s/mod", RealDir, ActionUser);
    } else if(mod == 2 ) {
        sprintf(Buffer2, "%s/%s/digest", RealDir, ActionUser);
    } else {
        sprintf(Buffer2, "%s/%s/", RealDir, ActionUser);
    }
    execl(Buffer1, "ezmlm-unsub", Buffer2, Newu, NULL);
    exit(127);
  } else wait(&pid);

  if(mod == 1) {
    sprintf(StatusMessage, "%s %s %s@%s\n", Newu, get_html_text(197),
        ActionUser, Domain);
  } else if(mod == 2) {
    sprintf(StatusMessage, "%s %s %s@%s\n", Newu, get_html_text(242),
        ActionUser, Domain);
  } else {
    sprintf(StatusMessage, "%s %s %s@%s\n", Newu, get_html_text(203),
        ActionUser, Domain);
  }
  show_mailing_lists(Username, Domain, Mytime);
  return(999);
}

/*
dellistusernow() { dellistgroupnow(0); }
dellistmodnow() { dellistgroupnow(1); }
dellistdignow() { dellistgroupnow(2); }
*/

modmailinglist()
{
  /* name of list to modify is stored in ActionUser */

 int i;
 FILE *fs;
 char Buffer1[MAX_BUFF];
 char Buffer2[MAX_BUFF];

  if ( AdminType!=DOMAIN_ADMIN ) {
    sprintf(StatusMessage,"%s", get_html_text(142));
    return(142);
  }

  strcpy (Alias, "");  /* initialize Alias (list owner) to empty string */

  /* get the current listowner and copy it to Alias */
  strcpy (dotqmail_name, ActionUser);
  str_replace (dotqmail_name, '.', ':');
  sprintf(Buffer1, ".qmail-%s-owner", dotqmail_name);
  if((fs=fopen(Buffer1, "r"))!=NULL) {
    while(fgets(Buffer2, sizeof(Buffer2), fs)) {
      if(strstr(Buffer2, "@")!=NULL) {
        /* strip leading & if present */
        sprintf(Alias, "%s", (*Buffer2 == '&' ? (Buffer2 + 1) : Buffer2) );
        i = strlen(Alias); --i; Alias[i] = '\0'; /* strip newline */
      }
    }
    fclose(fs);
  }

  /* set default to "replies go to original sender" */
  replyto = REPLYTO_SENDER;  /* default */
  *replyto_addr = '\0';
  sprintf(Buffer1, "%s/headeradd", ActionUser);
  /* get the Reply-To setting for the list */
  if ((fs = fopen (Buffer1, "r")) != NULL) {
    while (fgets (Buffer2, sizeof(Buffer2), fs)) {
      if (strncasecmp ("Reply-To: ", Buffer2, 10) == 0) {
        i = strlen(Buffer2); --i; Buffer2[i] = '\0'; /* strip newline */
        if (strcmp ("<#l#>@<#h#>", Buffer2 + 10) == 0) {
          replyto = REPLYTO_LIST;
        } else {
          replyto = REPLYTO_ADDRESS;
          strcpy (replyto_addr, Buffer2 + 10);
        }
      }
    }
    fclose(fs);
  }

  /* read in options for the current list */
  set_options();

#ifdef EZMLMIDX
  send_template( "mod_mailinglist-idx.html" );
#else
  send_template( "show_mailinglists.html" );
#endif

}

modmailinglistnow()
{
  ezmlm_make(0);
  
  sprintf(StatusMessage, "%s %s@%s\n", get_html_text(226),
    ActionUser, Domain);
  show_mailing_lists(Username, Domain, Mytime);
}

build_option_str (char *type, char *param, char *options, char *description)
{
  int selected;
  char *optptr;
  
  selected = 1;
  for (optptr = options; *optptr; optptr++)
  {
    selected = selected && checkopt[*optptr];
  }
  /* selected is now true if all options for this radio button are true */

  fprintf(actout, "<INPUT TYPE=%s NAME=\"%s\" VALUE=\"%s\"%s> %s\n", 
    type, param, options, selected ? " CHECKED" : "", description);
}

int file_exists (char *filename)
{
  FILE *fs;
  if( (fs=fopen(filename, "r")) !=NULL ) {
    fclose(fs);
    return 1;
  } else {
    return 0;
  }
}

int get_ezmlmidx_line_arguments(char *line, char *program, char argument)
{
  char *begin; 
  char *end;
  char *arg;

  // does line contain program name?
  if ((strstr(line, program)) != NULL) {
    // find the options
    begin=strchr(line, ' ');
    begin++;
    if (*begin == '-') {
      end=strchr(begin, ' ');
      arg=strchr(begin, argument);
      // if arg is found && it's in the options (before the trailing space), return 1
      if (arg && (arg < end)) return 1;
    }       
  }       
  return 0;
}

void set_options() {
 char c;
 FILE *fs;
 char Buffer1[MAX_BUFF];
 char Buffer2[MAX_BUFF];

  /*
   * Note that with ezmlm-idx it might be possible to replace most
   * of this code by reading the config file in the list's directory.
   */

  /* make dotqmail name (ActionUser with '.' replaced by ':') */
  strcpy(dotqmail_name, ActionUser);
  for(dotnum=0;dotqmail_name[dotnum]!='\0';dotnum++) {
    if(dotqmail_name[dotnum]=='.') dotqmail_name[dotnum] = ':';
  }

  // default to false for lowercase letters
  for (c = 'a'; c <= 'z'; checkopt[c++] = 0);

  // figure out some options in the -default file
  sprintf(Buffer1, ".qmail-%s-default", dotqmail_name);
  if( (fs=fopen(Buffer1, "r")) !=NULL ) {
    while(fgets(Buffer2, sizeof(Buffer2), fs)) {
      if((get_ezmlmidx_line_arguments(Buffer2, "ezmlm-get", 'P')) > 0) {
        checkopt['b'] = 1;
      }
      if((get_ezmlmidx_line_arguments(Buffer2, "ezmlm-get", 's')) > 0) {
        checkopt['g'] = 1;
      }
      if((get_ezmlmidx_line_arguments(Buffer2, "ezmlm-manage", 'S')) > 0) {
        checkopt['h'] = 1;
      }
      if((get_ezmlmidx_line_arguments(Buffer2, "ezmlm-manage", 'U')) > 0) {
        checkopt['j'] = 1;
      }
      if((get_ezmlmidx_line_arguments(Buffer2, "ezmlm-manage", 'l')) > 0) {
        checkopt['l'] = 1;
      }
      if((get_ezmlmidx_line_arguments(Buffer2, "ezmlm-manage", 'e')) > 0) {
        checkopt['n'] = 1;
      }
      if((strstr(Buffer2, "ezmlm-request")) != 0) {
        checkopt['q'] = 1;
      }
    }
    fclose(fs);
  }

  // figure out some options in the -accept-default file
  sprintf(Buffer1, ".qmail-%s-accept-default", dotqmail_name);
  if( (fs=fopen(Buffer1, "r")) !=NULL ) {
    while(fgets(Buffer2, sizeof(Buffer2), fs)) {
      if(strstr(Buffer2, "ezmlm-archive") !=0) {
        checkopt['i'] = 1;
      }
    }
    fclose(fs);
  }

  // figure out some options in the qmail file
  sprintf(Buffer1, ".qmail-%s", dotqmail_name);
  if( (fs=fopen(Buffer1, "r")) !=NULL ) {
    while(fgets(Buffer2, sizeof(Buffer2), fs)) {
      if((get_ezmlmidx_line_arguments(Buffer2, "ezmlm-store", 'P')) > 0) {
        checkopt['o'] = 1;
      }
      if((strstr(Buffer2, "ezmlm-gate")) != 0 || (strstr(Buffer2, "ezmlm-issubn")) != 0) {
        checkopt['u'] = 1;
      }
      if(strstr(Buffer2, "ezmlm-archive") !=0) {
        checkopt['i'] = 1;
      }
    }
    fclose(fs);
  }

  sprintf(Buffer1, ".qmail-%s-accept-default", dotqmail_name);
  checkopt['m'] = file_exists(Buffer1);

  sprintf(Buffer1, "%s/archived", ActionUser);
  checkopt['a'] = file_exists(Buffer1);
  
  sprintf(Buffer1, "%s/digest/bouncer", ActionUser);
  checkopt['d'] = file_exists(Buffer1);
  
  sprintf(Buffer1, "%s/prefix", ActionUser);
  checkopt['f'] = file_exists(Buffer1);

  sprintf(Buffer1, "%s/public", ActionUser);
  checkopt['p'] = file_exists(Buffer1);
  
  sprintf(Buffer1, "%s/remote", ActionUser);
  checkopt['r'] = file_exists(Buffer1);
  
  sprintf(Buffer1, "%s/modsub", ActionUser);
  checkopt['s'] = file_exists(Buffer1);
  
  sprintf(Buffer1, "%s/text/trailer", ActionUser);
  checkopt['t'] = file_exists(Buffer1);
  
  /* update the uppercase option letters (just the opposite of the lowercase) */
  for (c = 'A'; c <= 'Z'; c++)
  {
    checkopt[c] = !checkopt[(c - 'A' + 'a')];
  }
}

void default_options() {
  char c;
  
  *dotqmail_name = '\0';
  replyto = REPLYTO_SENDER;
  *replyto_addr = '\0';

  /* These are currently set to defaults for a good, generic list.
   * Basically, make it safe/friendly and don't turn anything extra on.
   */
   
  /* for the options below, use 1 for "on" or "yes" */
  checkopt['a'] = 1; /* Archive */
  checkopt['b'] = 1; /* Moderator-only access to archive */
  checkopt['c'] = 0; /* ignored */
  checkopt['d'] = 0; /* Digest */
  checkopt['e'] = 0; /* ignored */
  checkopt['f'] = 1; /* Prefix */
  checkopt['g'] = 1; /* Guard Archive */
  checkopt['h'] = 0; /* Subscribe doesn't require conf */
  checkopt['i'] = 0; /* Indexed */
  checkopt['j'] = 0; /* Unsubscribe doesn't require conf */
  checkopt['k'] = 0; /* Create a blocked sender list */
  checkopt['l'] = 0; /* Remote admins can access subscriber list */
  checkopt['m'] = 0; /* Moderated */
  checkopt['n'] = 0; /* Remote admins can edit text files */
  checkopt['o'] = 0; /* Others rejected (for Moderated lists only */
  checkopt['p'] = 1; /* Public */
  checkopt['q'] = 1; /* Service listname-request */
  checkopt['r'] = 0; /* Remote Administration */
  checkopt['s'] = 0; /* Subscriptions are moderated */
  checkopt['t'] = 0; /* Add Trailer to outgoing messages */
  checkopt['u'] = 1; /* Only subscribers can post */
  checkopt['v'] = 0; /* ignored */
  checkopt['w'] = 0; /* special ezmlm-warn handling (ignored) */
  checkopt['x'] = 0; /* enable some extras (ignored) */
  checkopt['y'] = 0; /* ignored */
  checkopt['z'] = 0; /* ignored */

  /* update the uppercase option letters (just the opposite of the lowercase) */
  for (c = 'A'; c <= 'Z'; c++)
  {
    checkopt[c] = !checkopt[(c - 'A' + 'a')];
  }
}

show_current_list_values() {
 FILE *fs;
 int sqlfileok = 0;
 int usesql = 0;
 int i,j;
 char checked1[MAX_BUFF] = "";
 char listname[128];
 int checked;
 char Buffer1[MAX_BUFF];
 char Buffer2[MAX_BUFF];
 char Buffer3[MAX_BUFF];
 char Buffer4[MAX_BUFF];
   
  /* Note that we do not support the following list options:
   *   k - posts from addresses in listname/deny are rejected
   *   x - strip annoying MIME parts (spreadsheets, rtf, html, etc.)
   *   0 - make the list a sublist of another list
   *   3 - replace the From: header with another address
   *   4 - digest options (limits related to sending digest out)
   *   7, 8, 9 - break moderators up into message, subscription and admin
   */
  
  /* IMPORTANT: If you change the behavior of the checkboxes, you need
   * to update the default settings in modmailinglistnow so qmailadmin
   * will use the proper settings when a checkbox isn't checked.
   */
    
  if (*dotqmail_name) { /* modifying an existing list */
    strcpy (listname, dotqmail_name);
    str_replace (listname, ':', '.');
  } else {
    sprintf (listname, "<I>%s</I>", get_html_text(261));
  }

  /* Posting Messages */
  fprintf(actout, "<P><B><U>%s</U></B><BR>\n", get_html_text(262));
  build_option_str ("RADIO", "opt1", "MU", get_html_text(263));
  fprintf(actout, "<BR>\n");
  build_option_str ("RADIO", "opt1", "Mu", get_html_text(264));
  fprintf(actout, "<BR>\n");
  build_option_str ("RADIO", "opt1", "mu", get_html_text(265));
  fprintf(actout, "<BR>\n");
  build_option_str ("RADIO", "opt1", "mUo", get_html_text(266));
  fprintf(actout, "<BR>\n");
  build_option_str ("RADIO", "opt1", "mUO", get_html_text(267));
  fprintf(actout, "</P>\n");

  /* List Options */
  fprintf(actout, "<P><B><U>%s</U></B><BR>\n", get_html_text(268));
  /* this next option isn't necessary since we use the edit box to
   * set/delete the prefix
  sprintf (Buffer4, get_html_text(269), listname);
  build_option_str ("CHECKBOX", "opt3", "f", Buffer4);
  fprintf(actout, "<BR>\n");
  */
  fprintf(actout, "<TABLE><TR><TD ROWSPAN=3 VALIGN=TOP>%s</TD>",
    get_html_text(310));
  fprintf(actout, "<TD><INPUT TYPE=RADIO NAME=\"replyto\" VALUE=\"%d\"%s>%s</TD></TR>\n",
    REPLYTO_SENDER, (replyto == REPLYTO_SENDER) ? " CHECKED" : "", get_html_text(311));
  fprintf(actout, "<TR><TD><INPUT TYPE=RADIO NAME=\"replyto\" VALUE=\"%d\"%s>%s</TD></TR>\n",
    REPLYTO_LIST, (replyto == REPLYTO_LIST) ? " CHECKED" : "", get_html_text(312));
  fprintf(actout, "<TR><TD><INPUT TYPE=RADIO NAME=\"replyto\" VALUE=\"%d\"%s>%s ",
    REPLYTO_ADDRESS, (replyto == REPLYTO_ADDRESS) ? " CHECKED" : "", get_html_text(313));
  fprintf(actout, "<INPUT TYPE=TEXT NAME=\"replyaddr\" VALUE=\"%s\" SIZE=30></TD></TR>\n",
    replyto_addr);
  fprintf(actout, "</TABLE><BR>\n");
  build_option_str ("CHECKBOX", "opt4", "t", get_html_text(270));
  fprintf(actout, "<BR>\n");
  build_option_str ("CHECKBOX", "opt5", "d", get_html_text(271));
  sprintf (Buffer4, get_html_text(272), listname);
  fprintf(actout, "<SMALL>(%s)</SMALL>", Buffer4);
  fprintf(actout, "<BR>\n");
  sprintf (Buffer4, get_html_text(273), listname);
  build_option_str ("CHECKBOX", "opt6", "q", Buffer4);
  fprintf(actout, "<BR>\n");
  sprintf (Buffer4, get_html_text(274), listname, listname, listname);
  fprintf(actout, "&nbsp; &nbsp; <SMALL>(%s)</SMALL></P>", Buffer4);

  /* Remote Administration */
  fprintf(actout, "<P><B><U>%s</U></B><BR>\n", get_html_text(275));
  build_option_str ("CHECKBOX", "opt7", "r", get_html_text(276));
  fprintf(actout, "<BR>\n");
  build_option_str ("CHECKBOX", "opt8", "P", get_html_text(277));
  fprintf(actout, "<SMALL>(%s)</SMALL><BR>", get_html_text(278));
  fprintf(actout, "<TABLE><TR><TD ROWSPAN=2 VALIGN=TOP>%s</TD>",
    get_html_text(279));
  fprintf(actout, "<TD>");
  build_option_str ("CHECKBOX", "opt9", "l", get_html_text(280));
  fprintf(actout, "</TD>\n</TR><TR>\n<TD>");
  build_option_str ("CHECKBOX", "opt10", "n", get_html_text(281));
  fprintf(actout, "<SMALL>(%s)</SMALL>.</TD>\n", get_html_text(282));
  fprintf(actout, "</TR></TABLE>\n</P>\n");

  fprintf(actout, "<P><B><U>%s</U></B><BR>\n", get_html_text(283));
  fprintf(actout, "%s<BR>\n&nbsp; &nbsp; ", get_html_text(284));
  build_option_str ("CHECKBOX", "opt11", "H", get_html_text(285));
  fprintf(actout, "<BR>\n&nbsp; &nbsp; ");
  build_option_str ("CHECKBOX", "opt12", "s", get_html_text(286));
  fprintf(actout, "<BR>\n%s<BR>\n&nbsp; &nbsp; ", get_html_text(287));
  build_option_str ("CHECKBOX", "opt13", "J", get_html_text(285));
  fprintf(actout, "<BR>\n");
  fprintf(actout, "<SMALL>%s</SMALL>\n</P>\n", get_html_text(288));

  fprintf(actout, "<P><B><U>%s</U></B><BR>\n", get_html_text(289));
  build_option_str ("CHECKBOX", "opt14", "a", get_html_text(290));
  fprintf(actout, "<BR>\n");
  /* note that if user doesn't have ezmlm-cgi installed, it might be
     a good idea to default to having option i off. */
  build_option_str ("CHECKBOX", "opt15", "i", get_html_text(291));
  fprintf(actout, "<BR>\n%s\n<SELECT NAME=\"opt15\">", get_html_text(292));
  fprintf(actout, "<OPTION VALUE=\"BG\"%s>%s\n",
  	checkopt['B'] && checkopt['G'] ? " SELECTED" : "", get_html_text(293));
  fprintf(actout, "<OPTION VALUE=\"Bg\"%s>%s\n",
  	checkopt['B'] && checkopt['g'] ? " SELECTED" : "", get_html_text(294));
  fprintf(actout, "<OPTION VALUE=\"b\"%s>%s\n",
  	checkopt['b'] ? " SELECTED" : "", get_html_text(295));
  fprintf(actout, "</SELECT>.</P>\n");

  /***********************/
  /* begin MySQL options */
  /***********************/

  /* See if sql is turned on */
  checked = 0;
  sprintf(Buffer4, "%s/sql", ActionUser);
  if( (fs=fopen(Buffer4, "r")) !=NULL ) {
    checked = 1;
    while(fgets(Buffer2, sizeof(Buffer2), fs)) {
      strcpy(Buffer1, Buffer2);
      i = strlen(Buffer1); --i; Buffer1[i] = 0; /* take off newline */
      if((strstr(Buffer1, ":")) != NULL) { 
        sqlfileok = 1;
      }
    }
    usesql = 1;
    fclose(fs);
  }
#ifdef ENABLE_MYSQL
  fprintf(actout, "<P><B><U>%s</U></B><BR>\n", get_html_text(99));
  fprintf(actout, "<input type=checkbox name=\"sqlsupport\" value=\"-6\"%s> %s",
    checked ? " CHECKED" : "", get_html_text(53));

  /* parse dir/sql file for SQL settings */
  fprintf(actout, "    <table cellpadding=0 cellspacing=2 border=0>\n");
#else
  if (checked)
    fprintf(actout, "<INPUT TYPE=HIDDEN NAME=sqlsupport VALUE=\"-6\">\n");
#endif

  /* get hostname */
  strcpy(checked1, "localhost");
  if(usesql == 1 && sqlfileok == 1) {
    strncpy(Buffer3, Buffer1, 1);       
    if((strstr(Buffer3, ":")) == NULL) { 
      for(i=0,j=0;Buffer1[i]!=':'&&Buffer1[i]!='\0';++j,++i) checked1[j] = Buffer1[i];
      checked1[j] = '\0'; 
    }       
  }       

#ifdef ENABLE_MYSQL
  fprintf(actout, "      <tr>\n");
  fprintf(actout, "        <td ALIGN=RIGHT>%s:\n", get_html_text(54));
  fprintf(actout, "          </td><td>\n");
  fprintf(actout, "          <input type=text name=sql1 value=\"%s\"></td>\n", checked1);
#else
  fprintf(actout, "<INPUT TYPE=HIDDEN NAME=sql1 VALUE=\"%s\">\n", checked1);
#endif

  /* get port */
  strcpy(checked1, "3306");
  if(usesql == 1 && sqlfileok == 1) {
    strncpy(Buffer3, &Buffer1[++i], 1);       
    if((strstr(Buffer3, ":")) == NULL) { 
      for(j=0;Buffer1[i]!=':'&&Buffer1[i]!='\0';++j,++i) checked1[j] = Buffer1[i];
      checked1[j] = '\0'; 
    }       
  }       
#ifdef ENABLE_MYSQL
  fprintf(actout, "        <td ALIGN=RIGHT>%s:\n", get_html_text(055));
  fprintf(actout, "          </td><td>\n");
  fprintf(actout, "          <input type=text size=7 name=sql2 value=\"%s\"></td>\n", checked1);
  fprintf(actout, "      </tr>\n");
#else
  fprintf(actout, "<INPUT TYPE=HIDDEN NAME=sql2 VALUE=\"%s\">\n", checked1);
#endif

  /* get user */
  strcpy(checked1, "");
  if(usesql == 1 && sqlfileok == 1) {
    strncpy(Buffer3, &Buffer1[++i], 1);       
    if((strstr(Buffer3, ":")) == NULL) { 
      for(j=0;Buffer1[i]!=':'&&Buffer1[i]!='\0';++j,++i) checked1[j] = Buffer1[i];
      checked1[j] = '\0'; 
    }       
  }       
#ifdef ENABLE_MYSQL
  fprintf(actout, "      <tr>\n");
  fprintf(actout, "        <td ALIGN=RIGHT>%s:\n", get_html_text(56));
  fprintf(actout, "          </td><td>\n");
  fprintf(actout, "          <input type=text name=sql3 value=\"%s\"></td>\n", checked1);
#else
  fprintf(actout, "<INPUT TYPE=HIDDEN NAME=sql3 VALUE=\"%s\">\n", checked1);
#endif

  /* get password */
  strcpy(checked1, "");
  if(usesql == 1 && sqlfileok == 1) {
    strncpy(Buffer3, &Buffer1[++i], 1);
    if((strstr(Buffer3, ":")) == NULL) {
      for(j=0;Buffer1[i]!=':'&&Buffer1[i]!='\0';++j,++i) checked1[j] = Buffer1[i];
      checked1[j] = '\0';
    }
  }
#ifdef ENABLE_MYSQL
  fprintf(actout, "        <td ALIGN=RIGHT>%s:\n", get_html_text(57));
  fprintf(actout, "          </td><td>\n");
  fprintf(actout, "          <input type=text name=sql4 value=\"%s\"></td>\n", checked1);
  fprintf(actout, "      </tr>\n");
#else
  fprintf(actout, "<INPUT TYPE=HIDDEN NAME=sql4 VALUE=\"%s\">\n", checked1);
#endif

  /* get database name */
  strcpy(checked1, "");
  if(usesql == 1 && sqlfileok == 1) {
    strncpy(Buffer3, &Buffer1[++i], 1);       
    if((strstr(Buffer3, ":")) == NULL) { 
      for(j=0;Buffer1[i]!=':'&&Buffer1[i]!='\0';++j,++i) checked1[j] = Buffer1[i];
      checked1[j] = '\0'; 
    }       
  }       
#ifdef ENABLE_MYSQL
  fprintf(actout, "      <tr>\n");
  fprintf(actout, "        <td ALIGN=RIGHT>%s:\n", get_html_text(58));
  fprintf(actout, "          </td><td>\n");
  fprintf(actout, "          <input type=text name=sql5 value=\"%s\"></td>\n", checked1);
#else
  fprintf(actout, "<INPUT TYPE=HIDDEN NAME=sql5 VALUE=\"%s\">\n", checked1);
#endif

  /* get table name */
  strcpy(checked1, "ezmlm");
  if(usesql == 1 && sqlfileok == 1) {
    ++i;
    if(strlen(Buffer1) != i) {
      for(j=0;Buffer1[i]!=':'&&Buffer1[i]!='\0';++j,++i) checked1[j] = Buffer1[i];
      checked1[j] = '\0'; 
    }       
  }       
#ifdef ENABLE_MYSQL
  fprintf(actout, "        <td ALIGN=RIGHT>%s:\n", get_html_text(59));
  fprintf(actout, "          </td><td>\n");
  fprintf(actout, "          <input type=text name=\"sql6\" value=\"%s\"></td>\n", checked1);
  fprintf(actout, "      </tr>\n");
  fprintf(actout, "    </table>\n");
#else
  fprintf(actout, "<INPUT TYPE=HIDDEN NAME=sql6 VALUE=\"%s\">\n", checked1);
#endif

}

int get_mailinglist_prefix(char* prefix)
{
  char buffer[MAX_BUFF];
  char *b, *p;
  FILE* file;

  sprintf(buffer, "%s/%s/prefix", RealDir, ActionUser);
  file=fopen(buffer , "r");

  if (file) {
    fgets(buffer, sizeof(buffer), file);
    fclose(file);

    b = buffer;
    p = prefix;
    while (*b == '[') b++;
    while ((*b != ']') && (*b != '\n') && (*b != '\0')) *p++ = *b++;
    *p++ = '\0';
  }
  else
  {
    return 1;
  }
  return 0;
}

