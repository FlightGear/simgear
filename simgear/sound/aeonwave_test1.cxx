#include <stdio.h>
#include <cstdlib> // EXIT_FAILURE
#include <string>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>	// sleep()
#endif

#include <aax/aeonwave.hpp>

#define AUDIOFILE	SRC_DIR"/jet.wav"

bool testForError(aax::AeonWave& p, std::string s)
{
    enum aaxErrorType error = p.error_no();
    if (error != AAX_ERROR_NONE) {
       std::cout << "AeonWave Error: "
                 << aax::strerror(error) << " at " << s << std::endl;
        return true;
    }
    return false;
}


int main( int argc, char *argv[] ) 
{
    aax::AeonWave aax = aax::AeonWave(AAX_MODE_WRITE_STEREO);

    aax.set(AAX_INITIALIZED);
    testForError(aax, "initialization");

    aax.set(AAX_PLAYING);
    testForError(aax, "mixer palying");

    aax::Emitter emitter(AAX_ABSOLUTE);
    aax.add(emitter);
    testForError(aax, "emitter registering");

    aax::Emitter emitter2;
    emitter2 = aax::Emitter(AAX_ABSOLUTE);
    aax.add(emitter2);
    testForError(aax, "emitter reasignment");

    aax::dsp dsp;
    dsp = aax::dsp(aax, AAX_VOLUME_FILTER);
    dsp.set(AAX_GAIN, 0.0f);
    aax.set(dsp);

    dsp = aax::dsp(aax, AAX_DISTANCE_FILTER);
    dsp.set(AAX_AL_INVERSE_DISTANCE_CLAMPED);
    aax.set(dsp);

    dsp = aax::dsp(aax, AAX_VELOCITY_EFFECT);
    dsp.set(AAX_DOPPLER_FACTOR, 1.0f);
    dsp.set(AAX_SOUND_VELOCITY, 340.3f);
    aax.set(dsp);

    testForError(aax, "scenery setup");

    std::string _vendor = (const char *)aax.info(AAX_VENDOR_STRING);
    std::string _renderer = (const char *)aax.info(AAX_RENDERER_STRING);

    std::cout << "Vendor: " << _vendor << std::endl;
    std::cout << "Renderer: " << _renderer << std::endl;

    aax::Matrix mtx;
    mtx.translate(-5000.0, 12500.0, 1000.0);

    aax::Matrix64 mtx64 = mtx.toMatrix64();
    emitter.matrix(mtx64);

    mtx.translate(-5.0, 2.0, 1.0);
    mtx64.inverse();
    aax.sensor_matrix(mtx64);

    aax.set(AAX_PLAYING);
    emitter.set(AAX_PLAYING);
    aax.set(AAX_UPDATE);
     
    return 0;
}
