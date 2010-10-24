/* Copyright 2010 Greg Olszewski, all rights reserved 
   gno@8kb.net
   650 castro st ste 120 box 215 mountain view,ca 94041

*/
#define _BSD_SOURCE
#include <yajl/yajl_gen.h>
#include <yajl/yajl_parse.h>
#include <assert.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include "config.h"
struct dent { 
  char dir[MAXPATHLEN];
  char nam[NAME_MAX];
} queue[JT_MAX_DEPTH];
int  e = 0;
int  d = 0;


static int enqueue (const char *dir, const char *subdir) {
  int size;
  if (e == JT_MAX_DEPTH) {
    e = 0;
  }
  memset(&queue[e],0,sizeof(struct dent));
  size = strlen(dir);
  if (subdir) { size += strlen(subdir); }
  if (size > MAXPATHLEN) {
    fprintf(stderr,"name too long for system\n");
    return 1;
  }
  strcpy(queue[e].dir,dir);
  if (subdir) {
    strcat(queue[e].dir,"/");
    strcat(queue[e].dir,subdir);
    strcpy(queue[e].nam,subdir);
  }
  e++;
  return 0;
}

static struct dent *dequeue(void) {
  if(e == d) { return NULL; }

  return &queue[d++];
}
static void yajl_print(void *ctx, const char *str, unsigned int len) {
  write(1,str,len);
}

static char buffer[MAX_STRING];
static int json_of_file(const char *dir, const char *file, yajl_gen yg) {
  char fn[MAXPATHLEN];
  int  fd, len;
  struct stat sb;
  yajl_parser_config pc = { 1, 1 };
  yajl_handle yh        = yajl_alloc(NULL,&pc,NULL,NULL);
  yajl_status ys;
  int is_literal        = 1;
  strcpy(fn,dir);
  strcat(fn,"/");
  strcat(fn,file);
  if (stat(fn, &sb) < 0) { 
    fprintf(stderr, "stat %s failed: %s\n",fn,strerror(errno));
    return 1;
  }

  fd = open(fn,O_RDONLY);
  if (fd == -1) { 
    fprintf(stderr, "open %s failed: %s\n",fn,strerror(errno));
    return 1;
  }
  yajl_gen_array_open(yg);
  while ((len = read(fd,buffer,MAX_STRING)) > 0) {
    ys =  yajl_parse(yh,buffer,len);
    if (ys != yajl_status_ok && ys != yajl_status_insufficient_data) {
      is_literal = 0; break;
    }
  }
  if (is_literal) {
    if (yajl_status_ok != yajl_parse_complete(yh)) {
      is_literal = 0;
    }
  }
  
  if (lseek(fd, 0, SEEK_SET) ) { 
    fprintf(stderr, "lseek 0 failed: %s\n",strerror(errno));
    return 1;
  }
  if (is_literal) { 
    yajl_gen_array_open(yg);
    yajl_gen_clear(yg);
    while ((len = read(fd,buffer,MAX_STRING)) > 0) {
      write(1,buffer,len); 
    }
    yajl_gen_array_close(yg);
  } else {
    if ((len = read(fd,buffer,MAX_STRING)) > 0) {
      yajl_gen_string(yg,buffer,len);
    }
  }
  close(fd);
  yajl_gen_array_close(yg);

  return 0;
}

int json_of_dir(const char *path, const char *subdir, yajl_gen yg) {
  DIR *d;
  struct dirent *de;
  char dir[MAXPATHLEN];

  strcpy(dir,path);
  if (subdir) {
    strcat(dir,"/");
    strcat(dir,subdir);
    yajl_gen_string(yg,subdir, strlen(subdir));

  }
  yajl_gen_map_open(yg);      
  d = opendir(dir);
  if (!d) { 
    fprintf(stderr,"opendir('%s') failed: %s", dir,strerror(errno));
    return 1;
  }
  while (de = readdir(d)) {
    if ((de->d_name[0] == '.' 
         && (de->d_name[1] == 0 || (de->d_name[1] == '.'
                                    && de->d_name[2] == '\0')))) {
      continue;
    }

    if (de->d_type == DT_DIR) {
      json_of_dir(dir,de->d_name,yg);
    } else if (de->d_type == DT_REG) {
      yajl_gen_string(yg,de->d_name,strlen(de->d_name));
      
      if (json_of_file(dir,de->d_name,yg)) { 
        fprintf(stderr,"file failed");return 1;
      }
    }
  }
  if (-1 == closedir(d)) {
    fprintf(stderr,"closedir(%s) failed %s\n",dir,strerror(errno));
    return 1;
  }
  yajl_gen_map_close(yg);      

  return 0;
}

int main (int ac, char **av) {
  yajl_gen_config ygc = { 1, "  " };
  return json_of_dir(realpath(av[1],NULL),NULL,yajl_gen_alloc2(yajl_print,&ygc,NULL,NULL));

}
