#include "Shell.h"
#include "gmock/gmock.h"

int main(int argc, char** argv)
{
#if DEBUG
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#else
    Shell shell;
    shell.run();
    return 0;
#endif
}
