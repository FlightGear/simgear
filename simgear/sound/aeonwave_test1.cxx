#include <stdio.h>
#include <cstdlib> // EXIT_FAILURE
#include <string>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>	// sleep()
#endif

#include <aax/AeonWave>

#define AUDIOFILE	SRC_DIR"/jet.wav"

bool testForError(AAX::AeonWave& p, std::string s)
{
    enum aaxErrorType error = p.error_no();
    if (error != AAX_ERROR_NONE) {
       std::cout << "AeonWave Error: "
                 << p.error(error) << " at " << s << std::endl;
        return true;
    }
    return false;
}


int main( int argc, char *argv[] ) 
{
    AAX::AeonWave aax = AAX::AeonWave(AAX_MODE_WRITE_STEREO);

    aax.set(AAX_INITIALIZED);
    testForError(aax, "initialization");

    AAX::DSP dsp;
    dsp = AAX::DSP(aax, AAX_VOLUME_FILTER);
    dsp.set(AAX_GAIN, 0.0f);
    aax.set(dsp);

    dsp = AAX::DSP(aax, AAX_DISTANCE_FILTER);
    dsp.set(AAX_AL_INVERSE_DISTANCE_CLAMPED);
    aax.set(dsp);

    dsp = AAX::DSP(aax, AAX_VELOCITY_EFFECT);
    dsp.set(AAX_DOPPLER_FACTOR, 1.0f);
    dsp.set(AAX_SOUND_VELOCITY, 340.3f);
    aax.set(dsp);

    testForError(aax, "scenery setup");

    std::string _vendor = (const char *)aax.info(AAX_VENDOR_STRING);
    std::string _renderer = (const char *)aax.info(AAX_RENDERER_STRING);

    std::cout << "Vendor: " << _vendor << std::endl;
    std::cout << "Renderer: " << _renderer << std::endl;
     
    return 0;
}
