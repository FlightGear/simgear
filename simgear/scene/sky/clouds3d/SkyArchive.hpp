//------------------------------------------------------------------------------
// File : SkyArchive.hpp
//------------------------------------------------------------------------------
// SkyWorks : Copyright 2002 Mark J. Harris and
//						The University of North Carolina at Chapel Hill
//------------------------------------------------------------------------------
// Permission to use, copy, modify, distribute and sell this software and its 
// documentation for any purpose is hereby granted without fee, provided that 
// the above copyright notice appear in all copies and that both that copyright 
// notice and this permission notice appear in supporting documentation. 
// Binaries may be compiled with this software without any royalties or 
// restrictions. 
//
// The author(s) and The University of North Carolina at Chapel Hill make no 
// representations about the suitability of this software for any purpose. 
// It is provided "as is" without express or 
// implied warranty.
/**
 * @file SkyArchive.hpp
 * 
 * A hierarchical archive Class for storing data.                           
 */
#ifndef __SKYARCHIVE_HPP__
#define __SKYARCHIVE_HPP__

// #pragma warning( disable : 4786 )

#include "vec2f.hpp"
#include "vec3f.hpp"
#include "vec4f.hpp"
#include "SkyUtil.hpp"

#include <map> // for std::multimap

//! Types supported by the archive system
enum SkyArchiveTypeCode
{
    BOOL_TYPE,
    INT8_TYPE,
    INT16_TYPE,
    INT32_TYPE,
    UINT8_TYPE,
    UINT16_TYPE,
    UINT32_TYPE,
    FLOAT32_TYPE,
    FLOAT64_TYPE,
    STRING_TYPE,
    VEC2F_TYPE,
    VEC3F_TYPE,
    VEC4F_TYPE,
    ARCHIVE_TYPE,
    ANY_TYPE,
    NULL_TYPE
};

struct SkyArchiveEntry;

struct StringLessFunctor
{
    bool operator() (const char* p1, const char* p2) const
    {
       return ::strcmp( p1, p2) < 0;
    }
};

//============================================================================
//
//  Class       :   SkyArchive
//             
//! A simple hierarchical archive file useful for loading and saving data.
//
/*! SkyArchive bundles information so that an application can store, 
    manipulate, and retrieve data in a storage-independent manner.  Information 
    stored in an SkyArchive file can be modified without breaking 
    compatibility with the code that retrieves the data.  In essence, it can be 
    thought of as a very basic database mechanism.

    A SkyArchive is simply a container.  The class defines methods that allow 
    you to put information in a SkyArchive, determine the information in a
    SkyArchive, and retrieve information from a SkyArchive.  It's important 
    to note that the SkyArchive is a recursive storage mechanism; a 
    SkyArchive itself can hold one or more other SkyArchives.

    Data are added to an archive in fields.  The datum in a field is associated 
    with a name, number of bytes, and a type code.  The name can be anything you 
    choose and is not required to be unique.  The number of bytes must be accurate.  
    The type code must be one of the SkyArchiveTypeCode enums.  Several of the Add 
    functions have been specialized for the common base types.  It isn't necessary 
    to provide the number of bytes when using the specialized functions, since it can be 
    inferred from the type code.

    The functions that retrieve fields from an archive are similar to the ones 
    that add data, only their roles are reversed.  As with the Add functions, 
    there are several specialized functions for the common base types while 
    custom information can be retrieved using the generic FindData function.
    
    Querying the contents of an archive is provided through the GetInfo 
    functions.  These functions are important as they allow you to write 
    code that can intelligently determine how to retrieve information at 
    run-time: you no longer have to hardcode the order in which you retrieve 
    data from your files.

    archive data fields are held in using an STL multimap.  The multimap key
    is a QString (field name) and the data is held in a SkyArchiveEntry
    structure (see SkyArchive.cpp for details of this structure).
    The fields are stored in alphabetical order based on the key names.
*/
class SkyArchive
{
public:
    //=========================================================================
    // Creation & Destruction
    //=========================================================================
    // Empty archive: no name, no data fields.
    SkyArchive();
    // Creates a named archive with no data fields.
    SkyArchive( const char* pName);
    // Deep copy the contents of one archive to another.
    SkyArchive( const SkyArchive& src);
    // Deep copy the contents of one archive to another.
    SkyArchive& operator=( const SkyArchive& src);
    
    ~SkyArchive();

    //=========================================================================
    //  Basic SkyArchive Information
    //=========================================================================
    // Returns true if the archive contains no data fields.
    bool        IsEmpty() const;
    //! Returns the archive's name.
    const char* GetName() const { return _pName; }; 

    //=========================================================================
    //  Adding Content to SkyArchive
    //=========================================================================
    // Adds a new datafield to the archive.
    SKYRESULT AddData(const char*         pName, 
                      SkyArchiveTypeCode  eType, 
                      const void*         pData,
                      unsigned int        iNumBytes, 
                      unsigned int        iNumItems = 1);

    SKYRESULT AddBool(    const char* pName, bool           aBool);
    SKYRESULT AddInt8(    const char* pName, char           anInt8);
    SKYRESULT AddInt16(   const char* pName, short          anInt16);
    SKYRESULT AddInt32(   const char* pName, int            anInt32);
    SKYRESULT AddUInt8(   const char* pName, unsigned char  anUInt8);
    SKYRESULT AddUInt16(  const char* pName, unsigned short anUInt16);
    SKYRESULT AddUInt32(  const char* pName, unsigned int   anUInt32);
    SKYRESULT AddFloat32( const char* pName, float          aFloat32);
    SKYRESULT AddFloat64( const char* pName, double         aFloat64);
    SKYRESULT AddString(  const char* pName, const char*    pString);
       
    SKYRESULT AddArchive( const SkyArchive& anArchive);
   
    // Vector types (MJH::    only supports float versions for now!!!)
    SKYRESULT AddVec2f(   const char* pName, const Vec2f&   aPoint2f);
    SKYRESULT AddVec3f(   const char* pName, const Vec3f&   aPoint3f);
    SKYRESULT AddVec4f(   const char* pName, const Vec4f&   aPoint4f);
   
    //=========================================================================
    // Retrieving Content from SkyArchive
    //=========================================================================
    // Retrieves the specified datafield the archive, if it exists.
    SKYRESULT FindData(   const char*         pName, 
                          SkyArchiveTypeCode  eType, 
                          void** const        pData,
                          unsigned int*       pNumBytes, 
                          unsigned int        index = 0) const;

    SKYRESULT FindBool(   const char* pName, bool*           pBool,    unsigned int index = 0) const;
    SKYRESULT FindInt8(   const char* pName, char*           pInt8,    unsigned int index = 0) const;
    SKYRESULT FindInt16(  const char* pName, short*          pInt16,   unsigned int index = 0) const;
    SKYRESULT FindInt32(  const char* pName, int*            pInt32,   unsigned int index = 0) const;
    SKYRESULT FindUInt8(  const char* pName, unsigned char*  pUInt8,   unsigned int index = 0) const;
    SKYRESULT FindUInt16( const char* pName, unsigned short* pUInt16,  unsigned int index = 0) const;
    SKYRESULT FindUInt32( const char* pName, unsigned int*   pUInt32,  unsigned int index = 0) const;
    SKYRESULT FindFloat32(const char* pName, float*          pFloat32, unsigned int index = 0) const;
    SKYRESULT FindFloat64(const char* pName, double*         pFloat64, unsigned int index = 0) const;
    SKYRESULT FindString( const char* pName, char** const    pString,  unsigned int index = 0) const;
    SKYRESULT FindArchive(const char* pName, SkyArchive*     pArchive, unsigned int index = 0) const;

    SKYRESULT FindVec2f(  const char* pName, Vec2f*          pVec2f,   unsigned int index = 0) const;
    SKYRESULT FindVec3f(  const char* pName, Vec3f*          pVec3f,   unsigned int index = 0) const;
    SKYRESULT FindVec4f(  const char* pName, Vec4f*          pVec4f,   unsigned int index = 0) const;

    SKYRESULT AccessArchive(const char* pName, SkyArchive**  pArchive, unsigned int index = 0) const;

    //=========================================================================
    // Querying Contents of SkyArchive
    //=========================================================================
    // Computes the number of fields that contain the given name and type.
    SKYRESULT GetInfo(const char*         pName, 
                      SkyArchiveTypeCode  eType, 
                      unsigned int*       pNumFound = NULL) const;

    // Returns information about the key at the specified index.
    SKYRESULT GetInfo(unsigned int        iNameIndex, 
                      char**              pNameFound, 
                      SkyArchiveTypeCode* pTypeCode,
                      unsigned int*       pNumFound);
                    
    // Computes the number of unique key names in _dataTableable.
    unsigned int GetNumUniqueNames() const;

    
    // Remove the contents of the SkyArchive.
    SKYRESULT MakeEmpty();

    // Loads the contents from a file.
    bool Load(const char* pFileName);
    
    // Commits the contents of a SkyArchive to file storage.
    SKYRESULT Save(const char* pFileName) const;

private:

    char*           _pName;  // this archive's name

    //=========================================================================
    //  Data storage
    //=========================================================================
    typedef std::multimap<char*, SkyArchiveEntry*, StringLessFunctor> SkyArchiveMMap;
    typedef SkyArchiveMMap::const_iterator                            SkyMMapConstIter;
    typedef SkyArchiveMMap::iterator                                  SkyMMapIter;

    SkyArchiveMMap  _dataTable;     // this is where the data reside.

    // Performs a deep-copy of one archive's archive_mmap_t to another.
    void                    _CopyDataTable( const SkyArchiveMMap& src);

    // Locates an archive entry in _dataTable
    const SkyArchiveEntry*  _FindEntry( const char*        pName, 
                                        unsigned int       index, 
                                        SkyArchiveTypeCode eType) const;
   
    // Saves the archive to a file stream.
    SKYRESULT       _Save(FILE* pDestFile) const;
    // Initializes the archive from a file stream.
    SKYRESULT       _Load(FILE* pSrcFile);
};

#endif //__SKYARCHIVE_HPP__
