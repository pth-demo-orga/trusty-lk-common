/*
 * Copyright (c) 2019 LK Trusty Authors. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <arch/ops.h>
#include <assert.h>
#include <bits.h>
#include <stdint.h>

#include "arm_gic_common.h"
#include "gic_v3.h"

#define WAKER_QSC_BIT (0x1u << 31)
#define WAKER_CA_BIT (0x1u << 2)
#define WAKER_PS_BIT (0x1u << 1)
#define WAKER_SL_BIT (0x1u << 0)

static void gicv3_gicr_exit_sleep(uint32_t cpu) {
    uint32_t val = GICRREG_READ(0, cpu, GICR_WAKER);

    if (val & WAKER_QSC_BIT) {
        /* clear sleep bit */
        GICRREG_WRITE(0, cpu, GICR_WAKER, val & ~WAKER_SL_BIT);
        while (GICRREG_READ(0, cpu, GICR_WAKER) & WAKER_QSC_BIT) {
        }
    }
}

static void gicv3_gicr_mark_awake(uint32_t cpu) {
    uint32_t val = GICRREG_READ(0, cpu, GICR_WAKER);

    if (val & WAKER_CA_BIT) {
        /* mark CPU as awake */
        GICRREG_WRITE(0, cpu, GICR_WAKER, val & ~WAKER_PS_BIT);
        while (GICRREG_READ(0, cpu, GICR_WAKER) & WAKER_CA_BIT) {
        }
    }
}

#if GIC600
/*
 * GIC-600 implements an additional GICR power control register
 */
#define GICR_PWRR (GICR_OFFSET + 0x0024)

#define PWRR_ON (0x0u << 0)
#define PWRR_OFF (0x1u << 0)
#define PWRR_RDGPD (0x1u << 2)
#define PWRR_RDGPO (0x1u << 3)
#define PWRR_RDGP_MASK (PWRR_RDGPD | PWRR_RDGPO)

static void gicv3_gicr_power_on(uint32_t cpu) {
    /* Initiate power up */
    GICRREG_WRITE(0, cpu, GICR_PWRR, PWRR_ON);

    /* wait until it is complete (both bits are clear) */
    while (GICRREG_READ(0, cpu, GICR_PWRR) & PWRR_RDGP_MASK) {
    }
}

static void gicv3_gicr_off(uint32_t cpu) {
    /* initiate power down */
    GICRREG_WRITE(0, cpu, GICR_PWRR, PWRR_OFF);

    /* wait until it is complete (both bits are set) */
    while ((GICRREG_READ(0, cpu, GICR_PWRR) & PWRR_RDGP_MASK) !=
           PWRR_RDGP_MASK) {
    }
}
#else /* GIC600 */

static void gicv3_gicr_power_on(uint32_t cpu) {}
static void gicv3_gicr_power_off(uint32_t cpu) {}

#endif /* GIC600 */

static void gicv3_gicr_init(void) {
    uint32_t cpu = arch_curr_cpu_num();

    gicv3_gicr_exit_sleep(cpu);
    gicv3_gicr_power_on(cpu);
    gicv3_gicr_mark_awake(cpu);
}


/* GICD_CTRL Register write pending bit */
#define GICD_CTLR_RWP  (0x1U << 31)

static void gicv3_gicd_ctrl_write(uint32_t val) {
    /* write CTRL register */
    GICDREG_WRITE(0, GICD_CTLR, val);

    /* wait until write complete */
    while (GICDREG_READ(0, GICD_CTLR) & GICD_CTLR_RWP) {
    }
}

static void gicv3_gicd_setup_irq_group(uint32_t vector, uint32_t grp) {
    uint32_t val;
    uint32_t mask;

    ASSERT((vector > 32) && (vector < MAX_INT));

    mask = (0x1u << (vector % 32));

    val = GICDREG_READ(0, GICD_IGROUPR(vector / 32));
    if (grp & 0x1u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    GICDREG_WRITE(0, GICD_IGROUPR(vector / 32), val);

    val = GICDREG_READ(0, GICD_IGRPMODR(vector / 32));
    if (grp & 0x2u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    GICDREG_WRITE(0, GICD_IGRPMODR(vector / 32), val);
}

static void gicv3_gicd_setup_default_group(uint32_t grp) {
    uint32_t i;

    /* Assign all interrupts to selected group */
    for (i = 32; i < MAX_INT; i += 32) {
        GICDREG_WRITE(0, GICD_IGROUPR(i / 32), (grp & 0x1u) ? ~0U : 0);
        GICDREG_WRITE(0, GICD_IGRPMODR(i / 32), (grp & 0x2u) ? ~0U : 0);
    }
}

static void gicv3_gicr_setup_irq_group(uint32_t vector, uint32_t grp) {
    uint32_t val;
    uint32_t mask;
    uint32_t cpu = arch_curr_cpu_num();

    ASSERT(vector < 32);

    mask = (0x1u << vector);

    val = GICRREG_READ(0, cpu, GICR_IGROUPR0);
    if (grp & 0x1u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    GICRREG_WRITE(0, cpu, GICR_IGROUPR0, val);

    val = GICRREG_READ(0, cpu, GICR_IGRPMODR0);
    if (grp & 0x2u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    GICRREG_WRITE(0, cpu, GICR_IGRPMODR0, val);
}

static void gicv3_gicr_setup_default_group(uint32_t grp) {
    uint32_t cpu = arch_curr_cpu_num();

    GICRREG_WRITE(0, cpu, GICR_IGROUPR0, (grp & 0x1u) ? ~0U : 0);
    GICRREG_WRITE(0, cpu, GICR_IGRPMODR0, (grp & 0x2u) ? ~0U : 0);
}

void arm_gicv3_init(void) {
    uint32_t grp_mask = (0x1u << GICV3_IRQ_GROUP);

#if !WITH_LIB_SM
    /* non-TZ */
    int i;

    /* Disable all groups before making changes */
    gicv3_gicd_ctrl_write(GICDREG_READ(0, GICD_CTLR) & ~0x7);

    for (i = 0; i < MAX_INT; i += 32) {
        GICDREG_WRITE(0, GICD_ICENABLER(i / 32), ~0U);
        GICDREG_WRITE(0, GICD_ICPENDR(i / 32), ~0U);
    }

    /* Direct SPI interrupts to any core */
    for (i = 32; i < MAX_INT; i++) {
        GICDREG_WRITE64(0, GICD_IROUTER(i), 0x80000000);
    }
#endif

    /* Enable selected group */
    gicv3_gicd_ctrl_write(GICDREG_READ(0, GICD_CTLR) | grp_mask);
}

void arm_gicv3_init_percpu(void) {
#if WITH_LIB_SM
    /* TZ */
    /* Initialized by ATF */
#else
    /* non-TZ */

    /* Init registributor interface */
    gicv3_gicr_init();

    /* Enable CPU interface access */
    GICCREG_WRITE(0, icc_sre_el1, (GICCREG_READ(0, icc_sre_el1) | 0x7));
#endif

    /* enable selected percpu group */
    if (GICV3_IRQ_GROUP == 0) {
        GICCREG_WRITE(0, icc_igrpen0_el1, 1);
    } else {
        GICCREG_WRITE(0, icc_igrpen1_el1, 1);
    }

    /* Unmask interrupts at all priority levels */
    GICCREG_WRITE(0, icc_pmr_el1, 0xFF);
}

void arm_gicv3_configure_irq_locked(unsigned int cpu, unsigned int vector) {
    uint32_t grp = GICV3_IRQ_GROUP;

    ASSERT(vector < MAX_INT);

    if (vector < 32) {
        /* PPIs */
        gicv3_gicr_setup_irq_group(vector, grp);
    } else {
        /* SPIs */
        gicv3_gicd_setup_irq_group(vector, grp);
    }
}
