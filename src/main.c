/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <zephyr/net/socket.h>
#include <nrf_modem_gnss.h>
#include "ui_led.h"
#include "ui_buzzer.h"
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(main, 3);

#define UDP_IP_HEADER_SIZE 28

static int client_fd;
static struct sockaddr_storage host_addr;
static struct k_work_delayable server_transmission_work;

K_SEM_DEFINE(lte_connected, 0, 1);


/*victor add variables*/
#define USER_WORK_Q_STACK_SIZE 4096
#define USER_WORK_Q_PRIORITY 5
K_THREAD_STACK_DEFINE(user_work_q_stack, USER_WORK_Q_STACK_SIZE);
static struct k_work_q user_work_q;

/*gnss*/
static struct k_work_delayable gnss_data_process_dwork;

static struct k_work_delayable multi_cell_request_dwork;

static K_SEM_DEFINE(pvt_data_sem, 0, 1);
static struct nrf_modem_gnss_pvt_data_frame last_pvt;

/*ui led*/
#define BRIGHTNESS_MAX   100U

static bool state[NUM_LEDS];
static uint8_t brightness_val[NUM_LEDS];
static uint8_t colour_val[NUM_LEDS];

/*ui buzzer*/
#define INTENSITY_START_VAL 100.0
#define FREQUENCY_START_VAL 440U

static void server_transmission_work_fn(struct k_work *work)
{
	int err;
	char buffer[CONFIG_UDP_DATA_UPLOAD_SIZE_BYTES] = {"hello from thingy\0"};

	printk("Transmitting UDP/IP payload of %d bytes to the ",
	       CONFIG_UDP_DATA_UPLOAD_SIZE_BYTES + UDP_IP_HEADER_SIZE);
	printk("IP address %s, port number %d\n",
	       CONFIG_UDP_SERVER_ADDRESS_STATIC,
	       CONFIG_UDP_SERVER_PORT);

	err = send(client_fd, buffer, sizeof(buffer), 0);
	if (err < 0) {
		printk("Failed to transmit UDP packet, %d\n", errno);
		return;
	}

	k_work_schedule(&server_transmission_work,
			K_SECONDS(CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS));
}

static void work_init(void)
{
	k_work_init_delayable(&server_transmission_work,
			      server_transmission_work_fn);
}

#if defined(CONFIG_NRF_MODEM_LIB)
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		printk("Network registration status: %s\n",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming\n");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		printk("PSM parameter update: TAU: %d, Active time: %d\n",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f\n",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			printk("%s\n", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC mode: %s\n",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle\n");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
		       evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

static int configure_low_power(void)
{
	int err;

#if defined(CONFIG_UDP_PSM_ENABLE)
	/** Power Saving Mode */
	err = lte_lc_psm_req(true);
	if (err) {
		printk("lte_lc_psm_req, error: %d\n", err);
	}
#else
	err = lte_lc_psm_req(false);
	if (err) {
		printk("lte_lc_psm_req, error: %d\n", err);
	}
#endif

#if defined(CONFIG_UDP_EDRX_ENABLE)
	/** enhanced Discontinuous Reception */
	err = lte_lc_edrx_req(true);
	if (err) {
		printk("lte_lc_edrx_req, error: %d\n", err);
	}
#else
	err = lte_lc_edrx_req(false);
	if (err) {
		printk("lte_lc_edrx_req, error: %d\n", err);
	}
#endif

#if defined(CONFIG_UDP_RAI_ENABLE)
	/** Release Assistance Indication  */
	err = lte_lc_rai_req(true);
	if (err) {
		printk("lte_lc_rai_req, error: %d\n", err);
	}
#endif

	return err;
}

static void modem_init(void)
{
	int err;
	uint8_t at_buf[64];

	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		err = lte_lc_init();
		if (err) {
			printk("Modem initialization failed, error: %d\n", err);
			return;
		}
	}

	nrf_modem_at_cmd(at_buf, sizeof(at_buf), "AT%%XEPCO=0");
	printk("AT%%XEPCO: %s\n", at_buf);

	nrf_modem_at_cmd(at_buf, sizeof(at_buf), "AT+CGMR");
	printk("Current modem firmware version: %s\n", at_buf);	
}

static void modem_connect(void)
{
	int err;

	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		err = lte_lc_connect_async(lte_handler);
		if (err) {
			printk("Connecting to LTE network failed, error: %d\n",
			       err);
			return;
		}
	}
}
#endif

static void server_disconnect(void)
{
	(void)close(client_fd);
}

static int server_init(void)
{
	struct sockaddr_in *server4 = ((struct sockaddr_in *)&host_addr);

	server4->sin_family = AF_INET;
	server4->sin_port = htons(CONFIG_UDP_SERVER_PORT);

	inet_pton(AF_INET, CONFIG_UDP_SERVER_ADDRESS_STATIC,
		  &server4->sin_addr);

	return 0;
}

static int server_connect(void)
{
	int err;

	client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {
		printk("Failed to create UDP socket: %d\n", errno);
		err = -errno;
		goto error;
	}

	err = connect(client_fd, (struct sockaddr *)&host_addr,
		      sizeof(struct sockaddr_in));
	if (err < 0) {
		printk("Connect failed : %d\n", errno);
		goto error;
	}

	return 0;

error:
	server_disconnect();

	return err;
}




/*victor add functions */

static void gnss_data_process_dwork_fn(struct k_work *work)
{
	printf("\033[1;1H");
	printf("\033[2J");
	//print_satellite_stats(&last_pvt);
	if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		//print_fix_data(&last_pvt);
	}
}

void user_work_init(void)
{
        k_work_queue_init(&user_work_q);
        k_work_queue_start(&user_work_q, user_work_q_stack,
                           K_THREAD_STACK_SIZEOF(user_work_q_stack), USER_WORK_Q_PRIORITY,
                           NULL);

        k_work_init_delayable(&gnss_data_process_dwork, gnss_data_process_dwork_fn);
}


static void user_buzzer_init(void)
{
	int ret;
	double start_intensity = INTENSITY_START_VAL;

	ret = ui_buzzer_init();
	if (ret) {
		LOG_ERR("Init ui buzzer failed (%d)", ret);
	}

	ret = ui_buzzer_set_intensity(INTENSITY_START_VAL);
	if (ret) {
		LOG_ERR("Set buzzer intensity failed (%d)", ret);
	}

	ret = ui_buzzer_set_frequency(FREQUENCY_START_VAL);
	if (ret) {
		LOG_ERR("Set buzzer frequency failed (%d)", ret);
	}
}

static uint8_t calculate_intensity(uint8_t colour_value, uint8_t brightness_value)
{
	uint32_t numerator = (uint32_t)colour_value * brightness_value;
	uint32_t denominator = BRIGHTNESS_MAX;

	return (uint8_t)(numerator / denominator);
}

static void user_led_init(void)
{
	uint8_t intensity;

	if (IS_ENABLED(CONFIG_UI_LED_USE_PWM)) {
		ui_led_pwm_init();
		for (int i = 0; i < NUM_LEDS; ++i) {
			colour_val[i] = UINT8_MAX;
			brightness_val[i] = BRIGHTNESS_MAX;
			intensity = calculate_intensity(colour_val[i], brightness_val[i]);
			ui_led_pwm_set_intensity(i, intensity);
		}
	} else if (IS_ENABLED(CONFIG_UI_LED_USE_GPIO)) {
		ui_led_gpio_init();
	}
}

static void button_event_handler(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & button_state & DK_BTN1_MSK) {
		/*
		if (!atomic_get(&connected)) {
			LOG_INF("Ignoring button press, not connected to network");
			return;
		}
		*/
		printk("Button 1 pressed\n");
		//start_cell_measurements();
	}
}
void main(void)
{
	int err;

	printk("Thing simple example start\n");

	work_init();

	user_work_init();

	user_led_init();

	ui_led_pwm_on_off(0, true); 
	ui_led_pwm_on_off(1, true);
	ui_led_pwm_on_off(2, true);

	ui_led_pwm_set_intensity(0, 0);     /*Red*/
	ui_led_pwm_set_intensity(1, 0);     /*Green*/
	ui_led_pwm_set_intensity(2, 255);   /*Blue*/	

	user_buzzer_init();
	//ui_buzzer_on_off(true);

	err = dk_buttons_init(button_event_handler);
	if (err) {
		LOG_ERR("Could not initialize buttons (%d)", err);
	}

#if defined(CONFIG_NRF_MODEM_LIB)

	/* Initialize the modem before calling configure_low_power(). This is
	 * because the enabling of RAI is dependent on the
	 * configured network mode which is set during modem initialization.
	 */
	modem_init();

	err = configure_low_power();
	if (err) {
		printk("Unable to set low power configuration, error: %d\n",
		       err);
	}

	modem_connect();

	k_sem_take(&lte_connected, K_FOREVER);
#endif

	printk("LTE connected\n");

	err = server_init();
	if (err) {
		printk("Not able to initialize UDP server connection\n");
		return;
	}

	err = server_connect();
	if (err) {
		printk("Not able to connect to UDP server\n");
		return;
	}

	k_work_schedule(&server_transmission_work, K_NO_WAIT);
}
