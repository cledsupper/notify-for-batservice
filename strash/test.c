#include "s.h"

int main() {
  string_t * espaco = S(s_from(" "));
  string_t * strs = S(s_builderv(
    S(s_from("Uma string")), espaco, S(s_from("nova")), NULL
  ));
  printf("%s\n", strs->arr);

  S_tmp_free();

  return 0;
}
