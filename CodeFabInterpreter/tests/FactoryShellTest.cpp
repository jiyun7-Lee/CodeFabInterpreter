#include <gtest/gtest.h>
#include "../Shell.h"

class FactoryShellTest : public ::testing::Test
{
protected:
    FactoryShell shell;
};

// TC-FS-01: 인자 없음 → REPL 모드
TEST_F(FactoryShellTest, TC_FS_01_NoArgs_ReturnsReplMode)
{
    int argc = 1;
    char* argv[] = { (char*)"factory", nullptr };
    ASSERT_EQ(shell.detectMode(argc, argv), ShellMode::REPL);
}

// TC-FS-02: run 명령 → File 모드
TEST_F(FactoryShellTest, TC_FS_02_RunCommand_ReturnsFileMode)
{
    int argc = 2;
    char* argv[] = { (char*)"factory", (char*)"run", nullptr };
    ASSERT_EQ(shell.detectMode(argc, argv), ShellMode::FILE);
}

// TC-FS-03: debug 명령 → Debug 모드
TEST_F(FactoryShellTest, TC_FS_03_DebugCommand_ReturnsDebugMode)
{
    int argc = 2;
    char* argv[] = { (char*)"factory", (char*)"debug", nullptr };
    ASSERT_EQ(shell.detectMode(argc, argv), ShellMode::DEBUG);
}
