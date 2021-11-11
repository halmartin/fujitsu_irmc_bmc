/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * adc_hw.h
 * ADC driver Hardware Layer header file for Pilot-II
 *
 * Author: Samvinesh Christopher
 *
 *****************************************************************/
#ifndef _ADC_HW_H_
#define _ADC_HW_H_

#define ADC_CHANNEL0_NUM				0x00
#define ADC_CHANNEL7_NUM				0x07
#define ADC_CHANNEL8_NUM				0x08
#define ADC_CHANNEL15_NUM				0x0F
#define ADC_TEMP_SENSOR_CHANNEL			ADC_CHANNEL8_NUM
#define ADC_REF_VOLTAGE_IN_MVOLTS		2500
#define ADC_NUM_BITS_FOR_RESOLUTION		10
#define ADC_REGS_SIZE					128

#define ADC_HW_DEV_NAME        "adc_hw"

typedef struct {
    void     *pilot_ii_adc_addr;
}  __attribute__((packed)) adc_dev_t;


typedef struct 
{
		int (*adc_read_channel) (uint16_t *adc_value, int channel);
		int (*adc_get_resolution) (uint16_t *adc_resolution);
		int (*adc_get_reference_voltage) (uint16_t *adc_ref_volatge);
		int (*adc_reboot_notifier) (void);

} ADC_HW;



#endif /* _ADC_HW_H_ */

