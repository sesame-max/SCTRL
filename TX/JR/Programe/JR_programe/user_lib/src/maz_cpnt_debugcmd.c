/*
 * maz_cpnt_debugcmd.c
 *
 *  Created on: 2020年2月24日
 *      Author: wangbing
 *      Email : mz8023yt@163.com
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "maz_cpnt_debugcmd.h"
#include <string.h>
#include "sLog.h"

    /**
     * "dlvl" variables must be defined if debug message is to be printed through the dlog/dmsg, and "dlvl" must be declared static
     */

    /* 内部函数声明 */
    int MAZ_CPNT_debugcmd_main_help_handler(void *cmd, char *param);
    int MAZ_CPNT_debugcmd_sub_help_handler(void *cmd, char *param);
    int MAZ_CPNT_debugcmd_mcmd_find(const char *name, MAZCPNT_MAIN_CMD **cmd);
    int MAZ_CPNT_debugcmd_mcmd_insert(MAZCPNT_MAIN_CMD *mcmd);
    int MAZ_CPNT_debugcmd_scmd_find(MAZCPNT_MAIN_CMD *mcmd, const char *sname, MAZCPNT_SUB_CMD **scmd);
    int MAZ_CPNT_debugcmd_scmd_insert(MAZCPNT_MAIN_CMD *mcmd, MAZCPNT_SUB_CMD *scmd);

    /* 字符串支持函数 */
    int string_find_prefix(const char *s1, const char *s2, int *length);
    int string_find_spaces(char *str, int *length);

/**
 * @brief 获取链表成员
 */
#define MAZCPNT_TO_MAIN_CMD(x) container_of((x), MAZCPNT_MAIN_CMD, list)
#define MAZCPNT_TO_SUB_CMD(x) container_of((x), MAZCPNT_SUB_CMD, list)

    /**
     * @brief 调试命令链表头对象
     */
    static MAZCPNT_MAIN_CMD g_mazcpnt_cmd =
        {
            .name = MAZCPNT_CMD_HELP_COMMAND,
            .desc = "list all the main commands.",
            .handler = MAZ_CPNT_debugcmd_main_help_handler,
    };

    /**
     * @brief 调试命令框架初始化
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_init(void)
    {
        MAZ_CPNT_list_init(&g_mazcpnt_cmd.list);
        return 0;
    }

    /**
     * @brief 注册主命令
     * @param mcmd 主命令
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_mcmd_register(MAZCPNT_MAIN_CMD *mcmd)
    {
        int ret = MAZRET_OK;
        MAZCPNT_MAIN_CMD *tmp;

        /* 检查参数有效性 */
        MAZASSERT_RETVAL(NULL == mcmd, MAZRET_EINVAL, "err: the param mcmd is NULL.");

        /* 判断是否有同名的主命令, 已经有同名称的主命令则直接退出函数并报错 */
        ret = MAZ_CPNT_debugcmd_mcmd_find(mcmd->name, &tmp);
        MAZASSERT_RETVAL(MAZRET_ENOCMD != ret, MAZRET_EEXIST, "err: the command \"%s\" already exists.", mcmd->name);

        /* 添加新的主命令到主链表中 */
        MAZ_CPNT_list_init(&mcmd->list);
        MAZ_CPNT_debugcmd_mcmd_insert(mcmd);

        /* 初始化默认子命令(帮助命令) */
        mcmd->cmds.name = MAZCPNT_CMD_HELP_COMMAND;
        mcmd->cmds.desc = "list all the sub commands.";
        mcmd->cmds.handler = MAZ_CPNT_debugcmd_sub_help_handler;
        MAZ_CPNT_list_init(&mcmd->cmds.list);

        return MAZRET_OK;
    }

    /**
     * @brief 注册子命令
     * @param mcmd 指定在哪个主命令下注册子命令
     * @param scmd 子命令
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_scmd_register(MAZCPNT_MAIN_CMD *mcmd, MAZCPNT_SUB_CMD *scmd)
    {
        int ret = MAZRET_OK;
        MAZCPNT_SUB_CMD *tmp;

        /* 检查参数有效性 */
        MAZASSERT_RETVAL(NULL == mcmd, MAZRET_EINVAL, "err: the param mcmd is NULL.");
        MAZASSERT_RETVAL(NULL == scmd, MAZRET_EINVAL, "err: the param scmd is NULL.");

        /* 判断是否有同名的子命令, 已经有同名称的子命令则直接退出函数并报错 */
        ret = MAZ_CPNT_debugcmd_scmd_find(mcmd, scmd->name, &tmp);
        MAZASSERT_RETVAL(MAZRET_ENOSUBCMD != ret, MAZRET_EEXIST, "err: the command \"%s %s\" already exists.", mcmd->name, scmd->name);

        MAZ_CPNT_list_init(&scmd->list);
        MAZ_CPNT_debugcmd_scmd_insert(mcmd, scmd);
        return MAZRET_OK;
    }

    /**
     * @brief 执行指定命令的回调函数
     * @param str 命令字符串, 包含主子命令和参数
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_execute(char *str)
    {
        int ret = MAZRET_OK;
        char uart_cmd[MAZCPNT_CMD_MAX_STR_LEN];
        char *mname = NULL;
        char *sname = NULL;
        char *param = NULL;
        MAZCPNT_MAIN_CMD *mcmd = NULL;
        MAZCPNT_SUB_CMD *scmd = NULL;

        /* 检查参数有效性 */
        MAZASSERT_RETVAL(NULL == str, MAZRET_EINVAL, "err: the param str is NULL.");

        memset(uart_cmd, 0, MAZCPNT_CMD_MAX_STR_LEN);
        memcpy(uart_cmd, str, strlen(str));

        /* 解析主命令 */
        mname = strtok(uart_cmd, " ");
        if (NULL == mname)
        {
            return MAZRET_EINVAL;
        }
        if ('\0' == *mname)
        {
            return MAZRET_EINVAL;
        }

        ret = MAZ_CPNT_debugcmd_mcmd_find(mname, &mcmd);
        MAZASSERT_RETVAL(ret, MAZRET_ENOCMD, "%s: command not found.", mname);

        /* 解析子命令 */
        sname = strtok(NULL, " ");
        MAZASSERT_RETVAL(NULL == sname && mcmd->handler == NULL, MAZRET_ENULLP, "%s: handler not found.", mname);

        /* 如果仅输入了一个主命令, 并且主命令回调非空 */
        if (NULL == sname && mcmd->handler != NULL)
        {
            ret = mcmd->handler(scmd, param);
            MAZASSERT_RETVAL(ret, MAZRET_EEXE, "err: command handler execute failed.");
            return MAZRET_OK;
        }

        /* 获取对应的子命令句柄 */
        ret = MAZ_CPNT_debugcmd_scmd_find(mcmd, sname, &scmd);
        MAZASSERT_RETVAL(ret, MAZRET_ENOSUBCMD, "%s: %s: sub command not found.", mname, sname);

        ret = scmd->handler(mcmd, param);
        MAZASSERT_RETVAL(ret, MAZRET_EEXE, "err: command handler execute failed.");

        return MAZRET_OK;
    }

    /**
     * @brief 命令自动补全接口
     * @param str 命令字符串, 包含主子命令和参数
     * @param fill 自动填充的字符串, 内存大小必须大于 MAZCPNT_CMD_MAX_STR_LEN
     * @param length 自动填充的长度
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_automatic_completion(char *str, char *fill, int *length)
    {
        int ret = MAZRET_OK;
        char uart_cmd[MAZCPNT_CMD_MAX_STR_LEN]; // 暂存目前已经输入的命令信息
        char *mname;                            // 暂存解析得到的主命令字符串
        char *sname;                            // 暂存解析得到的子命令字符串
        char *param;                            // 暂存剩余未解析的字符串
        MAZCPNT_MAIN_CMD *mcmd;                 // 记录匹配到的主命令
        MAZCPNT_SUB_CMD *scmd;                  // 记录匹配到的子命令
        MAZCPNT_MAIN_CMD *tmp_mcmd;             // 用于遍历主命令链表的临时变量
        MAZCPNT_SUB_CMD *tmp_scmd;              // 用于遍历子命令链表的临时变量
        struct list_head *tmp_list;             // 遍历链表的临时变量
        int in_mcmd_len = 0;                    // 输入字符中主命令的长度
        int in_scmd_len = 0;                    // 输入字符中子命令的长度
        int in_len = 0;                         // 输入字符的长度
        int match_count = 0;                    // 记录有多少个包含项
        int in_mspace_len = 0;                  // 记录输入主命令字符串前面有多少个空格
        int in_sspace_len = 0;                  // 记录输入子主命令字符串前面有多少个空格
        int predix_len = 0;                     // 用于多个命令匹配时记录多个命令共同的前缀长度
        int predix_len_tmp = 0;                 // 用于多个命令匹配时记录多个命令共同的前缀长度, 临时暂存

        /* 检查参数有效性 */
        MAZASSERT_RETVAL(NULL == str, MAZRET_EINVAL, "err: the param str is NULL.");
        MAZASSERT_RETVAL(NULL == fill, MAZRET_EINVAL, "err: the param fill is NULL.");
        MAZASSERT_RETVAL(NULL == length, MAZRET_EINVAL, "err: the param length is NULL.");

        /* fill 字符串内存大小必须大于 MAZCPNT_CMD_MAX_STR_LEN */
        memset(fill, 0, MAZCPNT_CMD_MAX_STR_LEN);
        *length = 0;

        /* 暂存目前已经输入的命令信息 */
        memset(uart_cmd, 0, MAZCPNT_CMD_MAX_STR_LEN);
        memcpy(uart_cmd, str, strlen(str));

        /* 解析主命令 */
        mname = strtok(uart_cmd, " ");
        MAZASSERT_RETVAL(NULL == mname, MAZRET_ENULLP, "err: strtok main cmd failed.");
        string_find_spaces(str, &in_mspace_len);
        in_len = strlen(str);
        in_mcmd_len = strlen(mname);

        /* 尝试解析子命令 */
        sname = strtok(NULL, " ");

        /* 解析结果有两种情况
         * 1. 仅有主命令
         * 2. 既有主命令又有子命令
         */
        if (NULL == sname)
        {
            /* 仅有主命令, 有下面几种场景:
             * 1. 末尾无多余空格
             *      1. 主命令完全, 无多余空格, 仅有一个匹配项 ---> 加一个空格, 并列举子命令
             *      2. 主命令不全, 无多余空格, 仅有一个匹配项 ---> 直接补全主命令, 并追加一个空格
             *      3. 主命令完全, 无多余空格, 且有多个匹配项 ---> 列举所有匹配项目
             *      4. 主命令不全, 无多余空格, 且有多个匹配项 ---> 补全到差异位置, 并列举所有匹配项目
             *      5. 主命令错误, 无多余空格  ---> 输出提示音
             * 2.末尾有多余空格, 则默认主命令完全
             *      6. 有匹配项 ---> 列举子命令
             *      7. 无匹配项 ---> 输出提示音
             */

            if (in_len == (in_mcmd_len + in_mspace_len))
            {
                /* 末尾无多余空格走此流程 */
                match_count = 0;

                /* 先遍历一遍所有主命令, 看有几个包含项 */
                tmp_mcmd = &g_mazcpnt_cmd;
                tmp_list = tmp_mcmd->list.next;
                tmp_mcmd = MAZCPNT_TO_MAIN_CMD(tmp_list);

                while (strcmp(tmp_mcmd->name, MAZCPNT_CMD_HELP_COMMAND))
                {
                    if (!strncmp(tmp_mcmd->name, mname, in_mcmd_len))
                    {
                        match_count++;
                        if (1 == match_count)
                        {
                            /* 第一个匹配的命令, 将其记录下来 */
                            mcmd = tmp_mcmd;
                        }
                        else if (2 == match_count)
                        {
                            /* 由于第一个匹配项没有打印, 在第二个匹配项的时候补打印 */
                            sLog_print("\r\n%s%s", mcmd->name, MAZCPNT_DEBUGCMD_SEPARATOR);
                            sLog_print("%s%s", tmp_mcmd->name, MAZCPNT_DEBUGCMD_SEPARATOR);
                            string_find_prefix(mcmd->name, tmp_mcmd->name, &predix_len);
                        }
                        else
                        {
                            /* 再后面的匹配项直接打印就好 */
                            sLog_print("%s%s", tmp_mcmd->name, MAZCPNT_DEBUGCMD_SEPARATOR);
                            string_find_prefix(mcmd->name, tmp_mcmd->name, &predix_len_tmp);
                            predix_len = predix_len < predix_len_tmp ? predix_len : predix_len_tmp;
                        }
                    }

                    tmp_list = tmp_mcmd->list.next;
                    tmp_mcmd = MAZCPNT_TO_MAIN_CMD(tmp_list);
                }

                if (0 == match_count)
                {
                    /* 场景5: 主命令错误, 无多余空格  ---> 输出提示音 */
                    *length = 0;
                    return MAZRET_ERING;
                }
                else if (1 == match_count)
                {
                    /* 有一个包含项, 对应场景1和2 */
                    if (!strcmp(mcmd->name, mname))
                    {
                        /* 场景1: 主命令完全, 无多余空格, 仅有一个匹配项 ---> 列举子命令*/
                        sLog_print("\r\n");
                        ret = mcmd->cmds.handler(mcmd, NULL);
                        MAZASSERT_RETVAL(ret, MAZRET_EEXE, "err: help sub cmd handler failed.");
                        *length = 1;
                        *fill = ' ';
                        return MAZRET_ENEWLINE;
                    }
                    else
                    {
                        /* 场景2: 主命令不全, 无多余空格, 仅有一个匹配项 ---> 直接补全主命令 */
                        *length = strlen(mcmd->name) - in_mcmd_len;
                        memcpy(fill, &mcmd->name[in_mcmd_len], *length);
                        strcat(fill, " ");
                        (*length)++;
                        return MAZRET_EAUTO;
                    }
                }
                else
                {
                    /* 有多个包含项, 对应场景3和4 */
                    sLog_print("\r\n");
                    *length = predix_len - in_mcmd_len;
                    if (*length != 0)
                    {
                        memcpy(fill, &mcmd->name[in_mcmd_len], *length);
                    }
                    return MAZRET_ENEWLINE;
                }
            }
            else
            {
                /* 如果末尾有空格, 则主命令必须是正确的 */
                ret = MAZ_CPNT_debugcmd_mcmd_find(mname, &mcmd);
                if (ret != 0)
                {
                    /* 场景7: 无匹配项 ---> 不做任何操作 */
                    *length = 0;
                    return MAZRET_ERING;
                }

                /* 场景6: 有匹配项 ---> 列举子命令 */
                sLog_print("\r\n");
                ret = mcmd->cmds.handler(mcmd, NULL);
                MAZASSERT_RETVAL(ret, MAZRET_EEXE, "err: help sub cmd handler failed.");
                *length = 0;
                return MAZRET_ENEWLINE;
            }
        }
        else
        {
            /* 如果既有主命令又有子命令, 则主命令必须是正确的 */
            ret = MAZ_CPNT_debugcmd_mcmd_find(mname, &mcmd);
            if (ret != 0)
            {
                *length = 0;
                return MAZRET_ERING;
            }

            /* 主命令正确则继续处理子命令, 子命令也有下面几种场景:
             * 1. 末尾无多余空格
             *      1. 子命令完全, 无多余空格, 仅有一个匹配项 ---> 加一个空格, 表示已经正确匹配上了
             *      2. 子命令不全, 无多余空格, 仅有一个匹配项 ---> 补全子命令, 并追加一个空格
             *      3. 子命令完全, 无多余空格, 且有多个匹配项 ---> 列举所有匹配的子命令
             *      4. 子命令不全, 无多余空格, 且有多个匹配项 ---> 补全到差异位置, 并列举所有匹配项目
             *      5. 子命令错误, 无多余空格  ---> 输出提示音
             * 2. 末尾有多余空格, 则默认主命令完全
             *      6. 有匹配项 ---> 不做任何操作
             *      7. 无匹配项 ---> 输出提示音
             */
            string_find_spaces(str + in_mspace_len + in_mcmd_len, &in_sspace_len);
            in_scmd_len = strlen(sname);

            if (in_len == (in_mspace_len + in_mcmd_len + in_sspace_len + in_scmd_len))
            {
                /* 子命令后无多余字符将走到这 */
                match_count = 0;

                /* 先遍历一遍所有主命令, 看有几个包含项 */
                tmp_scmd = &mcmd->cmds;
                tmp_list = tmp_scmd->list.next;
                tmp_scmd = MAZCPNT_TO_SUB_CMD(tmp_list);

                while (strcmp(tmp_scmd->name, MAZCPNT_CMD_HELP_COMMAND))
                {
                    if (!strncmp(tmp_scmd->name, sname, in_scmd_len))
                    {
                        match_count++;
                        if (1 == match_count)
                        {
                            /* 第一个匹配的命令, 将其记录下来 */
                            scmd = tmp_scmd;
                        }
                        else if (2 == match_count)
                        {
                            /* 由于第一个匹配项没有打印, 在第二个匹配项的时候补打印 */
                            sLog_print("\r\n%s%s", scmd->name, MAZCPNT_DEBUGCMD_SEPARATOR);
                            sLog_print("%s%s", tmp_scmd->name, MAZCPNT_DEBUGCMD_SEPARATOR);
                            string_find_prefix(scmd->name, tmp_scmd->name, &predix_len);
                        }
                        else
                        {
                            /* 再后面的匹配项直接打印就好 */
                            sLog_print("%s%s", tmp_scmd->name, MAZCPNT_DEBUGCMD_SEPARATOR);
                            string_find_prefix(scmd->name, tmp_scmd->name, &predix_len_tmp);
                            predix_len = predix_len < predix_len_tmp ? predix_len : predix_len_tmp;
                        }
                    }

                    tmp_list = tmp_scmd->list.next;
                    tmp_scmd = MAZCPNT_TO_SUB_CMD(tmp_list);
                }

                if (0 == match_count)
                {
                    /* 场景5: 子命令错误, 无多余空格  ---> 输出提示音 */
                    *length = 0;
                    return MAZRET_ERING;
                }
                else if (1 == match_count)
                {
                    /* 有一个包含项, 对应场景1和2 */
                    if (!strcmp(scmd->name, sname))
                    {
                        /* 场景1: 子命令完全, 无多余空格, 仅有一个匹配项 ---> 加一个空格 */
                        *length = 1;
                        *fill = ' ';
                        return MAZRET_EAUTO;
                    }
                    else
                    {
                        /* 场景2: 子命令不全, 无多余空格, 仅有一个匹配项 ---> 补全子命令, 并追加一个空格 */
                        *length = strlen(scmd->name) - in_scmd_len;
                        memcpy(fill, &scmd->name[in_scmd_len], *length);
                        strcat(fill, " ");
                        (*length)++;
                        return MAZRET_EAUTO;
                    }
                }
                else
                {
                    /* 有多个包含项, 对应场景3和4 */
                    sLog_print("\r\n");
                    *length = predix_len - in_scmd_len;
                    if (*length != 0)
                    {
                        memcpy(fill, &scmd->name[in_scmd_len], *length);
                    }
                    return MAZRET_ENEWLINE;
                }
            }
            else
            {
                /* 如果末尾有空格, 则子命令必须是正确的 */
                ret = MAZ_CPNT_debugcmd_scmd_find(mcmd, sname, &scmd);
                if (ret != 0)
                {
                    /* 场景7: 无匹配项 ---> 输出提示音 */
                    *length = 0;
                    return MAZRET_ERING;
                }

                /* 場景6: 有匹配项 ---> 不做任何操作 */
                *length = 0;
                return MAZRET_EAUTO;
            }
        }

        return MAZRET_OK;
    }

    /**
     * @brief 判断输入字符串是否全是空格
     * @retval 返回值TRUE或FALSE
     */
    int MAZ_CPNT_debugcmd_param_only_space(char *str)
    {
        int length = 0;

        MAZASSERT_RETVAL(NULL == str, MAZRET_EINVAL, "err: the param str is NULL.");

        length = strlen(str);
        string_find_spaces(str, &length);

        if (strlen(str) != length)
        {
            return MAZRET_FALSE;
        }

        return MAZRET_TRUE;
    }

    /**
     * @brief 遍历命令列表, 返回对应的主命令
     * @param name 主命令名称
     * @param cmd 用于返回找到的主命令结构体指针
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_mcmd_find(const char *name, MAZCPNT_MAIN_CMD **cmd)
    {
        MAZCPNT_MAIN_CMD *tmp_cmd;
        struct list_head *tmp_list;

        /* 检查参数有效性 */
        MAZASSERT_RETVAL(NULL == name, MAZRET_EINVAL, "err: the param name is NULL.");
        MAZASSERT_RETVAL(NULL == cmd, MAZRET_EINVAL, "err: the param cmd is NULL.");

        tmp_cmd = &g_mazcpnt_cmd;

        /* 字符串相等返回0, 退出循环 */
        while (strcmp(tmp_cmd->name, name))
        {
            tmp_list = tmp_cmd->list.next;
            tmp_cmd = MAZCPNT_TO_MAIN_CMD(tmp_list);

            if (!strcmp(tmp_cmd->name, MAZCPNT_CMD_HELP_COMMAND))
            {
                return MAZRET_ENOCMD;
            }
        }

        *cmd = tmp_cmd;

        return MAZRET_OK;
    }

    /**
     * @brief 按照字母顺序将新注册的主命令添加到主命令链表中
     * @param mcmd 新注册的主命令
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_mcmd_insert(MAZCPNT_MAIN_CMD *mcmd)
    {
        MAZCPNT_MAIN_CMD *tmp_cmd;
        struct list_head *tmp_list;

        /* 检查参数有效性 */
        MAZASSERT_RETVAL(NULL == mcmd, MAZRET_EINVAL, "err: the param mcmd is NULL.");

        tmp_cmd = &g_mazcpnt_cmd;

        /* 只要 mcmd 名称字母排序比当前遍历的更大, 则继续遍历 */
        while (strcasecmp(mcmd->name, tmp_cmd->name) > 0)
        {
            tmp_list = tmp_cmd->list.next;
            tmp_cmd = MAZCPNT_TO_MAIN_CMD(tmp_list);

            /* 整个链表都遍历结束了, 说明新注册的命令是字母序是最小的 */
            if (!strcmp(tmp_cmd->name, MAZCPNT_CMD_HELP_COMMAND))
            {
                /* 直接加到链表的最后面 */
                MAZ_CPNT_list_add_tail(&mcmd->list, &g_mazcpnt_cmd.list);
                return MAZRET_OK;
            }
        }

        /* 发现新注册比某一个现有命令的字母序小, 则插到其前面 */
        MAZ_CPNT_list_insert(&mcmd->list, tmp_list->prev, tmp_list);
        return MAZRET_OK;
    }

    /**
     * @brief 遍历命令列表, 查询是否有对应的命令
     * @param mcmd 主命令结构体
     * @param scmd 用于返回找到的子命令结构体指针
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_scmd_find(MAZCPNT_MAIN_CMD *mcmd, const char *sname, MAZCPNT_SUB_CMD **scmd)
    {
        MAZCPNT_SUB_CMD *tmp_cmd;
        struct list_head *tmp_list;

        /* 检查参数有效性 */
        MAZASSERT_RETVAL(NULL == mcmd, MAZRET_EINVAL, "err: the param mcmd is NULL.");
        MAZASSERT_RETVAL(NULL == sname, MAZRET_EINVAL, "err: the param sname is NULL.");
        MAZASSERT_RETVAL(NULL == scmd, MAZRET_EINVAL, "err: the param scmd is NULL.");

        tmp_cmd = &mcmd->cmds;

        /* 字符串相等返回0, 退出循环 */
        while (strcmp(tmp_cmd->name, sname))
        {
            tmp_list = tmp_cmd->list.next;
            tmp_cmd = MAZCPNT_TO_SUB_CMD(tmp_list);

            if (!strcmp(tmp_cmd->name, MAZCPNT_CMD_HELP_COMMAND))
            {
                return MAZRET_ENOSUBCMD;
            }
        }

        *scmd = tmp_cmd;

        return MAZRET_OK;
    }

    /**
     * @brief 按照字母顺序将新注册的子命令添加到子命令链表中
     * @param mcmd 指定在哪个主命令下注册子命令
     * @param mcmd 新注册的子命令
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_scmd_insert(MAZCPNT_MAIN_CMD *mcmd, MAZCPNT_SUB_CMD *scmd)
    {
        MAZCPNT_SUB_CMD *tmp_cmd;
        struct list_head *tmp_list;

        /* 检查参数有效性 */
        MAZASSERT_RETVAL(NULL == mcmd, MAZRET_EINVAL, "err: the param mcmd is NULL.");
        MAZASSERT_RETVAL(NULL == scmd, MAZRET_EINVAL, "err: the param scmd is NULL.");

        tmp_cmd = &mcmd->cmds;

        /* 只要 scmd 名称字母排序比当前遍历的更大, 则继续遍历 */
        while (strcasecmp(scmd->name, tmp_cmd->name) > 0)
        {
            tmp_list = tmp_cmd->list.next;
            tmp_cmd = MAZCPNT_TO_SUB_CMD(tmp_list);

            /* 整个链表都遍历结束了, 说明新注册的命令是字母序是最小的 */
            if (!strcmp(tmp_cmd->name, MAZCPNT_CMD_HELP_COMMAND))
            {
                /* 直接加到链表的最后面 */
                MAZ_CPNT_list_add_tail(&scmd->list, &mcmd->cmds.list);
                return MAZRET_OK;
            }
        }

        /* 发现新注册比某一个现有命令的字母序小, 则插到其前面 */
        MAZ_CPNT_list_insert(&scmd->list, tmp_list->prev, tmp_list);
        return MAZRET_OK;
    }

    /**
     * @brief 主命令通用回调函数
     * @notes 遍历主命令链表, 打印链表中所有主命令的描述信息
     * @param cmd 主命令结构体
     * @param param 没有使用到, 传NULL即可
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_main_help_handler(void *cmd, char *param)
    {
        MAZCPNT_MAIN_CMD *tmp_cmd;
        struct list_head *tmp_list;

        tmp_cmd = &g_mazcpnt_cmd;
        sLog_print("%-12s %s\r\n", tmp_cmd->name, tmp_cmd->desc);

        tmp_cmd = MAZCPNT_TO_MAIN_CMD(tmp_cmd->list.next);

        /* 字符串相等返回0, 退出循环 */
        while (strcmp(tmp_cmd->name, MAZCPNT_CMD_HELP_COMMAND))
        {
            sLog_print("%-12s %s\r\n", tmp_cmd->name, tmp_cmd->desc);
            tmp_list = tmp_cmd->list.next;
            tmp_cmd = MAZCPNT_TO_MAIN_CMD(tmp_list);
        }

        return MAZRET_OK;
    }

    /**
     * @brief 子命令通用回调函数(默认的help命令回调函数)
     * @notes 遍历子命令链表, 打印链表中所有子命令的描述信息
     * @param cmd 主命令结构体
     * @param param 没有使用到, 传NULL即可
     * @retval 错误码
     */
    int MAZ_CPNT_debugcmd_sub_help_handler(void *cmd, char *param)
    {
        MAZCPNT_MAIN_CMD *mcmd;
        MAZCPNT_SUB_CMD *tmp_cmd;
        struct list_head *tmp_list;

        mcmd = (MAZCPNT_MAIN_CMD *)cmd;
        tmp_cmd = &mcmd->cmds;
        sLog_print("%s %-12s %s\r\n", mcmd->name, tmp_cmd->name, tmp_cmd->desc);

        tmp_cmd = MAZCPNT_TO_SUB_CMD(tmp_cmd->list.next);

        /* 字符串相等返回0, 退出循环 */
        while (strcmp(tmp_cmd->name, MAZCPNT_CMD_HELP_COMMAND))
        {
            sLog_print("%s %-12s %s\r\n", mcmd->name, tmp_cmd->name, tmp_cmd->desc);
            tmp_list = tmp_cmd->list.next;
            tmp_cmd = MAZCPNT_TO_SUB_CMD(tmp_list);
        }

        return MAZRET_OK;
    }

    /**
     * @brief 找出输入的两个字符串前面相同前缀的长度
     * @param length 返回相同前缀的长度
     * @retval 返回值为执行成功与否
     */
    int string_find_prefix(const char *s1, const char *s2, int *length)
    {
        MAZASSERT_RETVAL(NULL == s1, MAZRET_EINVAL, "err: the param s1 is NULL.");
        MAZASSERT_RETVAL(NULL == s2, MAZRET_EINVAL, "err: the param s2 is NULL.");
        MAZASSERT_RETVAL(NULL == length, MAZRET_EINVAL, "err: the param length is NULL.");

        int i = 0;
        int len1 = strlen(s1);
        int len2 = strlen(s2);
        int len = len1 < len2 ? len1 : len2; // 取较小的那一个

        for (i = 0; i < len; i++)
        {
            if (s1[i] != s2[i])
                break;
        }

        *length = i;

        return MAZRET_OK;
    }

    /**
     * @brief 找出输入的字符串前面空格的长度
     * @param length 返回空格字符的个数
     * @retval 返回值为执行成功与否
     */
    int string_find_spaces(char *str, int *length)
    {
        int i = 0;
        int len = 0;

        MAZASSERT_RETVAL(NULL == str, MAZRET_EINVAL, "err: the param str is NULL.");
        MAZASSERT_RETVAL(NULL == length, MAZRET_EINVAL, "err: the param length is NULL.");

        len = strlen(str);

        for (i = 0; i < len; i++)
        {
            if (str[i] != ' ')
                break;
        }

        *length = i;

        return MAZRET_OK;
    }

#ifdef __cplusplus
}
#endif
