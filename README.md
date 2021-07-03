# Porting recent Android to the Onyx Boox T68 E-Reader

The Onyx Boox T68 was released with Android 4.0 and received an Android 4.4 beta version. The device is powered by a [Freescale i.MX 6SoloLite ](https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-6-processors/i-mx-6sololite-processors-single-core-low-power-epd-controller-arm-cortex-a9-core:i.MX6SL) SoC which contains a Vivante GC320 2D GPU and a Vivante GC355 OpenVG GPU. In particular, the device lacks OpenGL ES hardware support.

This unofficial partial port of Cyanogenmod 11 (Android 4.4) was an exercise to familiarize myself with the Android building and porting process. The port runs of the devices SD card which allows tethered boot using `fastboot` without interfering with the stock Android installation. Next to the SD card possibly impacting speed, the port does all rendering in software which leads to deteriorated performance,
unlike the stock ROM which uses the 2D GPU for display compositing.

## Porting the Linux kernel

Onyx Boox does [not](https://news.ycombinator.com/item?id=23735962) [release](https://www.mobileread.com/forums/showthread.php?t=277431) [kernel](https://news.ycombinator.com/item?id=21041543) sources.
However, the T68's build properties suggest the board may be related to NXP's (formerly Freescale's) [i.MX 6SoloLite Evaluation Kit](https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/i-mx-6sololite-evaluation-kit:IMX6SLEVK). The e-reader uses NXP's [Linux 3.0.35](https://source.codeaurora.org/external/imx/linux-imx/tree/?h=imx_3.0.35_android_kk4.4.2_1.y_caf) fork, the [Linux 5.4](https://source.codeaurora.org/external/imx/linux-imx/tree/?h=imx_5.4.70_2.3.0) downstream kernel [supports](https://source.codeaurora.org/external/imx/linux-imx/tree/arch/arm/boot/dts/imx6sl-evk.dts?h=imx_5.4.70_2.3.0) the EVK-board, too. 

Firmware files packaged with the Android beta version indicate the EInk panel is named `ED068OG1`. In contrast, the T68's kernel command line advertises the panel as `E68_V220`.
While the BSP kernel lacks [configuration](https://source.codeaurora.org/external/imx/linux-imx/tree/arch/arm/mach-mx6/board-mx6sl_evk.c?h=imx_3.0.35_android_kk4.4.2_1.y_caf) for either panel, 
the Kobo Aura HD and Kobo Aura H2O kernel packages contain two different [configurations](https://github.com/librereader/kobo-aura-h2o-linux/blob/526448215c4c30a1e952a3880772b7953aeaec2a/arch/arm/mach-mx5/mx50_rdp.c) for the former.
Steps include:

- Determine physical pin configuration: Extract [iomux](https://source.codeaurora.org/external/imx/linux-imx/tree/arch/arm/plat-mxc/iomux-v3.c?h=imx_3.0.35_android_kk4.4.2_1.y_caf) configuration from running device. Find a way to obtain GPIO labels, ...

- Adapt board specific [file(s)](https://source.codeaurora.org/external/imx/linux-imx/tree/arch/arm/mach-mx6/board-mx6sl_evk.c?h=imx_3.0.35_android_kk4.4.2_1.y_caf), configure drivers for power, panel, WiFi, audio, ...

- Convert the board support files into a device tree compatible with newer kernel versions

## Porting AOSP

The AOSP (Android Open Source Project) included [`libagl` / `pixelflinger`](https://cs.android.com/android/platform/superproject/+/android-10.0.0_r30:frameworks/native/opengl/libagl/Android.bp), a Open GL ES 1 software renderer to support the Android emulator while Open GL ES 2 was technically required even on older versions of Android. Around the release of Android 7, the emulator gained OpenGL ES 2 support via host and parts of the system began to rely on it:

- **Surface Flinger** consumes graphic buffers from applications, handles compositing, and hands the result to the display hardware. The composition is done via OpenGL ES and encapsulated in a component named [`RenderEngine`](https://cs.android.com/android/platform/superproject/+/master:frameworks/native/libs/renderengine/include/renderengine/RenderEngine.h). 

- **Bootanimation** relies directly on OpenGL ES 1.

- **Canvas** drawing in applications supports software rendering with the `skia` library. If enabled globally, the graphics framework *should* not depend on OpenGL ES. 

The above components essentially use OpenGL ES as a 2D graphics engine, adapting them to use OpenVG instead *may* be possible. However, SurfaceFlinger uses some OpenGL ES extensions to share contexts with applications. Rebuilding this behaviour with OpenVG will be difficult. Additionally, other components might require OpenGL ES and prove to be a roadblock.

