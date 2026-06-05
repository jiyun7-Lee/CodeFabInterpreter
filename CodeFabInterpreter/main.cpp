#include "Shell.h"
#include "gmock/gmock.h"
#include <windows.h>

int main(int argc, char** argv)
{
#ifdef _DEBUG
    SetConsoleOutputCP(CP_UTF8);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#else
    SetConsoleOutputCP(CP_UTF8);
    Shell shell;
    shell.run();
    return 0;
#endif
}
