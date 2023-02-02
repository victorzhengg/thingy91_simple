/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef UER_UI_EFFECT_H__
#define UER_UI_EFFECT_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USER_UI_EFFECT_RGB_TYPE_CONTINUE      0
#define USER_UI_EFFECT_RGB_TYPE_BLINKY        1


/** @brief A structure used to set the RBG LED color. */
struct user_ui_color {
	/* Red value range 0~255. */
	uint8_t red;

	/* Green value range 0~255. */
	uint8_t green;

	/* Blue value range 0~255. */
	uint8_t blue;
};

struct user_ui_effect {
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
    struct user_ui_color  color;
    struct user_ui_effect effect;
}__attribute__((aligned(4))) user_ui_message;

/**
 * @brief set the RBG LED effect 
 *
 * @return int 0 if successful, negative error code if not.
 */
int user_ui_effect_rgb_set(struct user_ui_color color, struct user_ui_effect effect);

#ifdef __cplusplus
}
#endif

#endif /* UI_INPUT_H__ */