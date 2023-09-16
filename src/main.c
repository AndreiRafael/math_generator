#include <stdio.h>
#include <stddef.h>

void create_mat(void);
void create_vec(void);

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    create_mat();
    create_vec();

    return 0;
}
