/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef UI_RGB_CONTROL_H__
#define UI_RGB_CONTROL_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USER_UI_EFFECT_RGB_TYPE_CONTINUE      0
#define USER_UI_EFFECT_RGB_TYPE_BLINKY        1


/** @brief A structure used to set the RBG LED color. */
struct ui_rgb_control_color {
	/* Red value range 0~255. */
	uint8_t red;

	/* Green value range 0~255. */
	uint8_t green;

	/* Blue value range 0~255. */
	uint8_t blue;
};

struct ui_rgb_control_effect {
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
    struct ui_rgb_control_color  color;
    struct ui_rgb_control_effect effect;
}__attribute__((aligned(4))) ui_rgb_control_message;

/**
 * @brief set the RBG LED effect 
 *
 * @return int 0 if successful, negative error code if not.
 */
int ui_rgb_control_set(struct ui_rgb_control_color color, struct ui_rgb_control_effect effect);

#ifdef __cplusplus
}
#endif

#endif /* UI_RGB_CONTROL_H__ */