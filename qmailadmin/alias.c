/* 
 * $Id: alias.c,v 1.4.2.11 2004-11-20 01:10:41 tomcollins Exp $
 * Copyright (C) 1999-2004 Inter7 Internet Technologies, Inc. 
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
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <dirent.h>
#include <vpopmail.h>
#include <vpopmail_config.h>
/* undef some macros that get redefined in config.h below */
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include <vauth.h>
#include "alias.h"
#include "config.h"
#include "forward.h"
#include "qmailadmin.h"
#include "qmailadminx.h"
#include "dotqmail.h"
#include "limits.h"
#include "util.h"
#include "printh.h"
#include "show.h"
#include "template.h"

char* dotqmail_alias_command(char* command);
int bkscandir(const char *dirname,
              struct dirent ***namelist,
            int (*select)(struct dirent *),
            int (*compar)(const void *, const void *));

int show_aliases(void)
{
  if ( AdminType!=DOMAIN_ADMIN ) {
    snprintf(StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }
  send_template("show_alias.html");
  return 0;
}

int qa_sort(const void * a, const void * b)
{
  return strcasecmp ((*(const struct dirent **) a)->d_name,
                     (*(const struct dirent **) b)->d_name);
}

struct aliasentry {
  char alias_name[MAX_FILE_NAME];
  char alias_command[MAX_BUFF];
  struct aliasentry *next;
};

struct aliasentry *firstalias = NULL, *curalias = NULL;

void add_alias_entry (char *alias_name, char *alias_command)
{
  if (firstalias == NULL) {
    firstalias = malloc (sizeof(struct aliasentry));
    curalias = firstalias;
  } else {
    curalias->next = malloc (sizeof (struct aliasentry));
    curalias = curalias->next;
  }
  curalias->next = NULL;
  strcpy (curalias->alias_name, alias_name);
  strcpy (curalias->alias_command, alias_command);
}
struct aliasentry *get_alias_entry()
{
  struct aliasentry *temp;
  
  temp = curalias->next;
  free (curalias);
  return temp;
}

void show_dotqmail_lines(char *user, char *dom, time_t mytime)
{
 int moreusers=0;
 DIR *mydir;
 struct dirent *mydirent;
 FILE *fs;
 char alias_user[MAX_BUFF];
 char alias_name[MAX_FILE_NAME];
 char *alias_domain;
 char *alias_name_from_command;
 int i,j,stop,k,startnumber;
 int m,n;
 int page;
 struct dirent **namelist;
 char this_alias[MAX_FILE_NAME];
#ifdef VALIAS
 char *alias_line;
#endif
 struct stat sbuf;

  if ( AdminType!=DOMAIN_ADMIN ) {
    snprintf(StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }

  page = atoi(Pagenumber);
  if (page == 0) page = 1;

  startnumber = MAXALIASESPERPAGE * (page - 1);
  k=0;

#ifdef VALIAS
  alias_line = valias_select_all(alias_name, Domain);
  while (alias_line != NULL) {
    strcpy (this_alias, alias_name);
    alias_name_from_command = dotqmail_alias_command(alias_line);
    if ( alias_name_from_command != NULL || *alias_line == '#') {
      k++;
      
      if (k > MAXALIASESPERPAGE + startnumber) {
        moreusers = 1;
        break;
      }
      if (k > startnumber) {
        if (*alias_line == '#') {
          alias_line = valias_select_all_next (alias_name);
          if (strcmp (this_alias, alias_name) != 0) {
            /* single comment, treat as blackhole */
            add_alias_entry (this_alias, "#");
            continue;
          } else {
            alias_name_from_command = dotqmail_alias_command(alias_line);
          }
        }
        while (1) {
          if (alias_name_from_command != NULL) {
            add_alias_entry (alias_name, alias_name_from_command);
          }
          alias_line = valias_select_all_next(alias_name);
          
          /* exit if we run out of alias lines, or go to a new alias name */
          if ((alias_line == NULL) || (strcmp (this_alias, alias_name) != 0)) break;
            
          alias_name_from_command = dotqmail_alias_command(alias_line);
        }
      }
    }
    /* burn through remaining lines for this alias, if necessary */
    while ((alias_line != NULL) && (strcmp (this_alias, alias_name) == 0)) {
      alias_line = valias_select_all_next(alias_name);
    }
  }
#else
  /* We can't use valias code here, because it doesn't return a sorted
     list of aliases.  If we update vpopmail's vpalias.c to do that,
     then qmailadmin could use the single set of valias_ functions above.
   */
  if ( (mydir = opendir(".")) == NULL ) {
    printf ("<tr><td colspan=\"4\">");
    printf ("%s %d</td></tr>", get_html_text("143"), 1);
    return;
  }

  n = bkscandir(".", &namelist, 0, qa_sort);
  
  for (m=0; m<n; m++) {
    mydirent=namelist[m];
    if ( strncmp(".qmail-", mydirent->d_name, 7) == 0 ) {
      if ( strcmp(mydirent->d_name, ".qmail-default") == 0 ) continue; 

      /* check for ezmlm lists (file is symbolic link) */
      memset (&sbuf, 0, sizeof(sbuf));
      if (lstat (mydirent->d_name, &sbuf) == 0) {
        if ((sbuf.st_mode & S_IFLNK) == S_IFLNK) continue;
      }

      if ( (fs=fopen(mydirent->d_name,"r"))==NULL) {
        printf ("<tr><td colspan=4>");
        printf ("%s %s", get_html_text("144"), mydirent->d_name);
        printf ("</td></tr>\n");
        continue;
      }

      for(i=7,j=0;j<MAX_FILE_NAME-1&&mydirent->d_name[i]!=0;++i,++j) {
        alias_name[j] = mydirent->d_name[i] == ':' ? '.' : mydirent->d_name[i];
      }
      alias_name[j] = 0;

      memset(TmpBuf2, 0, sizeof(TmpBuf2));
      fgets(TmpBuf2, sizeof(TmpBuf2), fs);
      alias_name_from_command = dotqmail_alias_command(TmpBuf2);

      if ( alias_name_from_command != NULL || *TmpBuf2 == '#') {
        k++;
      
        if ( k >MAXALIASESPERPAGE + startnumber) {
          moreusers=1;
          fclose(fs);
          break;
        }
        if (k <= startnumber) {
          fclose (fs);
          continue;
        }

        if (*TmpBuf2 == '#') {
          if (fgets(TmpBuf2, sizeof(TmpBuf2), fs) == NULL) {
            /* just a single comment, this is a blackhole account */
            add_alias_entry (alias_name, "#");
            fclose(fs);
            continue;
          } else {
            alias_name_from_command = dotqmail_alias_command(TmpBuf2);
          }
        }
        while (1) {
          if (alias_name_from_command != NULL) {
            add_alias_entry (alias_name, alias_name_from_command);
          }

          if (fgets(TmpBuf2, sizeof(TmpBuf2), fs) == NULL) break;
          alias_name_from_command = dotqmail_alias_command(TmpBuf2);
        }
      }
      fclose(fs);
    }
  }
  closedir(mydir);
  /* free memory allocated by bkscandir */
  for (m=0; m<n; m++) free(namelist[m]);
  free(namelist);
#endif

  curalias = firstalias;
  while (curalias != NULL) {
    strcpy (this_alias, curalias->alias_name);
    /* display the entry */
    
    /* We assume that if first char is '#', this is a blackhole.
     * This is a big assumption, and may cause problems at some point.
     */
    
    printf ("<tr>\n");
    qmail_button (this_alias, "deldotqmail", user, dom, mytime, "trash.png");
    if (*curalias->alias_command == '#')
      printf ("<td> </td>");   /* don't allow modify on blackhole */
    else
      qmail_button (this_alias, "moddotqmail", user, dom, mytime, "modify.png");
    printh ("<td align=left>%H</td>\n", this_alias);
    printf ("<td align=left>");
    
    stop=0;
    if (*curalias->alias_command == '#') {
      /* this is a blackhole account */
      printf ("<I>%s</I>", get_html_text("303"));
      stop = 1;
    }
    
    while (!stop) {
      strcpy (alias_user, curalias->alias_command);
      /* get the domain alone from alias_user */
      for(alias_domain = alias_user;
        *alias_domain != '\0' && *alias_domain != '@' && *alias_domain != ' ';
        alias_domain++);
        
      /* if a local user, strip domain name from address */
      if ((*alias_domain == '@') && (strcasecmp (alias_domain+1, Domain) == 0)) {
        /* strip domain name from address */
        *alias_domain = '\0';
  
        if (!check_local_user(alias_user)) {
          /* make it red so it jumps out -- this is no longer a valid forward */
          snprintf(alias_user, sizeof(alias_user), "<font color=\"red\">%s</font>", 
            curalias->alias_command);
        }
      }
      
      /* find next entry, so we know if we should print a , or not */
      while (1) {
        curalias = get_alias_entry();
        
        /* exit if we run out of alias lines, or go to a new alias name */
        if ((curalias == NULL) || (strcmp (this_alias, curalias->alias_name) != 0)) {
          stop = 1;
          printf ("%s", alias_user);
          break;
        }
        
        printf ("%s, ", alias_user);
        break;
      }
    }
    /* burn through any remaining entries */
    while ((curalias != NULL) && (strcmp (this_alias, curalias->alias_name) == 0)) {
      curalias = get_alias_entry();
    }
    printf ("</td>\n</tr>\n");
  }

  if (AdminType == DOMAIN_ADMIN) {
    printf ("<tr><td align=\"right\" colspan=\"4\">");
    printf ("[&nbsp;");
    if(page > 1 ) {
      printh ("<a href=\"%s/com/showforwards?user=%C&dom=%C&time=%d&page=%d\">%s</a>",
        CGIPATH,user,dom,mytime,page - 1,get_html_text("135"));
      printf ("&nbsp;|&nbsp;");
    }
    printh ("<a href=\"%s/com/showforwards?user=%C&dom=%C&time=%d&page=%d\">%s</a>",
      CGIPATH,user,dom,mytime,page,get_html_text("136"));
    printf ("&nbsp;|&nbsp;");
    if (moreusers) {
      printh ("<a href=\"%s/com/showforwards?user=%C&dom=%C&time=%d&page=%d\">%s</a>",
        CGIPATH,user,dom,mytime,page+1,get_html_text("137"));    
      printf ("&nbsp;]");
    }
    printf ("</td></tr>");                                    
  }
}

/* 
 * This Function shows the inside of a .qmail file, 
 * with the edit mode
 *
 */
void show_dotqmail_file(char *user) 
{
 char alias_user[MAX_BUFF];
 char *alias_domain;
 char *alias_name_from_command;
 char *alias_line;
 int j;

  if ( AdminType!=DOMAIN_ADMIN ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }
                
  printf ("<tr>");
  printh ("<td align=\"center\" valign=\"top\"><b>%H</b></td>", user);
    
  alias_line = valias_select (user, Domain);
  while (alias_line != NULL) {
    alias_name_from_command = dotqmail_alias_command(alias_line);
                        
    /* Make sure it is valid before displaying it. */
    if (alias_name_from_command != NULL )
      add_alias_entry (user, alias_line);

    alias_line = valias_select_next();
  }
  
  curalias = firstalias;
  while (curalias != NULL) {
    alias_line = curalias->alias_command;
    alias_name_from_command = dotqmail_alias_command (alias_line);
    strcpy (alias_user, alias_name_from_command);
    /* get the domain alone from alias_user */
    alias_domain = alias_user;
    for(;*alias_domain != '\0' && *alias_domain != '@'
      && *alias_domain != ' '; alias_domain++);
    alias_domain++;
    if(strcmp(alias_domain, Domain)==0) {
       /* if a local user, exclude the domain */
       strcpy(TmpBuf3, alias_user);
       for(j=0; TmpBuf3[j]!=0 && TmpBuf3[j]!='@';j++);
       TmpBuf3[j]=0;
       if (check_local_user(TmpBuf3)) {
          strcpy (alias_user, TmpBuf3);
       } else {
          /* make it red so it jumps out -- this is no longer a valid forward */
          snprintf (alias_user, sizeof(alias_user), "<font color=\"red\">%s</font>", 
                  alias_name_from_command);
       }
    }
    printf ("<td align=\"center\" valign=\"top\">%s</td>\n", alias_user);
    printf ("<td align=\"center\" valign=\"top\">\n");
    printf ("<form method=\"post\" name=\"moddotqmail\" action=\"%s/com/moddotqmailnow\">\n", CGIPATH);
    printh ("<input type=\"hidden\" name=\"user\" value=\"%H\">\n", Username);
    printh ("<input type=\"hidden\" name=\"dom\" value=\"%H\">\n", Domain);
    printf ("<input type=\"hidden\" name=\"time\" value=\"%u\">\n", (unsigned int) Mytime);
    printh ("<input type=\"hidden\" name=\"modu\" value=\"%H\">\n", user);
    printh ("<input type=\"hidden\" name=\"linedata\" value=\"%H\">\n", alias_line);
    printf ("<input type=\"hidden\" name=\"action\" value=\"delentry\">\n");
    printf ("<input type=\"image\" border=\"0\" src=\"%s/delete.png\">\n", IMAGEURL);
    printf ("</form>\n");


    printf ("</td>\n");
    printf ("</tr>\n");
    printf ("<tr>\n");
    printf ("<td align=\"left\">&nbsp;</td>\n");
    curalias = get_alias_entry();
  }
  /* finish up the last line (all empty) */
  printf ("<td align=\"left\">&nbsp;</td>");
  printf ("<td align=\"left\">&nbsp;</td>");
  printf ("</tr>");
}

int onevalidonly(char *user) {
 char *alias_line;
 int lines;

  lines=0;
  alias_line = valias_select (user, Domain);
  while( alias_line != NULL ) {
    /* check to see if it is an invalid line , if so skip to next */
    if (dotqmail_alias_command(alias_line) != NULL ) lines++;
    alias_line = valias_select_next();
  }

  return (lines < 2);
}

void moddotqmail()
{
  if ( AdminType!=DOMAIN_ADMIN ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }
  send_template("mod_dotqmail.html");
}

void moddotqmailnow() 
{
  if ( strcmp(ActionUser,"default")==0) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }

  if (strcmp(Action,"delentry")==0) {
    if (onevalidonly(ActionUser) ) {
      snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("149"));
      moddotqmail();
      vclose();
      exit(0);
    }
        
    if (dotqmail_del_line(ActionUser,LineData) ) {
      snprintf (StatusMessage, sizeof(StatusMessage), "%s %d\n", get_html_text("150"), 1);
      moddotqmail();
      vclose();
      exit(0);
    }
    snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("151") );
    moddotqmail();
    vclose();
    exit(0);
    
  } else if (strcmp(Action,"add")==0) {
    if( adddotqmail_shared(ActionUser, Newu, 0)) {
      moddotqmail();
      vclose();
      exit(0);
    } else {
      snprinth (StatusMessage, sizeof(StatusMessage), "%s %H\n", get_html_text("152"), Newu);
      moddotqmail();
      vclose();
      exit(0);
    }
  } else {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("155"));
    vclose();
    exit(0);
  }
}

void adddotqmail()
{
  count_forwards();
  load_limits();
  if ( MaxForwards != -1 && CurForwards >= MaxForwards ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s %d\n", 
      get_html_text("157"), MaxForwards);
    show_menu(Username, Domain, Mytime);
    vclose();
    exit(0);
  }
  send_template( "add_forward.html" );
}


void adddotqmailnow()
{
  if (AdminType!=DOMAIN_ADMIN && 
      !(AdminType==USER_ADMIN && strcmp(ActionUser, Username)==0)) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }

  count_forwards();
  load_limits();
  if ( MaxForwards != -1 && CurForwards >= MaxForwards ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s %d\n", get_html_text("157"), MaxForwards);
    send_template( "add_forward.html" );
    vclose();
    exit(0);
  }



  if (adddotqmail_shared(Alias, ActionUser, -1)) {
    adddotqmail();
    vclose();
    exit(0);
  } else {

    snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("152"));
    show_forwards(Username,Domain,Mytime);
  }
}

int adddotqmail_shared(char *forwardname, char *dest, int create) {
   /* adds line to .qmail for forwardname to dest             */
   /* (if create is 0, this is modifying an existing forward) */
   /* returns -1 if error orccured, 0 if successful           */
   /* fills StatusMessage if error occurs                     */
   /* jeff.hedlund@matrixsi.com                               */

  if (strlen(forwardname)<=0) {
    snprinth (StatusMessage, sizeof(StatusMessage), "%s %H\n", get_html_text("163"), forwardname);
    return(-1);
    
  /* make sure forwardname is valid */
  } else if (fixup_local_name(forwardname)) {
    snprinth (StatusMessage, sizeof(StatusMessage), "%s %H\n", get_html_text("163"), forwardname);
    return(-1);
    
  /* check to see if we already have a user with this name (only for create) */
  } else if (create != 0 && check_local_user(forwardname)) {
    snprinth (StatusMessage, sizeof(StatusMessage), "%s %H\n", get_html_text("175"), forwardname);
    return(-1);
  }

  if (strcmp (dest, "#") == 0) {
    if (dotqmail_add_line(forwardname, "#")) {
       snprintf (StatusMessage, sizeof(StatusMessage), "%s %d\n", get_html_text("150"), 2);
       return(-1);
    }
    return 0;
  }

  /* see if forwarding to a local user */
  if (strstr(dest, "@") == NULL) {
    if (check_local_user(dest) == 0) {
       snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("161"));
       return(-1);
    } else {
       /* make it an email address */
       sprintf (dest, "%s@%s", dest, Domain);
    }
  }
  
  /* check that it's a valid email address */
  if (check_email_addr(dest)) {
     snprinth (StatusMessage, sizeof(StatusMessage), "%s %H\n", get_html_text("162"), dest);
     return(-1);
  }

  snprintf (TmpBuf2, sizeof(TmpBuf2), "&%s", dest);
  if (dotqmail_add_line(forwardname, TmpBuf2)) {
     snprintf (StatusMessage, sizeof(StatusMessage), "%s %d\n", get_html_text("150"), 2);
     return(-1);
  }

  return(0);
}

void deldotqmail()
{

  if ( AdminType!=DOMAIN_ADMIN ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }
  send_template( "del_forward_confirm.html" );

}

void deldotqmailnow()
{

  if (AdminType!=DOMAIN_ADMIN && 
       !(AdminType==USER_ADMIN && !strcmp(ActionUser, Username))) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    show_menu(Username, Domain, Mytime);
    vclose();
    exit(0);
  }


  /* check to see if we already have a user with this name */
  if (fixup_local_name(ActionUser)) {
    snprinth (StatusMessage, sizeof(StatusMessage), "%s %H\n", get_html_text("160"), Alias);
    deldotqmail();
    vclose();
    exit(0);
  }

  if (!(dotqmail_delete_files(ActionUser))) {
    snprinth (StatusMessage, sizeof(StatusMessage), "%s %H %H\n", get_html_text("167"), 
      Alias, ActionUser);
  } else {
    snprinth (StatusMessage, sizeof(StatusMessage), "%s %H %H\n", get_html_text("168"), 
      Alias, ActionUser);
  }

  /* don't display aliases/forwards if we just deleted the last one */
  count_forwards();
  if(CurForwards == 0 && CurBlackholes == 0) {
    show_menu(Username, Domain, Mytime);
  } else {
    show_forwards(Username,Domain,Mytime);
  }
}

char* dotqmail_alias_command(char* line)
{
 int len;
 static char user[501];
 static char command[MAX_BUFF];
 char *s;
 char *b;

  if (line == NULL) return NULL;   /* null pointer */
  if (*line == 0) return NULL;     /* blank line */
  if (*line == '#') return NULL;   /* comment */

  /* copy everything up to the first whitespace */
  for (len = 0; line[len] != 0 && isspace(line[len]) == 0; len++ ) {
    command[len] = line[len];
  }
  command[len] = 0;

  /* If it ends with a slash and starts with a / or . then
   * this is a Maildir delivery, local alias
   */
  if( (command[len - 1]=='/') && ((command[0] =='/') || (command[0] =='.')) ) { 
    strcpy(user, command); user[len - 1] = 0;
    b = NULL;  /* pointer to mailbox name */

    if ((s = strrchr(user, '/')) == NULL) return NULL;
    if (strcmp(s, "/" MAILDIR) != 0) {
      b = s + 2;  /* point to name of submailbox */
      *s = '\0';  /* add NULL */
      if ((s = strrchr(user, '/')) == NULL) return NULL;
      if (strcmp(s, "/" MAILDIR) != 0) return NULL;
    }

    *s = '\0';
    if ((s = strrchr(user, '/')) == NULL) return NULL;
    if (b != NULL) { snprinth (user, sizeof(user), "%H <I>(%H)</I>", s+1, b); }
    else { snprinth (user, sizeof(user), "%H", s+1); }

    return (user);
        
  /* if it's an email address then display the forward */
  } else if ( !check_email_addr( (command+1) ) ){
    if (*command == '&' ) return (&command[1]);
    else return (command);

  /* if it is a program then */
  } else if ( command[0] == '|' ) {

    /* do not display ezmlm programs */
    if ( strstr(command, "ezmlm" ) != 0 ) return(NULL);

    /* do not display autorespond programs */
    if ( strstr(command, "autorespond" ) != 0 ) return(NULL);

    /* otherwise, display the program */

    /* back up to pipe or first slash to remove path */
    while (line[len] != '/' && line[len] != '|') len--;
    len++;   /* len is now first char of program name */
    snprinth (command, sizeof(command), "<I>%H</I>", &line[len]);
    return(command);

  } else {

    /* otherwise just report nothing */
    return(NULL);
  }
}

/*
 * Brian Kolaci
 * quick implementation of the scandir() BSD function
 */
int bkscandir(const char *dirname,
              struct dirent ***namelist,
            int (*select)(struct dirent *),
            int (*compar)(const void *, const void *))
{
  int i;
  int entries;
  int esize;
  struct dirent* dp;
  struct dirent* dent;
  DIR * dirp;

  *namelist = NULL;
  entries = esize = 0;

  /* load the names */
  if ((dirp = opendir(dirname)) == NULL)
    return -1;

  while ((dp = readdir(dirp)) != NULL) {
    if (select == NULL || (*select)(dp)) {
      if (entries >= esize) {
        void* mem;
        esize += 10;
        if ((mem = realloc(*namelist, esize * sizeof(struct dirent*))) == NULL) {
          for (i = 0; i < entries; i++)
            free((*namelist)[i]);
          free(*namelist);
          closedir(dirp);
          return -1;
        }
        *namelist = (struct dirent**)mem;
      }
      if ((dent = (struct dirent*)malloc(sizeof(struct dirent)+MAX_FILE_NAME)) == NULL) {
        for (i = 0; i < entries; i++)
          free((*namelist)[i]);
        free(*namelist);
        closedir(dirp);
        return -1;
      }
      memcpy(dent, dp, sizeof(*dp)+MAX_FILE_NAME);
      (*namelist)[entries] = dent;
      entries++;
    }
  }
  closedir(dirp);

  /* sort them */
  if (compar)
    qsort((void*)*namelist, entries, sizeof(struct dirent*), compar);
  return entries;
}
