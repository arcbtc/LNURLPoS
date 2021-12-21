#include "minunit.h"
#include "Bitcoin.h"

using namespace std;

#define MNEMONIC "arch volcano urge cradle turn labor skin secret squeeze denial jacket vintage fix glad lemon"
#define PASSWORD "my secret password"

MU_TEST(test_password) {
  HDPrivateKey hd(MNEMONIC, PASSWORD);
  mu_assert(bool(hd), "hd wallet should be valid");
  mu_assert(strcmp(hd.xprv().c_str(), "xprv9s21ZrQH143K3a5zf698hDA7tWk75bUs2aK5ZUzsSHPxk6MUv2NqUM8NwzFLKqeLeeaH3VGxTcLBgyE9vHYWVnY6JjkuCw9k4HpxHPnodhs") == 0, "Root xprv is invalid");
}

MU_TEST_SUITE(test_mnemonic) {
  MU_RUN_TEST(test_password);
}

int main(int argc, char *argv[]) {
  MU_RUN_SUITE(test_mnemonic);
  MU_REPORT();
  return MU_EXIT_CODE;
}
