#define CLIOPTS_ENABLE_CXX
#include "cliopts.h"
using namespace cliopts;

static IntOption An_Integer('i', "int", 0, "A simple integer");
static UIntOption An_Unsigned('u', "unsigned", 0, "Unsigned value");
static StringOption A_String('s', "string", "", "String value");
static HexOption HexVal('h', "hex", 0, "Hex Value");
static BoolOption A_Boolean('v', "verbose", false, "Boolean value");
static BoolOption ALongOption(0, "long-option", false, "Long Option");
static BoolOption AShortOption('C', NULL, false, "Short Option");
static BoolOption Required('R', "required", false, "Required Option", NULL, true);
static BoolOption ComplexOption('X', "complicated-option", false, "Very long help");

int main(int argc, char **argv)
{
    Parser parser("Test App");

    parser.addOption(An_Integer);
    parser.addOption(An_Unsigned);
    parser.addOption(A_String);
    parser.addOption(HexVal);
    parser.addOption(A_Boolean);
    parser.addOption(ALongOption);
    parser.addOption(AShortOption);
    parser.addOption(Required);
    parser.addOption(ComplexOption);
    parser.parse(argc, argv);

    if (A_String.passed()) {
        printf("Have string %s\n", A_String.result().c_str());
    }

    if (An_Integer.passed()) {
        printf("Have integer %d\n", (int)An_Integer);
    }

    if (An_Unsigned) {
        printf("Have unsigned: %u\n", (unsigned)An_Unsigned);
    }
    if (HexVal) {
        printf("Have hex: %x\n", (int)HexVal);
    }
    printf("Boolean value: %d\n", (bool)A_Boolean);
    printf("Boolean value specified %d times\n", A_Boolean.numSpecified());

    return 0;
}
