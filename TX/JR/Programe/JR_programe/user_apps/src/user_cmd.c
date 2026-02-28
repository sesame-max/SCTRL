#include "user_cmd.h"
#include "maz_cpnt_debugcmd.h"
#include "sLog.h"

int testCmdCallBack(void *cmd, char *param);

MAZCPNT_MAIN_CMD testCmd = {
    "test",
    "this is test cmd\r\n",
    testCmdCallBack,
    NULL};

int testCmdCallBack(void *cmd, char *param)
{
    sLog_print("test secuss\r\n");
    return 0;
}

void user_cmd_init(void)
{
    MAZ_CPNT_debugcmd_init();
    MAZ_CPNT_debugcmd_mcmd_register(&testCmd);
}