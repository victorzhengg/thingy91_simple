/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/shell/shell.h>

#include "ui_rgb_control.h"
#include "user_shell_cmd.h"

static int cmd_gnss(const struct shell *shell, size_t argc,
                         char **argv)
{
	int cnt;
	shell_print(shell, "cmd_gnss argc = %d", argc);
	for (cnt = 0; cnt < argc; cnt++) {
			shell_print(shell, "cmd_gnss argv[%d] = %s", cnt, argv[cnt]);
	}
	return 0;
}

static int cmd_fftt(const struct shell *shell, size_t argc,
                           char **argv)
{
	int cnt;
	shell_print(shell, "cmd_fftt argc = %d", argc);
	for (cnt = 0; cnt < argc; cnt++) {
			shell_print(shell, "cmd_fftt argv[%d] = %s", cnt, argv[cnt]);
	}
	return 0;
}

static int cmd_rgb(const struct shell *shell, size_t argc,
                           char **argv)
{
	int cnt;
	int ret;
	struct user_ui_color rgb_color;
	struct user_ui_effect rgb_effect;
	uint32_t arg_val;
	uint16_t arg_flag = 0;
	uint8_t arg_opt_flag = 2;

	for (cnt = 0; cnt < argc; cnt++) {
			switch(cnt)
			{
				case CMD_RGB_ARG_RED:
					arg_val = strtol(argv[cnt], NULL, 10);
					if((arg_val >= 0) && (arg_val <= 255)) {
						rgb_color.red = arg_val;
					}
					else {
						arg_flag = cnt;
					}					
					break;
				case CMD_RGB_ARG_GREEN:
					arg_val = strtol(argv[cnt], NULL, 10);
					if((arg_val >= 0) && (arg_val <= 255)) {
						rgb_color.green = arg_val;
					}
					else {
						arg_flag = cnt; 
					}	
					break;
				case CMD_RGB_ARG_BLUE:
					arg_val = strtol(argv[cnt], NULL, 10);
					if((arg_val >= 0) && (arg_val <= 255)) {
						rgb_color.blue = arg_val;
					}
					else {
						arg_flag = cnt; 
					}	
					break;
				case CMD_RGB_ARG_TYPE:
					arg_val = strtol(argv[cnt], NULL, 10);
					if((arg_val >= 0) && (arg_val <= 1)) {
						rgb_effect.type = arg_val;
					}
					else {
						arg_flag = cnt; 
					}					
					break;
				case CMD_RGB_ARG_DURATION:
					arg_val = strtol(argv[cnt], NULL, 10);
					if((arg_val >= 0) && (arg_val <= 255)) {
						rgb_effect.duration = arg_val;
					}
					else {
						arg_flag = cnt; 
					}					
					break;				
				case CMD_RGB_ARG_INTERVAL:
					arg_val = strtol(argv[cnt], NULL, 10);
					if((arg_val >= 0) && (arg_val <= 255)) {
						rgb_effect.interval = arg_val;
						arg_opt_flag = arg_opt_flag - 1;
					}
					else {
						arg_flag = cnt; 
					}				
					break;
				case CMD_RGB_ARG_DUTYCYCLE:
					arg_val = strtol(argv[cnt], NULL, 10);
					if((arg_val >= 0) && (arg_val < 100)) {
						rgb_effect.duty = arg_val;
						arg_opt_flag = arg_opt_flag - 1;
					}
					else {
						arg_flag = cnt; 
					}					
					break;															
				default:
					break;															
			}
	}

	if(rgb_effect.type == USER_UI_EFFECT_RGB_TYPE_BLINKY) {
		if(arg_opt_flag != 0) {
			arg_flag = 1001;
		}
	}
	if(arg_flag == 0) {
		ret = user_ui_effect_rgb_set(rgb_color,rgb_effect);
		if(ret) {
			shell_print(shell, "cmd_rgb excute fail due to user_ui_effect_rgb_set retrun: %d", ret);
		}
		else {
			shell_print(shell, "cmd_rgb excute success");
		}	
	}
	else {
		shell_print(shell, "cmd_rgb excute fail due to wrong arg : %d", arg_flag);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_thingy,
        SHELL_CMD(gnss, NULL, "Start gnss test", cmd_gnss),
        SHELL_CMD(fftt, NULL, "Start first fix time test.", cmd_fftt),
		SHELL_CMD_ARG(rgb, NULL, "rgb led control", cmd_rgb, 6, 2),
        SHELL_SUBCMD_SET_END
);
/* Creating root (level 0) command "demo" without a handler */
SHELL_CMD_REGISTER(thingy, &sub_thingy, "Thingy 91 command line interface", NULL);

