/* http://people.freebsd.org/~kientzle/libarchive/man/tar.5.txt */
struct header_old_tar {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char linkflag[1];
  char linkname[100];
  char pad[255];
};

