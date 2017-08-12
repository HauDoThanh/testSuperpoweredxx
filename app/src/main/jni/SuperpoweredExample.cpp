#include "SuperpoweredExample.h"
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <malloc.h>

static void playerEventCallbackA(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event,
                                 void *__unused value) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
        SuperpoweredAdvancedAudioPlayer *playerA = *((SuperpoweredAdvancedAudioPlayer **) clientData);
        playerA->setBpm(126.0f);
        playerA->setFirstBeatMs(353);
        playerA->setPosition(playerA->firstBeatMs, false, false);
    };
}

static void playerEventCallbackB(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event,
                                 void *__unused value) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
        SuperpoweredAdvancedAudioPlayer *playerB = *((SuperpoweredAdvancedAudioPlayer **) clientData);
        playerB->setBpm(123.0f);
        playerB->setFirstBeatMs(40);
        playerB->setPosition(playerB->firstBeatMs, false, false);
    };
}

static bool audioProcessing(void *clientdata, short int *audioIO, int numberOfSamples,
                            int __unused samplerate) {
    return ((SuperpoweredExample *) clientdata)->process(audioIO, (unsigned int) numberOfSamples);
}

SuperpoweredExample::SuperpoweredExample(unsigned int samplerate, unsigned int buffersize,
                                         const char *path, int fileAoffset, int fileAlength,
                                         int fileBoffset, int fileBlength) : activeFx(0),
                                                                             crossValue(0.0f),
                                                                             volB(0.0f),
                                                                             volA(1.0f * headroom) {
    stereoBuffer = (float *) memalign(16, (buffersize + 16) * sizeof(float) * 2);

    playerA = new SuperpoweredAdvancedAudioPlayer(&playerA, playerEventCallbackA, samplerate, 0);
    playerA->open(path, fileAoffset, fileAlength);
    playerB = new SuperpoweredAdvancedAudioPlayer(&playerB, playerEventCallbackB, samplerate, 0);
    playerB->open(path, fileBoffset, fileBlength);

    playerA->syncMode = playerB->syncMode = SuperpoweredAdvancedAudioPlayerSyncMode_TempoAndBeat;

    roll = new SuperpoweredRoll(samplerate);
    filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, samplerate);
    flanger = new SuperpoweredFlanger(samplerate);
    reverb = new SuperpoweredReverb(samplerate);
    echo = new SuperpoweredEcho(samplerate);
    limiter = new SuperpoweredLimiter(samplerate);

    audioSystem = new SuperpoweredAndroidAudioIO(samplerate, buffersize, false, true,
                                                 audioProcessing, this, -1, SL_ANDROID_STREAM_MEDIA,
                                                 buffersize * 2);
}

SuperpoweredExample::~SuperpoweredExample() {
    delete audioSystem;
    delete playerA;
    delete playerB;
    free(stereoBuffer);
}

void SuperpoweredExample::onPlayPause(bool play) {
    if (!play) {
        playerA->pause();
        playerB->pause();
    } else {
        playerA->play(true);
        playerB->play(true);
    };
    SuperpoweredCPU::setSustainedPerformanceMode(play); // <-- Important to prevent audio dropouts.
}


void SuperpoweredExample::onFxReverbValue(int param, int value) {
    double scaleValue = 0.01 * value;
    __android_log_print(ANDROID_LOG_VERBOSE, "SuperpoweredExample", "Param: %i , Value: %0.2f",
                        param, scaleValue);
    reverb->enable(true);
    switch (param) {
       /* case REVERB_DRY:
            reverb->setDry(scaleValue);
            break;*/
        case REVERB_MIX:
            reverb->setMix(scaleValue);
            break;
        case REVERB_WIDTH:
            reverb->setWidth(scaleValue);
            break;
        case REVERB_ROOMSIZE:
            reverb->setRoomSize(scaleValue);
            break;
       /* case REVERB_WET:
            reverb->setWet(scaleValue);
            break;
        case REVERB_DAMP:
            reverb->setDamp(scaleValue);
            break;*/
    }

}

bool SuperpoweredExample::process(short int *output, unsigned int numberOfSamples) {
    bool masterIsA = (crossValue <= 0.5f);
    double masterBpm = masterIsA ? playerA->currentBpm : playerB->currentBpm;
    double msElapsedSinceLastBeatA = playerA->msElapsedSinceLastBeat; // When playerB needs it, playerA has already stepped this value, so save it now.

    bool silence = !playerA->process(stereoBuffer, false, numberOfSamples, volA, masterBpm,
                                     playerB->msElapsedSinceLastBeat);
    if (playerB->process(stereoBuffer, !silence, numberOfSamples, volB, masterBpm,
                         msElapsedSinceLastBeatA))
        silence = false;

    roll->bpm = flanger->bpm = (float) masterBpm; // Syncing fx is one line.

    if (roll->process(silence ? NULL : stereoBuffer, stereoBuffer, numberOfSamples) &&
        silence)
        silence = false;
    if (!silence) {
        filter->process(stereoBuffer, stereoBuffer, numberOfSamples);
        flanger->process(stereoBuffer, stereoBuffer, numberOfSamples);
        reverb->process(stereoBuffer, stereoBuffer, numberOfSamples);
        echo->process(stereoBuffer, stereoBuffer, numberOfSamples);
        limiter->process(stereoBuffer, stereoBuffer, numberOfSamples);
    };

    // The stereoBuffer is ready now, let's put the finished audio into the requested buffers.
    if (!silence) SuperpoweredFloatToShortInt(stereoBuffer, output, numberOfSamples);
    return !silence;
}

void SuperpoweredExample::startRecord(JNIEnv *env, jstring path) {
    const char *pathTemp = env->GetStringUTFChars(path, false);
    SuperpoweredRecorder *recorder = new SuperpoweredRecorder(pathTemp, 44100, 1);
}

void SuperpoweredExample::onEchoValue(int value) {
    float scaleValue = (float) (0.01 * value);

    echo->enable(true);
    echo->setMix(scaleValue);
}

static SuperpoweredExample *example = NULL;

extern "C" JNIEXPORT void
Java_com_haudo_testsuperpoweredxx_MainActivity_SuperpoweredExample(JNIEnv *javaEnvironment,
                                                                    jobject __unused obj,
                                                                    jint samplerate,
                                                                    jint buffersize,
                                                                    jstring apkPath,
                                                                    jint fileAoffset,
                                                                    jint fileAlength,
                                                                    jint fileBoffset,
                                                                    jint fileBlength) {
    const char *path = javaEnvironment->GetStringUTFChars(apkPath, JNI_FALSE);
    example = new SuperpoweredExample((unsigned int) samplerate, (unsigned int) buffersize, path,
                                      fileAoffset, fileAlength, fileBoffset, fileBlength);
    javaEnvironment->ReleaseStringUTFChars(apkPath, path);
}

extern "C" JNIEXPORT void
Java_com_haudo_testsuperpoweredxx_MainActivity_onPlayPause(JNIEnv *__unused javaEnvironment,
                                                           jobject __unused obj, jboolean play) {
    example->onPlayPause(play);
}

extern "C" JNIEXPORT void
Java_com_haudo_testsuperpoweredxx_MainActivity_startRecord(JNIEnv *javaEnvironment,
                                                           jobject __unused obj, jstring path) {
    example->startRecord(javaEnvironment, path);
}

extern "C" JNIEXPORT void
Java_com_haudo_testsuperpoweredxx_MainActivity_onFxReverbValue(JNIEnv *__unused javaEnvironment,
                                                               jobject __unused obj, jint param,
                                                               jint value) {
    example->onFxReverbValue(param, value);
}

extern "C" JNIEXPORT void
Java_com_haudo_testsuperpoweredxx_MainActivity_onEcho(JNIEnv *__unused javaEnvironment,
                                                      jobject __unused obj,
                                                      jint mix) {
    example->onEchoValue(mix);
}