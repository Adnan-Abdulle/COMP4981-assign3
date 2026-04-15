#include <fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  DBM *db;
  datum key;
  datum value;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <key>\n", argv[0]);
    return EXIT_FAILURE;
  }

  db = dbm_open("test/postdata", O_RDONLY, 0644);
  if (db == NULL) {
    perror("dbm_open");
    return EXIT_FAILURE;
  }

  key.dptr = argv[1];
  key.dsize = (size_t)(short)strlen(argv[1]);

  value = dbm_fetch(db, key);

  if (value.dptr == NULL) {
    fprintf(stderr, "Key not found: %s\n", argv[1]);
    dbm_close(db);
    return EXIT_FAILURE;
  }

  fwrite(value.dptr, 1, value.dsize, stdout);
  printf("\n");

  dbm_close(db);
  return EXIT_SUCCESS;
}
