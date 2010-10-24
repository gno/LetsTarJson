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

#include "tar.h"
typedef struct {
  char  dirstack[256][256];
  int   sp;
  int  literal;
  yajl_gen ylg;
} state;
static  yajl_gen_config ygc = { 1, "  " };

struct header_old_tar dir;
const char nulls[512] = { 0 };
void checksum_header(struct header_old_tar *ot) {
  unsigned char *ptr = (unsigned char*) ot;
  unsigned char *end = (unsigned char*) ot + sizeof(struct header_old_tar);
  unsigned int checksum = 0;

  memset(ot->checksum,' ',sizeof(ot->checksum));

  while (ptr < end) {
    checksum += *ptr++;
  }

  sprintf(ot->checksum,"%06o",(unsigned) (checksum & 07777777));
  ot->checksum[7] = ' ';

}
void emit_file(state *s, const char *contents,unsigned int len)  {

  int sptr = 0;
  int nullcnt = 512 - (len % 512); 
  memset(&dir,0,sizeof(struct header_old_tar));
  while (sptr <= s->sp) {
    strcat(dir.name,s->dirstack[sptr]);
    if (sptr != s->sp) strcat(dir.name,"/");
    sptr++;
  } 

  sprintf(dir.mode,"%07o",0777);
  sprintf(dir.size,"%011o",len);
  sprintf(dir.uid,"%07o",0);
  sprintf(dir.gid,"%07o",0);
  sprintf(dir.mtime,"%011o",1<<24);
  dir.mtime[11] = 0;
  dir.linkflag[0] = '0';
  checksum_header(&dir);
  write(1,&dir,sizeof(struct header_old_tar));
  if (len) {
    write(1,contents,len);
    write(1,nulls,nullcnt);
  }
}


void emit_dir (state *s) {
  int sptr = 0;

  memset(&dir,0,sizeof(struct header_old_tar));
  while (sptr <= s->sp) {
    strcat(dir.name,s->dirstack[sptr++]);
    strcat(dir.name,"/");
  } 
  sprintf(dir.mode,"%07o",0755);
  sprintf(dir.size,"%011o",0);
  sprintf(dir.uid,"%07o",0);
  sprintf(dir.gid,"%07o",0);
  sprintf(dir.mtime,"%011o",1<<24);
  checksum_header(&dir);
  write(1,&dir,sizeof(struct header_old_tar));

}
void file_done(state *s) {
  const unsigned char *buf;
  unsigned int len;
  yajl_gen_get_buf(s->ylg,&buf,&len);
  emit_file(s,buf,len);
  yajl_gen_free(s->ylg);
  s->ylg=NULL;
}

int replace_key (state *s, const unsigned char *name, int len) {
  char buf[100];
  if (len > 255) { return 1; }
  memcpy(s->dirstack[s->sp],name,len);
  s->dirstack[s->sp][len] = '\0';
  memcpy(buf,name,len);
  buf[len]=0;
  return 1;
}

int pop_directory(state *s) { 
  s->sp--;
}
static int jt_string(void *ctx, const unsigned char *str, unsigned int sl) {
  state *s = ctx;
  if (s->literal) {
    yajl_gen_string(s->ylg,str,sl);
  } else {
    emit_file(ctx,str,sl);
  }
  return 1;
}

static int bad_json (void *ctx) { return 1; }
static int jt_boolean(void *ctx, int bool) { 
  state *s = ctx;
  if (s->literal) {
    yajl_gen_bool(s->ylg,bool);
  } else {
    jt_string(ctx, bool ? "true" : "false", bool ? 4 : 5); 
  }
  return 1;
}
static int jt_number(void *ctx, char *nv, int nl) {
  return jt_string(ctx,nv,nl);     
}
static int jt_null(void *ctx) { 
  return 1;
}
                   
static int jt_start_map(void *ctx) {
  state *s = ctx;
  if (s->literal) { /* toplevel map is directory '.' */
    yajl_gen_map_open(s->ylg);
  } else {
    if (s->sp != -1) emit_dir(s);
    s->sp++;
  }
  return 1;
}

static int jt_start_array(void *ctx) {
  state *s    = ctx;
  if (s->literal) {
    yajl_gen_array_open(s->ylg);
  } else {
    s->ylg = yajl_gen_alloc(&ygc,NULL);
  }
  s->literal++;


  return 1;
}

static int jt_end_array(void *ctx) {
  state *s    = ctx;
  if (s->literal) { 
    s->literal--;
  }
  if (s->literal) {
    yajl_gen_array_close(s->ylg);
  } else {
    file_done(s);
  }
  return 1;
}

static int jt_map_key(void *ctx, const unsigned char *key, unsigned int sl) { 
  state *s = ctx;
  if (s->literal) {
    yajl_gen_string(s->ylg,key,sl);
  } else {
    replace_key(ctx,key,sl); 
  }
  return 1;
}

static int jt_end_map(void *ctx) {
  state *s = ctx;
  if (s->literal) {
    yajl_gen_map_close(s->ylg);
  } else {
    pop_directory(ctx);
  }
  return 1;
}




yajl_callbacks yc = {
  jt_null,
  jt_boolean,
  NULL,
  NULL,
  jt_string,
  jt_string,
  jt_start_map,
  jt_map_key,
  jt_end_map,
  jt_start_array,
  jt_end_array
};





int main(int ac, char **av) {
  state s = {};
  yajl_parser_config pc = { 1, 1 };
  yajl_handle yh        = yajl_alloc(&yc,&pc,NULL,&s);
  char buffer[8192];
  int len;

  s.sp = -1;

  while((len=read(0,buffer,8192)) > 0) {
    
    if (yajl_parse(yh,buffer,len) == yajl_status_error ) break;
  }
  if (len == -1) {
    fprintf(stderr,"read failed: %s\n",strerror(errno));
  }
  if (len != 0) {
    fprintf(stderr, "yajl returned error\n");
  }
  yajl_parse_complete(yh);

  write(1, nulls,512);
  write(1, nulls,512);
  return 0;
}
