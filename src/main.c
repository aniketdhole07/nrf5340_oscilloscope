#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/pwm.h>

#if defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP) ||  defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPPNS) || defined(CONFIG_BOARD_NRF9160DK_NRF9160NS) || defined(CONFIG_BOARD_NRF9160DK_NRF9160)

/*ADC definitions and includes*/
#include <hal/nrf_saadc.h>
#define ADC_DEVICE_NAME DT_LABEL(DT_INST(0, nordic_nrf_saadc))
#define ADC_RESOLUTION 10
#define ADC_GAIN ADC_GAIN_1_6
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_1ST_CHANNEL_ID 0  
#define ADC_1ST_CHANNEL_INPUT NRF_SAADC_INPUT_AIN0

#define PWM_DEVICE_NAME DT_PROP(DT_NODELABEL(pwm0), label)
#define PWM_CH0_PIN DT_PROP(DT_NODELABEL(pwm0), ch0_pin)

#else
#error "Choose supported board or add new board for the application"
#endif
#include <drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include "gui.h"


#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <string.h>
#include <drivers/pwm.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(app);

#define PWM_MAX 253
#define TIMER_INTERVAL_MSEC 200
#define BUFFER_SIZE 1

struct k_timer my_timer;
const struct device *adc_dev;
const struct device *pwm_dev;

static const struct adc_channel_cfg m_1st_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_1ST_CHANNEL_INPUT,
#endif
};

static s16_t m_sample_buffer[BUFFER_SIZE];

static int adc_sample(void)
{
	int ret;

	const struct adc_sequence sequence = {
		.channels = BIT(ADC_1ST_CHANNEL_ID),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};

	if (!adc_dev) {
		return -1;
	}

	ret = adc_read(adc_dev, &sequence);
	if (ret) {
        printk("adc_read() failed with code %d\n", ret);
	}

	for (int i = 0; i < BUFFER_SIZE; i++) {
                float val = ((float)PWM_MAX/(float)568)*(float)m_sample_buffer[i];
				gui_add_point_to_chart( val);
	}

	return ret;
}

void adc_sample_event(struct k_timer *timer_id){
    int err = adc_sample();
    if (err) {
        printk("Error in adc sampling: %d\n", err);
    }
}

bool flag=true;
void on_gui_event(gui_event_t *event)
{
	switch(event->evt_type) {
		case GUI_EVT_BUTTON_PRESSED:
			break;
	}
}

void main(void)
{
	gui_config_t gui_config = {.event_callback = on_gui_event};
	gui_init(&gui_config);
    int err;

    //PWM0 setup
    pwm_dev = device_get_binding(PWM_DEVICE_NAME);
    if (!pwm_dev) {
	    printk("device_get_binding() PWM0 failed\n");
	}
    
    //ADC0 setup
    adc_dev = device_get_binding(ADC_DEVICE_NAME);
	if (!adc_dev) {
        printk("device_get_binding ADC_0 (=%s) failed\n", ADC_DEVICE_NAME);
    } 
    
    err = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
    if (err) {
	    printk("Error in adc setup: %d\n", err);
	}

    #if defined(CONFIG_BOARD_NRF9160DK_NRF9160NS) ||  defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPPNS)
    NRF_SAADC_NS->TASKS_CALIBRATEOFFSET = 1;
    #elif defined(CONFIG_BOARD_NRF9160DK_NRF9160) || defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
    NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
    #else
    #error "Choose supported board or add new board for the application"
    #endif
	
	while (1) {
		int err1 = adc_sample();
		if (err1) {
			printk("Error in adc sampling: %d\n", err);
		}
		
        
		printk("in main\n");
		k_sleep(K_MSEC(50));
	}
}


