/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include "ui_led.h"
#include "ui_buzzer.h"
#include "user_ui_effect.h"

K_MSGQ_DEFINE(user_ui_msgq, sizeof(user_ui_message), 10, 4);

#define USER_UI_WORK_Q_STACK_SIZE 1024
#define USER_UI_WORK_Q_PRIORITY      K_LOWEST_APPLICATION_THREAD_PRIO
K_THREAD_STACK_DEFINE(user_ui_work_q_stack, USER_UI_WORK_Q_STACK_SIZE);
static struct k_work_q user_ui_work_q;

static struct k_work_delayable rgb_open_dwork;
static struct k_work_delayable rgb_close_dwork;
static struct k_work_delayable rgb_set_color_dwork;
static struct k_work_delayable rgb_interval_dwork;

/******* user ui effect thread *******/
#define USER_UI_THREAD_STACK_SIZE 1024
#define USER_UI_THREAD_PRIORITY      K_LOWEST_APPLICATION_THREAD_PRIO - 1

int16_t total_lift_span_cnt = 0;
static user_ui_message message;

static void rgb_open_dwork_fn(struct k_work *work)
{}

static void rgb_close_dwork_fn(struct k_work *work)
{}

static void rgb_set_color_dwork_fn(struct k_work *work)
{}

static void rgb_interval_dwork_fn(struct k_work *work)
{
    //struct user_ui_message *message = CONTAINER_OF(work, struct user_ui_message, work);
}

void user_ui_effect_task(void)
{

    k_work_queue_init(&user_ui_work_q);
    k_work_queue_start(&user_ui_work_q, user_ui_work_q_stack,
                        K_THREAD_STACK_SIZEOF(user_ui_work_q_stack), USER_UI_WORK_Q_PRIORITY,
                        NULL);

    k_work_init_delayable(&rgb_open_dwork, rgb_open_dwork_fn);
    k_work_init_delayable(&rgb_close_dwork, rgb_close_dwork_fn);
    k_work_init_delayable(&rgb_set_color_dwork, rgb_set_color_dwork_fn);
    k_work_init_delayable(&rgb_interval_dwork, rgb_interval_dwork_fn);

	for (;;) {                                   
        k_msgq_get(&user_ui_msgq, &message, K_FOREVER);
        if(message.effect.type == USER_UI_EFFECT_RGB_TYPE_CONTINUE){
            k_work_schedule_for_queue(&user_ui_work_q, &rgb_set_color_dwork, K_NO_WAIT);
            k_work_schedule_for_queue(&user_ui_work_q, &rgb_open_dwork,K_NO_WAIT);
            k_work_schedule_for_queue(&user_ui_work_q, &rgb_close_dwork,K_SECONDS(message.effect.duration));
        }
        
        printk("user_ui_effect_task get message from user app\n");            
	}
}


K_THREAD_DEFINE(user_ui_thread, 
                USER_UI_THREAD_STACK_SIZE,
                user_ui_effect_task,
		        NULL, NULL, NULL,
                USER_UI_THREAD_PRIORITY,
                0, 0);


/**
 * @brief set the RBG LED effect 
 *
 * @return int 0 if successful, negative error code if not.
 */
int user_ui_effect_rgb_set(struct user_ui_color color_in, struct user_ui_effect effect_in)
{
    int ret;
    user_ui_message message;

    message.color = color_in;
    message.effect = effect_in;

    ret = k_msgq_put(&user_ui_msgq, &message, K_NO_WAIT);

    return ret;
}
