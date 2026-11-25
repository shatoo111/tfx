#include <cstdio>

// Exchange two existing bundles on the same filesystem in a single system call.
// The caller can reverse the exchange if the new bundle fails to start.
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    if (renamex_np(argv[1], argv[2], RENAME_SWAP) == 0) return 0;
    std::perror("Could not exchange TFX bundles");
    return 1;
}
