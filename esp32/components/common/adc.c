#include "adc.h"
#include "pins.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"

static adc_oneshot_unit_handle_t s_adc1 = NULL;

void ADC_Init(void)
{
	adc_oneshot_unit_init_cfg_t unit_cfg = {
		.unit_id = ADC_UNIT,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc1));

	adc_oneshot_chan_cfg_t chan_cfg = {
		.atten = ADC_ATTEN_DB_12,
		.bitwidth = ADC_BITWIDTH_12,
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1, ADC_CHANNEL, &chan_cfg));
}

uint32_t Get_ADC_Value(void)
{
	int raw = 0;
	adc_oneshot_read(s_adc1, ADC_CHANNEL, &raw);
	return (raw > 0) ? (uint32_t)raw : 0;
}
