#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_IMPLEMENTATION
#include "tml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <switch.h>
#include <SDL.h>

static tsf* g_TinySoundFont;

static double g_Msec;
static tml_message* g_MidiMessage;

static void AudioCallback(void* data, Uint8 *stream, int len)
{
    int SampleBlock, SampleCount = (len / (2 * sizeof(float)));
    for (SampleBlock = TSF_RENDER_EFFECTSAMPLEBLOCK; SampleCount; SampleCount -= SampleBlock, stream += (SampleBlock * (2 * sizeof(float))))
    {
        if (SampleBlock > SampleCount)
            SampleBlock = SampleCount;

        for (g_Msec += SampleBlock * (1000.0 / 44100.0); g_MidiMessage && g_Msec >= g_MidiMessage->time; g_MidiMessage = g_MidiMessage->next)
        {
            switch (g_MidiMessage->type)
            {
                case TML_PROGRAM_CHANGE:
                    tsf_channel_set_presetnumber(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->program, (g_MidiMessage->channel == 9));
                    break;
                case TML_NOTE_ON:
                    tsf_channel_note_on(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key, g_MidiMessage->velocity / 127.0f);
                    break;
                case TML_NOTE_OFF:
                    tsf_channel_note_off(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key);
                    break;
                case TML_PITCH_BEND:
                    tsf_channel_set_pitchwheel(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->pitch_bend);
                    break;
                case TML_CONTROL_CHANGE:
                    tsf_channel_midi_control(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->control, g_MidiMessage->control_value);
                    break;
            }
        }

        tsf_render_float(g_TinySoundFont, (float*)stream, SampleBlock, 0);
    }
}

int main(int argc, char *argv[])
{
    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState Pad;
    padInitializeDefault(&Pad);

    printf("nx-midi by Cyuubi\n");

    tml_message* TinyMidiLoader = NULL;

    SDL_AudioSpec OutputAudioSpec;
    OutputAudioSpec.freq = 44100;
    OutputAudioSpec.format = AUDIO_F32;
    OutputAudioSpec.channels = 2;
    OutputAudioSpec.samples = 4096;
    OutputAudioSpec.callback = AudioCallback;

    if (SDL_AudioInit(TSF_NULL) < 0)
        printf("Could not initialize audio hardware or driver.\n");

    TinyMidiLoader = tml_load_filename("sdmc:/default.mid");
    if (!TinyMidiLoader)
        printf("Could not load MIDI file.\n");

    g_MidiMessage = TinyMidiLoader;

    g_TinySoundFont = tsf_load_filename("sdmc:/default.sf2");
    if (!g_TinySoundFont)
        printf("Could not load SoundFont.\n");

    tsf_channel_set_bank_preset(g_TinySoundFont, 9, 128, 0);
    tsf_set_max_voices(g_TinySoundFont, 416);
    tsf_set_output(g_TinySoundFont, TSF_STEREO_INTERLEAVED, OutputAudioSpec.freq, 0.0f);

    if (SDL_OpenAudio(&OutputAudioSpec, TSF_NULL) < 0)
        printf("Could not open the audio hardware or the desired audio output format.\n");

    SDL_PauseAudio(0);

    while (appletMainLoop())
    {
        padUpdate(&Pad);

        u64 kDown = padGetButtonsDown(&Pad);
        if (kDown & HidNpadButton_Plus)
            break;

        consoleUpdate(NULL);
    }

    SDL_Quit();
    
    consoleExit(NULL);
    return 0;
}