/*
 * Copyright (C) 2016, The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "audio_amplifier"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cutils/log.h>
#include <hardware/audio_amplifier.h>
#include <system/audio.h>
#include <tinyalsa/asoundlib.h>

#define UNUSED __attribute__((unused))

#define AMP_PATH    "/dev/tfa9888"
#define INIT_SEQ    "/etc/tfa9888_init.asq"
#define ENABLE_SEQ  "/etc/tfa9888_enable.asq"
#define DISABLE_SEQ "/etc/tfa9888_disable.asq"

#define SND_CARD        0
#define AMP_PCM_DEV     47
#define AMP_MIXER_CTL   "QUAT_MI2S_RX_DL_HL Switch"

typedef struct amp_device {
    amplifier_device_t amp_dev;
    audio_mode_t current_mode;
} amp_device_t;

static amp_device_t *amp_dev = NULL;

static struct pcm_config amp_pcm_config = {
    .channels = 1,
    .rate = 48000,
    .period_size = 0,
    .period_count = 4,
    .format = 0,
    .start_threshold = 0,
    .stop_threshold = INT_MAX,
    .avail_min = 0,
};

struct pcm *tfa_clocks_on(struct mixer *mixer)
{
    struct mixer_ctl *ctl;
    struct pcm *pcm;
    struct pcm_params *pcm_params;

    ctl = mixer_get_ctl_by_name(mixer, AMP_MIXER_CTL);
    if (ctl == NULL) {
        ALOGE("%s: Could not find %s", __func__, AMP_MIXER_CTL);
        return NULL;
    }

    pcm_params = pcm_params_get(SND_CARD, AMP_PCM_DEV, PCM_OUT);
    if (pcm_params == NULL) {
        ALOGE("Could not get the pcm_params");
        return NULL;
    }

    amp_pcm_config.period_count = pcm_params_get_max(pcm_params, PCM_PARAM_PERIODS);
    pcm_params_free(pcm_params);

    mixer_ctl_set_value(ctl, 0, 1);
    pcm = pcm_open(SND_CARD, AMP_PCM_DEV, PCM_OUT, &amp_pcm_config);
    if (!pcm) {
        ALOGE("failed to open pcm at all??");
        return NULL;
    }
    if (!pcm_is_ready(pcm)) {
        ALOGE("failed to open pcm device: %s", pcm_get_error(pcm));
        pcm_close(pcm);
        return NULL;
    }

    ALOGV("clocks: ON");
    return pcm;
}

int tfa_clocks_off(struct mixer *mixer, struct pcm *pcm)
{
    struct mixer_ctl *ctl;

    pcm_close(pcm);

    ctl = mixer_get_ctl_by_name(mixer, AMP_MIXER_CTL);
    if (ctl == NULL) {
        ALOGE("%s: Could not find %s", __func__, AMP_MIXER_CTL);
        return -ENODEV;
    } else {
        mixer_ctl_set_value(ctl, 0, 0);
    }

    ALOGV("clocks: OFF");
    return 0;
}

static int amp_load_sequence(FILE *seq, int amp_fd) {
    char buff[255];
    char buff2[255];
    int ret = 0;
    unsigned int do_write, length;
    size_t len_read, len_written;

    while (true) {
        do_write = fgetc(seq);
        if ((int)do_write == EOF) {
            // this is the normal place to EOF
            break;
        }

        length = fgetc(seq);
        if ((int)length == EOF) {
            ret = -4;
            break;
        }
        length &= 0xFF;

        len_read = fread(buff, 1, length, seq);
        if (len_read < length) {
            ret = -4;
            break;
        }

        // nibble swap hack
        for (unsigned i = 0; i < length; i++)
            buff[i] = buff[i] >> 4 | ((buff[i] & 0xF) << 4);

        if (do_write) {
            len_written = write(amp_fd, buff, length);
            if (len_written < length) {
                ret = -5;
                break;
            }
        } else {
            len_read = read(amp_fd, buff2, length);
            if (len_read < length) {
                ret = -5;
                break;
            }
            if (memcmp(buff, buff2, length) != 0) {
                // don't give up
                ret = -6;
            }
        }
    }

    return ret;
}

static int amp_load_seq(char *seq_path) {
    FILE *seq;
    int amp;
    int ret;

    amp = open(AMP_PATH, O_RDWR);
    if (amp < 0) {
        ALOGE("Failed to open amplifier %s\n", AMP_PATH);
        return -errno;
    }

    seq = fopen(seq_path, "rb");
    if (!seq) {
        ALOGE("Failed to open loading sequence %s\n", seq_path);
        close(amp);
        return -3;
    }

    ret = amp_load_sequence(seq, amp);
    if (ret == -4) {
        ALOGE("Unexpected EOF\n");
    } else if (ret == -5) {
        ALOGE("IO error\n");
    }

    fclose(seq);
    close(amp);

    return ret;
}

static int amp_set_mode(amplifier_device_t *device, audio_mode_t mode)
{
    amp_device_t *dev = (amp_device_t *) device;
    dev->current_mode = mode;
    return 0;
}

#define TFA_DEVICE_MASK (AUDIO_DEVICE_OUT_EARPIECE | AUDIO_DEVICE_OUT_SPEAKER)

static int amp_enable_output_devices(amplifier_device_t *device,
        uint32_t devices, bool enable)
{
    if ((devices & TFA_DEVICE_MASK) != 0) {
        if (enable) {
            ALOGW("%s: enabling TFA9888 amplifier", __func__);
            amp_load_seq(ENABLE_SEQ);
        } else {
            ALOGW("%s: disabling TFA9888 amplifier", __func__);
            amp_load_seq(DISABLE_SEQ);
        }
    }

    return 0;
}

static int amp_dev_close(hw_device_t *device)
{
    amp_device_t *dev = (amp_device_t *) device;
    free(dev);
    return 0;
}

static int amp_module_open(const hw_module_t *module, UNUSED const char *name,
        hw_device_t **device)
{
    struct mixer *mixer;
    struct pcm *pcm;

    if (amp_dev) {
        ALOGE("%s:%d: Unable to open second instance of TFA9888 amplifier\n",
                __func__, __LINE__);
        return -EBUSY;
    }

    amp_dev = calloc(1, sizeof(amp_device_t));
    if (!amp_dev) {
        ALOGE("%s:%d: Unable to allocate memory for amplifier device\n",
                __func__, __LINE__);
        return -ENOMEM;
    }

    amp_dev->amp_dev.common.tag = HARDWARE_DEVICE_TAG;
    amp_dev->amp_dev.common.module = (hw_module_t *) module;
    amp_dev->amp_dev.common.version = HARDWARE_DEVICE_API_VERSION(1, 0);
    amp_dev->amp_dev.common.close = amp_dev_close;

    amp_dev->amp_dev.set_input_devices = NULL;
    amp_dev->amp_dev.set_output_devices = NULL;
    amp_dev->amp_dev.enable_input_devices = NULL;
    amp_dev->amp_dev.enable_output_devices = amp_enable_output_devices;
    amp_dev->amp_dev.set_mode = amp_set_mode;
    amp_dev->amp_dev.output_stream_start = NULL;
    amp_dev->amp_dev.input_stream_start = NULL;
    amp_dev->amp_dev.output_stream_standby = NULL;
    amp_dev->amp_dev.input_stream_standby = NULL;

    amp_dev->current_mode = AUDIO_MODE_NORMAL;

    mixer = mixer_open(SND_CARD);
    if (mixer == NULL) {
        ALOGE("failed to open mixer");
        return -ENOMEM;
    }

    *device = (hw_device_t *) amp_dev;

    ALOGW("%s: initializing TFA9888 amplifier", __func__);
    pcm = tfa_clocks_on(mixer);
    amp_load_seq(INIT_SEQ);
    tfa_clocks_off(mixer, pcm);

    mixer_close(mixer);

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = amp_module_open,
};

amplifier_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AMPLIFIER_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AMPLIFIER_HARDWARE_MODULE_ID,
        .name = "HTC 10 audio amplifier HAL",
        .author = "The CyanogenMod Open Source Project",
        .methods = &hal_module_methods,
    },
};
