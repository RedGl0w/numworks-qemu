/*
 * STM32F4XX SoC
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HW_ARM_STM32F4XX_SOC_H
#define HW_ARM_STM32F4XX_SOC_H

#include "hw/gpio/stm32f2xx_gpio.h"
#include "hw/misc/stm32f2xx_rcc.h"
#include "hw/misc/stm32f2xx_crc.h"
#include "hw/misc/stm32f2xx_syscfg.h"
#include "hw/misc/stm32f2xx_usb_otg_fs.h"
#include "hw/timer/stm32f2xx_timer.h"
#include "hw/char/stm32f2xx_usart.h"
#include "hw/adc/stm32f2xx_adc.h"
#include "hw/misc/stm32f4xx_exti.h"
#include "hw/or-irq.h"
#include "hw/ssi/stm32f2xx_spi.h"
#include "hw/arm/armv7m.h"
#include "qemu/units.h"
#include "qom/object.h"

#define TYPE_STM32F4XX_SOC "stm32f4xx-soc"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F4XXState, STM32F4XX_SOC)

#define VARIANT_STM32F405_SOC "stm32f405-soc"
#define STM32F405_SOC_FLASH_SIZE (1 * MiB)
#define STM32F405_SOC_RAM_SIZE (192 * KiB)

#define VARIANT_STM32F412_SOC "stm32f412-soc"
#define STM32F412_SOC_FLASH_SIZE (1 * MiB)
#define STM32F412_SOC_RAM_SIZE (256 * KiB)

#define STM_NUM_GPIOS 9
#define STM_NUM_USARTS 7
#define STM_NUM_TIMERS 4
#define STM_NUM_ADCS 6
#define STM_NUM_SPIS 6

#define FLASH_BASE_ADDRESS 0x08000000
#define SRAM_BASE_ADDRESS 0x20000000

struct STM32F4XXState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    char *soc_type;

    ARMv7MState armv7m;

    STM32F2xxGpioState gpio[STM_NUM_GPIOS];
    STM32F2XXRccState rcc;
    STM32F2XXCrcState crc;
    STM32F2XXSyscfgState syscfg;
    STM32F4xxExtiState exti;
    STM32F2XXUsartState usart[STM_NUM_USARTS];
    STM32F2XXTimerState timer[STM_NUM_TIMERS];
    qemu_or_irq adc_irqs;
    STM32F2XXADCState adc[STM_NUM_ADCS];
    STM32F2XXSPIState spi[STM_NUM_SPIS];
    STM32F2XXUsbOtgFsState usb_otg_fs;

    MemoryRegion sram;
    MemoryRegion flash;
    MemoryRegion flash_alias;

    Clock *sysclk;
    Clock *refclk;
};

#endif
