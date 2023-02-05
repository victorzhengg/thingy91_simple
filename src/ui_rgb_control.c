/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include "ui_led.h"
#include "ui_rgb_control.h"

K_MSGQ_DEFINE(ui_rgb_control_msgq, sizeof(ui_rgb_control_message), 5, 4);

#define UI_RGB_CONTROL_WORK_Q_STACK_SIZE 512
#define UI_RGB_CONTROL_WORK_Q_PRIORITY      K_LOWEST_APPLICATION_THREAD_PRIO
K_THREAD_STACK_DEFINE(ui_rgb_control_work_q_stack, UI_RGB_CONTROL_WORK_Q_STACK_SIZE);
static struct k_work_q ui_rgb_control_work_q;

static struct k_work_delayable rgb_open_dwork;
static struct k_work_delayable rgb_close_dwork;
static struct k_work_delayable rgb_set_color_dwork;
static struct k_work_delayable rgb_interval_dwork;

/******* user ui effect thread *******/
#define UI_RGB_CONTROL_THREAD_STACK_SIZE 512
#define UI_RGB_CONTROL_THREAD_PRIORITY      K_LOWEST_APPLICATION_THREAD_PRIO - 1

static int16_t total_lift_span_cnt = 0;
static ui_rgb_control_message message;

static void rgb_open_dwork_fn(struct k_work *work)
{
    //printk("rgb_open_dwork_fn\n");
    ui_led_pwm_on_off(0, true); 
	ui_led_pwm_on_off(1, true);
	ui_led_pwm_on_off(2, true);    
}

static void rgb_close_dwork_fn(struct k_work *work)
{
    //printk("rgb_close_dwork_fn\n");
    ui_led_pwm_on_off(0, false); 
	ui_led_pwm_on_off(1, false);
	ui_led_pwm_on_off(2, false);    
}

static void rgb_set_color_dwork_fn(struct k_work *work)
{
    //printk("rgb_set_color_dwork_fn\n");
	ui_led_pwm_set_intensity(0, message.color.red);     /*Red*/
	ui_led_pwm_set_intensity(1, message.color.green);   /*Green*/
	ui_led_pwm_set_intensity(2, message.color.blue);    /*Blue*/	
}

static void rgb_interval_dwork_fn(struct k_work *work)
{
    //printk("rgb_interval_dwork_fn\n");
    uint32_t duty_value;
    k_timeout_t delay_value;
   
    k_work_schedule_for_queue(&ui_rgb_control_work_q, &rgb_set_color_dwork, K_NO_WAIT);
    k_work_schedule_for_queue(&ui_rgb_control_work_q, &rgb_open_dwork, K_NO_WAIT);

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

    k_work_schedule_for_queue(&ui_rgb_control_work_q, &rgb_close_dwork, delay_value);

    if(message.effect.duration > 0) {
        total_lift_span_cnt = total_lift_span_cnt - message.effect.interval;
        if(total_lift_span_cnt > 0) {
            k_work_schedule_for_queue(&ui_rgb_control_work_q, &rgb_interval_dwork, 
                                      K_SECONDS(message.effect.interval));
        }
        else {
            total_lift_span_cnt = 0;
        }
    }
    else {
        k_work_schedule_for_queue(&ui_rgb_control_work_q, &rgb_interval_dwork, 
                                  K_SECONDS(message.effect.interval));
    }    
}

static void ui_rgb_control_task(void)
{
    struct k_work_sync sync;

    printk("ui_rgb_control_task initial\n");
    k_work_queue_init(&ui_rgb_control_work_q);
    k_work_queue_start(&ui_rgb_control_work_q, ui_rgb_control_work_q_stack,
                        K_THREAD_STACK_SIZEOF(ui_rgb_control_work_q_stack), 
                        UI_RGB_CONTROL_WORK_Q_PRIORITY,
                        NULL);

    k_work_init_delayable(&rgb_open_dwork, rgb_open_dwork_fn);
    k_work_init_delayable(&rgb_close_dwork, rgb_close_dwork_fn);
    k_work_init_delayable(&rgb_set_color_dwork, rgb_set_color_dwork_fn);
    k_work_init_delayable(&rgb_interval_dwork, rgb_interval_dwork_fn);

	for (;;) {                                   
        k_msgq_get(&ui_rgb_control_msgq, &message, K_FOREVER);
        printk("ui_rgb_control_task get a message from msgq\n");
        k_work_cancel_delayable_sync(&rgb_close_dwork, &sync);
        k_work_cancel_delayable_sync(&rgb_interval_dwork, &sync);

        if(message.effect.type == UI_RGB_CONTROL_TYPE_CONTINUE){
            printk("message.effect.type == UI_RGB_CONTROL_TYPE_CONTINUE\n");
            k_work_schedule_for_queue(&ui_rgb_control_work_q, &rgb_set_color_dwork, K_NO_WAIT);
            k_work_schedule_for_queue(&ui_rgb_control_work_q, &rgb_open_dwork, K_NO_WAIT);
            if(message.effect.duration > 0) {
                k_work_schedule_for_queue(&ui_rgb_control_work_q, &rgb_close_dwork,
                                        K_SECONDS(message.effect.duration));
            }
        }
        else {
            printk("message.effect.type == UI_RGB_CONTROL_TYPE_BLINKY\n");
            total_lift_span_cnt = message.effect.duration;
            k_work_schedule_for_queue(&ui_rgb_control_work_q, &rgb_interval_dwork, K_NO_WAIT);
        }          
	}
}


K_THREAD_DEFINE(ui_rgb_control_thread, 
                UI_RGB_CONTROL_THREAD_STACK_SIZE,
                ui_rgb_control_task,
		        NULL, NULL, NULL,
                UI_RGB_CONTROL_THREAD_PRIORITY,
                0, 0);


/**
 * @brief set the RBG LED effect 
 *
 * @return int 0 if successful, negative error code if not.
 */
int ui_rgb_control_set(struct ui_rgb_control_color color_in, struct ui_rgb_control_effect effect_in)
{
    printk("user_ui_effect_rgb_set\n");
    int ret;
    ui_rgb_control_message message;

    message.color = color_in;
    message.effect = effect_in;

    ret = k_msgq_put(&ui_rgb_control_msgq, &message, K_NO_WAIT);

    return ret;
}
