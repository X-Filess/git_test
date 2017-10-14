#ifndef __COMMAND_H_
#define __COMMAND_H_

#define CMD_END_FLAG			"\n"
#define MAX_CMDBUF_SIZE         256
#define CONFIG_SYS_MAXARGS      16
#define MAX_COMMAND_NUM 		(32)

typedef struct cmd_s	cmd_t;

struct cmd_s {
	char	*name;		/* Command Name			*/
	int	 maxargs;	/* maximum number of arguments	*/
	int	 repeatable;	/* autorepeat allowed?		*/
				/* Implementation function	*/
	signed int (*cmd)(struct cmd_s *,int, int,char *[], char *);
	char	*usage;		/* Usage message	(short)	*/
};

/* common/command.c */
int  register_uart_cmd(cmd_t  *cmd_item);
cmd_t *find_uart_cmd(const char *cmd);

#endif

