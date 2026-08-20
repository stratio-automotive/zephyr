/*
 * Copyright 2025, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mc_cgm

#include <errno.h>
#include <zephyr/drivers/clock_control/nxp_clock_controller_sources.h>
#include <zephyr/dt-bindings/clock/nxp_mc_cgm.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(fxosc), nxp_fxosc, okay)
const fxosc_config_t fxosc_config = {.freqHz = NXP_FXOSC_FREQ,
				     .workMode = NXP_FXOSC_WORKMODE,
				     .startupDelay = NXP_FXOSC_DELAY,
				     .overdriveProtect = NXP_FXOSC_OVERDRIVE};
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), nxp_plldig, okay)
const pll_config_t pll_config = {.workMode = NXP_PLL_WORKMODE,
				 .preDiv = NXP_PLL_PREDIV, /* PLL input clock predivider: 2 */
				 .postDiv = NXP_PLL_POSTDIV,
				 .multiplier = NXP_PLL_MULTIPLIER,
				 .fracLoopDiv = NXP_PLL_FRACLOOPDIV,
				 .stepSize = NXP_PLL_STEPSIZE,
				 .stepNum = NXP_PLL_STEPNUM,
				 .accuracy = NXP_PLL_ACCURACY,
				 .outDiv = NXP_PLL_OUTDIV_POINTER};
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mc_cgm), nxp_mc_cgm, okay)
const clock_pcfs_config_t pcfs_config = {.maxAllowableIDDchange = NXP_PLL_MAXIDOCHANGE,
					 .stepDuration = NXP_PLL_STEPDURATION,
					 .clkSrcFreq = NXP_PLL_CLKSRCFREQ};
#endif

/*
 * SDK defines FSL_FEATURE_SOC_<IP>_COUNT as `(N)` with parentheses, which
 * breaks Zephyr's LISTIFY (it token-pastes LEN into a macro name and needs
 * a bare integer). Strip the parens before passing to LISTIFY.
 */
#define MC_CGM_UNWRAP(...) __VA_ARGS__
#define MC_CGM_COUNT(n)   MC_CGM_UNWRAP n

/*
 * The SDK exposes peripheral clocks via two distinct enum namespaces:
 *   - clock_ip_name_t (kCLOCK_Lpuart0)    feeds CLOCK_EnableClock() /
 *                                         CLOCK_DisableClock() — the gate
 *                                         path used by _on()/_off().
 *   - clock_name_t   (kCLOCK_Lpuart0Clk)  feeds CLOCK_GetFreq() — the
 *                                         clock-tree query used by
 *                                         _get_rate().
 * Keep two parallel tables so each entry carries the right typed enum,
 * and let _on()/_off() share the gate table.
 *
 * `dt` is the DT-binding prefix (uppercase, e.g. LPUART -> MCUX_LPUART0_CLK);
 * `sdk` is the SDK enum prefix (CamelCase, e.g. Lpuart -> kCLOCK_Lpuart0).
 * LISTIFY iterates 0..(N-1) where N is FSL_FEATURE_SOC_<IP>_COUNT, so
 * undefined SDK identifiers on derivative SoCs are never referenced.
 */
struct mc_cgm_gate_entry {
	uint32_t subsys;
	clock_ip_name_t sdk_enum;
};

struct mc_cgm_rate_entry {
	uint32_t subsys;
	clock_name_t sdk_enum;
};

#define MC_CGM_GATE_ENTRY(i, dt, sdk) \
	{ MCUX_##dt##i##_CLK, kCLOCK_##sdk##i }

#define MC_CGM_RATE_ENTRY(i, dt, sdk) \
	{ MCUX_##dt##i##_CLK, kCLOCK_##sdk##i##Clk }

static const struct mc_cgm_gate_entry mc_cgm_gate_map[] = {
#if defined(CONFIG_CAN_MCUX_FLEXCAN) && defined(FSL_FEATURE_SOC_FLEXCAN_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_FLEXCAN_COUNT),
		MC_CGM_GATE_ENTRY, (,), FLEXCAN, Flexcan),
#endif
#if defined(CONFIG_UART_MCUX_LPUART) && defined(FSL_FEATURE_SOC_LPUART_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPUART_COUNT),
		MC_CGM_GATE_ENTRY, (,), LPUART, Lpuart),
#endif
#if defined(CONFIG_SPI_NXP_LPSPI) && defined(FSL_FEATURE_SOC_LPSPI_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPSPI_COUNT),
		MC_CGM_GATE_ENTRY, (,), LPSPI, Lpspi),
#endif
#if defined(CONFIG_I2C_MCUX_LPI2C) && defined(FSL_FEATURE_SOC_LPI2C_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPI2C_COUNT),
		MC_CGM_GATE_ENTRY, (,), LPI2C, Lpi2c),
#endif
#if (defined(CONFIG_COUNTER_MCUX_STM) || defined(CONFIG_MCUX_STM_TIMER)) && \
	defined(FSL_FEATURE_SOC_STM_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_STM_COUNT),
		MC_CGM_GATE_ENTRY, (,), STM, Stm),
#endif
#if defined(CONFIG_COUNTER_NXP_PIT) && defined(FSL_FEATURE_SOC_PIT_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_PIT_COUNT),
		MC_CGM_GATE_ENTRY, (,), PIT, Pit),
#endif
#if defined(CONFIG_COMPARATOR_NXP_LPCMP) && defined(FSL_FEATURE_SOC_LPCMP_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPCMP_COUNT),
		MC_CGM_GATE_ENTRY, (,), CMP, Lpcmp),
#endif
#if defined(CONFIG_ADC_NXP_SAR_ADC) && defined(FSL_FEATURE_SOC_ADC_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_ADC_COUNT),
		MC_CGM_GATE_ENTRY, (,), ADC, Adc),
#endif
#if defined(CONFIG_I2S_MCUX_SAI) && defined(FSL_FEATURE_SOC_I2S_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_I2S_COUNT),
		MC_CGM_GATE_ENTRY, (,), SAI, Sai),
#endif
#if defined(CONFIG_MSPI_NXP_QSPI)
	{ MCUX_QSPISF_CLK, kCLOCK_Qspi },
#endif
};

/*
 * LPCMP: SDK naming is inconsistent between the two namespaces —
 * clock_ip_name_t uses "Lpcmp" (kCLOCK_Lpcmp0), but clock_name_t
 * drops the "Lp" (kCLOCK_Cmp0Clk). Pass "Cmp" as the SDK prefix
 * here so CLOCK_GetFreq receives the right identifier.
 */
static const struct mc_cgm_rate_entry mc_cgm_rate_map[] = {
#if defined(CONFIG_CAN_MCUX_FLEXCAN) && defined(FSL_FEATURE_SOC_FLEXCAN_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_FLEXCAN_COUNT),
		MC_CGM_RATE_ENTRY, (,), FLEXCAN, Flexcan),
#endif
#if defined(CONFIG_UART_MCUX_LPUART) && defined(FSL_FEATURE_SOC_LPUART_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPUART_COUNT),
		MC_CGM_RATE_ENTRY, (,), LPUART, Lpuart),
#endif
#if defined(CONFIG_SPI_NXP_LPSPI) && defined(FSL_FEATURE_SOC_LPSPI_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPSPI_COUNT),
		MC_CGM_RATE_ENTRY, (,), LPSPI, Lpspi),
#endif
#if defined(CONFIG_I2C_MCUX_LPI2C) && defined(FSL_FEATURE_SOC_LPI2C_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPI2C_COUNT),
		MC_CGM_RATE_ENTRY, (,), LPI2C, Lpi2c),
#endif
#if (defined(CONFIG_COUNTER_MCUX_STM) || defined(CONFIG_MCUX_STM_TIMER)) && \
	defined(FSL_FEATURE_SOC_STM_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_STM_COUNT),
		MC_CGM_RATE_ENTRY, (,), STM, Stm),
#endif
#if defined(CONFIG_COUNTER_NXP_PIT) && defined(FSL_FEATURE_SOC_PIT_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_PIT_COUNT),
		MC_CGM_RATE_ENTRY, (,), PIT, Pit),
#endif
#if defined(CONFIG_COMPARATOR_NXP_LPCMP) && defined(FSL_FEATURE_SOC_LPCMP_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_LPCMP_COUNT),
		MC_CGM_RATE_ENTRY, (,), CMP, Cmp),
#endif
#if defined(CONFIG_ADC_NXP_SAR_ADC) && defined(FSL_FEATURE_SOC_ADC_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_ADC_COUNT),
		MC_CGM_RATE_ENTRY, (,), ADC, Adc),
#endif
#if defined(CONFIG_I2S_MCUX_SAI) && defined(FSL_FEATURE_SOC_I2S_COUNT)
	LISTIFY(MC_CGM_COUNT(FSL_FEATURE_SOC_I2S_COUNT),
		MC_CGM_RATE_ENTRY, (,), SAI, Sai),
#endif
#if defined(CONFIG_MSPI_NXP_QSPI)
	{ MCUX_QSPISF_CLK, kCLOCK_QspiSfClk },
#endif
};

static const struct mc_cgm_gate_entry *mc_cgm_lookup_gate(uint32_t subsys)
{
	for (size_t i = 0; i < ARRAY_SIZE(mc_cgm_gate_map); i++) {
		if (mc_cgm_gate_map[i].subsys == subsys) {
			return &mc_cgm_gate_map[i];
		}
	}

	return NULL;
}

static const struct mc_cgm_rate_entry *mc_cgm_lookup_rate(uint32_t subsys)
{
	for (size_t i = 0; i < ARRAY_SIZE(mc_cgm_rate_map); i++) {
		if (mc_cgm_rate_map[i].subsys == subsys) {
			return &mc_cgm_rate_map[i];
		}
	}

	return NULL;
}

static int mc_cgm_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clock_name = (uint32_t)sub_system;
	const struct mc_cgm_gate_entry *entry = mc_cgm_lookup_gate(clock_name);

	if (entry != NULL) {
		CLOCK_EnableClock(entry->sdk_enum);
		return 0;
	}

	switch (clock_name) {
#if defined(CONFIG_CAN_MCUX_FLEXCAN)
	case MCUX_FLEXCAN0_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan0);
		break;
	case MCUX_FLEXCAN1_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan1);
		break;
	case MCUX_FLEXCAN2_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan2);
		break;
#if defined(FSL_FEATURE_SOC_FLEXCAN_COUNT) && (FSL_FEATURE_SOC_FLEXCAN_COUNT > 3U)
	case MCUX_FLEXCAN3_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan3);
		break;
#endif /* FSL_FEATURE_SOC_FLEXCAN_COUNT > 3U */
#if defined(FSL_FEATURE_SOC_FLEXCAN_COUNT) && (FSL_FEATURE_SOC_FLEXCAN_COUNT > 4U)
	case MCUX_FLEXCAN4_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan4);
		break;
#endif /* FSL_FEATURE_SOC_FLEXCAN_COUNT > 4U */
#if defined(FSL_FEATURE_SOC_FLEXCAN_COUNT) && (FSL_FEATURE_SOC_FLEXCAN_COUNT > 5U)
	case MCUX_FLEXCAN5_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan5);
		break;
#endif /* FSL_FEATURE_SOC_FLEXCAN_COUNT > 5U */
#endif /* defined(CONFIG_CAN_MCUX_FLEXCAN) */

#if defined(CONFIG_UART_MCUX_LPUART)
	case MCUX_LPUART0_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart0);
		break;
	case MCUX_LPUART1_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart1);
		break;
	case MCUX_LPUART2_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart2);
		break;
	case MCUX_LPUART3_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart3);
		break;
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 4U)
	case MCUX_LPUART4_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart4);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 4U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 5U)
	case MCUX_LPUART5_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart5);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 5U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 6U)
	case MCUX_LPUART6_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart6);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 6U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 7U)
	case MCUX_LPUART7_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart7);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 7U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 8U)
	case MCUX_LPUART8_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart8);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 8U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 9U)
	case MCUX_LPUART9_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart9);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 9U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 10U)
	case MCUX_LPUART10_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart10);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 10U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 11U)
	case MCUX_LPUART11_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart11);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 11U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 12U)
	case MCUX_LPUART12_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart12);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 12U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 13U)
	case MCUX_LPUART13_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart13);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 13U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 14U)
	case MCUX_LPUART14_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart14);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 14U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 15U)
	case MCUX_LPUART15_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart15);
		break;
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 15U */
#endif /* defined(CONFIG_UART_MCUX_LPUART) */

#if defined(CONFIG_SPI_NXP_LPSPI)
	case MCUX_LPSPI0_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi0);
		break;
	case MCUX_LPSPI1_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi1);
		break;
	case MCUX_LPSPI2_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi2);
		break;
	case MCUX_LPSPI3_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi3);
		break;
#if defined(FSL_FEATURE_SOC_LPSPI_COUNT) && (FSL_FEATURE_SOC_LPSPI_COUNT > 4U)
	case MCUX_LPSPI4_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi4);
		break;
#endif /* FSL_FEATURE_SOC_LPSPI_COUNT > 4U */
#if defined(FSL_FEATURE_SOC_LPSPI_COUNT) && (FSL_FEATURE_SOC_LPSPI_COUNT > 5U)
	case MCUX_LPSPI5_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi5);
		break;
#endif /* FSL_FEATURE_SOC_LPSPI_COUNT > 5U */
#endif /* defined(CONFIG_SPI_NXP_LPSPI) */

#if defined(CONFIG_I2C_MCUX_LPI2C)
	case MCUX_LPI2C0_CLK:
		CLOCK_EnableClock(kCLOCK_Lpi2c0);
		break;
	case MCUX_LPI2C1_CLK:
		CLOCK_EnableClock(kCLOCK_Lpi2c1);
		break;
#endif /* defined(CONFIG_I2C_MCUX_LPI2C) */

#if defined(CONFIG_COUNTER_MCUX_STM)
	case MCUX_STM0_CLK:
		CLOCK_EnableClock(kCLOCK_Stm0);
		break;
#if defined(FSL_FEATURE_SOC_STM_COUNT) && (FSL_FEATURE_SOC_STM_COUNT > 1U)
	case MCUX_STM1_CLK:
		CLOCK_EnableClock(kCLOCK_Stm1);
		break;
#endif /* FSL_FEATURE_SOC_STM_COUNT > 1U */
#endif /* defined(CONFIG_COUNTER_MCUX_STM) */

#ifdef CONFIG_COUNTER_NXP_PIT
	case MCUX_PIT0_CLK:
		CLOCK_EnableClock(kCLOCK_Pit0Clk);
		break;
	case MCUX_PIT1_CLK:
		CLOCK_EnableClock(kCLOCK_Pit1Clk);
		break;
	case MCUX_PIT2_CLK:
		CLOCK_EnableClock(kCLOCK_Pit2Clk);
		break;
#endif

#if defined(CONFIG_COMPARATOR_NXP_LPCMP)
	case MCUX_CMP0_CLK:
		CLOCK_EnableClock(kCLOCK_Lpcmp0);
		break;
#if defined(FSL_FEATURE_SOC_LPCMP_COUNT) && (FSL_FEATURE_SOC_LPCMP_COUNT > 1U)
	case MCUX_CMP1_CLK:
		CLOCK_EnableClock(kCLOCK_Lpcmp1);
		break;
#endif /* FSL_FEATURE_SOC_LPCMP_COUNT > 1U */
#if defined(FSL_FEATURE_SOC_LPCMP_COUNT) && (FSL_FEATURE_SOC_LPCMP_COUNT > 2U)
	case MCUX_CMP2_CLK:
		CLOCK_EnableClock(kCLOCK_Lpcmp2);
		break;
#endif /* FSL_FEATURE_SOC_LPCMP_COUNT > 2U */
#endif /* CONFIG_COMPARATOR_NXP_HSCMP */

#if defined(CONFIG_ADC_NXP_SAR_ADC)
	case MCUX_ADC0_CLK:
		CLOCK_EnableClock(kCLOCK_Adc0);
		break;
	case MCUX_ADC1_CLK:
		CLOCK_EnableClock(kCLOCK_Adc1);
		break;
#if defined(FSL_FEATURE_SOC_ADC_COUNT) && (FSL_FEATURE_SOC_ADC_COUNT > 2U)
	case MCUX_ADC2_CLK:
		CLOCK_EnableClock(kCLOCK_Adc2);
		break;
#endif /* FSL_FEATURE_SOC_ADC_COUNT > 2U */
#endif /* CONFIG_ADC_NXP_SAR_ADC */

#if defined(CONFIG_NXP_TEMPSENSE)
	case MCUX_TEMPSENSE_CLK:
		CLOCK_EnableClock(kCLOCK_TempSensor);
		break;
#endif /* CONFIG_NXP_TEMPSENSE */
	case MCUX_SIRC_CLK:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mc_cgm_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	return 0;
}

static int mc_cgm_get_subsys_rate(const struct device *dev, clock_control_subsys_t sub_system,
				  uint32_t *rate)
{
	uint32_t clock_name = (uint32_t)sub_system;
	const struct mc_cgm_rate_entry *entry = mc_cgm_lookup_rate(clock_name);

	if (entry != NULL) {
		*rate = CLOCK_GetFreq(entry->sdk_enum);
		return 0;
	}

	switch (clock_name) {
	case MCUX_SIRC_CLK:
		*rate = CLOCK_SIRC_CLK_FREQ;
		break;
#if defined(CONFIG_UART_MCUX_LPUART)
	case MCUX_LPUART0_CLK:
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 8U)
	case MCUX_LPUART8_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 8U */
		*rate = CLOCK_GetAipsPlatClkFreq();
		break;
	case MCUX_LPUART1_CLK:
	case MCUX_LPUART2_CLK:
	case MCUX_LPUART3_CLK:
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 4U)
	case MCUX_LPUART4_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 4U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 5U)
	case MCUX_LPUART5_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 5U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 6U)
	case MCUX_LPUART6_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 6U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 7U)
	case MCUX_LPUART7_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 7U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 9U)
	case MCUX_LPUART9_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 9U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 10U)
	case MCUX_LPUART10_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 10U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 11U)
	case MCUX_LPUART11_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 11U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 12U)
	case MCUX_LPUART12_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 12U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 13U)
	case MCUX_LPUART13_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 13U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 14U)
	case MCUX_LPUART14_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 14U */
#if defined(FSL_FEATURE_SOC_LPUART_COUNT) && (FSL_FEATURE_SOC_LPUART_COUNT > 15U)
	case MCUX_LPUART15_CLK:
#endif /* FSL_FEATURE_SOC_LPUART_COUNT > 15U */
		*rate = CLOCK_GetAipsSlowClkFreq();
		break;
#endif /* defined(CONFIG_UART_MCUX_LPUART) */

#if defined(CONFIG_SPI_NXP_LPSPI)
	case MCUX_LPSPI0_CLK:
		*rate = CLOCK_GetAipsPlatClkFreq();
		break;
	case MCUX_LPSPI1_CLK:
	case MCUX_LPSPI2_CLK:
	case MCUX_LPSPI3_CLK:
#if defined(FSL_FEATURE_SOC_LPSPI_COUNT) && (FSL_FEATURE_SOC_LPSPI_COUNT > 4U)
	case MCUX_LPSPI4_CLK:
#endif /* FSL_FEATURE_SOC_LPSPI_COUNT > 4U */
#if defined(FSL_FEATURE_SOC_LPSPI_COUNT) && (FSL_FEATURE_SOC_LPSPI_COUNT > 5U)
	case MCUX_LPSPI5_CLK:
#endif /* FSL_FEATURE_SOC_LPSPI_COUNT > 5U */
		*rate = CLOCK_GetAipsSlowClkFreq();
		break;
#endif /* defined(CONFIG_SPI_NXP_LPSPI) */

#if defined(CONFIG_I2C_MCUX_LPI2C)
	case MCUX_LPI2C0_CLK:
	case MCUX_LPI2C1_CLK:
		*rate = CLOCK_GetAipsSlowClkFreq();
		break;
#endif /* defined(CONFIG_I2C_MCUX_LPI2C) */

#if defined(CONFIG_CAN_MCUX_FLEXCAN)
	case MCUX_FLEXCAN0_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(0);
		break;
	case MCUX_FLEXCAN1_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(1);
		break;
	case MCUX_FLEXCAN2_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(2);
		break;
#if defined(FSL_FEATURE_SOC_FLEXCAN_COUNT) && (FSL_FEATURE_SOC_FLEXCAN_COUNT > 3U)
	case MCUX_FLEXCAN3_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(3);
		break;
#endif /* FSL_FEATURE_SOC_FLEXCAN_COUNT > 3U */
#if defined(FSL_FEATURE_SOC_FLEXCAN_COUNT) && (FSL_FEATURE_SOC_FLEXCAN_COUNT > 4U)
	case MCUX_FLEXCAN4_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(4);
		break;
#endif /* FSL_FEATURE_SOC_FLEXCAN_COUNT > 4U */
#if defined(FSL_FEATURE_SOC_FLEXCAN_COUNT) && (FSL_FEATURE_SOC_FLEXCAN_COUNT > 5U)
	case MCUX_FLEXCAN5_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(5);
		break;
#endif /* FSL_FEATURE_SOC_FLEXCAN_COUNT > 5U */
#endif /* defined(CONFIG_CAN_MCUX_FLEXCAN) */

#if defined(CONFIG_COUNTER_MCUX_STM)
	case MCUX_STM0_CLK:
		*rate = CLOCK_GetStmClkFreq(0);
		break;
#if defined(FSL_FEATURE_SOC_STM_COUNT) && (FSL_FEATURE_SOC_STM_COUNT > 1U)
	case MCUX_STM1_CLK:
		*rate = CLOCK_GetStmClkFreq(1);
		break;
#endif /* FSL_FEATURE_SOC_STM_COUNT > 1U */
#endif /* defined(CONFIG_COUNTER_MCUX_STM) */

#if defined(CONFIG_ADC_NXP_SAR_ADC)
	case MCUX_ADC0_CLK:
	case MCUX_ADC1_CLK:
#if defined(FSL_FEATURE_SOC_ADC_COUNT) && (FSL_FEATURE_SOC_ADC_COUNT > 2U)
	case MCUX_ADC2_CLK:
#endif /* FSL_FEATURE_SOC_ADC_COUNT > 2U */
		*rate = CLOCK_GetCoreClkFreq();
		break;

#endif /* CONFIG_ADC_NXP_SAR_ADC */

#if defined(CONFIG_COUNTER_NXP_PIT)
	case MCUX_PIT0_CLK:
	case MCUX_PIT1_CLK:
	case MCUX_PIT2_CLK:
		*rate = CLOCK_GetAipsSlowClkFreq();
		break;
#endif /* defined(CONFIG_COUNTER_NXP_PIT) */
#if defined(CONFIG_MCUX_FLEXIO)
	case MCUX_FLEXIO_CLK:
		*rate = CLOCK_GetCoreClkFreq();
		break;
#endif /* defined(CONFIG_MCUX_FLEXIO) */

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mc_cgm_init(const struct device *dev)
{
#if defined(FSL_FEATURE_PMC_HAS_LAST_MILE_REGULATOR) && (FSL_FEATURE_PMC_HAS_LAST_MILE_REGULATOR)
	/* Enables PMC last mile regulator before enable PLL.  */
	if ((PMC->LVSC & PMC_LVSC_LVD15S_MASK) != 0U) {
		/* External bipolar junction transistor is connected between external voltage and
		 * V15 input pin.
		 */
		PMC->CONFIG |= PMC_CONFIG_LMBCTLEN_MASK;
	}
	while ((PMC->LVSC & PMC_LVSC_LVD15S_MASK) != 0U) {
	}
	PMC->CONFIG |= PMC_CONFIG_LMEN_MASK;
	while ((PMC->CONFIG & PMC_CONFIG_LMSTAT_MASK) == 0u) {
	}
#endif /* FSL_FEATURE_PMC_HAS_LAST_MILE_REGULATOR */

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(firc), nxp_firc, okay)
	/* Switch the FIRC_DIV_SEL to the desired diveder. */
	CLOCK_SetFircDiv(NXP_FIRC_DIV);
	/* Disable FIRC in standby mode. */
	CLOCK_DisableFircInStandbyMode();
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(sirc), nxp_sirc, okay)
	/* Disable SIRC in standby mode. */
	CLOCK_DisableSircInStandbyMode();
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(fxosc), nxp_fxosc, okay)
	/* Enable FXOSC. */
	CLOCK_InitFxosc(&fxosc_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), nxp_plldig, okay)
	/* Enable PLL. */
	CLOCK_InitPll(&pll_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mc_cgm), nxp_mc_cgm, okay)
	CLOCK_SelectSafeClock(kFIRC_CLK_to_MUX0);
	/* Configure MUX_0_CSC dividers */
	CLOCK_SetClkMux0DivTriggerType(KCLOCK_CommonTriggerUpdate);
	CLOCK_SetClkDiv(kCLOCK_DivCoreClk, NXP_PLL_MUX_0_DC_0_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivAipsPlatClk, NXP_PLL_MUX_0_DC_1_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivAipsSlowClk, NXP_PLL_MUX_0_DC_2_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivHseClk, NXP_PLL_MUX_0_DC_3_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivDcmClk, NXP_PLL_MUX_0_DC_4_DIV);
#ifdef MC_CGM_MUX_0_DC_5_DIV_MASK
	CLOCK_SetClkDiv(kCLOCK_DivLbistClk, NXP_PLL_MUX_0_DC_5_DIV);
#endif
#ifdef MC_CGM_MUX_0_DC_6_DIV_MASK
	CLOCK_SetClkDiv(kCLOCK_DivQspiClk, NXP_PLL_MUX_0_DC_6_DIV);
#endif
	CLOCK_CommonTriggerClkMux0DivUpdate();
	CLOCK_ProgressiveClockFrequencySwitch(kPLL_PHI0_CLK_to_MUX0, &pcfs_config);
#if defined(CONFIG_COUNTER_MCUX_STM)
	CLOCK_SetClkDiv(kCLOCK_DivStm0Clk, NXP_PLL_MUX_1_DC_0_DIV);
	CLOCK_AttachClk(kAIPS_PLAT_CLK_to_STM0);
#if defined(FSL_FEATURE_SOC_STM_COUNT) && (FSL_FEATURE_SOC_STM_COUNT == 2U)
	CLOCK_SetClkDiv(kCLOCK_DivStm1Clk, NXP_PLL_MUX_2_DC_0_DIV);
	CLOCK_AttachClk(kAIPS_PLAT_CLK_to_STM1);
#endif /* FSL_FEATURE_SOC_STM_COUNT == 2U */
#endif /* defined(CONFIG_COUNTER_MCUX_STM) */
#endif
#if defined(CONFIG_CAN_MCUX_FLEXCAN)
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_0)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_1)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_2))
		CLOCK_SetClkDiv(kCLOCK_DivFlexcan012PeClk, 1U);
		CLOCK_AttachClk(kAIPS_PLAT_CLK_to_FLEXCAN012_PE);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_3)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_4)) || \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan_5))
		CLOCK_SelectSafeClock(kFIRC_CLK_to_FLEXCAN345_PE);
		CLOCK_SetClkDiv(kCLOCK_DivFlexcan345PeClk, 1U);
#endif
#endif /* defined(CONFIG_CAN_MCUX_FLEXCAN) */

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_qspi)
	CLOCK_SetClkDiv(kCLOCK_DivQspiSfckClk, NXP_PLL_MUX_10_DC_0_DIV);
	CLOCK_AttachClk(kPLL_PHI1_CLK_to_QSPI_SFCK);
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClockUpdate();

	return 0;
}

static DEVICE_API(clock_control, mcux_mcxe31x_clock_api) = {
	.on = mc_cgm_clock_control_on,
	.off = mc_cgm_clock_control_off,
	.get_rate = mc_cgm_get_subsys_rate,
};

DEVICE_DT_INST_DEFINE(0, mc_cgm_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &mcux_mcxe31x_clock_api);
