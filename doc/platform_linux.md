Linux platform specific {#platform_linux}
=========================================

# Media clock driver

The media clock driver provides media clock generation and recovery mechanisms for the AVB stack.
Media clock generation generates timestamps and wake-up events whereas the media clock recovery consumes
timestamps to synchronize an audio clock.

The HW devices supported by the media clock driver are configured through the Linux device tree.
Currently, the stack supports only media clock recovery with the internal audio PLL tuning.

## Internal audio PLL recovery

Available on most of the recent i.MX SoCs (i.MX 6ULL, i.MX 8MM, i.MX8 MP and i.MX 93). On these platforms, the audio PLL has the capability to be tuned on-the-fly which provides a more
integrated solution that doesn't require additional external devices. The audio clock needs to be generated by the
SoC and measured using gPTP based events.

Current implementation uses a timer block (GPT or TPM) to measure the audio clock. The capture is triggered by timestamps coming from
the remote stream and therefore provides measurements in the remote clock domain. These measurements are fed to a PID loop
used to tune the audio PLL.

#### AVB internal recovery node

Required properties:
- compatible: should be "fsl,avb-gpt" (for GPT node) or "fsl,avb-tpm" (for TPM)
- domain: domain ID.
- rec-channel: array of 3 elements. GPT capture channel (capture channel 1 or 2), ethernet port and ENET Timer Compare ID.
- prescale: pre-divider of GPT Timer IP.
- clocks: references to "ipg" , "per" , "audio_pll" and optionally "clk_in" clocks.
- clock-names: should be "ipg", "per", "audio_pll and "clk_in".
	- "ipg" is the block clock gate
	- "per" is the peripheral clock : in case of internal routing of the root clock to be derived from the audio pll, this
	  clock will be the counter clock. In that case, no need for the (external) clk_in clock for GPT.
	- "clk_in" (only for GPT): is the clock connected to the GPT in case of no internal routing is possible and external clock is needed to feed
	  the gpt clock from the audio pll
	- "audio_pll": The Audio PLL to be tuned

Example with an external clk source:

	&gpt2 {
		compatible = "fsl,avb-gpt";
		pinctrl-names = "default";
		pinctrl-0 = <&pinctrl_avb>;
		prescale = <1>;

		rec-channel = <1 0 0>; /* capture channel, eth port, ENET TC id */
		domain = <0>;

		clocks = <&clks IMX6UL_CLK_GPT2_BUS>, <&clks IMX6UL_CLK_GPT2_SERIAL>,
		         <&clks IMX6UL_CLK_SAI2>, <&clks IMX6UL_CLK_PLL4>;
		clock-names = "ipg", "per", "clk_in", "audio_pll";

		status = "okay";
	};

Example with the gpt root clock derived from the audio pll internally:

	&gpt1 {
		compatible = "fsl,avb-gpt";
		rec-channel = <1 0 1>; // capture channel, eth port, ENET TC id
		prescale = <1>;
		domain = <0>;

		clocks = <&clk IMX8MP_CLK_GPT1_ROOT>,
		         <&clk IMX8MP_CLK_GPT1_ROOT>,
		         <&clk IMX8MP_AUDIO_PLL1>;

		clock-names = "ipg", "per", "audio_pll";

		/* Make the GPT clk root derive from the audio PLL*/
		assigned-clocks = <&clk IMX8MP_CLK_GPT1>;
		assigned-clock-parents = <&clk IMX8MP_AUDIO_PLL1_OUT>;

		status = "okay";
	};

# AVB HW timer Interrupt

The AVB kernel module uses a HW timer to generate a periodic interrupt (currently every 125us).
This interrupt is used to poll the FEC driver for RX frames, clean the FEC TX descriptors, perform TX software scheduling and generate an interrupt for
the media clock driver.

#### AVB HW timer node

Required properties:
- compatible: should be "fsl,avb-gpt" (for GPT node) or "fsl,avb-tpm" (for TPM)
- timer-channel: output compare channel (output  channel 1, 2 or 3)
- prescale: pre-divider of Timer IP.
- clocks: references to "ipg" and "per" clocks.
- clock-names: should be "ipg" and "per". "ipg" is the clock gate, "per" is the peripheral clock used as counter clock


Example of TPM used for HW timer rooted internally from the audio pll:

	&tpm4 {
		compatible = "fsl,avb-tpm";
		timer-channel = <1>; /* Use output compare channel 1*/
		prescale = <1>;
		domain = <0>;

		clocks = <&clk IMX93_CLK_BUS_WAKEUP>, <&clk IMX93_CLK_TPM4_GATE>, <&clk IMX93_CLK_AUDIO_PLL>;
		clock-names = "ipg", "per", "audio_pll";

		/* Set TPM4 clock root to Audio PLL. */
		assigned-clocks = <&clk IMX93_CLK_TPM4>;
		assigned-clock-parents = <&clk IMX93_CLK_AUDIO_PLL>;

		status = "okay";
	};

---
**NOTE**

The GPT and TPM drivers supports both functioning modes : Internal recovery and HW timer.
Thus, specifying both properties "rec-channel" and "timer-channel" in the AVB specific node will enable both modes.

---
