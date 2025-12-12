#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include "chmod.h"

int main() {
    char mode1[] = "r-xr-xr-x";
    unsigned int permission1[9];
    printf("Letter to bit r-xr-xr-x: \t");
    letterToBit(mode1, permission1);
    for (int i = 0; i < 9; i++) printf("%d ", permission1[i]);
    printf("\n");

    char mode2[] = "555";
    unsigned int permission2[9];
    printf("Numeric to bit 555: \t\t");
    numericToBit(mode2, permission2);
    for (int i = 0; i < 9; i++) printf("%d ", permission2[i]);
    printf("\n\n");

    const char* filename = "temp_file.txt";
    struct stat file_stat;
    if (stat(filename, &file_stat) != 0) {
        perror("stat");
        return 1;
    }
    unsigned int mode = file_stat.st_mode & 0x1FF;
    printf("File %s mode: \t\t\t%04o\n\n", filename, mode);

    char permission3[10] = "---------";
    permission3[9] = '\0';
    printf("Letter permission %04o: \t", mode);
    letterPermissions(mode, permission3);
    printf("%s\n", permission3);

    unsigned int permission4[3];
    printf("Numeric permission %04o: \t", mode);
    numericPermissions(mode, permission4);
    for (int i = 0; i < 3; i++) printf("%d", permission4[i]);
    printf("\n");

    unsigned int permission5[9];
    printf("Bit permission %04o: \t\t", mode);
    bitPermissions(mode, permission5);
    for (int i = 0; i < 9; i++) printf("%d ", permission5[i]);
    printf("\n\n");

    unsigned int new_mode = badchmod("u+x", filename);
    printf("chmod u+x %s: \t\t\t%04o\n", filename, new_mode);
    printf("chmod 764 %s: \t\t\t%04o\n", filename, badchmod("764", filename));
    printf("\n");

    chmod(filename, badchmod("u-rwx", filename));

    char permission6[10] = "---------";
    permission6[9] = '\0';
    printf("Letter permission после изменений: \t");
    letterPermissions(new_mode, permission6);
    printf("%s\n", permission6);

    unsigned int permission7[3];
    printf("Numeric permission после изменений: \t");
    numericPermissions(new_mode, permission7);
    for (int i = 0; i < 3; i++) printf("%d", permission7[i]);
    printf("\n");

    unsigned int permission8[9];
    printf("Bit permission после изменений: \t");
    bitPermissions(new_mode, permission8);
    for (int i = 0; i < 9; i++) printf("%d ", permission8[i]);
    printf("\n\n");

    return 0;
}