/** \file debug_types.h
 *  Define the various logging classes and priorities
 */

/** 
 * Define the possible classes/categories of logging messages
 */
typedef enum {
    SG_NONE        = 0x00000000,

    SG_TERRAIN     = 0x00000001,
    SG_ASTRO       = 0x00000002,
    SG_FLIGHT      = 0x00000004,
    SG_INPUT       = 0x00000008,
    SG_GL          = 0x00000010,
    SG_VIEW        = 0x00000020,
    SG_COCKPIT     = 0x00000040,
    SG_GENERAL     = 0x00000080,
    SG_MATH        = 0x00000100,
    SG_EVENT       = 0x00000200,
    SG_AIRCRAFT    = 0x00000400,
    SG_AUTOPILOT   = 0x00000800,
    SG_IO          = 0x00001000,
    SG_CLIPPER     = 0x00002000,
    SG_NETWORK     = 0x00004000,
    SG_ATC         = 0x00008000,
    SG_NASAL       = 0x00010000,
    SG_INSTR       = 0x00020000,
    SG_SYSTEMS     = 0x00040000,
    SG_AI          = 0x00080000,
    SG_ENVIRONMENT = 0x00100000,
    SG_SOUND       = 0x00200000,
    SG_NAVAID      = 0x00400000,
    SG_GUI         = 0x00800000,
    SG_UNDEFD      = 0x01000000, // For range checking

    SG_ALL         = 0xFFFFFFFF
} sgDebugClass;


/**
 * Define the possible logging priorities (and their order).
 */
typedef enum {
    SG_BULK = 1,       // For frequent messages
    SG_DEBUG,          // Less frequent debug type messages
    SG_INFO,           // Informatory messages
    SG_WARN,           // Possible impending problem
    SG_ALERT           // Very possible impending problem
    // SG_EXIT,        // Problem (no core)
    // SG_ABORT        // Abandon ship (core)
} sgDebugPriority;

