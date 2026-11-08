/* =============================================
 * test.c  –  covers all Source2HTML token types
 * ============================================= */

#include <stdio.h>
#include <stdlib.h>
#include "my_header.h"

#define MAX_SIZE   100
#define PI         3.14159

// single-line comment

/* multi-line
   comment block */

typedef struct {
    int   id;
    float score;
    char  grade;
} Student;

int main(void)
{
    /* ---- data types (reserved_key1) ---- */
    int    count  = 0;
    float  value  = 3.14;
    double ratio  = 0xFF;      /* hex constant  */
    char   letter = 'A';       /* ASCII char    */
    char   newline = '\n';     /* escaped char  */
    void  *ptr    = NULL;

    /* ---- string literals ---- */
    char *msg = "Hello, World!\n";
    char *esc = "She said \"hi\"";

    /* ---- control flow (reserved_key2) ---- */
    if (count > 0) {
        for (int i = 0; i < MAX_SIZE; i++) {
            while (value > 0) {
                value--;
            }
        }
    } else {
        switch (count) {
            case 0:
                return 0;
            default:
                break;
        }
    }

    do {
        count++;
    } while (count < 10);

    return 0;
}
