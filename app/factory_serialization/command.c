#include <stddef.h>
#include <stdio.h>
#include "register.h"
#include "customer.h"

#define log_printf  printf
enum
{
	IS_SUCCESSFUL = 0,
	IS_CODE_ERROR,
	PARAMETER_IS_INVALID,
};
extern int uart_reply(char *cmd, char *str, unsigned int error_code, char *result);

static signed int do_get_card_id(cmd_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
    char buffer[17] = {0};

	if(argc > cmdtp->maxargs)
	{
		// parameter error
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = get_card_id(buffer, 16);
	if(ret < 0)
	{
		pResult = "failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	// clear all
	pResult = "successful";
	// TODO: replay the ts compare result
	uart_reply(cmdtp->name, pResult, result_code, buffer);
	return 0;
err:
	// TODO: failed 
	uart_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}

static signed int do_set_card_id(cmd_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
    char buffer[17] = {0};
    unsigned int bytes = 0;

	if((argc > cmdtp->maxargs) || (argv[1] == NULL))
	{
		// parameter error
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
    bytes = strlen(argv[1]);
    if(17 <= bytes)
    {
        // parameter error
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
    }
    memcpy(buffer, argv[1], bytes);
	ret = set_card_id(buffer, bytes);
	if(ret < 0)
	{
		pResult = "failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	// clear all
	pResult = "successful";
	// TODO: replay the ts compare result
	uart_reply(cmdtp->name, pResult, result_code, buffer);
	return 0;
err:
	// TODO: failed 
	uart_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}

static signed int do_get_provider_id(cmd_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
    char buffer[17] = {0};

	if(argc > cmdtp->maxargs)
	{
		// parameter error
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = get_provider_id(buffer, 16);
	if(ret < 0)
	{
		pResult = "failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	// clear all
	pResult = "successful";
	// TODO: replay the ts compare result
	uart_reply(cmdtp->name, pResult, result_code, buffer);
	return 0;
err:
	// TODO: failed 
	uart_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}

static signed int do_get_dsn_id(cmd_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
    char buffer[17] = {0};

	if(argc > cmdtp->maxargs)
	{
		// parameter error
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = get_dsn_id(buffer, 16);
	if(ret < 0)
	{
		pResult = "failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	// clear all
	pResult = "successful";
	// TODO: replay the ts compare result
	uart_reply(cmdtp->name, pResult, result_code, buffer);
	return 0;
err:
	// TODO: failed 
	uart_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}

static signed int do_set_provider_id(cmd_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
    char buffer[17] = {0};
    unsigned int bytes = 0;

	if((argc > cmdtp->maxargs) || (argv[1] == NULL))
	{
		// parameter error
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
    bytes = strlen(argv[1]);
    if(17 <= bytes)
    {
        // parameter error
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
    }
    memcpy(buffer, argv[1], bytes);
	ret = set_provider_id(buffer, bytes);
	if(ret < 0)
	{
		pResult = "failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	// clear all
	pResult = "successful";
	// TODO: replay the ts compare result
	uart_reply(cmdtp->name, pResult, result_code, buffer);
	return 0;
err:
	// TODO: failed 
	uart_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}

static signed int do_get_chip_id(cmd_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
    char buffer[17] = {0};

	if(argc > cmdtp->maxargs)
	{
		// parameter error
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = get_chip_id(buffer, 16);
	if(ret < 0)
	{
		pResult = "failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	// clear all
	pResult = "successful";
	// TODO: chip string
	uart_reply(cmdtp->name, pResult, result_code, buffer);
	return 0;
err:
	// TODO: failed 
	uart_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}

static signed int do_lock_otp(cmd_t *cmdtp, int flag, int argc, char *argv[], char *orgcmd)
{
	int ret = 0;
	char *pResult = NULL;
	unsigned int result_code = IS_SUCCESSFUL;
    //unsigned int bytes = 0;

	if((argc > cmdtp->maxargs) || (argv[1] == NULL))
	{
		// parameter error
		pResult = "failed: parameter is wrong!";
		result_code = PARAMETER_IS_INVALID;
		goto err;
	}
	ret = lock_otp();
	if(ret < 0)
	{
		pResult = "failed!";
		result_code = IS_CODE_ERROR;
		goto err;
	}
	// clear all
	pResult = "successful";
	// TODO: replay the ts compare result
	uart_reply(cmdtp->name, pResult, result_code, NULL);
	return 0;
err:
	// TODO: failed 
	uart_reply(cmdtp->name, pResult, result_code, NULL);
	return -1;
}


cmd_t cmd_get_card_id ={
	.name = "get_card_id",
	.maxargs = 1,
	.repeatable = 1,
	.cmd = do_get_card_id,
	.usage = "get smart card id",
};

cmd_t cmd_set_card_id ={
	.name = "set_card_id",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_set_card_id,
	.usage = "set smart card id",
};

cmd_t cmd_get_provider_id ={
	.name = "get_provider_id",
	.maxargs = 1,
	.repeatable = 1,
	.cmd = do_get_provider_id,
	.usage = "get provider id",
};

cmd_t cmd_set_provider_id ={
	.name = "set_provider_id",
	.maxargs = 2,
	.repeatable = 1,
	.cmd = do_set_provider_id,
	.usage = "set provider id",
};

cmd_t cmd_get_dsn_id ={
	.name = "get_dsn_id",
	.maxargs = 1,
	.repeatable = 1,
	.cmd = do_get_dsn_id,
	.usage = "get dsn",
};

cmd_t cmd_get_chip_id ={
	.name = "get_chip_id",
	.maxargs = 1,
	.repeatable = 1,
	.cmd = do_get_chip_id,
	.usage = "get chip id",
};

cmd_t cmd_lock_otp ={
	.name = "lock_otp",
	.maxargs = 1,
	.repeatable = 1,
	.cmd = do_lock_otp,
	.usage = "lock otp",
};



