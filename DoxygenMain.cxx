/* This is a dummy code file that only contains doxygen main page
   documentation.  It has a .cxx extension so that emacs will happily
   autoindent correctly. */

/**
 * \namespace simgear
 * \brief \ref index "SimGear" main namespace.
 */
/** \mainpage SimGear
 * Simulation, Visualization, and Game development libraries.

 * \section intro Introduction
 *
 * SimGear is a collection of libraries which provide a variety of
 * functionality useful for building simulations, visualizations, and
 * even games.  All the SimGear code is designed to be portable across
 * a wide variety of platforms and compilers.  It has primarily been
 * developed in support of the FlightGear project, but as development
 * moves forward, we are generalizing the code to make more of it
 * useful for other types of applications.
 *
 * Some of the functionality provide includes
 *
 * - Compiler and platform abstractions for many tricky differences.
 *   (compiler.h)
 *
 * - A whole earth tiling/indexing scheme.  (SGBucket)
 *
 * - A console debugging output scheme that tracks severity and
 *   category that can be completely compiled out for a final build release.
 *   (logstream.hxx)
 *
 * - Code to manage "real" time (SGTime), time zones (SGTimeZone), and
 *   millesecond time differences (SGTimeStamp).
 *
 * - Code to calculate accurate positions of sun, moon, stars, and
 *   planets for a given time, date, season, earth location, etc.
 *   (SGEphemeris)
 *
 * - Code to render a realistic sky dome, cloud layers, sun, moon,
 *   stars, and planets all with realistic day/night/sunset/sunrise
 *   effects.  Includes things like correct moon phase, textured moon,
 *   sun halo, etc. (SGSky is built on top of SGCloudLayer ...)
 *
 * - Simple serial (SGSerial), file (SGFile), socket (SGSocket), and
 *   UDP socket (SGSocketUDP) I/O abstractions.
 *
 * - Code to calculate magnetic variation. (SGMagVar)
 *
 * - A variety of classes and functions for interpolation tables
 *   (SGInterpTable), least squares computation (leastsqs.hxx), 3D
 *   point/vectors (Point3D), 3D polar math and conversions (polar3d.hxx),
 *   WGS-84 math and conversions (sg_geodesy.hxx), random number abstraction
 *   (sg_random.h), STL conglomerates for common list types (sg_types.hxx),
 *   and other vector and linear algebra routines (vector.hxx)
 *
 * - An abstraction to hide platform dependent path naming schemes. (SGPath)
 *
 * - A C++ streams wrapper to handle compress input/output streams.
 *   (sg_gzifstream)
 *
 * - An optimized "property manager" which associates ascii property
 *   names with their corresponding value.  This can be a great way to build
 *   loose linkages between modules, or build linkages/connections that can
 *   be determined from config files or at runtime.  (SGPropertyNode)
 *   Also included is a set of functions to dump the property tree into a
 *   standard xml file and subsequently read/parse a standard xml file and
 *   rebuild the associated property tree.  (props_io.hxx)
 *
 * - Scene management and drawing routines:
 *   - material property management
 *   - object management
 *   - terrain tile management and paging
 *   - sky dome rendering (with ephemeral objects)
 *
 * - Code to handle screen dumps (screen-dump.hxx) and ultra-hires
 *   tile rendered screen dumps (tr.h)
 *
 * - A sound effects manager.  (SGSoundMgr, SGSimpleSound, SGSound)
 *
 * - A threading abstraction.  (SGThread)
 *
 * - A simple but highly functional XML parser that interfaces nicely
 *   with the property manager.  (easyxml.hxx)

 * \section supports Supported Platforms
 * SimGear has been built on the following platforms:
 *
 *   - Linux (x86)
 *   - Windows (MSVC, Cygwin, Mingwin)
 *   - SGI (native compilers)
 *   - Mac OS X
 *   - FreeBSD
 
 * \section depends Dependencies
 *
 * SimGear depends on several other open source packages.  These must
 * be installed before SimGear can be installed:
 *
 *   - glut and opengl
 *   - plib (http://plib.sf.net)
 *   - metakit
 *   - zlib
 *   - libjpeg (optional)
 *   - pthread (optional)
 
 * \section license Licensing
 *
 * SimGear is licensed under the terms of the LGPL

 */

