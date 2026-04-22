#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(){
  char buf[100];
  int fd;

  uint64 initial_val = getreadcount();
  printf("Initial readcount value: %ld\n", initial_val);      // Printing initial value

  if((fd = open("README", O_RDONLY)) < 0){
    fprintf(2, "readcount: cannot open README\n");
    exit(1);
  }

  read(fd, buf, 100);       // Read 100 bytes
  close(fd);        // Close file

  uint64 final_val = getreadcount();
  printf("Final readcount value: %ld\n", final_val);

  if(final_val == initial_val + 100){
    printf("Success: readcount value incremented by 100 as desired.\n");
  }
  else{
    printf("Failure: readcount value not incremented by 100.\n");
  }

  exit(0);
}
