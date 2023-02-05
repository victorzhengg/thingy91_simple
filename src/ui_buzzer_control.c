/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include "ui_buzzer.h"
#include "ui_buzzer_control.h"

K_MSGQ_DEFINE(ui_buzzer_control_msgq, sizeof(ui_buzzer_control_message), 5, 4);

#define UI_BUZZER_CONTROL_WORK_Q_STACK_SIZE 512
#define UI_BUZZER_CONTROL_WORK_Q_PRIORITY      K_LOWEST_APPLICATION_THREAD_PRIO
K_THREAD_STACK_DEFINE(ui_buzzer_control_work_q_stack, UI_BUZZER_CONTROL_WORK_Q_STACK_SIZE);
static struct k_work_q ui_buzzer_control_work_q;

static struct k_work_delayable buzzer_open_dwork;
static struct k_work_delayable buzzer_close_dwork;
static struct k_work_delayable buzzer_set_color_dwork;
static struct k_work_delayable buzzer_interval_dwork;

/******* user ui effect thread *******/
#define UI_BUZZER_CONTROL_THREAD_STACK_SIZE 512
#define UI_BUZZER_CONTROL_THREAD_PRIORITY      K_LOWEST_APPLICATION_THREAD_PRIO - 1

static int16_t total_lift_span_cnt = 0;
static ui_buzzer_control_message message;

static void buzzer_open_dwork_fn(struct k_work *work)
{
    //printk("buzzer_open_dwork_fn\n");
    ui_buzzer_on_off(true);   
}

static void buzzer_close_dwork_fn(struct k_work *work)
{
    //printk("buzzer_close_dwork_fn\n");
    ui_buzzer_on_off(false); 
}

static void buzzer_set_color_dwork_fn(struct k_work *work)
{
    //printk("buzzer_set_color_dwork_fn\n");
	ui_buzzer_set_frequency(message.tone.frequency);
	ui_buzzer_set_intensity(message.tone.intensity); 
}

static void buzzer_interval_dwork_fn(struct k_work *work)
{
    //printk("buzzer_interval_dwork_fn\n");
    uint32_t duty_value;
    k_timeout_t delay_value;
   
    k_work_schedule_for_queue(&ui_buzzer_control_work_q, &buzzer_set_color_dwork, K_NO_WAIT);
    k_work_schedule_for_queue(&ui_buzzer_control_work_q, &buzzer_open_dwork, K_NO_WAIT);

    if(message.effect.interval > 5) {
        duty_value = (uint32_t)message.effect.interval * message.effect.duty;
        duty_value = duty_value / 100;
        delay_value = K_SECONDS(duty_value);
    }
    else {
        duty_value = (uint32_t)message.effect.interval * 1000 * message.effect.duty;
        duty_value = duty_value / 100;
        delay_value = K_MSEC(duty_value);       
    }

    k_work_schedule_for_queue(&ui_buzzer_control_work_q, &buzzer_close_dwork, delay_value);

    if(message.effect.duration > 0) {
        total_lift_span_cnt = total_lift_span_cnt - message.effect.interval;
        if(total_lift_span_cnt > 0) {
            k_work_schedule_for_queue(&ui_buzzer_control_work_q, &buzzer_interval_dwork, 
                                      K_SECONDS(message.effect.interval));
        }
        else {
            total_lift_span_cnt = 0;
        }
    }
    else {
        k_work_schedule_for_queue(&ui_buzzer_control_work_q, &buzzer_interval_dwork, 
                                  K_SECONDS(message.effect.interval));
    }    
}

static void ui_buzzer_control_task(void)
{
    struct k_work_sync sync;

    printk("ui_buzzer_control_task initial\n");
    k_work_queue_init(&ui_buzzer_control_work_q);
    k_work_queue_start(&ui_buzzer_control_work_q, ui_buzzer_control_work_q_stack,
                        K_THREAD_STACK_SIZEOF(ui_buzzer_control_work_q_stack), 
                        UI_BUZZER_CONTROL_WORK_Q_PRIORITY,
                        NULL);

    k_work_init_delayable(&buzzer_open_dwork, buzzer_open_dwork_fn);
    k_work_init_delayable(&buzzer_close_dwork, buzzer_close_dwork_fn);
    k_work_init_delayable(&buzzer_set_color_dwork, buzzer_set_color_dwork_fn);
    k_work_init_delayable(&buzzer_interval_dwork, buzzer_interval_dwork_fn);

	for (;;) {                                   
        k_msgq_get(&ui_buzzer_control_msgq, &message, K_FOREVER);
        printk("ui_buzzer_control_task get a message from msgq\n");
        k_work_cancel_delayable_sync(&buzzer_close_dwork, &sync);
        k_work_cancel_delayable_sync(&buzzer_interval_dwork, &sync);

        if(message.effect.type == UI_BUZZER_CONTROL_TYPE_CONTINUE){
            printk("message.effect.type == UI_BUZZER_CONTROL_TYPE_CONTINUE\n");
            k_work_schedule_for_queue(&ui_buzzer_control_work_q, &buzzer_set_color_dwork, K_NO_WAIT);
            k_work_schedule_for_queue(&ui_buzzer_control_work_q, &buzzer_open_dwork, K_NO_WAIT);
            if(message.effect.duration > 0) {
                k_work_schedule_for_queue(&ui_buzzer_control_work_q, &buzzer_close_dwork,
                                        K_SECONDS(message.effect.duration));
            }
        }
        else {
            printk("message.effect.type == UI_BUZZER_CONTROL_TYPE_BLINKY\n");
            total_lift_span_cnt = message.effect.duration;
            k_work_schedule_for_queue(&ui_buzzer_control_work_q, &buzzer_interval_dwork, K_NO_WAIT);
        }       
	}
}


K_THREAD_DEFINE(ui_buzzer_control_thread, 
                UI_BUZZER_CONTROL_THREAD_STACK_SIZE,
                ui_buzzer_control_task,
		        NULL, NULL, NULL,
                UI_BUZZER_CONTROL_THREAD_PRIORITY,
                0, 0);


/**
 * @brief set the buzzer effect 
 *
 * @return int 0 if successful, negative error code if not.
 */
int ui_buzzer_control_set(struct ui_buzzer_control_tone tone_in, struct ui_buzzer_control_effect effect_in)
{
    printk("ui_buzzer_control_set\n");
    int ret;
    ui_buzzer_control_message message;

    message.tone = tone_in;
    message.effect = effect_in;

    ret = k_msgq_put(&ui_buzzer_control_msgq, &message, K_NO_WAIT);

    return ret;
}
