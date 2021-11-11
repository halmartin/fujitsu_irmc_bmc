/*********************************************************
 **                                                     **
 **    (C)Copyright 2009-2015, American Megatrends Inc. **
 **                                                     **
 **            All Rights Reserved.                     **
 **                                                     **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,      **
 **                                                     **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.  **
 **                                                     **
 ********************************************************/
 
 /*
 * 	GPIO Hardware Driver
 *
 * 	Hardware layer driver of GPIO for Pilot-II
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <mach/platform.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/cacheflush.h>
#include "helper.h"
#include "driver_hal.h"
#include "gpio_hw_private.h"
#include "gpio_pins.h"
#include "gpio.h"
#include "gpio_hw.h"

static int m_dev_id = 0;

static uint8_t GPIO_IRQ_ENABLED = 0;

typedef enum {
	GPIO,
	SGPIO,
	RESERVED
}eGPIOType;


eGPIOType g_GpioPortPinType[MAX_GPIOS] =
{
		GPIO, // {GPIO_0, PORT_0, 0},
		GPIO, // {GPIO_1, PORT_0, 1},
		GPIO, // {GPIO_2, PORT_0, 2},
		GPIO, // {GPIO_3, PORT_0, 3},
		GPIO, // {GPIO_4, PORT_0, 4},
		GPIO, // {GPIO_5, PORT_0, 5},
		GPIO, // {GPIO_6, PORT_0, 6},
		GPIO, // {GPIO_7, PORT_0, 7},

		GPIO, // {GPIO_8, PORT_1, 0},
		GPIO, // {GPIO_9, PORT_1, 1},
		GPIO, // {GPIO_10, PORT_1, 2},
		GPIO, // {GPIO_11, PORT_1, 3},
		GPIO, // {GPIO_12, PORT_1, 4},
		GPIO, // {GPIO_13, PORT_1, 5},
		GPIO, // {GPIO_14, PORT_1, 6},
		GPIO, // {GPIO_15, PORT_1, 7},

		GPIO, // {GPIO_16, PORT_2, 0},
		GPIO, // {GPIO_17, PORT_2, 1},
		GPIO, // {GPIO_18, PORT_2, 2},
		GPIO, // {GPIO_19, PORT_2, 3},
		GPIO, // {GPIO_20, PORT_2, 4},
		GPIO, // {GPIO_21, PORT_2, 5},
		GPIO, // {GPIO_22, PORT_2, 6},
		GPIO, // {GPIO_23, PORT_2, 7},

		GPIO, // {GPIO_24, PORT_3, 0},
		GPIO, // {GPIO_25, PORT_3, 1},
		GPIO, // {GPIO_26, PORT_3, 2},
		GPIO, // {GPIO_27, PORT_3, 3},
		GPIO, // {GPIO_28, PORT_3, 4},
		GPIO, // {GPIO_29, PORT_3, 5},
		GPIO, // {GPIO_30, PORT_3, 6},
		GPIO, // {GPIO_31, PORT_3, 7},

		SGPIO, // {GPIO_32, PORT_4, 0},
		SGPIO, // {GPIO_33, PORT_4, 1},
		SGPIO, // {GPIO_34, PORT_4, 2},
		SGPIO, // {GPIO_35, PORT_4, 3},
		SGPIO, // {GPIO_36, PORT_4, 4},
		SGPIO, // {GPIO_37, PORT_4, 5},
		SGPIO, // {GPIO_38, PORT_4, 6},
		SGPIO, // {GPIO_39, PORT_4, 7},

		SGPIO, // {GPIO_40, PORT_5, 0},
		SGPIO, // {GPIO_41, PORT_5, 1},
		SGPIO, // {GPIO_42, PORT_5, 2},
		SGPIO, // {GPIO_43, PORT_5, 3},
		SGPIO, // {GPIO_44, PORT_5, 4},
		SGPIO, // {GPIO_45, PORT_5, 5},
		SGPIO, // {GPIO_46, PORT_5, 6},
		SGPIO, // {GPIO_47, PORT_5, 7},
 
		SGPIO, // {GPIO_48, PORT_6, 0},
		SGPIO, // {GPIO_49, PORT_6, 1},
		SGPIO, // {GPIO_50, PORT_6, 2},
		SGPIO, // {GPIO_51, PORT_6, 3},
		SGPIO, // {GPIO_52, PORT_6, 4},
		SGPIO, // {GPIO_53, PORT_6, 5},
		SGPIO, // {GPIO_54, PORT_6, 6},
		SGPIO, // {GPIO_55, PORT_6, 7},

		SGPIO, // {GPIO_56, PORT_7, 0},
		SGPIO, // {GPIO_57, PORT_7, 1},
		SGPIO, // {GPIO_58, PORT_7, 2},
		SGPIO, // {GPIO_59, PORT_7, 3},
		SGPIO, // {GPIO_60, PORT_7, 4},
		SGPIO, // {GPIO_61, PORT_7, 5},
		SGPIO, // {GPIO_62, PORT_7, 6},
		SGPIO, // {GPIO_63, PORT_7, 7},

		SGPIO, // {GPIO_64, PORT_8, 0},
		SGPIO, // {GPIO_65, PORT_8, 1},
		SGPIO, // {GPIO_66, PORT_8, 2},
		SGPIO, // {GPIO_67, PORT_8, 3},
		SGPIO, // {GPIO_68, PORT_8, 4},
		SGPIO, // {GPIO_69, PORT_8, 5},
		SGPIO, // {GPIO_70, PORT_8, 6},
		SGPIO, // {GPIO_71, PORT_8, 7},

		SGPIO, // {GPIO_72, PORT_9, 0},
		SGPIO, // {GPIO_73, PORT_9, 1},
		SGPIO, // {GPIO_74, PORT_9, 2},
		SGPIO, // {GPIO_75, PORT_9, 3},
		SGPIO, // {GPIO_76, PORT_9, 4},
		SGPIO, // {GPIO_77, PORT_9, 5},
		SGPIO, // {GPIO_78, PORT_9, 6},
		SGPIO, // {GPIO_79, PORT_9, 7},

		SGPIO, // {GPIO_80, PORT_10, 0},
		SGPIO, // {GPIO_81, PORT_10, 1},
		SGPIO, // {GPIO_82, PORT_10, 2},
		SGPIO, // {GPIO_83, PORT_10, 3},
		SGPIO, // {GPIO_84, PORT_10, 4},
		SGPIO, // {GPIO_85, PORT_10, 5},
		SGPIO, // {GPIO_86, PORT_10, 6},
		SGPIO, // {GPIO_87, PORT_10, 7},

		SGPIO, // {GPIO_88, PORT_11, 0},
		SGPIO, // {GPIO_89, PORT_11, 1},
		SGPIO, // {GPIO_90, PORT_11, 2},
		SGPIO, // {GPIO_91, PORT_11, 3},
		SGPIO, // {GPIO_92, PORT_11, 4},
		SGPIO, // {GPIO_93, PORT_11, 5},
		SGPIO, // {GPIO_94, PORT_11, 6},
		SGPIO, // {GPIO_95, PORT_11, 7},
		
#if defined(SOC_PILOT_IV) || defined(SOC_PILOT_III)
		SGPIO, // {GPIO_96, PORT_12, 0},
		SGPIO, // {GPIO_97, PORT_12, 1},
		SGPIO, // {GPIO_98, PORT_12, 2},
		SGPIO, // {GPIO_99, PORT_12, 3},
		SGPIO, // {GPIO_100, PORT_12, 4},
		SGPIO, // {GPIO_101, PORT_12, 5},
		SGPIO, // {GPIO_102, PORT_12, 6},
		SGPIO, // {GPIO_103, PORT_12, 7},

		SGPIO, // {GPIO_104, PORT_13, 0},
		SGPIO, // {GPIO_105, PORT_13, 1},
		SGPIO, // {GPIO_106, PORT_13, 2},
		SGPIO, // {GPIO_107, PORT_13, 3},
		SGPIO, // {GPIO_108, PORT_13, 4},
		SGPIO, // {GPIO_109, PORT_13, 5},
		SGPIO, // {GPIO_110, PORT_13, 6},
		SGPIO, // {GPIO_111, PORT_13, 7},

#if defined(SOC_PILOT_IV)
		RESERVED, // {GPIO_112, PORT_14, 0},
		RESERVED, // {GPIO_113, PORT_14, 1},
		RESERVED, // {GPIO_114, PORT_14, 2},
		RESERVED, // {GPIO_115, PORT_14, 3},
		RESERVED, // {GPIO_116, PORT_14, 4},
#elif defined(SOC_PILOT_III)				// As per spec GPIO_14_0 - GPIO_14_4 is GPIO
		GPIO, // {GPIO_112, PORT_14, 0},
		GPIO, // {GPIO_113, PORT_14, 1},
		GPIO, // {GPIO_114, PORT_14, 2},
		GPIO, // {GPIO_115, PORT_14, 3},
		GPIO, // {GPIO_116, PORT_14, 4},
#endif
		RESERVED, // {GPIO_117, PORT_14, 5},
		RESERVED, // {GPIO_118, PORT_14, 6},
		RESERVED, // {GPIO_119, PORT_14, 7},

		RESERVED, // {GPIO_120, PORT_15, 0},
		RESERVED, // {GPIO_121, PORT_15, 1},
		RESERVED, // {GPIO_122, PORT_15, 2},
		RESERVED, // {GPIO_123, PORT_15, 3},
		RESERVED, // {GPIO_124, PORT_15, 4},
		RESERVED, // {GPIO_125, PORT_15, 5},
		RESERVED, // {GPIO_126, PORT_15, 6},
		RESERVED, // {GPIO_127, PORT_15, 7},
		
		GPIO, // {GPIO_128, PORT_16, 0},
		GPIO, // {GPIO_129, PORT_16, 1},
		GPIO, // {GPIO_130, PORT_16, 2},
		GPIO, // {GPIO_131, PORT_16, 3},
		GPIO, // {GPIO_132, PORT_16, 4},
		GPIO, // {GPIO_133, PORT_16, 5},
		GPIO, // {GPIO_134, PORT_16, 6},
		GPIO, // {GPIO_135, PORT_16, 7},

		GPIO, // {GPIO_136, PORT_17, 0},
		GPIO, // {GPIO_137, PORT_17, 1},
		GPIO, // {GPIO_138, PORT_17, 2},
		GPIO, // {GPIO_139, PORT_17, 3},
		GPIO, // {GPIO_140, PORT_17, 4},
		GPIO, // {GPIO_141, PORT_17, 5},
		GPIO, // {GPIO_142, PORT_17, 6},
		GPIO, // {GPIO_143, PORT_17, 7},

		GPIO, // {GPIO_144, PORT_18, 0},
		GPIO, // {GPIO_145, PORT_18, 1},
		GPIO, // {GPIO_146, PORT_18, 2},
		GPIO, // {GPIO_147, PORT_18, 3},
		GPIO, // {GPIO_148, PORT_18, 4},
		GPIO, // {GPIO_149, PORT_18, 5},
		GPIO, // {GPIO_150, PORT_18, 6},
		GPIO, // {GPIO_151, PORT_18, 7},

		GPIO, // {GPIO_152, PORT_19, 0},
		GPIO, // {GPIO_153, PORT_19, 1},
		GPIO, // {GPIO_154, PORT_19, 2},
		GPIO, // {GPIO_155, PORT_19, 3},
		GPIO, // {GPIO_156, PORT_19, 4},
		GPIO, // {GPIO_157, PORT_19, 5},
		GPIO, // {GPIO_158, PORT_19, 6},
		GPIO, // {GPIO_159, PORT_19, 7},

		GPIO, // {GPIO_160, PORT_20, 0},
		GPIO, // {GPIO_161, PORT_20, 1},
		GPIO, // {GPIO_162, PORT_20, 2},
		GPIO, // {GPIO_163, PORT_20, 3},
		GPIO, // {GPIO_164, PORT_20, 4},
		GPIO, // {GPIO_165, PORT_20, 5},
		GPIO, // {GPIO_166, PORT_20, 6},
		GPIO, // {GPIO_167, PORT_20, 7},

		GPIO, // {GPIO_168, PORT_21, 0},
		GPIO, // {GPIO_169, PORT_21, 1},
		GPIO, // {GPIO_170, PORT_21, 2},
		GPIO, // {GPIO_171, PORT_21, 3},
		GPIO, // {GPIO_172, PORT_21, 4},
		GPIO, // {GPIO_173, PORT_21, 5},
		GPIO, // {GPIO_174, PORT_21, 6},
		GPIO, // {GPIO_175, PORT_21, 7},

		GPIO, // {GPIO_176, PORT_22, 0},
		GPIO, // {GPIO_177, PORT_22, 1},
		GPIO, // {GPIO_178, PORT_22, 2},
		GPIO, // {GPIO_179, PORT_22, 3},
		GPIO, // {GPIO_180, PORT_22, 4},
		GPIO, // {GPIO_181, PORT_22, 5},
		GPIO, // {GPIO_182, PORT_22, 6},
		GPIO, // {GPIO_183, PORT_22, 7},

		GPIO, // {GPIO_184, PORT_23, 0},
		GPIO, // {GPIO_185, PORT_23, 1},
		GPIO, // {GPIO_186, PORT_23, 2},
		GPIO, // {GPIO_187, PORT_23, 3},
		GPIO, // {GPIO_188, PORT_23, 4},
		GPIO, // {GPIO_189, PORT_23, 5},
		GPIO, // {GPIO_190, PORT_23, 6},
		GPIO, // {GPIO_191, PORT_23, 7},

		GPIO, // {GPIO_192, PORT_24, 0},
		GPIO, // {GPIO_193, PORT_24, 1},
		GPIO, // {GPIO_194, PORT_24, 2},
		GPIO, // {GPIO_195, PORT_24, 3},
		GPIO, // {GPIO_196, PORT_24, 4},
		GPIO, // {GPIO_197, PORT_24, 5},
		GPIO, // {GPIO_198, PORT_24, 6},
		GPIO, // {GPIO_199, PORT_24, 7},

		GPIO, // {GPIO_200, PORT_25, 0},
		GPIO, // {GPIO_201, PORT_25, 1},
		GPIO, // {GPIO_202, PORT_25, 2},
		GPIO, // {GPIO_203, PORT_25, 3},
		GPIO, // {GPIO_204, PORT_25, 4},
		GPIO, // {GPIO_205, PORT_25, 5},
		GPIO, // {GPIO_206, PORT_25, 6},
		GPIO, // {GPIO_207, PORT_25, 7},

		GPIO, // {GPIO_208, PORT_26, 0},
		GPIO, // {GPIO_209, PORT_26, 1},
		GPIO, // {GPIO_210, PORT_26, 2},
		GPIO, // {GPIO_211, PORT_26, 3},
		GPIO, // {GPIO_212, PORT_26, 4},
		GPIO, // {GPIO_213, PORT_26, 5},
		GPIO, // {GPIO_214, PORT_26, 6},
		GPIO, // {GPIO_215, PORT_26, 7},

		GPIO, // {GPIO_216, PORT_27, 0},
		GPIO, // {GPIO_217, PORT_27, 1},
		GPIO, // {GPIO_218, PORT_27, 2},
		GPIO, // {GPIO_219, PORT_27, 3},
		GPIO, // {GPIO_220, PORT_27, 4},
		GPIO, // {GPIO_221, PORT_27, 5},
		GPIO, // {GPIO_222, PORT_27, 6},
		GPIO, // {GPIO_223, PORT_27, 7},
		
		GPIO, // {GPIO_224, PORT_28, 0},
		GPIO, // {GPIO_225, PORT_28, 1},
		GPIO, // {GPIO_226, PORT_28, 2},
		GPIO, // {GPIO_227, PORT_28, 3},
		GPIO, // {GPIO_228, PORT_28, 4},
		GPIO, // {GPIO_229, PORT_28, 5},
		GPIO, // {GPIO_230, PORT_28, 6},
		GPIO, // {GPIO_231, PORT_28, 7},

		GPIO, // {GPIO_232, PORT_29, 0},
		GPIO, // {GPIO_233, PORT_29, 1},
		GPIO, // {GPIO_234, PORT_29, 2},
		GPIO, // {GPIO_235, PORT_29, 3},
		GPIO, // {GPIO_236, PORT_29, 4},
		GPIO, // {GPIO_237, PORT_29, 5},
		RESERVED, // {GPIO_238, PORT_29, 6},
		RESERVED, // {GPIO_239, PORT_29, 7},
		
#if defined(SOC_PILOT_IV)
		SGPIO, // {GPIO_240, PORT_30, 0},
		SGPIO, // {GPIO_241, PORT_30, 1},
		SGPIO, // {GPIO_242, PORT_30, 2},
		SGPIO, // {GPIO_243, PORT_30, 3},
		SGPIO, // {GPIO_244, PORT_30, 4},
		SGPIO, // {GPIO_245, PORT_30, 5},
		SGPIO, // {GPIO_246, PORT_30, 6},
		SGPIO, // {GPIO_247, PORT_30, 7},
		
		SGPIO, // {GPIO_248, PORT_31, 0},
		SGPIO, // {GPIO_249, PORT_31, 1},
		SGPIO, // {GPIO_250, PORT_31, 2},
		SGPIO, // {GPIO_251, PORT_31, 3},
		SGPIO, // {GPIO_252, PORT_31, 4},
		SGPIO, // {GPIO_253, PORT_31, 5},
		SGPIO, // {GPIO_254, PORT_31, 6},
		SGPIO, // {GPIO_255, PORT_31, 7},
		
		SGPIO, // {GPIO_256, PORT_32, 0},
		SGPIO, // {GPIO_257, PORT_32, 1},
		SGPIO, // {GPIO_258, PORT_32, 2},
		SGPIO, // {GPIO_259, PORT_32, 3},
		SGPIO, // {GPIO_260, PORT_32, 4},
		SGPIO, // {GPIO_261, PORT_32, 5},
		SGPIO, // {GPIO_262, PORT_32, 6},
		SGPIO, // {GPIO_263, PORT_32, 7},
		
		SGPIO, // {GPIO_264, PORT_33, 0},
		SGPIO, // {GPIO_265, PORT_33, 1},
		SGPIO, // {GPIO_266, PORT_33, 2},
		SGPIO, // {GPIO_267, PORT_33, 3},
		SGPIO, // {GPIO_268, PORT_33, 4},
		SGPIO, // {GPIO_269, PORT_33, 5},
		SGPIO, // {GPIO_270, PORT_33, 6},
		SGPIO, // {GPIO_271, PORT_33, 7},
		
		SGPIO, // {GPIO_272, PORT_34, 0},
		SGPIO, // {GPIO_273, PORT_34, 1},
		SGPIO, // {GPIO_274, PORT_34, 2},
		SGPIO, // {GPIO_275, PORT_34, 3},
		SGPIO, // {GPIO_276, PORT_34, 4},
		SGPIO, // {GPIO_277, PORT_34, 5},
		SGPIO, // {GPIO_278, PORT_34, 6},
		SGPIO, // {GPIO_279, PORT_34, 7},
		
		SGPIO, // {GPIO_280, PORT_35, 0},
		SGPIO, // {GPIO_281, PORT_35, 1},
		SGPIO, // {GPIO_282, PORT_35, 2},
		SGPIO, // {GPIO_283, PORT_35, 3},
		SGPIO, // {GPIO_284, PORT_35, 4},
		SGPIO, // {GPIO_285, PORT_35, 5},
		SGPIO, // {GPIO_286, PORT_35, 6},
		SGPIO, // {GPIO_287, PORT_35, 7},
		
		SGPIO, // {GPIO_288, PORT_36, 0},
		SGPIO, // {GPIO_289, PORT_36, 1},
		SGPIO, // {GPIO_290, PORT_36, 2},
		SGPIO, // {GPIO_291, PORT_36, 3},
		SGPIO, // {GPIO_292, PORT_36, 4},
		SGPIO, // {GPIO_293, PORT_36, 5},
		SGPIO, // {GPIO_294, PORT_36, 6},
		SGPIO, // {GPIO_295, PORT_36, 7},
		
		SGPIO, // {GPIO_296, PORT_37, 0},
		SGPIO, // {GPIO_297, PORT_37, 1},
		SGPIO, // {GPIO_298, PORT_37, 2},
		SGPIO, // {GPIO_299, PORT_37, 3},
		SGPIO, // {GPIO_300, PORT_37, 4},
		SGPIO, // {GPIO_301, PORT_37, 5},
		SGPIO, // {GPIO_302, PORT_37, 6},
		SGPIO, // {GPIO_303, PORT_37, 7},
		
		SGPIO, // {GPIO_304, PORT_38, 0},
		SGPIO, // {GPIO_305, PORT_38, 1},
		SGPIO, // {GPIO_306, PORT_38, 2},
		SGPIO, // {GPIO_307, PORT_38, 3},
		SGPIO, // {GPIO_308, PORT_38, 4},
		SGPIO, // {GPIO_309, PORT_38, 5},
		SGPIO, // {GPIO_310, PORT_38, 6},
		SGPIO, // {GPIO_311, PORT_38, 7},
		
		SGPIO, // {GPIO_312, PORT_39, 0},
		SGPIO, // {GPIO_313, PORT_39, 1},
		SGPIO, // {GPIO_314, PORT_39, 2},
		SGPIO, // {GPIO_315, PORT_39, 3},
		SGPIO, // {GPIO_316, PORT_39, 4},
		SGPIO, // {GPIO_317, PORT_39, 5},
		SGPIO, // {GPIO_318, PORT_39, 6},
		SGPIO, // {GPIO_319, PORT_39, 7},
		
		GPIO, // {GPIO_320, PORT_40, 0},
		GPIO, // {GPIO_321, PORT_40, 1},
		GPIO, // {GPIO_322, PORT_40, 2},
		GPIO, // {GPIO_323, PORT_40, 3},
		GPIO, // {GPIO_324, PORT_40, 4},
		GPIO, // {GPIO_325, PORT_40, 5},
		GPIO, // {GPIO_326, PORT_40, 6},
		GPIO, // {GPIO_327, PORT_40, 7},
		
		GPIO, // {GPIO_328, PORT_41, 0},
		GPIO, // {GPIO_329, PORT_41, 1},
		GPIO, // {GPIO_330, PORT_41, 2},
		GPIO, // {GPIO_331, PORT_41, 3},
		GPIO, // {GPIO_332, PORT_41, 4},
		GPIO, // {GPIO_333, PORT_41, 5},
		GPIO, // {GPIO_334, PORT_41, 6},
		GPIO, // {GPIO_335, PORT_41, 7},
		
		GPIO, // {GPIO_336, PORT_42, 0},
		GPIO, // {GPIO_337, PORT_42, 1},
		GPIO, // {GPIO_338, PORT_42, 2},
		GPIO, // {GPIO_339, PORT_42, 3},
		GPIO, // {GPIO_340, PORT_42, 4},
		GPIO, // {GPIO_341, PORT_42, 5},
		GPIO, // {GPIO_342, PORT_42, 6},
		GPIO, // {GPIO_343, PORT_42, 7},
		
		GPIO, // {GPIO_344, PORT_43, 0},
		GPIO, // {GPIO_345, PORT_43, 1},
		GPIO, // {GPIO_346, PORT_43, 2},
		GPIO, // {GPIO_347, PORT_43, 3},
		GPIO, // {GPIO_348, PORT_43, 4},
		GPIO, // {GPIO_349, PORT_43, 5},
		GPIO, // {GPIO_350, PORT_43, 6},
		GPIO, // {GPIO_351, PORT_43, 7},
		
		GPIO, // {GPIO_352, PORT_44, 0},
		GPIO, // {GPIO_353, PORT_44, 1},
		GPIO, // {GPIO_354, PORT_44, 2},
		GPIO, // {GPIO_355, PORT_44, 3},
		GPIO, // {GPIO_356, PORT_44, 4},
		GPIO, // {GPIO_357, PORT_44, 5},
		GPIO, // {GPIO_358, PORT_44, 6},
		GPIO, // {GPIO_359, PORT_44, 7},
		
		GPIO, // {GPIO_360, PORT_45, 0},
		GPIO, // {GPIO_361, PORT_45, 1},
		GPIO, // {GPIO_362, PORT_45, 2},
		GPIO, // {GPIO_363, PORT_45, 3},
		RESERVED, // {GPIO_364, PORT_45, 4},
		RESERVED, // {GPIO_365, PORT_45, 5},
		RESERVED, // {GPIO_366, PORT_45, 6},
		RESERVED  // {GPIO_367, PORT_45, 7}
#endif
#endif
};

unsigned long g_GPIOPortBaseOffset[MAX_GPIO_PORTS] = {
		0x00,
		0x10,
		0x20,
		0x30,
		0x40,
		0x50,
		0x60,
		0x70,
		0x80,
		0x90,
		0xA0,
		0xB0,
#if defined(SOC_PILOT_IV) || defined(SOC_PILOT_III)
		0xC0,
		0xD0,
		0xE0, //port 14 Only 5 pins
		0x00, //port 15 doesn't exist
		0x100,//16
		0x110,//17
		0x120,//18
		0x130,//19
		0x140,//20
		0x150,//21
		0x160,//22
		0x170,//23
		0x180,//24
		0x190,//25
		0x1A0,//26
		0x1B0,//27
		0x1C0,//28
		0x1D0,//29
#if defined(SOC_PILOT_IV)
		0x200,//30
		0x210,//31
		0x220,//32
		0x230,//33
		0x240,//34
		0x250,//35
		0x260,//36
		0x270,//37
		0x280,//38
		0x290,//39
		0x300,//40
		0x310,//41
		0x320,//42
		0x330,//43
		0x340,//44
		0x350 //45
#endif
#endif
};
/* ------------------------------------------------------------------------- *
 *  Function prototypes
 * ------------------------------------------------------------------------- */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static int gpio_hw_init (void);
static void gpio_hw_exit (void);
#else
static int __init gpio_hw_init (void);
static void __exit gpio_hw_exit (void);
#endif

static irqreturn_t gpio_intr_handler( int this_irq, void *dev_id);
inline static u8 gpio_read_reg8(uint32_t offset);
inline static void gpio_write_reg8(uint8_t value, uint32_t offset);

static gpio_interrupt_sensor m_intr_sensors[MAX_GPIOS];
static int m_total_intr_sensors = 0;

#define IS_INPUT_PIN(port, pin) (!(gpio_read_reg8((uint32_t) dev->pilot_gpio_base + pin + g_GPIOPortBaseOffset[port]) & GPIO_OUTPUT_ENB))

/* ------------------------------------------------------------------------- *
 * private data
 * ------------------------------------------------------------------------- */


static gpio_dev_t gpio_dev;

static gpio_core_funcs_t *pgpio_core_funcs;

static gpio_hal_operations_t gpio_hw_ops = {
	pilot_readgpio,
	pilot_writegpio,
	pilot_getdir,
	pilot_setdir,
	pilot_getpol,
	pilot_setpol,
	pilot_getpull_up_down,
	pilot_setpull_up_down,
	pilot_regsensorints,
	pilot_unregsensorints,
	pilot_regchassisints,
	pilot_unregchassisints,
	NULL,
	NULL,
	NULL,
	NULL,
	pilot_EnableGpioToggle,
	pilot_set_gpio_property,
	pilot_get_gpio_property,
	pilot_getdebounce,
	pilot_setdebounce,
	pilot_reg_reading_on_ints,
	pilot_get_reading_on_ints,
	pilot_readgpios,
	pilot_writegpios,
    pilot_setgpiosdir,
    pilot_getgpiosdir,
    pilot_getdebounce_clock,
    pilot_setdebounce_clock,
// Function handler for gpio pulse function
    pilot_writegpio_pulsedata,
};

static hw_hal_t hw_hal = {
	.dev_type = EDEV_TYPE_GPIO,
	.owner = THIS_MODULE,
	.devname = GPIO_HW_DEV_NAME,
	.num_instances = MAX_GPIO_HW_PORT,
	.phal_ops = (void *)&gpio_hw_ops
};

struct reading_on_ints {
	unsigned long va_address;
	unsigned char value;
};
struct reading_on_ints reading_on_int_data[MAX_GPIO_IDX];

#define MAX_NUMBER_OF_TOGGLE_INSTANCE 4
#define MAX_DATA_GROUP_INSTANCES 2
static ToggleData GpioToggle_Data[MAX_DATA_GROUP_INSTANCES];
typedef struct
{
		unsigned int RegAddr;
		unsigned char Data;
		unsigned char Mask;
		unsigned char Valid;
		unsigned char Dummy;
}GpioGroup;
typedef struct
{
	GpioGroup Group[MAX_NUMBER_OF_TOGGLE_INSTANCE];
}ToggleGroup;
ToggleGroup GpioToggle_Group[MAX_DATA_GROUP_INSTANCES];

struct timer_list ToggleTimer;
typedef struct
{
	unsigned char Num 			: 1; //< bit[0] current structure instance
	unsigned char Reserved 		: 6; //< bit[1-6] 
	unsigned char TimerRun 	: 1; //< bit[7] first call, timer should be started  

}Instance;
static Instance ToggleInst={0x01};
inline unsigned char SoftGpioToggle_IsTimerRunning(void)
{
	return ToggleInst.TimerRun;
}
inline void SoftGpioToggle_SetTimerRunning(void)
{
	ToggleInst.TimerRun = 1;
}
inline void SoftGpioToggle_IncreaseInst(void)
{
	++ToggleInst.Num;
};
inline unsigned char SoftGpioToggle_GetCurrentInst(void)
{
	return (unsigned char)ToggleInst.Num;
};
inline unsigned char SoftGpioToggle_GetNextInst(void)
{
	Instance a = ToggleInst;
	return (unsigned char)(++a.Num);
};

typedef struct 
{
	unsigned short gpionum;
	unsigned char status;
}INTRBLOCKGPIOS;
static INTRBLOCKGPIOS GPIOArray[32];

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
DEFINE_SPINLOCK(intrblockgpio_lock);
#else
spinlock_t intrblockgpio_lock = SPIN_LOCK_UNLOCKED;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static int gpio_hw_init (void)
#else
static int __init gpio_hw_init (void)
#endif
{
#ifndef CONFIG_SPX_FEATURE_DO_NOT_INITIALIZE_GPIO_PINS
	unsigned char direction = 0;
	int i;
	Gpio_data_t pin_data;
#endif
	int ret,err;
	gpio_dev_t	*dev = &gpio_dev;
	//pgpio_core_funcs = (gpio_core_funcs_t*) kmalloc (sizeof(gpio_core_funcs_t), GFP_KERNEL);
	//if (!pgpio_core_funcs)
		//return -ENOMEM;

	if ((ret = register_hw_hal_module (&hw_hal, (void **)&pgpio_core_funcs)) < 0)
	{
		printk (KERN_ERR "%s: failed to register gpio hal module\n", GPIO_HW_DEV_NAME);
		return ret;
	}
	m_dev_id = ret;

	//gpio = kmalloc(sizeof(gpio_dev_t), GFP_KERNEL);

	//dev->pilot_gpio_base = ioremap_nocache(GPIO_BASE_OFFS, GPIO_SIZE);
	if((dev->pilot_gpio_base = ioremap_nocache(GPIO_BASE_OFFS, GPIO_SIZE))<0)
	{
		printk("failed to map GPIO  IO space %08x-%08x to memory",GPIO_BASE_OFFS, GPIO_BASE_OFFS + GPIO_SIZE - 1);
        	ret = -EIO;
        	goto fail;
	}

	 /* check gpio memory region */
    	if ((err = check_mem_region(GPIO_BASE_OFFS, GPIO_SIZE)) < 0)
	{
		printk("GPIO IO space %08x-%08x already in use (err %d)",GPIO_BASE_OFFS, GPIO_BASE_OFFS + GPIO_SIZE - 1, err);
		ret = -EBUSY;
		goto fail;
    	}

   	 /* request for the memory region */
   	 if (request_mem_region(GPIO_BASE_OFFS, GPIO_SIZE, "gpio")  == NULL)
	{
		printk("IO space %08x-%08x gpio request mem region failed (err %d)",GPIO_BASE_OFFS, GPIO_BASE_OFFS + GPIO_SIZE - 1, err);
 	    	ret = -ENODEV;
		goto fail;
	}

#ifndef CONFIG_SPX_FEATURE_DO_NOT_INITIALIZE_GPIO_PINS
	/* initialize only first 4 ports right now - the reason is that *
 	 * some of the SGPIO lines are directly being manipulated by 	*
 	 * other modules like port80 code in LPC driver and we don't 	*
 	 * want to change any GPIOs there 				*/
	for (i = 0; i < 32; i++) {
		direction = GPIO_DIR_IN;
		pin_data.PinNum = i;
		pin_data.data = direction;

		/* set direction first */
		pilot_setdir((void *) &pin_data);

		if (direction == GPIO_DIR_OUT) {
			pin_data.data = 0;
			pilot_writegpio((void *) &pin_data);
		}
	}
#endif
	/* enable SGPIO */
#ifdef CONFIG_SPX_FEATURE_GLOBAL_SGPIO_SIGNAL_INVERTED
	// inverted signal
	gpio_write_reg8(0x86, (uint32_t) dev->pilot_gpio_base + GPIO_SINF_CTRL_REG_OFFSET);
#ifdef SOC_PILOT_IV
	gpio_write_reg8(0x86, (uint32_t) dev->pilot_gpio_base + GPIO_SG2SIC_OFFSET);
#endif
#else
	// normal signal
	gpio_write_reg8(0x07, (uint32_t) dev->pilot_gpio_base + GPIO_SINF_CTRL_REG_OFFSET);
#ifdef SOC_PILOT_IV
	gpio_write_reg8(0x06, (uint32_t) dev->pilot_gpio_base + GPIO_SG2SIC_OFFSET);
#endif
#endif

	memset (&reading_on_int_data[0], 0, sizeof (struct reading_on_ints) * MAX_GPIO_IDX);
	printk("GPIO HW Driver, (c) 2009-2016 American Megatrends Inc.\n");

	return 0;
fail:
	gpio_hw_exit();
	return -1;
}


/* Gpio  Device Functions */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static void gpio_hw_exit (void)
#else 
static void __exit gpio_hw_exit (void)
#endif
{
	//disable_gpio_interrupt();
	gpio_dev_t *dev = &gpio_dev;

  if(GPIO_IRQ_ENABLED != 0)
  {
      disable_irq(IRQ_GPIO);
      free_irq(IRQ_GPIO,NULL);
      GPIO_IRQ_ENABLED = 0;
  }
	
	unregister_hw_hal_module (EDEV_TYPE_GPIO, m_dev_id);

	release_mem_region(GPIO_BASE_OFFS, GPIO_SIZE);

	if (dev->pilot_gpio_base)
		iounmap(dev->pilot_gpio_base);
	dev->pilot_gpio_base = NULL;
	

	if(SoftGpioToggle_IsTimerRunning())
	{
		del_timer (&ToggleTimer); 
	}


	return;
}


/*
 * pilot_readgpio
 * Reads a gpio pin
 */
int pilot_readgpio (void* gpin_data  )
{
	int     port;
	uint8_t tpin;
	volatile uint32_t  Addr;
	volatile uint8_t  Value;
	Gpio_data_t *gpindata = gpin_data;
	gpio_dev_t	*dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] == RESERVED) ) return -1;

    /* Get the register Addr  */
    port = gpindata->PinNum / 8;
    tpin = gpindata->PinNum % 8;

	if(gpindata->PinNum <= 31) {
		/* check is it input pin */
		if (IS_INPUT_PIN(port, tpin))
			Addr = (uint32_t) dev->pilot_gpio_base + GPIO_GPDI_OFFSET + g_GPIOPortBaseOffset[port];
		else
			Addr = (uint32_t) dev->pilot_gpio_base + GPIO_GPDO_OFFSET + g_GPIOPortBaseOffset[port];
	}
	else {
		Addr = (uint32_t) dev->pilot_gpio_base + GPIO_GPDI_OFFSET + g_GPIOPortBaseOffset[port];
	}

	/* Get the particular bit */
	Value = gpio_read_reg8 ( Addr );
	gpindata->data = (Value & ( 0x01 << tpin ))?1:0;
	//printk ("data is :%d\n", gpindata->data);

	return 0;
}

/*
 * pilot_readgpios
 * Reads from a set of gpio pins provided in an array
 */
int pilot_readgpios (void* gpio_list_array, unsigned int count)
{
    gpio_list_data_info     *gpindata=(gpio_list_data_info*)gpio_list_array;
    int  cnt;
	int rStatus = 0;

	for (cnt=0; cnt < count; cnt++, gpindata++)
	{
		if (pilot_readgpio (gpindata) < 0)
		{
			gpindata->data = -1;
			rStatus = -1;
		}
	}

	return rStatus;
}


/*
 * pilot_writegpio
 * Writes to a gpio pin
 */
int pilot_writegpio (void *gpin_data  )
{
	int     port;
	uint8_t tpin;
	volatile uint32_t  Addr;
	volatile uint8_t  Value;
	Gpio_data_t *gpindata = gpin_data;
	gpio_dev_t	*dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] == RESERVED) ) return -1;

    /* Get the register Addr  */
    port = gpindata->PinNum / 8;
    tpin = gpindata->PinNum % 8;


	Addr = (uint32_t) dev->pilot_gpio_base + GPIO_GPDO_OFFSET + g_GPIOPortBaseOffset[port];

	Value = gpio_read_reg8 ( Addr );

	if ( gpindata->data )
		Value = Value | ( 0x01 << tpin );
	else
		Value = Value & ~ ( 0x01 << tpin );

	gpio_write_reg8 ( Value, Addr );
	return 0;
}

/*
 * pilot_writegpios
 * Writes to a set of gpio pins provided in an array
 */
int pilot_writegpios (void* gpio_list_array, unsigned int count)
{
        gpio_list_data_info     *gpindata=(gpio_list_data_info*)gpio_list_array;
        int  cnt;
	int rStatus = 0;

	for (cnt=0; cnt < count; cnt++, gpindata++)
	{
		if (pilot_writegpio (gpindata) < 0)
		{
			gpindata->data = -1;
			rStatus = -1;
		}
	}

	return rStatus;
}

/*
 * pilot_getgpiosdir
 * Gets the direction of a set of gpio pins provided in an array
 */
int pilot_getgpiosdir (void* gpio_list_array, unsigned int count)
{
        gpio_list_data_info     *gpindata=(gpio_list_data_info*)gpio_list_array;
        int  cnt;
        int rStatus = 0;

        for (cnt=0; cnt < count; cnt++, gpindata++)
        {
                if (pilot_getdir (gpindata) < 0)
                {
                        gpindata->data = -1;
                        rStatus = -1;
                }
        }

        return rStatus;
}

/*
 * pilot_setgpiosdir
 * Sets the direction of a set of gpio pins provided in an array
 */
int pilot_setgpiosdir (void* gpio_list_array, unsigned int count)
{
        gpio_list_data_info     *gpindata=(gpio_list_data_info*)gpio_list_array;
        int  cnt;
        int rStatus = 0;

        for (cnt=0; cnt < count; cnt++, gpindata++)
        {
                if (pilot_setdir (gpindata) < 0)
                {
                        gpindata->data = -1;
                        rStatus = -1;
                }
        }

        return rStatus;
}

// Handler to make to gpio a pulse of the data line.
/*
 * pilot_writegpio
 * Writes to a gpio pin
 */
int pilot_writegpio_pulsedata (void *gpin_data  )
{
    int port;
    uint8_t tpin;
    volatile uint32_t  Addr;
    volatile uint8_t  Value;
    Gpio_data_t *gpindata = gpin_data;
    gpio_dev_t  *dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] != GPIO) ) return -1;

    /* Get the register Addr  */
    port = gpindata->PinNum / 8;
    tpin = gpindata->PinNum % 8;

    Addr = (uint32_t) dev->pilot_gpio_base + GPIO_GPDO_OFFSET + g_GPIOPortBaseOffset[port];
    Value = gpio_read_reg8 ( Addr );

    if ( gpindata->data )
        Value = Value | ( 0x01 << tpin );
    else
        Value = Value & ~ ( 0x01 << tpin );

    gpio_write_reg8 ( Value, Addr );

    udelay(1);

    if ( 0 == gpindata->data )
        Value = Value | ( 0x01 << tpin );
    else
        Value = Value & ~ ( 0x01 << tpin );

    gpio_write_reg8 ( Value, Addr );

    return 0;
}

/*
 * pilot_getdir
 * Get direction of the pin
 */
int pilot_getdir (void *gpin_data)
{
	int     port;
	uint8_t tpin;
	volatile uint32_t  Addr;
	volatile uint8_t  Value;
	Gpio_data_t *gpindata = gpin_data;
	gpio_dev_t	*dev = &gpio_dev;

	if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] != GPIO) ) return -1;

	/* Get the register Addr  */
	port = gpindata->PinNum / 8;
	tpin = gpindata->PinNum % 8;

	Addr = (uint32_t) dev->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port];
printk("dev->pilot_gpio_base=%x\n",(unsigned int)dev->pilot_gpio_base);
	/* Get the Pin dir */
	Value  = gpio_read_reg8 ( Addr );
	gpindata->data = Value & GPIO_OUTPUT_ENB;

	//printk ("direction is :%d\n", gpindata->data);
	return 0;
}



/*
 * pilot_setdir
 * Set direction of the pin
 */
int pilot_setdir (void *gpin_data)
{
	int     port;
	uint8_t tpin;
	volatile uint32_t  Addr;
	volatile uint8_t  Value;
	Gpio_data_t *gpindata = gpin_data;
	gpio_dev_t	*dev = &gpio_dev;

	if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] != GPIO) ) return -1;

	/* Get the register Addr  */
	port = gpindata->PinNum / 8;
	tpin = gpindata->PinNum % 8;
	
	Addr = (uint32_t) dev->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port];

	Value = gpio_read_reg8 ( Addr );

	if ( gpindata->data == GPIO_DIR_OUT )
		Value = Value | GPIO_OUTPUT_ENB;
	else if ( gpindata->data == GPIO_DIR_IN )
		Value = Value & ~GPIO_OUTPUT_ENB;

	gpio_write_reg8 ( Value, Addr );

	return 0;
}

/*
 * pilot_setpol
 * Set polarity of the pin
 */
int pilot_setpol (void *gpin_data)
{
    int port;
	uint8_t tpin;
    volatile uint32_t  Addr;
    volatile uint8_t  Value, gpimen_pilot_value;
    Gpio_data_t *gpindata = gpin_data;
    gpio_dev_t  *dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] == RESERVED) ) return -1;

	/* Get the register Addr  */
	port = gpindata->PinNum / 8;
	tpin = gpindata->PinNum % 8;

    
    Addr = (uint32_t) dev->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port];

    Value = gpio_read_reg8 ( Addr );

    if ( gpindata->data == GPIO_POL_HIGH )
        Value = Value | GPIO_EVENT_POLARITY;
    else if ( gpindata->data == GPIO_POL_LOW )
        Value = Value & ~GPIO_EVENT_POLARITY;

	//disable interrupt
	gpimen_pilot_value = gpio_read_reg8((uint32_t) dev->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[port]);
	gpimen_pilot_value &= ~((0x01 << tpin));
	gpio_write_reg8 (gpimen_pilot_value, (uint32_t) dev->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[port]);
	
	//change setpol
	gpio_write_reg8 ( Value, Addr );
	
	//clear GPIO Event Status register before enabling interrupt by writing 1(R/W1C )  
	gpimen_pilot_value  = gpio_read_reg8((uint32_t) dev->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
	gpimen_pilot_value &= (0x01 << tpin);
	if(gpimen_pilot_value) //clear only if the GPIO Even is present
		gpio_write_reg8(gpimen_pilot_value, (uint32_t) dev->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
	
	//enable interrupt
	gpimen_pilot_value = gpio_read_reg8((uint32_t) dev->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[port]);
	gpimen_pilot_value |= (0x01 << tpin);
	gpio_write_reg8 (gpimen_pilot_value, (uint32_t) dev->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[port]);
    
	return 0;
}

/*
 * pilot_getpol
 * Get polarity of the pin
 */
int pilot_getpol (void *gpin_data)
{
    int port;
	uint8_t tpin;
    volatile uint32_t  Addr;
    volatile uint8_t  Value;
    Gpio_data_t *gpindata = gpin_data;
    gpio_dev_t  *dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] == RESERVED) ) return -1;

	/* Get the register Addr  */
	port = gpindata->PinNum / 8;
	tpin = gpindata->PinNum % 8;


    Addr = (uint32_t) dev->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port];

    /* Get the Pin dir */
    Value  = gpio_read_reg8 ( Addr );
    gpindata->data = (Value & GPIO_EVENT_POLARITY)?1:0;

    //printk ("direction is :%d\n", gpindata->data);
    return 0;
}


/*
 * pilot_setpull_up_down
 * Set pull-up and pull-down control bits of the pin
 */
int pilot_setpull_up_down (void *gpin_data)
{
    int port;
    uint8_t tpin;
    volatile uint32_t  Addr;
    volatile uint8_t  Value;
    Gpio_data_t *gpindata = gpin_data;
    gpio_dev_t  *dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] != GPIO) ) return -1;

    /* Get the register Addr  */
    port = gpindata->PinNum / 8;
    tpin = gpindata->PinNum % 8;
    
    Addr = (uint32_t) dev->pilot_gpio_base+ tpin + g_GPIOPortBaseOffset[port];

    Value = gpio_read_reg8 ( Addr );

    if ( gpindata->data == GPIO_ENABLE_PULL_DOWN )
    {
        Value = (Value & (~GPIO_NO_PULL_UP_DOWN)) | GPIO_ENABLE_INTERNAL_PULL_DOWN;
    }
    else if ( gpindata->data == GPIO_ENABLE_PULL_UP )
    {
        Value = (Value & (~GPIO_NO_PULL_UP_DOWN)) | GPIO_ENABLE_INTERNAL_PULL_UP;
    }
    else if ( gpindata->data == GPIO_DISABLE_PULL_UP_DOWN )
    {
        Value = (Value | GPIO_NO_PULL_UP_DOWN);
    }

    gpio_write_reg8 ( Value, Addr );

    return 0;
}

/*
 * pilotii_getpull_up_down
 * Get pull-up and pull-down control bits of the pin
 */
int pilot_getpull_up_down (void *gpin_data)
{
    int port;
	uint8_t tpin;
    volatile uint32_t  Addr;
    volatile uint8_t  Value;
    Gpio_data_t *gpindata = gpin_data;
    gpio_dev_t  *dev = &gpio_dev;

	if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] != GPIO) ) return -1;

	/* Get the register Addr  */
	port = gpindata->PinNum / 8;
	tpin = gpindata->PinNum % 8;

    Addr = (uint32_t) dev->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port];

    /* Get the Pin dir */
    Value  = gpio_read_reg8 ( Addr );
    Value = Value & GPIO_NO_PULL_UP_DOWN;
    
    if (Value == GPIO_ENABLE_INTERNAL_PULL_DOWN)
    {
        gpindata->data = GPIO_ENABLE_PULL_DOWN;
    }
    else if (Value == GPIO_ENABLE_INTERNAL_PULL_UP)
    {
        gpindata->data = GPIO_ENABLE_PULL_UP;
    }
    else
    {
        gpindata->data = GPIO_DISABLE_PULL_UP_DOWN;
    }

    return 0;
}

/*
 * pilot_getdebounce
 * Get debounce event of the pin
 */
int pilot_getdebounce (void *gpin_data)
{
    int port;
	uint8_t tpin;
    volatile uint32_t  Addr;
    volatile uint8_t  Value;
    Gpio_data_t *gpindata = gpin_data;
    gpio_dev_t  *dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] != GPIO) ) return -1;

    /* Get the register Addr  */
    port = gpindata->PinNum / 8;
    tpin = gpindata->PinNum % 8;

    Addr = (uint32_t) dev->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port];

    /* Get the Pin dir */
    Value  = gpio_read_reg8 ( Addr );
    gpindata->data = (Value & GPIO_EVENT_DEBOUNCE_ENB)?1:0;

return 0;
}


/*
 * pilot_setdebounce
 * Set debounce event of the pin
 */
int pilot_setdebounce (void *gpin_data)
{
    int port;
	uint8_t tpin;
    volatile uint32_t  Addr;
    volatile uint8_t  Value;
    Gpio_data_t *gpindata = gpin_data;
    gpio_dev_t  *dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] != GPIO) ) return -1;

    /* Get the register Addr  */
    port = gpindata->PinNum / 8;
    tpin = gpindata->PinNum % 8;
    
    Addr = (uint32_t) dev->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port];

    Value = gpio_read_reg8 ( Addr );

    if ( gpindata->data == GPIO_ENABLE_DEBOUNCE )
        Value = Value | GPIO_EVENT_DEBOUNCE_ENB;
    else if ( gpindata->data == GPIO_DISABLE_DEBOUNCE )
        Value = Value & ~GPIO_EVENT_DEBOUNCE_ENB;

    gpio_write_reg8 ( Value, Addr );

return 0;
}

/*
 * pilot_getdebounce_clock
 * Get debounce clock of the pin
 */
int pilot_getdebounce_clock (void *gpin_data)
{
	int port;
	uint8_t tpin;
    volatile uint32_t  Addr;
    volatile uint8_t  Value;
    Gpio_data_t *gpindata = gpin_data;
    gpio_dev_t  *dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] != GPIO) ) return -1;

    /* Get the register Addr  */
    port = gpindata->PinNum / 8;
    tpin = gpindata->PinNum % 8;

	if (tpin < 4)
	{
		Addr = (uint32_t) dev->pilot_gpio_base + GPIO_DBC0_OFFSET + g_GPIOPortBaseOffset[port];
	}
	else
	{
		Addr = (uint32_t) dev->pilot_gpio_base + GPIO_DBC1_OFFSET + g_GPIOPortBaseOffset[port];
		tpin -= 4;
	}
	
	Value = gpio_read_reg8 ( Addr );
	gpindata->data = (Value >> (tpin * 2)) & 0x3;
	
return 0;
}


/*
 * pilot_setdebounce_clock
 * Set debounce clock of the pin
 */
int pilot_setdebounce_clock (void *gpin_data)
{
	int     port;
	uint8_t tpin;
	volatile uint32_t  Addr;
	volatile uint8_t  Value;
	Gpio_data_t *gpindata = gpin_data;
	gpio_dev_t	*dev = &gpio_dev;

    if ( (gpin_data == NULL) || (gpindata->PinNum > MAX_GPIO_IDX) || (g_GpioPortPinType[gpindata->PinNum] != GPIO) || ( gpindata->data > 3 )) return -1;

    /* Get the register Addr  */
    port = gpindata->PinNum / 8;
    tpin = gpindata->PinNum % 8;

	if (tpin < 4)
	{
		Addr = (uint32_t) dev->pilot_gpio_base + GPIO_DBC0_OFFSET + g_GPIOPortBaseOffset[port];
	}
	else
	{
		Addr = (uint32_t) dev->pilot_gpio_base + GPIO_DBC1_OFFSET + g_GPIOPortBaseOffset[port];
		tpin -= 4;
	}
	
	Value = gpio_read_reg8 ( Addr );
	gpindata->data = (gpindata->data & 0x3) << (tpin * 2);
	
	Value = Value & (~(0x3 << (tpin * 2)));
	Value |= gpindata->data;
	
	gpio_write_reg8 ( Value, Addr );
	
return 0;
}


/*
 * gpio_reverse_triggerType
 * Switch the polarity of GPIO pins whose interrupt trigger type registered as GPIO_BOTH_EDGES
 */
static int gpio_reverse_triggerType(int gpio_num)
{
    int port, pin, i=0;
    uint8_t tpin;
    volatile unsigned char trigger_type;
    gpio_dev_t *gpio = &gpio_dev;

    for(i=0; i<m_total_intr_sensors; i++)
    {
        if((m_intr_sensors[i].trigger_type == GPIO_BOTH_EDGES) && (m_intr_sensors[i].gpio_number == gpio_num))
        {
            pin = m_intr_sensors[i].gpio_number;
            port = pin / 8;
            tpin = pin % 8;
            trigger_type = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port]);

            if(trigger_type & GPIO_EVENT_POLARITY)
                trigger_type &= ~GPIO_EVENT_POLARITY;
            else
                trigger_type |= GPIO_EVENT_POLARITY;

            gpio_write_reg8(trigger_type, (uint32_t) gpio->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port]);
        }
    }
    return 0;
}

static void handle_interrupt(unsigned char gpio_cmb_reg_num, unsigned short gpio_cmb_reg_offset)
{
    volatile uint8_t intr_state_pilot_value = 0;
    volatile uint8_t event_state_pilot_value = 0;
    uint8_t current_pilot_intrs = 0;
    uint8_t value = 0;
    uint32_t i = 0, j = 0,k=0;
    gpio_dev_t *gpio = &gpio_dev;
    Gpio_data_t gpin_data;
    int intr_type = 0;
    unsigned char icr = 0,pwrgdstatus = 0xff,flag = 0;
    unsigned short gpionum = 0;
    unsigned long  flags;
    /* check which port has the interrupt */
    intr_state_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + gpio_cmb_reg_offset);

/*    if( PORT_29 < intr_state_pilot_value) && (intr_state_pilot_value > PORT_40)
event_state_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + (i + 0x2 + gpio_cmb_reg_num*8) * 0x10);
    else if ( PORT_39 < intr_state_pilot_value)
event_state_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + (i + gpio_cmb_reg_num*8) * 0x10);
    else
*/
    /* Handle the interrupts */
    for(i = 0; i < 8; i++)
    {
        if(intr_state_pilot_value & 0x01)
        {
            /* check which pin in this port caused an interrupt */
            // port vaule is i value
//            event_state_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + (i + gpio_cmb_reg_num*8) * 0x10);
            event_state_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[i+(gpio_cmb_reg_num*8)]);

            for (j = 0; j < 8; j++)
            {
                gpionum = (((gpio_cmb_reg_num * 8) + i) * 8) + j;
                if (g_GpioPortPinType[gpionum] == RESERVED)  continue;  // The GPIO number may be RESERVED
                if (event_state_pilot_value & 0x01)
                {
                    value = 1 << j;
                    current_pilot_intrs |= value;
                    //gpionum = (i + gpio_cmb_reg_num*8) * 8 + j;
                    spin_lock_irqsave(&intrblockgpio_lock, flags);
                    for(k = 0; k < sizeof(GPIOArray)/sizeof(INTRBLOCKGPIOS); k++)
                    {
                        if(GPIOArray[k].status == 1)
                            if(GPIOArray[k].gpionum == gpionum)
                            {
                                icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT0_REG));
                                if ((icr & PWRGDSTATE) )
                                {
                                    pwrgdstatus = 1;
                                }
                                else
                                {
                                    pwrgdstatus = 0;
                                }
                                if(pwrgdstatus == 0)
                                {
                                    gpio_reverse_triggerType(gpionum);
                                    flag = 1;
                                    break;
                                }
                            }
                    }
                    spin_unlock_irqrestore(&intrblockgpio_lock, flags);
                    if(flag == 0)
                    {
                        //common module's callback interrupt handler
                        if (reading_on_int_data[gpionum].va_address)
                            reading_on_int_data[gpionum].value = *((unsigned char *)(reading_on_int_data[gpionum].va_address));
                       
			gpin_data.PinNum = gpionum; 
			gpio_hw_ops.readgpio(&gpin_data);
			intr_type = pgpio_core_funcs->process_gpio_intr(gpionum, 0, gpin_data.data, NOT_LEVEL_TRIGGER);
                        pgpio_core_funcs->wakeup_intr_queue(intr_type);
                        gpio_reverse_triggerType(gpionum);
                    }
                    else
                    {
                        flag = 0;
                    }
                }
                event_state_pilot_value >>= 1;
            }
            gpio_write_reg8(current_pilot_intrs, (uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET +  g_GPIOPortBaseOffset[i+(gpio_cmb_reg_num*8)]);
        }
        intr_state_pilot_value >>= 1;
    }
    return ;
}

static irqreturn_t gpio_intr_handler( int this_irq, void *dev_id)
{
    /* Determine the reason for this interrupt */
    if(this_irq == IRQ_GPIO)
    {
        handle_interrupt(0, GPIO_CMB_INTRSTAT_GPISR0_OFFSET);
        handle_interrupt(1, GPIO_CMB_INTRSTAT_GPISR1_OFFSET);
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV)
        handle_interrupt(2, GPIO_CMB_INTRSTAT_GPISR2_OFFSET);
        handle_interrupt(3, GPIO_CMB_INTRSTAT_GPISR3_OFFSET);
#endif
#ifdef SOC_PILOT_IV
        handle_interrupt(4, GPIO_SG2ISR0_OFFSET);
        handle_interrupt(5, GPIO_SG2ISR1_OFFSET);
#endif
    }
#if 0 // remove needless printk 
    else
        printk("Spurious GPIO interrupt\n");
#endif
    return( IRQ_HANDLED );
}


int pilot_regsensorints ( void* gpin_data, uint32_t total_interrupt_sensors, void* sensor_itr)
{
	int     port;
	uint8_t tpin;
	gpio_dev_t *gpio = &gpio_dev;
	gpio_interrupt_sensor* intr_sensors = (gpio_interrupt_sensor *) sensor_itr;
	uint32_t i = 0, count = 0;
	volatile uint8_t gpcfg_pilot_value;
	volatile uint8_t gpimen_pilot_value;
	int initial_polarity = -1, final_polarity = -1;
	Gpio_data_t gpindata = {0};
	volatile uint8_t  gpevnt_status;
	int val = -1;
	int check_for_status = 0;


//	printk("total sensors = %d\n",total_interrupt_sensors);
	for(i = 0; i < total_interrupt_sensors; i++)
	{
		port = intr_sensors[i].gpio_number / 8;
		tpin = intr_sensors[i].gpio_number % 8;

		gpcfg_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port]);
		initial_polarity = (gpcfg_pilot_value  >> 4) & 1;

		/* Set the interrupt trigger method/type */
		if(intr_sensors[i].trigger_method == GPIO_EDGE)
		{
			gpcfg_pilot_value &= ~GPIO_EVENT_TYPE;
			if(intr_sensors[i].trigger_type == GPIO_FALLING_EDGE)
				gpcfg_pilot_value &= ~GPIO_EVENT_POLARITY;
			else if(intr_sensors[i].trigger_type == GPIO_RISING_EDGE)
				gpcfg_pilot_value |= GPIO_EVENT_POLARITY;
			else if(intr_sensors[i].trigger_type == GPIO_BOTH_EDGES)
			{
				gpindata.PinNum = intr_sensors[i].gpio_number;
				pilot_readgpio(&gpindata);

				if(gpindata.data == 0) //If current polarity is Rising Edge
					gpcfg_pilot_value |= GPIO_EVENT_POLARITY;
				else
					gpcfg_pilot_value &= ~GPIO_EVENT_POLARITY;
			}
			else
				printk("Wrong edge trigger type for sensor int\n");
			final_polarity = (gpcfg_pilot_value  >> 4) & 1;
		}
		else if(intr_sensors[i].trigger_method == GPIO_LEVEL)
		{
			gpcfg_pilot_value |= GPIO_EVENT_TYPE;
			if(intr_sensors[i].trigger_type == GPIO_HIGH_LEVEL)
				gpcfg_pilot_value |= GPIO_EVENT_POLARITY;
			else if(intr_sensors[i].trigger_type == GPIO_LOW_LEVEL)
				gpcfg_pilot_value &= ~GPIO_EVENT_POLARITY;
			else
				printk("Wrong level trigger type for sensor int\n");
		}
		else
			printk("Wrong trigger type for sensor int\n");

		gpcfg_pilot_value |= GPIO_EVENT_TO_IRQ_ENB;
		/* Workaround for the Pilot3 hardware BUG */
		/* If there is a change in Event Polarity */
		if(final_polarity != initial_polarity)
		{
			gpindata.PinNum = intr_sensors[i].gpio_number;
			val = pilot_readgpio(&gpindata);
			if(val == -1)
				printk("read_pin_val returned -1 !!\n");
			/* If final polarity matches pin value */
			if(final_polarity == gpindata.data)
				check_for_status = 1;
		}

		if(check_for_status)
		{
			printk("Need to check Inter status\n");
			disable_irq(IRQ_GPIO);
			/* Write back the GPCFG register */
			gpio_write_reg8 (gpcfg_pilot_value, (uint32_t) gpio->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port]);
		}
		else
		{
			/* Write back the GPCFG register */
			gpio_write_reg8 (gpcfg_pilot_value, (uint32_t) gpio->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port]);
			continue; // don't end after processing the first sensor, so continue
		}

		while(1)
		{
			gpevnt_status = gpio_read_reg8 ( (uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
			if(gpevnt_status & (1 << tpin ))
			{
				gpevnt_status = (1 << tpin );
				gpio_write_reg8 (gpevnt_status, (uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
				break;
			}
			if(count > 50)
			{
				printk("Time out : GPIO Event status didn't get set\n");
				break;
			}
			count++;
			msleep(1);
		}
		enable_irq(IRQ_GPIO);
	}

	if(!GPIO_IRQ_ENABLED)
	{
		if( request_irq(IRQ_GPIO, gpio_intr_handler, 0, NAME, NULL) < 0 )
		{
			printk( KERN_ERR "Request for irq %d failed\n", IRQ_GPIO );
			return( -ENODEV );
		}
		else
		{
			GPIO_IRQ_ENABLED = 1;
		}
	}

	/* enable the interrupts now */
	for(i = 0; i < total_interrupt_sensors; i++)
	{
		port = intr_sensors[i].gpio_number / 8;
		tpin = intr_sensors[i].gpio_number % 8;

        memcpy(&m_intr_sensors[m_total_intr_sensors + i],&intr_sensors[i],sizeof(gpio_interrupt_sensor));
        
		//clear GPIO Event Status register before enabling interrupt by writing 1(R/W1C )  
		gpimen_pilot_value  = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
		gpimen_pilot_value &= (0x01 << tpin);
		if(gpimen_pilot_value) //clear only if the GPIO Even is present
			gpio_write_reg8(gpimen_pilot_value, (uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
		//enable interrupt
		gpimen_pilot_value 	= gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[port]);

		gpimen_pilot_value |= (0x01 << tpin);

		gpio_write_reg8 (gpimen_pilot_value, (uint32_t) gpio->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[port]);
	}
    m_total_intr_sensors += total_interrupt_sensors;
	return 0;
}

int pilot_unregsensorints (  void* gpin_data)
{
	gpio_dev_t *gpio = &gpio_dev;
	uint8_t value = 0, read_value = 0;
	int bank = 0;
	int tpin = 0;

	if(gpin_data == NULL)
	    return 0;
	
	bank = (((Gpio_data_t*)gpin_data)->PinNum / 8);
	tpin = (((Gpio_data_t*)gpin_data)->PinNum % 32);
    
    if (g_GpioPortPinType[((Gpio_data_t*)gpin_data)->PinNum] == RESERVED)
        value = 0x1 << tpin;

    
    if (value != 0xFF)		// The port should be SGPIO or Reserved, so ignore
    {
        read_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[bank]);
        gpio_write_reg8(read_value & value, (uint32_t) gpio->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[bank]);
    }


	return 0;
}



/********************** Chassis Interrupts ***********************/
int pilot_regchassisints  ( void* gpin_data, uint32_t total_chassis_interrupts,  void* chassis_itr )
{
	int     port;
	uint8_t tpin;
	gpio_dev_t *gpio = &gpio_dev;
	gpio_interrupt_sensor* intr_chassis = (gpio_interrupt_sensor *) chassis_itr;
	uint32_t i = 0, count = 0;
	volatile uint8_t gpcfg_pilot_value;
	volatile uint8_t gpimen_pilot_value;
	int initial_polarity = -1, final_polarity = -1;
	volatile uint8_t  gpevnt_status;
	int val = -1;
	int check_for_status = 0;
        Gpio_data_t gpindata = {0};

	//printk("total chassis interrupts = %d\n",total_chassis_interrupts);
	for(i = 0; i < total_chassis_interrupts; i++)
	{
		//pin= intr_chassis[i].gpio_number;
		port = intr_chassis[i].gpio_number / 8;
		tpin = intr_chassis[i].gpio_number % 8;

		gpcfg_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port]);
		initial_polarity = (gpcfg_pilot_value  >> 4) & 1;

		/* Set the interrupt trigger method/type */
		if(intr_chassis[i].trigger_method == GPIO_EDGE)
		{
			gpcfg_pilot_value &= ~GPIO_EVENT_TYPE;
			if(intr_chassis[i].trigger_type == GPIO_FALLING_EDGE)
				gpcfg_pilot_value &= ~GPIO_EVENT_POLARITY;
			else if(intr_chassis[i].trigger_type == GPIO_RISING_EDGE)
				gpcfg_pilot_value |= GPIO_EVENT_POLARITY;
			else
				printk("Wrong edge trigger type for chassis int\n");
			final_polarity = (gpcfg_pilot_value  >> 4) & 1;
		}
		else if(intr_chassis[i].trigger_method == GPIO_LEVEL)
		{
			gpcfg_pilot_value |= GPIO_EVENT_TYPE;
			if(intr_chassis[i].trigger_type == GPIO_HIGH_LEVEL)
				gpcfg_pilot_value |= GPIO_EVENT_POLARITY;
			else if(intr_chassis[i].trigger_type == GPIO_LOW_LEVEL)
				gpcfg_pilot_value &= ~GPIO_EVENT_POLARITY;
			else
				printk("Wrong level trigger type for chassis int\n");
		}
		else
			printk("Wrong trigger type for sensor int\n");

		gpcfg_pilot_value |= GPIO_EVENT_TO_IRQ_ENB;
		/* Workaround for the Pilot3 hardware BUG */
		/* If there is a change in Event Polarity */
		if(final_polarity != initial_polarity)
		{
			gpindata.PinNum = intr_chassis[i].gpio_number;
			val = pilot_readgpio(&gpindata);
			if(val == -1)
				printk("read_pin_val returned -1 !!\n");
			/* If final polarity matches pin value */
			if(final_polarity == gpindata.data)
				check_for_status = 1;
		}

		if(check_for_status)
		{
			printk("Need to check Inter status in regchassisintr\n");
			disable_irq(IRQ_GPIO);
			/* Write back the GPCFG register */
			gpio_write_reg8 (gpcfg_pilot_value, (uint32_t) gpio->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port]);
		}
		else
		{
			/* Write back the GPCFG register */
			gpio_write_reg8 (gpcfg_pilot_value, (uint32_t) gpio->pilot_gpio_base + tpin + g_GPIOPortBaseOffset[port]);
			goto finish_check;
		}

		while(1)
		{
			gpevnt_status = gpio_read_reg8 ( (uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
			if(gpevnt_status & (1 << tpin ))
			{
				gpevnt_status = (1 << tpin );
				gpio_write_reg8 (gpevnt_status, (uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
				break;
			}
			if(count > 50)
			{
				printk("Time out : GPIO Event status didn't get set\n");
				break;
			}
			count++;
			msleep(1);
		}
		enable_irq(IRQ_GPIO);
	}

finish_check:
	if(!GPIO_IRQ_ENABLED)
	{
		if( request_irq(IRQ_GPIO, gpio_intr_handler, 0, NAME, NULL) < 0 )
		{
			printk( KERN_ERR "Request for irq %d failed\n", IRQ_GPIO );
			return( -ENODEV );
		}
		else
		{
			GPIO_IRQ_ENABLED = 1;
		}
	}

	/* enable the interrupts now */
	for(i = 0; i < total_chassis_interrupts; i++)
	{
		port = intr_chassis[i].gpio_number / 8;
		tpin = intr_chassis[i].gpio_number % 8;

		
		//clear GPIO Event Status register before enabling interrupt by writing 1(R/W1C )  
		gpimen_pilot_value  = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
		gpimen_pilot_value &= (0x01 << tpin);
		if(gpimen_pilot_value) //clear only if the GPIO Even is present
			gpio_write_reg8(gpimen_pilot_value, (uint32_t) gpio->pilot_gpio_base + GPIO_EVST_OFFSET + g_GPIOPortBaseOffset[port]);
		//enable interrupt
		gpimen_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[port]);

		gpimen_pilot_value |= (0x01 << tpin);

		gpio_write_reg8 (gpimen_pilot_value, (uint32_t) gpio->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[port]);
	}

	return 0;
}

int pilot_unregchassisints (  void* gpin_data)
{
	gpio_dev_t *gpio = &gpio_dev;
    int bank = 0;
    
    if(gpin_data == NULL)//Null check
        return 0;
    
    bank = (((Gpio_data_t*)gpin_data)->PinNum / 8);

	/*Exclude the handling of unimplemented GPIO pins in the GPIO driver */
	gpio_write_reg8(0,(uint32_t) gpio->pilot_gpio_base + GPIO_IMEN_OFFSET + g_GPIOPortBaseOffset[bank]);

	return 0;
}

static unsigned short int toggleSampling = 1;
static void GpioToggleTimerFunc(unsigned long ptr)
{
	unsigned char loop;
	unsigned char Inst = SoftGpioToggle_GetCurrentInst();
	unsigned long long int  curJiffies = get_jiffies_64();
	GpioGroup *pGroup = GpioToggle_Group[Inst].Group;
	ToggleData *pData = &GpioToggle_Data[Inst];
	
    	for(loop=0; loop<MAX_NUMBER_OF_TOGGLE_INSTANCE; ++loop)
	{        
		if ( pData->Gpio[loop].Valid == 1)
        {
			if( (pData->Gpio[loop].enableTimeOut) && //is active but already time out
				(curJiffies > pData->Gpio[loop].timeout) )	{ 
				
				if(pData->Gpio[loop].defaultOff)
					pGroup[pData->Gpio[loop].Port].Data |= (1 << pData->Gpio[loop].Number);
				else
					pGroup[pData->Gpio[loop].Port].Data &= (~(1 << pData->Gpio[loop].Number));
			}
			else {
				if( pData->Gpio[loop].Pattern & toggleSampling )
					pGroup[pData->Gpio[loop].Port].Data |= (1 << pData->Gpio[loop].Number);
				else
					pGroup[pData->Gpio[loop].Port].Data &= (~(1 << pData->Gpio[loop].Number));
			}
		}
	}
	for(loop=0; loop<MAX_NUMBER_OF_TOGGLE_INSTANCE; ++loop)
	{
		if(pGroup[loop].Valid == 1)
        	{
			unsigned char value;
			value = gpio_read_reg8(pGroup[loop].RegAddr);
			if( (value & pGroup[loop].Mask) != pGroup[loop].Data)
            		{
				value &= (~pGroup[loop].Mask);
				value |= pGroup[loop].Data;
				gpio_write_reg8(value,pGroup[loop].RegAddr);
             		}
		}
	}
                
	toggleSampling = ( toggleSampling == pData->Conf.CyclePeriod )?(1):(toggleSampling << 1);
	ToggleTimer.expires = jiffies + (pData->ToggleCycle / 10);
	add_timer(&ToggleTimer);
}
int pilot_EnableGpioToggle( void* paramToggleData)
{
	unsigned char loop=0,GrpIndex=0;
	unsigned char Inst = SoftGpioToggle_GetNextInst();
	unsigned int GpioPhyAddr;
	GpioGroup *pGroup = GpioToggle_Group[Inst].Group;
	ToggleData *pData = &GpioToggle_Data[Inst];
	gpio_dev_t	*dev = &gpio_dev;
	
	//printk("inst: %x, size: %d size: %d\n",Inst,sizeof(ToggleData), sizeof(ToggleGroup));
	
	memcpy(pData,paramToggleData,sizeof(ToggleData));
	//SoftGpioToggle_ShowData(pData);
	
	//optimization: put the GPIO from same port into same group
	memset(pGroup, 0x00,sizeof(ToggleGroup));
	//SoftGpioToggle_ShowGroup(pGroup);
	pGroup[0].RegAddr = (uint32_t) dev->pilot_gpio_base + GPIO_GPDO_OFFSET + g_GPIOPortBaseOffset[pData->Gpio[0].Port];
	pGroup[0].Mask |= (1 << pData->Gpio[loop].Number);
	pGroup[0].Data = 0;
	pGroup[0].Valid = 1;
	pData->Gpio[0].Port = GrpIndex; //GPIO port is only needed for Reg Address
	
	for(loop=1; loop<MAX_NUMBER_OF_TOGGLE_INSTANCE; ++loop)
	{
		//printk("loop: %d, GrpIndex: %d\n",loop,GrpIndex);
		GpioPhyAddr = (uint32_t) dev->pilot_gpio_base + GPIO_GPDO_OFFSET + g_GPIOPortBaseOffset[pData->Gpio[loop].Port];
		if( pGroup[GrpIndex].RegAddr != GpioPhyAddr ) 
		{
			++GrpIndex;
			if(GrpIndex >= MAX_NUMBER_OF_TOGGLE_INSTANCE)
				return -1;
			pGroup[GrpIndex].RegAddr = GpioPhyAddr;
			pGroup[GrpIndex].Mask |= (1 << pData->Gpio[loop].Number);
			pGroup[GrpIndex].Data = 0;
			pGroup[GrpIndex].Valid = 1;
			pData->Gpio[loop].Port = GrpIndex; //GPIO port is only needed for Reg Address
		}
		else
		{
			pGroup[GrpIndex].Mask |= (1 << pData->Gpio[loop].Number);
			pData->Gpio[loop].Port = GrpIndex; //GPIO port is only needed for Reg Address
		}
	}
	
	//SoftGpioToggle_ShowData(pData);
	//SoftGpioToggle_ShowGroup(pGroup);
	
	if(!SoftGpioToggle_IsTimerRunning())
	{
		//start timer
		init_timer(&ToggleTimer);
		ToggleTimer.expires = jiffies + (pData->ToggleCycle / 10);
		ToggleTimer.function = GpioToggleTimerFunc;
		ToggleTimer.data = 0;
		add_timer(&ToggleTimer);	
		SoftGpioToggle_SetTimerRunning();
		SoftGpioToggle_IncreaseInst();
	}
	else 
		SoftGpioToggle_IncreaseInst();
		
	return 0;
}

int set_sgpio_data(unsigned short sgpio, unsigned char value)
{
	gpio_dev_t *gpio = &gpio_dev;
	if (SGPIO_1_DATA == sgpio)
		gpio_write_reg8(value, (uint32_t) gpio->pilot_gpio_base + GPIO_SINF_CTRL_REG_OFFSET);
#ifdef SOC_PILOT_IV
	else if (SGPIO_2_DATA == sgpio)
		gpio_write_reg8(value, (uint32_t) gpio->pilot_gpio_base + GPIO_SG2SIC_OFFSET);
#endif
	return 0;
}

int get_sgpio_data(unsigned short sgpio, unsigned char *value)
{
	gpio_dev_t *gpio = &gpio_dev;
	volatile uint8_t  Value = 0;
	if (SGPIO_1_DATA == sgpio)
		Value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_SINF_CTRL_REG_OFFSET);
#ifdef SOC_PILOT_IV
	else if (SGPIO_2_DATA == sgpio)
		Value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + GPIO_SG2SIC_OFFSET);
#endif
	*value = Value & 0xff;
	return 0;
}

/*
* enable_gpio_event
* Enable eventing for specified gpio
*/
int enable_gpio_event(unsigned short gpionum)
{
	unsigned char   port;
	uint8_t tpin;
	gpio_dev_t      *gpio = &gpio_dev;
	volatile uint8_t gpimen_pilot_value;
	if (( gpionum > MAX_GPIO_IDX ) || (g_GpioPortPinType[gpionum] == RESERVED))
		return -1;

	port = gpionum / 8;
	tpin = gpionum % 8;

	gpimen_pilot_value  = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + GPIO_IMEN_OFFSET);
	gpimen_pilot_value |= (0x01 << tpin);
	gpio_write_reg8 (gpimen_pilot_value, (uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + GPIO_IMEN_OFFSET);

	return 0;
}

/*
* disable_gpio_event
* Disable eventing for specified gpio
*/
int disable_gpio_event(unsigned short gpionum)
{
	unsigned char port;
	uint8_t tpin;
	gpio_dev_t      *gpio = &gpio_dev;
	volatile uint8_t gpimen_pilot_value;

	if (( gpionum > MAX_GPIO_IDX ) || (g_GpioPortPinType[gpionum] == RESERVED))
		return -1;

	port = gpionum / 8;
	tpin = gpionum % 8;
	
	gpimen_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + GPIO_IMEN_OFFSET);
	gpimen_pilot_value &= (~(0x01 << tpin));
	gpio_write_reg8 (gpimen_pilot_value, (uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + GPIO_IMEN_OFFSET);

	return 0;
}

/*
* clear_gpio_event
* Clear event status for specified gpio
*/
int clear_gpio_event(unsigned short gpionum)
{
	unsigned char port;
	uint8_t tpin;
	gpio_dev_t      *gpio = &gpio_dev;
	volatile uint8_t event_state_pilot_value = 0;

	if (( gpionum > MAX_GPIO_IDX ) || (g_GpioPortPinType[gpionum] == RESERVED))
		return -1;

	port = gpionum / 8;
	tpin = gpionum % 8;
	
	event_state_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + GPIO_EVST_OFFSET);
	event_state_pilot_value |= (0x01 << tpin);
	gpio_write_reg8(event_state_pilot_value, (uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + GPIO_EVST_OFFSET);

	return 0;
}

/*
 * get_gpio_event
 * Gets the event status for specified gpio
 */
int get_gpio_event(unsigned short gpionum, unsigned char *value)
{
	unsigned char port;
	uint8_t tpin;
	//volatile uint8_t  Val;
	gpio_dev_t      *gpio = &gpio_dev;
	volatile uint8_t event_state_pilot_value = 0;
	
	if (( gpionum > MAX_GPIO_IDX ) || (g_GpioPortPinType[gpionum] == RESERVED))
		return -1;

	port = gpionum / 8;
	tpin = gpionum % 8;

	event_state_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + GPIO_EVST_OFFSET);
	*value=(event_state_pilot_value & ( 0x01 << tpin ))?1:0;

	return 0;
}

/*
  * set_gpio_event_to_irq
  * Gets the event to irq enable status for specified gpio
  */ 
int set_gpio_event_to_irq(unsigned short gpionum, unsigned char value)
{
	volatile uint8_t gpcfg_pilot_value;
	unsigned char port;
	uint8_t tpin;
	gpio_dev_t      *gpio = &gpio_dev;

	if (( gpionum > MAX_GPIO_IDX ) || (g_GpioPortPinType[gpionum] == RESERVED))
		return -1;

	port = gpionum / 8;
	tpin = gpionum % 8;
	
	gpcfg_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + tpin);
	if (value)
		gpcfg_pilot_value = gpcfg_pilot_value | GPIO_EVENT_TO_IRQ_ENB;
	else
		gpcfg_pilot_value = gpcfg_pilot_value & (~GPIO_EVENT_TO_IRQ_ENB);
	gpio_write_reg8 (gpcfg_pilot_value, (uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + tpin); 

	return 0;
}

/*
 * get_gpio_event_to_irq
 * Gets the event to irq enable status for specified gpio
 */ 
int get_gpio_event_to_irq(unsigned short gpionum, unsigned char *value)
{
	volatile uint8_t gpcfg_pilot_value;
	unsigned char port;
	uint8_t tpin;
	gpio_dev_t      *gpio = &gpio_dev;
	
	if (( gpionum > MAX_GPIO_IDX ) || (g_GpioPortPinType[gpionum] == RESERVED))
		return -1;

	port = gpionum / 8;
	tpin = gpionum % 8;
	
	gpcfg_pilot_value = gpio_read_reg8((uint32_t) gpio->pilot_gpio_base + g_GPIOPortBaseOffset[port] + tpin);
	*value = gpcfg_pilot_value & GPIO_EVENT_TO_IRQ_ENB ? 1 : 0;

	return 0;
}

int pilot_set_gpio_property( unsigned short gpionum, unsigned char property, unsigned char value)
{
	int ret = -1;
	unsigned char flag = 0,i;
	switch(property)
	{
		case SGPIO_1_DATA:
			ret = set_sgpio_data(SGPIO_1_DATA,value);
			break;
		case SGPIO_2_DATA:
			ret = set_sgpio_data(SGPIO_2_DATA,value);
			break;
		case SET_GPEVEN:
			if(ENABLE == value)
			{
				ret = enable_gpio_event(gpionum);
			}
			else if(DISABLE == value)
			{
				ret = disable_gpio_event(gpionum);
			}
			else
				printk("Valid SET_GPEVEN data: 0-disable 1-enable \n");
			break;
		case CLEAR_GPEVST:
			ret = clear_gpio_event(gpionum);
			break;
		case REG_INTR_BLOCK_GPIONUM:
			ret = 0; //ret isn't currently used in this case.
			spin_lock(&intrblockgpio_lock);
			for (i= 0; i < sizeof(GPIOArray)/sizeof(INTRBLOCKGPIOS); i++)
			{
					if(GPIOArray[i].status == 1)
					if(GPIOArray[i].gpionum == gpionum)
					{
							flag = 1;
							break;
					}
			}
			if(flag == 1)
			{
					flag = 0;
					spin_unlock(&intrblockgpio_lock);
					break;
			}
			for (i= 0; i < sizeof(GPIOArray)/sizeof(INTRBLOCKGPIOS); i++)
			{
					if(GPIOArray[i].status == 0)
					{
							GPIOArray[i].gpionum = gpionum;
							GPIOArray[i].status = 1;
							break;
					}
			}
			spin_unlock(&intrblockgpio_lock);
			break;
		case DEREG_INTR_BLOCK_GPIONUM:
			ret = 0; //ret isn't currently used in this case.
			spin_lock(&intrblockgpio_lock);
			for(i = 0; i < sizeof(GPIOArray)/sizeof(INTRBLOCKGPIOS); i++)
			{
					if(GPIOArray[i].status == 1)
					if(GPIOArray[i].gpionum == gpionum)
					{
							GPIOArray[i].status = 0;
							break;
					}
			}
			spin_unlock(&intrblockgpio_lock);
			break;
		case EVENT_TO_IRQ_ENABLE:
			ret = set_gpio_event_to_irq(gpionum, value);
			break;
		default :
			return -1;
	}
	return ret;
}

int pilot_get_gpio_property( unsigned short gpionum, unsigned char property, unsigned char *value)
{
	int ret = -1;
	switch(property)
	{
		case SGPIO_1_DATA:
			ret = get_sgpio_data(SGPIO_1_DATA,value);
			break;
		case SGPIO_2_DATA:
			ret = get_sgpio_data(SGPIO_2_DATA,value);
			break;
		case GET_GPIO_TYPE:
			if ( gpionum > MAX_GPIO_IDX )
				return -1;
			
			if (g_GpioPortPinType[gpionum] == SGPIO)
			{
				*value = 2;     //GPIONUM belongs to serial gpio(SGPIO)
				ret = 0;
			}
			else if (g_GpioPortPinType[gpionum] == GPIO)
			{	
				*value = 1;     //GPIONUM belongs to  normal gpio
				ret = 0; 
			}
			else
			{
				printk("Invalid GPIONUM given for get_gpio_type command\n");    
				return -1;
			}
			break;
		case GET_GPEVST_DATA:
			ret = get_gpio_event(gpionum, value);
			break;
		case EVENT_TO_IRQ_ENABLE:
			ret = get_gpio_event_to_irq(gpionum, value);
			break;

		default :
			return -1;
	}
	return ret;
}

int pilot_reg_reading_on_ints (unsigned short gpionum, unsigned long va_address)
{
	reading_on_int_data[gpionum].va_address = va_address;
	reading_on_int_data[gpionum].value = 0;
	return 0;
}

int pilot_get_reading_on_ints (unsigned short gpionum, unsigned char *value)
{
	if (reading_on_int_data[gpionum].va_address)
		(*value) = reading_on_int_data[gpionum].value;
	
	return 0;
}

/*
 * gpio_read_reg8
 * reads a byte from the offset
 */
inline static u8 gpio_read_reg8(uint32_t offset)
{
    return (ioread8((void __iomem*)offset));
}

/*
 * gpio_write_reg8
 * writes a byte to the offset
 *
 */

inline static void gpio_write_reg8(uint8_t value, uint32_t offset)
{
    iowrite8(value, (void __iomem*)offset);
}

MODULE_AUTHOR("American Megatrends");
#if defined(SOC_PILOT_IV)
MODULE_DESCRIPTION("pilot-iv GPIO driver");
#elif defined(SOC_PILOT_III)
MODULE_DESCRIPTION("pilot-iii GPIO driver");
#elif defined(SOC_PILOT_II)
MODULE_DESCRIPTION("pilot-ii GPIO driver");
#endif
MODULE_LICENSE("GPL");

module_init(gpio_hw_init);
module_exit(gpio_hw_exit);

