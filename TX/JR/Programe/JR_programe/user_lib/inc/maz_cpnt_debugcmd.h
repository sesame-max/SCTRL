/*
 * maz_cpnt_debugcmd.h
 *
 *  Created on: 2020年2月24日
 *      Author: wangbing
 *      Email : mz8023yt@163.com
 */

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef MAZ_CPNT_DEBUGCMD_H_
#define MAZ_CPNT_DEBUGCMD_H_

#include "maz_cpnt_list.h"
#include "maz_cpnt_debugcmd_config.h"

/* 2. MAZRET: State code returned */
#define MTRUE 1
#define MFALSE 0
#define MAZRET_TRUE MTRUE
#define MAZRET_FALSE MFALSE
#define MENABLE 1
#define MDISABLE 0
#define MAZRET_ENBALE MENABLE
#define MAZRET_DISABLE MDISABLE

#define MAZRET_OK 0
#define MAZRET_NG -1
#define MAZRET_EINVAL -2    /* Invalid argument */
#define MAZRET_ENOITEM -3   /* No such item */
#define MAZRET_ENOCMD -4    /* No such command */
#define MAZRET_ENOSUBCMD -5 /* No such sub command */
#define MAZRET_ENULLP -6    /* Null pointer exception */
#define MAZRET_EEXIST -7    /* File exists */
#define MAZRET_EEXE -8      /* Command exe fail */
#define MAZRET_ENEWLINE -9  /* New line */
#define MAZRET_EAUTO -10    /* Auto completion */
#define MAZRET_ERING -11    /* Ring */
#define MAZRET_EDEV -12     /* No devices */

/* 3. MAZASSERT: Assert */
/*    The "dmsg" and "dlog" print function must be implemented in "maz_cpnt_deubg.h" file */
#define MAZASSERT_RETVAL(condition, ret, fmt, msg...) \
    if (condition)                                    \
    {                                                 \
        if (fmt)                                      \
        {                                             \
            sLog_print(fmt "\r\n", ##msg);            \
        }                                             \
        return ret;                                   \
    }

/**
 * @brief 调试命令组件版本号
 */
#define MAZCPNT_DEBUGCMD_MAIN_VER 1
#define MAZCPNT_DEBUGCMD_SUB_VER 1
#define MAZCPNT_DEBUGCMD_REV_VER 0

    /**
     * @brief 命令回调函数原型
     */
    typedef int (*MAZCPNT_DEBUGCMD_HANDLER)(void *cmd, char *param);

    /**
     * @brief 子命令类型
     */
    typedef struct _MAZCPNT_SUB_CMD_
    {
        const char *name;                 // 子命令名称
        const char *desc;                 // 子命令描述信息
        MAZCPNT_DEBUGCMD_HANDLER handler; // 子命令回调函数
        struct list_head list;            // 子命令链表成员
    } MAZCPNT_SUB_CMD;

    /**
     * @brief 主命令类型
     */
    typedef struct _MAZCPNT_MAIN_CMD_
    {
        const char *name;                 // 主命令名称
        const char *desc;                 // 主命令描述信息
        MAZCPNT_DEBUGCMD_HANDLER handler; // 主命令回调函数, 遍历子命令链表, 并打印字命令的描述信息
        MAZCPNT_SUB_CMD cmds;             // 主命令下绑定的子命令链表
        struct list_head list;            // 主命令链表成员
    } MAZCPNT_MAIN_CMD;

    /**
     * @brief API接口
     */
    int MAZ_CPNT_debugcmd_init(void);
    int MAZ_CPNT_debugcmd_mcmd_register(MAZCPNT_MAIN_CMD *mcmd);
    int MAZ_CPNT_debugcmd_scmd_register(MAZCPNT_MAIN_CMD *mcmd, MAZCPNT_SUB_CMD *scmd);
    int MAZ_CPNT_debugcmd_execute(char *str);
    int MAZ_CPNT_debugcmd_automatic_completion(char *str, char *fill, int *length);
    int MAZ_CPNT_debugcmd_param_only_space(char *str);

#endif /* MAZ_CPNT_DEBUGCMD_H_ */

#ifdef __cplusplus
}
#endif
