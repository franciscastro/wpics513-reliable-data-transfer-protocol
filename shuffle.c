#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

time_t t;
 
static int rand_int(int n) {
  int limit = RAND_MAX - RAND_MAX % n;
  int rnd;

  do {
    rnd = rand();
  } while (rnd >= limit);
  return rnd % n;
}

void shuffle(char *sentence, int n) {
  int i, j, tmp;

  for (i = n - 1; i > 0; i--) {
    j = rand_int(i + 1);
    tmp = sentence[j];
    sentence[j] = sentence[i];
    sentence[i] = tmp;
  }

  printf("%s\n", sentence);
}

int main() {

	srand((unsigned) time(&t));

	char sentence[80];

	fgets(sentence, 80, stdin);
	sentence[strlen( sentence ) - 1] = '\0';
	
	shuffle(sentence, strlen(sentence));

	return 0;
}