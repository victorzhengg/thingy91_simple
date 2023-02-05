#ifndef UER_SEHLL_CMD_H__
#define UER_SEHLL_CMD_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_RGB_ARG_RED        1
#define CMD_RGB_ARG_GREEN      2
#define CMD_RGB_ARG_BLUE       3
#define CMD_RGB_ARG_TYPE       4
#define CMD_RGB_ARG_DURATION   5
#define CMD_RGB_ARG_INTERVAL   6
#define CMD_RGB_ARG_DUTYCYCLE  7


#define CMD_BUZZER_ARG_FREQUENCY  1
#define CMD_BUZZER_ARG_INTENSITY  2
#define CMD_BUZZER_ARG_TYPE       3
#define CMD_BUZZER_ARG_DURATION   4
#define CMD_BUZZER_ARG_INTERVAL   5
#define CMD_BUZZER_ARG_DUTYCYCLE  6

#define CMD_BUZZER_ARG_FREQUENCY_MAX 10000
#define CMD_BUZZER_ARG_INTENSITY_MAX 100

#ifdef __cplusplus
}
#endif

#endif /* UER_SHELL_CMD_H__ */