#include "fs.h"
#include <klib.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
void segment_write(void *dest, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);
size_t fbsync_write(const void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin", 0, 0, 0, invalid_read, invalid_write},
  {"stdout", 0, 0, 0, invalid_read, serial_write},
  {"stderr", 0, 0, 0, invalid_read, serial_write},
  {"/dev/events", 0, 0, 0, events_read, invalid_write},
  {"/dev/fb", 0, 0, 0, invalid_read, fb_write},
  {"/dev/fdsync", 0, 0, 0, invalid_read, fbsync_write},
  {"/proc/dispinfo", 128, 0, 0, dispinfo_read, invalid_write},
  {"/dev/tty", 0, 0, 0, invalid_read, serial_write},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  file_table[4].size = screen_width() * screen_height() * 4;
  // TODO: initialize the size of /dev/fb
  Log("screen_width: %d, screen_height: %d\n", screen_width, screen_width);
}

int fs_close(int fd){
  file_table[fd].open_offset = 0;
  return 0;
}

int fs_open(const char *pathname, int flags, int mode){
  for (int i=0; i<NR_FILES; i ++){
    if (strcmp(pathname, file_table[i].name) == 0){
      file_table[i].open_offset = 0;
      return i;
    }
  }
  panic("pathname: %s -- File not found.\n", pathname);
  return -1;
}

size_t fs_filesz(int fd){
  if (fd <= NR_FILES)
    return file_table[fd].size;
  else
    panic("No such file in file_table.\n");
  
}

size_t fs_read(int fd, void *buf, size_t len){
  size_t free_size = file_table[fd].size - file_table[fd].open_offset;
  if (free_size < len) len = free_size;

  if (file_table[fd].read == NULL){
    // printf("file_table[%d].read == NULL\n", fd);
    // printf("len: %d, file_table[%d].size: %d, file_table[fd].read_offset: %d.\n", len, fd, file_table[fd].size, file_table[fd].read_offset);
    // printf("file_table[%d].disk_offset: %d.\n", fd, file_table[fd].disk_offset);
    ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
    // printf("have done ramdisk_read.\n");
    file_table[fd].open_offset += len;
    return len;
  }
  else{
    file_table[fd].open_offset += len;
    size_t num = file_table[fd].read(buf, file_table[fd].open_offset-len, len);
    return num;
  }
}

size_t fs_write(int fd, const void *buf, size_t len){
  // /*
  printf("begin write\n");
  if (fd==1 || fd==2){
    printf("fd==1 || fd==2\n");
    char *addr = (char *)(buf);
    uintptr_t count = len;
    while (count){
      _putc(*addr);
      addr ++;
      count --;
    }
    return len;
  }
  else{
    size_t free_size = file_table[fd].size - file_table[fd].open_offset;
    if(free_size < len) len = free_size;

    if (file_table[fd].write != NULL)
      len = file_table[fd].write(buf, file_table[fd].disk_offset+file_table[fd].open_offset, len);
    else
      len = ramdisk_write(buf, file_table[fd].disk_offset+file_table[fd].open_offset, len);

    file_table[fd].open_offset += len;
    return len;
  }
  // */
 /*
  //printf("begin write\n");
  size_t free_size = fs_filesz(fd) - file_table[fd].open_offset;
  if(free_size < len) len = free_size;

  if(file_table[fd].write == NULL) {
    	//size_t free_size = fs_filesz(fd) - file_table[fd].open_offset;
	    //if(free_size < len) len = free_size;
	  ramdisk_write(buf,file_table[fd].disk_offset+file_table[fd].open_offset,len);
	    //printf("write file\n");
	  file_table[fd].open_offset += len;
	  return len;
  }
  else{
   	size_t num = file_table[fd].write(buf,file_table[fd].open_offset+file_table[fd].disk_offset,len);
        //printf("common write file\n");
	  file_table[fd].open_offset += len;
	  return num;
  }
  */
}

size_t fs_lseek(int fd, size_t offset, int whence){
  switch (whence) {
    case SEEK_SET:
      file_table[fd].open_offset = offset;
      break;
    case SEEK_CUR:
      file_table[fd].open_offset += offset;
      break;
    case SEEK_END:
      file_table[fd].open_offset = file_table[fd].size;
      break;  
    default:
      panic("Invalid whence.\n");
  }
  return file_table[fd].open_offset;
}