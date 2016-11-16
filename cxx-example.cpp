#define CLIOPTS_ENABLE_CXX
#include "cliopts.h"
using namespace cliopts;

static IntOption An_Integer = IntOption("int")
        .setDefault(0)
        .abbrev('i')
        .description("Signed integer value");

static UIntOption An_Unsigned = UIntOption("unsigned")
        .setDefault(0)
        .abbrev('u')
        .description("Unsigned integer value");

static ULongLongOption A_LongLong = ULongLongOption("longlong-option")
        .abbrev('U')
        .description("Long long integer option!");

static StringOption A_String = StringOption("string")
        .setDefault("")
        .abbrev('s')
        .description("String option");

static HexOption HexVal = HexOption("hex")
        .setDefault(0)
        .abbrev('h')
        .description("Hex value");

static BoolOption A_Boolean = BoolOption("verbose")
        .abbrev('v')
        .setDefault(false)
        .description("Boolean value");

static BoolOption Required = BoolOption("required")
        .abbrev('R')
        .mandatory().
        description("Required Option");

static BoolOption ComplexOption = BoolOption("complicated-option")
        .abbrev('X')
        .description("This is a very very very long line. This option means "
            "nothing, but still should demonstrate text wrapping abilities");

static ListOption MultiOption = ListOption("list-option")
        .abbrev('D')
        .description("Long list option...");

int main(int argc, char **argv)
{
    Parser parser("Test App");

    parser.addOption(An_Integer);
    parser.addOption(An_Unsigned);
    parser.addOption(A_LongLong);
    parser.addOption(A_String);
    parser.addOption(HexVal);
    parser.addOption(A_Boolean);
    parser.addOption(Required);
    parser.addOption(ComplexOption);
    parser.addOption(MultiOption);
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
    if (A_LongLong) {
        printf("Have unsigned long long: %llu\n", (unsigned long long)A_LongLong);
    }
    if (HexVal) {
        printf("Have hex: %x\n", (int)HexVal);
    }
    printf("Boolean value: %d\n", (bool)A_Boolean);
    printf("Boolean value specified %d times\n", A_Boolean.numSpecified());

    const std::vector<std::string>& kv = MultiOption.const_result();
    printf("Multi Option:\n");
    for (size_t ii = 0; ii < kv.size(); ii++) {
        printf("  Multi: %s\n", kv[ii].c_str());
    }

    return 0;
}
