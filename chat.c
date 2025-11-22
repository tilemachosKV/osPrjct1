#include <stdio.h>
#include <string.h>

// checks whether the user input is correct
int check_input(int argc, char* argv[], const char *params[]) {
  if (argc != 4) return 0;
  if (strcmp(argv[1], params[0]) != 0 && strcmp(argv[1], params[1]) != 0) return 0;
  if (strcmp(argv[2], params[2]) != 0) return 0;

  return 1;
}

void init(int argc, char* argv[]) {
  printf("init\n");
}

void attach(int argc, char* argv[]) {
  printf("attach\n");
}

int main(int argc, char* argv[]) {
  const char *params[] = {"-i", "-a", "-k"};

  if (!check_input(argc, argv, params)) {
    printf("WRONG INPUT\nTry the bellow:\n./chat -i -k your_key   OR   ./chat -a -k your_key\n");
    printf("Exiting...\n");
    return -1;
  }

  if (strcmp(argv[1], params[0]) == 0) {
    init(argc, argv);
  }else {
    attach(argc, argv);
  }


  return 0;
}
