#include "nob.h"

int main(int argc, char *argv[])
{
    String_Builder sb = {0};
    sb_append_cstr(&sb, "Hello World");

    String_View sv = sb_to_sv(sb);
    printf(SV_Fmt "\n", SV_Arg(sv));

    return 0;
}
