#include "cliopts.h"
#include <stdlib.h>
#include <stdio.h>

int An_Integer = 0;
unsigned int An_Unsigned = 0;
char *A_String = NULL;
int A_Boolean = 0;
int HexVal = 0x0;
int ALongOption = -1;
int AShortOption;
int Required = 0;
int ComplicatedOption;
cliopts_list ValueList = { NULL };

cliopts_entry entries[] = {
        {'i', "int",        CLIOPTS_ARGT_INT, &An_Integer, "A simple integer", "INT" },
        {'u', "unsigned",   CLIOPTS_ARGT_UINT, &An_Unsigned, "An unsigned value"},
        {'s', "string",     CLIOPTS_ARGT_STRING, &A_String, "a string" },
        {'H', "hex",        CLIOPTS_ARGT_HEX, &HexVal, "Hexadecimal Value" },
        {'v', "verbose",    CLIOPTS_ARGT_NONE, &A_Boolean, "a boolean" },
        {0 , "long-option", CLIOPTS_ARGT_NONE, &ALongOption, "Long option" },
        {'C', NULL,         CLIOPTS_ARGT_NONE, &AShortOption, NULL },
        {'R', "required",   CLIOPTS_ARGT_NONE, &Required, "required", NULL, 1 },
        {'X', "complicated-option", CLIOPTS_ARGT_NONE, &ComplicatedOption,
                "This is a very long help text for a complicated option. "
                "You may ask why do we need such complicated options, well "
                "the answer may be, for example, that we need to test it; or"
        },
        {'D', "list",       CLIOPTS_ARGT_LIST, &ValueList, "OPTIONS" },
        { 0 }
};


int main(int argc, char **argv)
{
    int last_opt;
    cliopts_entry *cur;

    cliopts_parse_options(entries, argc, argv, &last_opt, NULL);

    /* Simple */

    if (A_String) {
        printf("Have string: %s\n", A_String);
    }
    if (An_Integer) {
        printf("Have integer %d\n", An_Integer);
    }
    if (An_Unsigned) {
        printf("Have unsigned: %u\n", An_Unsigned);
    }
    if (HexVal) {
        printf("Have hex: %x\n", HexVal);
    }

    /* If you feel masochistic, you can use a loop. This is necessary
     * if you want to know whether an option was specified on the command
     * line, or how many times it was specified
     */

    for (cur = entries; cur->dest; cur++) {
        printf("Option [%c,%s] found %d times\n",
               cur->kshort,
               cur->klong,
               cur->found);
    }

    printf("Boolean value: %d\n", A_Boolean);

    if (ValueList.nvalues) {
        size_t ii;
        printf("Have value list:\n");
        for (ii = 0; ii < ValueList.nvalues; ii++) {
            printf("  %s\n", ValueList.values[ii]);
        }
    }

    printf("Rest arguments begin at %d (%s) \n", last_opt, argv[last_opt]);
    cliopts_list_clear(&ValueList);
    return 0;

}
