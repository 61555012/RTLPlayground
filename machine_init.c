/*
 * Per-machine one-shot boot hooks, hosted in BANK2 so board-specific
 * tables and code do not consume the common bank.
 */
#include <stdint.h>
#include "machine.h"
#include "rtl837x_pins.h"
#include "rtl837x_leds.h"
#include "rtl837x_sfr.h"
#include "rtl837x_regs.h"
#include "rtl837x_common.h"

#pragma codeseg BANK2
#pragma constseg BANK2

#if defined(MACHINE_KP_9000_6XH_X2) || \
	defined(MACHINE_KP_9000_6XH_X2_V2_1) || \
	defined(MACHINE_KP_9000_6XHML_X2_V2_1) || \
	defined(MACHINE_HI_K0402WS) || \
	defined(MACHINE_PCB_K0402WS_V2) || \
	defined(MACHINE_PCB_K0402WS_V3) || \
	defined(MACHINE_FNS1200P)
void machine_custom_init(void) __banked
{
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 6);
}

#elif defined MACHINE_PCB_SWTG018AS_V2_1_0
// Stock-firmware values for what the LED-set encoding cannot express: the
// bi-color SFP LED (blue pin at 10G) and the PIN_MUX_0 routing of that pin
// to the LED controller. Runs after leds_setup(), which covers the rest.
static __code const struct { uint16_t reg; uint32_t val; } custom_init_regs[] = {
	{ RTL837X_REG_LED3_0_SET1,   0x00100000UL },
	{ RTL837X_REG_LED1_0_SET1,   0x01400155UL },
	{ RTL837X_REG_LED1_0_SET0,   0x01740141UL },
	{ RTL837X_REG_LED_GLB_IO_EN, 0x7f24977fUL },
	{ RTL837X_PIN_MUX_0,         0x20db6880UL },
};

void machine_custom_init(void) __banked
{
	uint8_t i;
	// REG_SET is a multi-statement macro without a do-while wrapper: braces required
	for (i = 0; i < sizeof(custom_init_regs) / sizeof(custom_init_regs[0]); i++) {
		REG_SET(custom_init_regs[i].reg, custom_init_regs[i].val);
	}
}

#elif defined MACHINE_PCB_SWTG024AS_A_2_0_1
void machine_custom_init(void) __banked
{
    reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 6);
    reg_bit_set(RTL837X_REG_LED_MODE, 17);
    reg_bit_clear(RTL837X_REG_LED_MODE, 9);
    reg_bit_clear(RTL837X_REG_LED_MODE, 7);
}

#elif defined(MACHINE_SWTG024AS_A_2_0_1_5C_1SFP) || \
      defined(MACHINE_SWTG024AS_V2_0)
void machine_custom_init(void) __banked
{
    uint16_t pval;

    reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 6);
    reg_bit_set(RTL837X_REG_LED_MODE, 17);
    reg_bit_clear(RTL837X_REG_LED_MODE, 9);
    reg_bit_clear(RTL837X_REG_LED_MODE, 7);

    // OEM firmware sets these companion SDS0 polarity bits for the RTL8221B.
    sds_read(0, 0, 0);
    pval = SFR_DATA_U16;
    sds_write_v(0, 0, 0, pval | 0x100);

    sds_read(0, 6, 2);
    pval = SFR_DATA_U16;
    sds_write_v(0, 6, 2, pval | 0x4000);
}

#elif defined MACHINE_ZX310S_4T2XT
void machine_custom_init(void) __banked
{
	// For this device, the reset value of RTL837X_PIN_MUX_0 is 0x30000000,
	// which would disables all LEDS, enable them manually:
	REG_SET(RTL837X_PIN_MUX_0, 0x30db68bf);
}

#elif defined MACHINE_FG_4GT_2SX_V2_0
void machine_custom_init(void) __banked
{
	REG_SET(RTL837X_REG_LED_GLB_IO_EN, 0x7624155b);
}

#elif defined MACHINE_FG_5GT_1SX_V1_1
/*
 * Ruiying RY-5GT-1SX / TY-5GT-1SX, PCB FG-5GT-1SX_V1.1
 * RTL8372N + RTL8221B(SDS0) + 1 x SFP+(SDS1)
 *
 * leds_setup() writes LED_MODE = 0x0021e6b0 unconditionally. That value still
 * has bits 7 and 9 set, and the OEM firmware of this board ends up with the
 * LED_MODE fields [5..8] = 1 and [9..12] = 2 (bit 17, "LED solid on", set),
 * i.e. the OEM clears bits 7 and 9. Without this the port LEDs stay dark on
 * this PCB. Cross-checked against the 4 MByte stock flash image:
 *   0x6520 (LED_MODE)       : [9..12] = 2, [5..8] = 1, bit 17 = 1
 *   0x7f8c (PIN_MUX_0)      : 0x20db68bf  (OEM)
 *
 * The OEM image carries several LED profiles. The one matching this board's
 * wiring is the custom mux profile at code 00c44e (0x65e0-0x65f4, see the
 * machine file) together with
 *   0x65dc (LED_GLB_IO_EN) = 0x3624155b
 * which enables exactly the 12 pads the 6 ports need:
 *   0,1 3,4 6,8 10,12 18,21 25,26  (plus SYS LEDs on bits 28/29)
 * i.e. 6 ports x 2 lamps, matching the mux table in the machine file.
 *
 * Unlike SWTG024AS this board does NOT need the extra SDS0 TX-polarity bits
 * (0x100 / 0x4000). sds_init() already sets the N-chip RX PN swap (0x200 /
 * 0x2000). See the note at the end of this function.
 */
void machine_custom_init(void) __banked
{
	// Enable the 12 LED pads the OEM firmware's real LED profile uses.
	// (0x65dc -> 0x3624155b, decoded: pads 0,1,3,4,6,8,10,12,18,21,25,26.)
	// Only ever reg_bit_set here — a full REG_SET of LED_GLB_IO_EN cleared
	// the pads' reset-default state and took down port 5 (RTL8221B SMI) +
	// management access. Do NOT full-write 0x65dc.
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 0);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 1);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 3);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 4);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 6);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 8);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 10);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 12);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 18);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 21);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 25);
	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 26);	// SFP 黄灯

	reg_bit_set(RTL837X_REG_LED_MODE, 17);
	reg_bit_clear(RTL837X_REG_LED_MODE, 9);
	reg_bit_clear(RTL837X_REG_LED_MODE, 7);

	// NO extra SDS0 polarity bits for this board.
	//
	// sds_init() already applies the N-chip RX PN swap for SDS0 when
	// isN && !n_10g && !isRTL8373 (which is this machine):
	//   sds_write_v(0, 6, 2, pval | 0x2000)   RX PN swap, 64B/66B
	//   sds_write_v(0, 0, 0, pval | 0x200)    RX PN swap, 8B/10B
	// The SWTG024AS boards additionally set 0x100 / 0x4000, i.e. the TX
	// polarity, because their PCB crosses the SDS0 pairs. This PCB does not:
	// adding those bits here made port 5 (RTL8221B) link up and light its LEDs
	// but transmit garbage, so the management UI was unreachable through it.
	// Do not re-add them unless the RTL8221B link turns out to need TX swap.
}

#else
void machine_custom_init(void) __banked { }
#endif
