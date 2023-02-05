/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef UI_BUZZER_CONTROL_H__
#define UI_BUZZER_CONTROL_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_BUZZER_CONTROL_TYPE_CONTINUE      0
#define UI_BUZZER_CONTROL_TYPE_BLINKY   1


/** @brief A structure used to set the RBG LED color. */
struct ui_buzzer_control_tone {
	/* buzzer frequency value range 0~FREQUENCY_MAX. */
	uint32_t frequency;

	/* buzzer intensity range 0~INTENSITY_MAX. */
	uint8_t intensity;
};

struct ui_buzzer_control_effect {
	/* type of RGB led effect. */
	uint8_t type;

	/* blinkyg duty cycle 0-100. */
	uint8_t duty;

	/* blinky interval value Range: 0~255 Unit:second*/
	uint8_t interval;

	/* effect duration value range 0~255. 0=forever Unit:second*/
	uint8_t duration;
};

typedef struct {
    struct ui_buzzer_control_tone   tone;
    struct ui_buzzer_control_effect effect;
}__attribute__((aligned(4))) ui_buzzer_control_message;

/**
 * @brief set the buzzer effect 
 *
 * @return int 0 if successful, negative error code if not.
 */
int ui_buzzer_control_set(struct ui_buzzer_control_tone tone_in, struct ui_buzzer_control_effect effect_in);

#ifdef __cplusplus
}
#endif

#endif /* UI_BUZZER_CONTROL_H__ */