/*
 * Ruiying RY-5GT-1SX / TY-5GT-1SX  (睿颖 5口2.5G + 1口万兆SFP+ 交换机)
 * PCB: FG-5GT-1SX_V1.1
 * SoC: RTL8372N (4 x 内部2.5G PHY) + 外挂 RTL8221B (第5个RJ45口, 走 SDS0)
 * Flash: PUYA P25Q32SH (4 MByte)
 * Ports: 5 x 2.5G RJ45 + 1 x 10G SFP+
 *
 * 端口映射与上游所有 RTL8372N "5电口+1光口" 机型一致:
 *   log_to_phys_port[3] = 5  -> 外挂 RTL8221B (第5个电口, SDS0)
 *   log_to_phys_port[4..7] = 1..4 -> SoC 内部 4 个 2.5G PHY
 *   log_to_phys_port[8] = 6  -> SFP+ (SDS1)
 * 原厂固件里的诊断信息 "sds0:port3, sds1:port8" 也印证了这一点。
 *
 * ==================== LED 映射 (来自原厂固件真实 LED profile) ====================
 *
 * 之前的 led_mux 是从 SWTG024AS 机型的表"旋转"出来的, 导致 1/5 口无灯、3 口常黄。
 * 后来直接反汇编原厂 4MB 固件, 提取到它真正写入 LED mux (0x65e0~0x65f4) 的自定义
 * profile (code:00c44e..00c725), 与本机 PCB 走线完全吻合, 才是权威值:
 *
 *   pad 对 -> 面板灯位 (绿/黄):
 *     口3(RTL8221B, 面板第5电口) -> pad(0,1)   0x0c/0x0d
 *     口4(SoC PHY,   面板第1电口) -> pad(3,4)   0x10/0x11
 *     口5(SoC PHY,   面板第2电口) -> pad(6,8)   0x14/0x15
 *     口6(SoC PHY,   面板第3电口) -> pad(10,12) 0x18/0x19   <-- 绿灯在 pad10, 不是 pad12
 *     口7(SoC PHY,   面板第4电口) -> pad(18,21) 0x1c/0x1d
 *     口8(SFP+)                   -> pad(26,25) 0x20/0x21   (绿 pad26, 黄 pad25)
 *
 * 原厂该 profile 对应的 LED_GLB_IO_EN = 0x3624155b, 使能的 pad 恰为上面 6 组共 12 个:
 *   0,1,3,4,6,8,10,12,18,21,25,26 (外加 bit28/29 的 SYS LED)。
 * 其余 pad (2,5,7,9,11,13,14,15,16,17,19,20,22,23,24,27) 原厂未使能,
 * 是每口第 3 盏灯 / 冗余灯, 本机面板未用。
 *
 * 注意: 机器初始化对 LED_GLB_IO_EN 只 reg_bit_set 使能上面 12 个 pad,
 * 不能 REG_SET 全写 0x65dc —— 之前全写清掉了复位默认使能的位, 导致网口灯全灭、
 * 第5口(RTL8221B)失联。leds_setup() 把 led_mux[i] 按每 5 个一组压进 0x65e0~0x65f4,
 * pad i 占 bit(i%5)*6, 数组下标 = pad 编号。
 */

#include "machine_defs.h"

/*
 * SFP 模块在位检测脚 (MOD-DEF0)。
 * 大多数 RTL8372N 5+1 机型(含同门的 SWTG024AS / ZX310S / FG-4GT-2SX)用 GPIO38。
 * 如果插上模块后串口没有打印 "<MODULE INSERTED>"、且 `port stats` 显示 "NO SFP",
 * 把这一行改成 GPIO30_ACL_BIT3_EN 重新编译即可; 也可以用 CLI 的 `gpio` 命令,
 * 在插拔模块时观察哪个脚的电平翻转, 那个脚就是检测脚。
 */
#define SFP_DETECT_PIN GPIO38

__code const struct machine machine = {
	.machine_name = MACHINE_MODEL,
	.isRTL8373 = 0,
	.min_port = 3,
	.max_port = 8,
	.n_sfp = 1,
	.n_10g = 0,
	.log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6},
	.phys_to_log_port = {4, 5, 6, 7, 3, 8, 0, 0, 0},
	.is_sfp = {0, 0, 0, 0, 0, 0, 0, 0, 1},

	// SFP+ 口: 逻辑端口 8 / 物理端口 6 / SDS1
	.sfp_port[0].pin_detect = SFP_DETECT_PIN,
	.sfp_port[0].pin_los = GPIO_NA,
	.sfp_port[0].pin_tx_disable = GPIO_NA,
	.sfp_port[0].sds = 1,
	.sfp_port[0].i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

	.reset_pin = GPIO_NA,
	.high_leds = { .mux =  LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	/* 电口 1-5 用 set 0, SFP 口用 set 1
	 * 电口: 绿灯 2.5G, 黄/橙灯 10/100/1000M
	 * SFP:  100M-2.5G 绿, 10G 另一个灯
	 */
	.led_sets = {
			{
				LEDS_2G5 | LEDS_LINK | LEDS_ACT,
				LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
				LEDS_DUPLEX,
				LEDS_2G5 | LEDS_LINK | LEDS_ACT
			},
			{
				LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT,
				LEDS_10G | LEDS_LINK | LEDS_ACT,
				LEDS_2G5 | LEDS_LINK,
				LEDS_COL | LEDS_DUPLEX
			},
		},
	.led_mux_custom = 1,
	/* led_mux[i] = pad i 的值; 每个值 6 bit: 逻辑口*4 + 灯号(0=绿,1=黄)。
	 *
	 * 本表直接取自原厂固件反汇编出的 LED mux profile (code:00c44e), 非 SWTG 表旋转:
	 *   pad 0,1  = 0x0c,0x0d -> 逻辑口3 (RTL8221B, 面板第5电口)
	 *   pad 3,4  = 0x10,0x11 -> 逻辑口4 (面板第1电口)
	 *   pad 6,8  = 0x14,0x15 -> 逻辑口5 (面板第2电口)
	 *   pad 10,12= 0x18,0x19 -> 逻辑口6 (面板第3电口, 绿灯在pad10)
	 *   pad 18,21= 0x1c,0x1d -> 逻辑口7 (面板第4电口)
	 *   pad 26,25= 0x20,0x21 -> 逻辑口8 (SFP+)
	 * pad2,5,7,9,11,13~17,19,20,22,23,24,27 原厂未使能(冗余/第3灯), 值保留原厂。
	 * 逐行 = 寄存器 0x65e0/65e4/65e8/65ec/65f0/65f4。 */
	.led_mux = {
		0x0c, 0x0d, 0x10, 0x10, 0x11,	// 65e0  pad0,1=口3(RTL8221B,5号位)  pad3,4=口4(1号位)
		0x14, 0x14, 0x3f, 0x15, 0x16,	// 65e4  pad6=口5绿(2号位)  pad8=口5黄
		0x18, 0x0e, 0x19, 0x11, 0x12,	// 65e8  pad10=口6绿(3号位)  pad12=口6黄
		0x1a, 0x15, 0x16, 0x1c, 0x19,	// 65ec  pad18=口7绿(4号位)
		0x1a, 0x1d, 0x1d, 0x1e, 0x1e,	// 65f0  pad21=口7黄
		0x21, 0x20, 0x22			// 65f4  pad25=口8黄(SFP)  pad26=口8绿
	},
};
