#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#if defined (__APPLE__)
#  ifdef __GNUC__
#    if ( __GNUC__ >= 3 ) && ( __GNUC_MINOR__ >= 3 )
//  #        include <math.h>
inline int (isnan)(double r) { return !(r <= 0 || r >= 0); }
#    else
    // any C++ header file undefines isinf and isnan
    // so this should be included before <iostream>
    // the functions are STILL in libm (libSystem on mac os x)
extern "C" int isnan (double);
extern "C" int isinf (double);
#    endif
#  else
//    inline int (isinf)(double r) { return isinf(r); }
//    inline int (isnan)(double r) { return isnan(r); }
#  endif
#endif

#if defined (__FreeBSD__)
#  if __FreeBSD_version < 500000
     extern "C" {
       inline int isnan(double r) { return !(r <= 0 || r >= 0); }
     }
#  endif
#endif

#if defined (__CYGWIN__)
#  include <ieeefp.h>
#endif

#if defined(__MINGW32__)
#  define isnan(x) _isnan(x)
#endif

#include "soundmgr_openal.hxx"
#include "sample_group.hxx"

SGSampleGroup::SGSampleGroup () :
    _smgr(NULL),
    _refname(""),
    _active(false),
    _tied_to_listener(false),
    _position(SGVec3d::zeros()),
    _velocity(SGVec3f::zeros()),
    _orientation(SGVec3f::zeros())
{
    _samples.clear();
}

SGSampleGroup::SGSampleGroup ( SGSoundMgr *smgr, const string &refname ) :
    _smgr(smgr),
    _refname(refname),
    _active(false), 
    _tied_to_listener(false),
    _position(SGVec3d::zeros()),
    _velocity(SGVec3f::zeros()),
    _orientation(SGVec3f::zeros())
{
    _smgr->add(this, refname);
    _active = _smgr->is_working();
    _samples.clear();
}

SGSampleGroup::~SGSampleGroup ()
{
    _active = false;

    sample_map_iterator sample_current = _samples.begin();
    sample_map_iterator sample_end = _samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;

        if ( sample->is_valid_source() && sample->is_playing() ) {
            sample->no_valid_source();
            _smgr->release_source( sample->get_source() );
            _smgr->release_buffer( sample );
        }
    }

    _smgr = 0;
}

void SGSampleGroup::update( double dt ) {

    if ( !_active ) return;

    // testForALError("start of update!!\n");

    sample_map_iterator sample_current = _samples.begin();
    sample_map_iterator sample_end = _samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;

        if ( !sample->is_valid_source() && sample->is_playing() ) {
            //
            // a request to start playing a sound has been filed.
            //
            if ( _smgr->request_buffer(sample) == SGSoundMgr::NO_BUFFER )
                continue;

            if ( _tied_to_listener && _smgr->has_changed() ) {
                sample->set_base_position( _smgr->get_position_vec() );
                sample->set_orientation( _smgr->get_direction() );
                sample->set_velocity( _smgr->get_velocity() );
            }

            // start playing the sample
            ALboolean looping = sample->get_looping() ? AL_TRUE : AL_FALSE;
            ALuint buffer = sample->get_buffer();
            ALuint source = _smgr->request_source();
            if (alIsSource(source) == AL_TRUE && alIsBuffer(buffer) == AL_TRUE)
            {
                sample->set_source( source );
                
                alSourcei( source, AL_BUFFER, buffer );
                testForALError("assign buffer to source");

                sample->set_source( source );
                update_sample_config( sample );

                alSourcei( source, AL_SOURCE_RELATIVE, AL_FALSE );
                alSourcei( source, AL_LOOPING, looping );
                alSourcef( source, AL_ROLLOFF_FACTOR, 1.2 );
                alSourcePlay( source );
                testForALError("sample play");
            } else {
                if (alIsBuffer(buffer) == AL_FALSE) 
                   SG_LOG( SG_GENERAL, SG_ALERT, "No such buffer!\n");
                // sample->no_valid_source();
                // sadly, no free source available at this time
            }

        } else if ( sample->is_valid_source() && sample->has_changed() ) {
            if ( !sample->is_playing() ) {
                // a request to stop playing the sound has been filed.

                sample->no_valid_source();
                sample->stop();
                _smgr->release_source( sample->get_source() );
            } else  {
                update_sample_config( sample );
            }

        } else if ( sample->is_valid_source() ) {
            // check if the sound has stopped by itself

            unsigned int source = sample->get_source();
            int result;

            alGetSourcei( source, AL_SOURCE_STATE, &result );
            if ( result == AL_STOPPED ) {
                // sample is stoped because it wasn't looping
                sample->no_valid_source();
                sample->stop();
                _smgr->release_source( source );
            }
        }
        testForALError("update");
    }
}

// add a sound effect, return true if successful
bool SGSampleGroup::add( SGSoundSample *sound, const string& refname ) {

    sample_map_iterator sample_it = _samples.find( refname );
    if ( sample_it != _samples.end() ) {
        // sample name already exists
        return false;
    }

    _samples[refname] = sound;
    return true;
}


// remove a sound effect, return true if successful
bool SGSampleGroup::remove( const string &refname ) {

    sample_map_iterator sample_it = _samples.find( refname );
    if ( sample_it == _samples.end() ) {
        // sample was not found
        return false;
    }

    // remove the sources buffer
    _smgr->release_buffer( sample_it->second );
    _samples.erase( refname );

    return true;
}


// return true of the specified sound exists in the sound manager system
bool SGSampleGroup::exists( const string &refname ) {
    sample_map_iterator sample_it = _samples.find( refname );
    if ( sample_it == _samples.end() ) {
        // sample was not found
        return false;
    }

    return true;
}


// return a pointer to the SGSoundSample if the specified sound exists
// in the sound manager system, otherwise return NULL
SGSoundSample *SGSampleGroup::find( const string &refname ) {
    sample_map_iterator sample_it = _samples.find( refname );
    if ( sample_it == _samples.end() ) {
        // sample was not found
        return NULL;
    }

    return sample_it->second;
}


// stop playing all associated samples
void
SGSampleGroup::suspend ()
{
    _active = false;
    sample_map_iterator sample_current = _samples.begin();
    sample_map_iterator sample_end = _samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;

        if ( sample->is_valid_source() && sample->is_playing() ) {
            unsigned int source = sample->get_source();
            alSourcePause( source );
        }
    }
    testForALError("suspend");
}

// resume playing all associated samples
void
SGSampleGroup::resume ()
{
    sample_map_iterator sample_current = _samples.begin();
    sample_map_iterator sample_end = _samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;

        if ( sample->is_valid_source() && sample->is_playing() ) {
            unsigned int source = sample->get_source();
            alSourcePlay( source );
        }
    }
    testForALError("resume");
    _active = true;
}


// tell the scheduler to play the indexed sample in a continuous loop
bool SGSampleGroup::play( const string &refname, bool looping = false ) {
    SGSoundSample *sample = find( refname );

    if ( sample == NULL ) {
        return false;
    }

    sample->play( looping );
    return true;
}


// return true of the specified sound is currently being played
bool SGSampleGroup::is_playing( const string& refname ) {
    SGSoundSample *sample = find( refname );

    if ( sample == NULL ) {
        return false;
    }

    return ( sample->is_playing() ) ? true : false;
}

// immediate stop playing the sound
bool SGSampleGroup::stop( const string& refname ) {
    SGSoundSample *sample  = find( refname );

    if ( sample == NULL ) {
        return false;
    }

    sample->stop();
    return true;
}


// set source position of all managed sounds
void SGSampleGroup::set_position( SGVec3d pos ) {
    if ( isnan(pos.data()[0]) || isnan(pos.data()[1]) || isnan(pos.data()[2]) )
    {
        SG_LOG( SG_GENERAL, SG_ALERT, "NAN's found in SampleGroup postion");
        return;
    }

    if ( !_tied_to_listener && _position != pos ) {
        sample_map_iterator sample_current = _samples.begin();
        sample_map_iterator sample_end = _samples.end();
        for ( ; sample_current != sample_end; ++sample_current ) {
            SGSoundSample *sample = sample_current->second;
            sample->set_base_position( pos );
        }
        _position = pos;
    }
}

// set source velocity of all managed sounds
void SGSampleGroup::set_velocity( SGVec3f vel ) {
    if ( isnan(vel.data()[0]) || isnan(vel.data()[1]) || isnan(vel.data()[2]) )
    {
        SG_LOG( SG_GENERAL, SG_ALERT, "NAN's found in SampleGroup velocity");
        return;
    }

    if (_velocity != vel) {
        sample_map_iterator sample_current = _samples.begin();
        sample_map_iterator sample_end = _samples.end();
        for ( ; sample_current != sample_end; ++sample_current ) {
            SGSoundSample *sample = sample_current->second;
            sample->set_velocity( vel );
        }
    }
}

// ste the source orientation of all managed sounds
void SGSampleGroup::set_orientation( SGVec3f ori ) {
    if ( isnan(ori.data()[0]) || isnan(ori.data()[1]) || isnan(ori.data()[2]) )
    {
        SG_LOG( SG_GENERAL, SG_ALERT, "NAN's found in SampleGroup orientation");
        return;
    }

    if (_orientation != ori) {
        sample_map_iterator sample_current = _samples.begin();
        sample_map_iterator sample_end = _samples.end();
        for ( ; sample_current != sample_end; ++sample_current ) {
            SGSoundSample *sample = sample_current->second;
            sample->set_orientation( ori );
        }
    }
}

void SGSampleGroup::update_sample_config( SGSoundSample *sample ) {
    if ( sample->is_valid_source() ) {
        unsigned int source = sample->get_source();

        alSourcefv( source, AL_POSITION, sample->get_position());
        alSourcefv( source, AL_DIRECTION, sample->get_orientation() );
        alSourcefv( source, AL_VELOCITY, sample->get_velocity() );
        testForALError("position and orientation");

        alSourcef( source, AL_PITCH, sample->get_pitch() );
        alSourcef( source, AL_GAIN, sample->get_volume() );
        testForALError("pitch and gain");

        if ( sample->has_static_data_changed() ) {
            alSourcef( source, AL_CONE_INNER_ANGLE, sample->get_innerangle() );
            alSourcef( source, AL_CONE_OUTER_ANGLE, sample->get_outerangle() );
            alSourcef( source, AL_CONE_OUTER_GAIN, sample->get_outergain() );
            testForALError("audio cone");

            alSourcef( source, AL_MAX_DISTANCE, sample->get_max_dist() );
            alSourcef( source, AL_REFERENCE_DISTANCE,
                               sample->get_reference_dist() );
            testForALError("distance rolloff");
        }
    }
}

void SGSampleGroup::set_volume( float vol )
{
    _volume = vol;
    if (_volume < 0.0) _volume = 0.0;
    if (_volume > 1.0) _volume = 1.0;

    sample_map_iterator sample_current = _samples.begin();
    sample_map_iterator sample_end = _samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;
        sample->set_master_volume( _volume );
    }
}

bool SGSampleGroup::testForError(void *p, string s)
{
   if (p == NULL) {
      SG_LOG( SG_GENERAL, SG_ALERT, "Error (sample group): " << s);
      return true;
   }
   return false;
}

bool SGSampleGroup::testForALError(string s)
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR)  {
       SG_LOG( SG_GENERAL, SG_ALERT, "AL Error (" << _refname << "): "
                                      << alGetString(error) << " at " << s);
       return true;
    }
    return false;
}

