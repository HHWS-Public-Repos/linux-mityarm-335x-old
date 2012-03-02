/*
 * ASoC driver for MityARM-335x SOM based platforms.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "davinci-pcm.h"
#include "davinci-i2s.h"
#include "davinci-mcasp.h"

#define AUDIO_FORMAT (SND_SOC_DAIFMT_LEFT_J | \
		SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_NB_IF)
static int devkit_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	unsigned sysclk = 24576000;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, AUDIO_FORMAT);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, AUDIO_FORMAT);
	if (ret < 0)
		return ret;

	/* set the codec system clock */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, sysclk, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops devkit_ops = {
	.hw_params = devkit_hw_params,
};

static struct snd_soc_dai_link mityarm335x_devkit_dai = {
	.name = "TLV320AIC26",
	.stream_name = "AIC26",
	.cpu_dai_name = "davinci-mcasp.1",
	.codec_dai_name = "tlv320aic26-hifi",
	.codec_name = "spi1.1",
	.platform_name = "davinci-pcm-audio",
/*	.init = devkit_aic3x_init, */
	.ops = &devkit_ops,
};

static struct snd_soc_card mityarm335x_devkit_soc_card = {
	.name = "MityARM-335X DevKit",
	.dai_link = &mityarm335x_devkit_dai,
	.num_links = 1,
};

static struct platform_device *devkit_snd_device;

static int __init devkit_init(void)
{
	struct snd_soc_card *devkit_snd_dev_data;
	int ret;

	devkit_snd_dev_data = &mityarm335x_devkit_soc_card;

	devkit_snd_device = platform_device_alloc("soc-audio", 0);
	if (!devkit_snd_device)
		return -ENOMEM;

	platform_set_drvdata(devkit_snd_device, devkit_snd_dev_data);
	ret = platform_device_add(devkit_snd_device);
	if (ret)
		platform_device_put(devkit_snd_device);

	return ret;
}

static void __exit devkit_exit(void)
{
	platform_device_unregister(devkit_snd_device);
}

module_init(devkit_init);
module_exit(devkit_exit);

MODULE_AUTHOR("Michael Williamson");
MODULE_DESCRIPTION("MityARM-335x DevKit ASoC driver");
MODULE_LICENSE("GPL");
