//------------------------------------------------------------------------------
// File : SkyArchive.cpp
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
 * @file SkyArchive.cpp
 * 
 * Implementation of class SkyArchive.
 */
#include "SkyArchive.hpp"

#include <assert.h>

struct SkyArchiveEntry
{
  SkyArchiveEntry() : type(0), pData(NULL), iDataSize(0) {}
  unsigned char type;
  void*         pData;
  unsigned int  iDataSize;
};

struct SkyArchiveFileEntry
{
  SkyArchiveFileEntry() : type(0), iDataSize(0) {}
  unsigned char type;
  char          pName[32];
  unsigned int  iDataSize;
};


//------------------------------------------------------------------------------
// Function     	  : SkyArchive::SkyArchive
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::SkyArchive()
 * @brief Default constructor.  Creates an empty, unnamed archive.                                             |
 */ 
SkyArchive::SkyArchive()
: _pName(NULL)
{
}


//------------------------------------------------------------------------------
// Function     	  : SkyArchive::SkyArchive
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::SkyArchive(const char* pName)
 * @brief Constructor.  Creates an empty, named archive. 
 */ 
SkyArchive::SkyArchive(const char* pName)
{
  _pName = new char[::strlen(pName)+1];
  ::strcpy( _pName, pName);  
}


//.---------------------------------------------------------------------------.
//|   Function   : SkyArchive::SkyArchive                                 |
//|   Description: 
//.---------------------------------------------------------------------------.

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::SkyArchive
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::SkyArchive(const SkyArchive& src)
 * @brief Copy constructor.  Deep-copies the contents of one archive to another.
 */ 
SkyArchive::SkyArchive(const SkyArchive& src)
{  
  _pName = new char[::strlen(src._pName)+1];
  ::strcpy( _pName, src._pName);
  
  _CopyDataTable( src._dataTable);
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::~SkyArchive
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::~SkyArchive()
 * @brief Destructor.
 */ 
SkyArchive::~SkyArchive()
{
  MakeEmpty();
  SAFE_DELETE_ARRAY(_pName);
}


//.---------------------------------------------------------------------------.
//|   Function   : SkyArchive::operator=                                    |
//|   Description: |
//.---------------------------------------------------------------------------.

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::operator=
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::operator=( const SkyArchive& src)
 * @brief @todo Deep-copies the contents of one archive to another.
 */ 
SkyArchive& SkyArchive::operator=( const SkyArchive& src)
{
  if (this != &src)
  {
    MakeEmpty();
    SAFE_DELETE_ARRAY(_pName);
    _pName = new char[::strlen(src._pName)+1];
    ::strcpy( _pName, src.GetName());
    
    _CopyDataTable( src._dataTable);
  }
  return *this;
}



//=============================================================================
//  Adding Content to SkyArchive
//=============================================================================

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddData
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyArchive::AddData(const char*         pName, 
SkyArchiveTypeCode  eType, 
const void*         pData, 
unsigned int        iNumBytes, 
unsigned int        iNumItems)
* @brief Adds a new data field to the archive.
* 
* Makes a copy of the data, stores it in a SkyArchiveEntry, and adds it to the
* database.  All specialized functions for the base types are implemented using 
* this function.
* 
* PARAMETERS
* @param name Field name.  This is used as the key for the database entry.
* @param type Data type of the field.
* @param pData Pointer to the data to be added to the archive.
* @param iNumBytes Size of each individual item in the data
* @param iNumItems Number of items to copy                                  
*/
SKYRESULT SkyArchive::AddData(const char*         pName, 
                              SkyArchiveTypeCode  eType, 
                              const void*         pData, 
                              unsigned int        iNumBytes, 
                              unsigned int        iNumItems /* = 1 */)
{
  // fill out a new archive entry with the supplied data
  SkyArchiveEntry* pNewEntry  = new SkyArchiveEntry;
  pNewEntry->type             = eType;
  pNewEntry->iDataSize        = iNumBytes * iNumItems;
  
  if (eType != ARCHIVE_TYPE)
  {
    pNewEntry->pData = new unsigned char[pNewEntry->iDataSize];
    ::memcpy(pNewEntry->pData, pData, pNewEntry->iDataSize);
  }
  else
  {
    pNewEntry->pData = (void*)pData;
  }
  
  char* pInternalName = new char[::strlen(pName)+1];
  ::strcpy( pInternalName, pName);
  _dataTable.insert(std::make_pair(pInternalName, pNewEntry));
  
  return SKYRESULT_OK;
}


//-----------------------------------------------------------------------------
//  SkyArchive :: AddBool( const char* pName, bool aBool)
//  SkyArchive :: AddInt8( const char* pName, Int8 anInt8)
//  SkyArchive :: AddInt16( const char* pName, Int16 anInt16)
//  SkyArchive :: AddInt32( const char* pName, Int32 anInt32)
//  ...
//-----------------------------------------------------------------------------
//
//  Specialized functions for the most common base types
//  
//-----------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddBool
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddBool(const char* pName, bool aBool)
 * @brief Adds a named bool to the archive.
 */ 
SKYRESULT SkyArchive::AddBool(const char* pName, bool aBool)
{
  return AddData( pName, BOOL_TYPE, &aBool, sizeof(bool));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddInt8
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddInt8(const char* pName, char anInt8)
 * @brief Adds a named 8-bit integer to the archive.
 */ 
SKYRESULT SkyArchive::AddInt8(const char* pName, char anInt8)
{
  return AddData( pName, INT8_TYPE, &anInt8, sizeof(char));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddInt16
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddInt16(const char* pName, short anInt16)
 * @brief Adds a named 16-bit integer to the archive.
 */ 
SKYRESULT SkyArchive::AddInt16(const char* pName, short anInt16)
{
  return AddData( pName, INT16_TYPE, &anInt16, sizeof(short));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddInt32
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddInt32(const char* pName, int anInt32)
 * @brief Adds a named 32-bit integer to the archive.
 */ 
SKYRESULT SkyArchive::AddInt32(const char* pName, int anInt32)
{
  return AddData( pName, INT32_TYPE, &anInt32, sizeof(int));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddUInt8
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddUInt8(const char* pName, unsigned char anUInt8)
 * @brief Adds a named unsigned 8-bit integer to the archive.
 */ 
SKYRESULT SkyArchive::AddUInt8(const char* pName, unsigned char anUInt8)
{
  return AddData( pName, UINT8_TYPE, &anUInt8, sizeof(unsigned char));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddUInt16
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddUInt16(const char* pName, unsigned short anUInt16)
 * @brief Adds a named unsigned 16-bit integer to the archive.
 */ 
SKYRESULT SkyArchive::AddUInt16(const char* pName, unsigned short anUInt16)
{
  return AddData( pName, UINT16_TYPE, &anUInt16, sizeof(unsigned short));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddUInt32
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddUInt32(const char* pName, unsigned int anUInt32)
 * @brief Adds a named unsigned 32-bit integer to the archive.
 */ 
SKYRESULT SkyArchive::AddUInt32(const char* pName, unsigned int anUInt32)
{
  return AddData( pName, UINT32_TYPE, &anUInt32, sizeof(unsigned int));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddFloat32
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddFloat32(const char* pName, float aFloat32)
 * @brief Adds a named 32-bit real number to the archive.
 */ 
SKYRESULT SkyArchive::AddFloat32(const char* pName, float aFloat32)
{
  return AddData( pName, FLOAT32_TYPE, &aFloat32, sizeof(float));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddFloat64
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddFloat64(const char* pName, double aFloat64)
 * @brief Adds a named 64-bit real number to the archive.
 */ 
SKYRESULT SkyArchive::AddFloat64(const char* pName, double aFloat64)
{
  return AddData( pName, FLOAT64_TYPE, &aFloat64, sizeof(double));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddString
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddString(const char* pName, const char* pString)
 * @brief Adds a named string to the archive.
 */ 
SKYRESULT SkyArchive::AddString(const char* pName, const char* pString)
{
  return AddData( pName, STRING_TYPE, pString, ::strlen(pString)+1);
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddArchive
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddArchive(const SkyArchive& anArchive)
 * @brief Adds a subarchive to this archive.
 *
 * This method allows hierarchical data structures to be stored in an archive.
 */
SKYRESULT SkyArchive::AddArchive(const SkyArchive& anArchive)
{
  SkyArchive* pCopy = new SkyArchive(anArchive);
  return AddData( pCopy->GetName(), ARCHIVE_TYPE, pCopy, sizeof(SkyArchive));
}

//-----------------------------------------------------------------------------
//  Adding Vector types
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddVec2f
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddVec2f(const char* pName, const Vec2f& aVec2f)
 * @brief Adds a 2-component 32-bit real number vector to the archive.
 */ 
SKYRESULT SkyArchive::AddVec2f(const char* pName, const Vec2f& aVec2f)
{
  return AddData( pName, VEC2F_TYPE, &aVec2f, sizeof(Vec2f));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddVec3f
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddVec3f(const char* pName, const Vec3f& aVec3f)
 * @brief Adds a 3-component 32-bit real number vector to the archive.
 */ 
SKYRESULT SkyArchive::AddVec3f(const char* pName, const Vec3f& aVec3f)
{
  return AddData( pName, VEC3F_TYPE, &aVec3f, sizeof(Vec3f));
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AddVec4f
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AddVec4f(const char* pName, const Vec4f& aVec4f)
 * @brief Adds a 4-component 32-bit real number vector to the archive.
 */ 
SKYRESULT SkyArchive::AddVec4f(const char* pName, const Vec4f& aVec4f)
{
  return AddData( pName, VEC4F_TYPE, &aVec4f, sizeof(Vec4f));
}

//=============================================================================
//  Retrieving Content from SkyArchive
//=============================================================================

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindData
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindData(const char*        pName, 
                               SkyArchiveTypeCode eType, 
                               void** const       pData, 
                               unsigned int*      pNumBytes, 
                               unsigned int       index) const
 * @brief Retrieves datafield from _dataTable.
 * 
 * PARAMETERS
 * @param name The field name.  used as the key for the multimap entry.
 * @param type Data type of the field.
 * @param pData Pointer to the returned data.
 * @param pNumBytes Returns the size of the field entry returned.
 * @param index Which item of the given \a name to locate.
 */ 
SKYRESULT SkyArchive::FindData(const char*        pName, 
                               SkyArchiveTypeCode eType, 
                               void** const       pData, 
                               unsigned int*      pNumBytes, 
                               unsigned int       index /* = 0 */) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, eType);
  if (pEntry)
  {
    if (pData)
    {
      *pData = new unsigned char[pEntry->iDataSize];
      ::memcpy( ((void*)*pData), pEntry->pData, pEntry->iDataSize);
    }
    
    if (pNumBytes)
    {
      *pNumBytes = pEntry->iDataSize;
    }
    
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}


//-----------------------------------------------------------------------------
//  SkyArchive :: FindBool( const char* pName, bool* aBool)
//  SkyArchive :: FindInt8( const char* pName, Int8* anInt8)
//  SkyArchive :: FindInt16( const char* pName, Int16* anInt16)
//  SkyArchive :: FindInt32( const char* pName, Int32* anInt32)
//  ...
//-----------------------------------------------------------------------------
//
//  specialized function for the most common base types
//  
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindBool
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindBool(const char* pName, bool* pBool, unsigned int index) const
 * @brief Finds a named bool in the archive.
 */ 
SKYRESULT SkyArchive::FindBool(const char* pName, bool* pBool, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, BOOL_TYPE);
  if (pEntry)
  {
    bool* pData = (bool*)(pEntry->pData);
    *pBool      = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindInt8
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindInt8(const char* pName, char* pInt8, unsigned int index) const
 * @brief Finds a named 8-bit integer in the archive.
 */ 
SKYRESULT SkyArchive::FindInt8(const char* pName, char* pInt8, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, INT8_TYPE);
  if (pEntry)
  {
    char* pData = (char*)(pEntry->pData);
    *pInt8      = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindInt16
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindInt16(const char* pName, short* pInt16, unsigned int index) const
 * @brief Finds a named 16-bit integer in the archive.
 */ 
SKYRESULT SkyArchive::FindInt16(const char* pName, short* pInt16, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, INT16_TYPE);
  if (pEntry)
  {
    short* pData  = (short*)(pEntry->pData);
    *pInt16       = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindInt32
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindInt32(const char* pName, int* pInt32, unsigned int index) const
 * @brief Finds a named 32-bit integer in the archive.
 */ 
SKYRESULT SkyArchive::FindInt32(const char* pName, int* pInt32, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, INT32_TYPE);
  if (pEntry)
  {
    int* pData  = (int*)(pEntry->pData);
    *pInt32     = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindUInt8
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindUInt8(const char* pName, unsigned char* pUInt8, unsigned int index) const
 * @brief Finds a named unsigned 8-bit integer in the archive.
 */ 
SKYRESULT SkyArchive::FindUInt8(const char* pName, unsigned char* pUInt8, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, UINT8_TYPE);
  if (pEntry)
  {
    unsigned char* pData  = (unsigned char*)(pEntry->pData);
    *pUInt8               = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindUInt16
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindUInt16(const char* pName, unsigned short* pUInt16, unsigned int index) const
 * @brief Finds a named unsigned 16-bit integer in the archive.
 */ 
SKYRESULT SkyArchive::FindUInt16(const char* pName, unsigned short* pUInt16, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, UINT16_TYPE);
  if (pEntry)
  {
    unsigned short* pData = (unsigned short*)(pEntry->pData);
    *pUInt16              = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindUInt32
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindUInt32(const char* pName, unsigned int* pUInt32, unsigned int index) const
 * @brief Finds a named unsigned 32-bit integer in the archive.
 */ 
SKYRESULT SkyArchive::FindUInt32(const char* pName, unsigned int* pUInt32, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, UINT32_TYPE);
  if (pEntry)
  {
    unsigned int* pData = (unsigned int*)(pEntry->pData);
    *pUInt32            = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindFloat32
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindFloat32(const char* pName, float* pFloat32, unsigned int index) const
 * @brief Finds a named 32-bit real number in the archive.
 */ 
SKYRESULT SkyArchive::FindFloat32(const char* pName, float* pFloat32, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, FLOAT32_TYPE);
  if (pEntry)
  {
    float* pData  = (float*)(pEntry->pData);
    *pFloat32     = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindFloat64
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindFloat64(const char* pName, double* pFloat64, unsigned int index) const
 * @brief Finds a named 64-bit real number in the archive.
 */ 
SKYRESULT SkyArchive::FindFloat64(const char* pName, double* pFloat64, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, FLOAT64_TYPE);
  if (pEntry)
  {
    double* pData = (double*)(pEntry->pData);
    *pFloat64     = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindString
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindString(const char* pName, char** const pString, unsigned int index) const
 * @brief Finds a named string in the archive.
 */ 
SKYRESULT SkyArchive::FindString(const char* pName, char** const pString, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, STRING_TYPE);
  if (pEntry)
  {
    char* pData = (char*)(pEntry->pData);
    *pString    = new char[pEntry->iDataSize];
    ::strcpy((char*)*pString, pData);
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindArchive
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindArchive(const char* pName, SkyArchive* pArchive, unsigned int index) const
 * @brief Finds a named sub-archive in the archive.
 */ 
SKYRESULT SkyArchive::FindArchive(const char* pName, SkyArchive* pArchive, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, ARCHIVE_TYPE);
  if (pEntry)
  {
    SkyArchive* pData  = (SkyArchive*)(pEntry->pData);
    *pArchive         = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindVec2f
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindVec2f(const char* pName, Vec2f* pVec2f, unsigned int index) const
 * @brief Finds a 2-component 32-bit real number vector in the archive.
 */ 
SKYRESULT SkyArchive::FindVec2f(const char* pName, Vec2f* pVec2f, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, VEC2F_TYPE);
  if (pEntry)
  {
    Vec2f* pData  = (Vec2f*)(pEntry->pData);
    *pVec2f       = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindVec3f
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindVec3f(const char* pName, Vec3f* pVec3f, unsigned int index) const
 * @brief Finds a 3-component 32-bit real number vector in the archive.
 */ 
SKYRESULT SkyArchive::FindVec3f(const char* pName, Vec3f* pVec3f, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, VEC3F_TYPE);
  if (pEntry)
  {
    Vec3f* pData  = (Vec3f*)(pEntry->pData);
    *pVec3f       = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::FindVec4f
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::FindVec4f(const char* pName, Vec4f* pVec4f, unsigned int index) const
 * @brief Finds a 4-component 32-bit real number vector in the archive.
 */ 
SKYRESULT SkyArchive::FindVec4f(const char* pName, Vec4f* pVec4f, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, VEC4F_TYPE);
  if (pEntry)
  {
    Vec4f* pData  = (Vec4f*)(pEntry->pData);
    *pVec4f       = *pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::AccessArchive
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::AccessArchive(const char* pName, SkyArchive** pArchive, unsigned int index) const
 * @brief Accesses a named sub-archive in an archive directly.
 * 
 * Note: The data are not copied!
 */ 
SKYRESULT SkyArchive::AccessArchive(const char* pName, SkyArchive** pArchive, unsigned int index) const
{
  const SkyArchiveEntry* pEntry = _FindEntry(pName, index, ARCHIVE_TYPE);
  if (pEntry)
  {
    SkyArchive* pData = (SkyArchive*)(pEntry->pData);
    *pArchive        = pData;
    return SKYRESULT_OK;
  }
  return SKYRESULT_FAIL;
}


//------------------------------------------------------------------------------
// Function     	  : SkyArchive::GetInfo
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::GetInfo(const char* pName, SkyArchiveTypeCode eType, unsigned int* pNumFound) const
 * @brief Computes the number of fields that contain the given name and type.
 * 
 * PARAMETERS
 * @param pName Field name to search for.
 * @param eType Field type to search for.
 * @param pNumFound Returns the number of fields that contain given name and type.
 */ 
SKYRESULT SkyArchive::GetInfo(const char*         pName, 
                              SkyArchiveTypeCode  eType, 
                              unsigned int*       pNumFound) const
{
  //
  // Find the range of entries in the mmap with the key matching pName
  //
  std::pair<SkyMMapConstIter, SkyMMapConstIter> b = _dataTable.equal_range((char*)pName);
  
  unsigned int count = 0;
  for ( SkyMMapConstIter i = b.first; i != b.second; ++i )
  {
    //
    // The entry's type must match...
    //
    const SkyArchiveEntry* pEntry = (*i).second;
    if (pEntry->type == eType || ANY_TYPE == eType)
    {
      // only increment the count when the type matches
      ++count;
    }
  }
  
  if (pNumFound)
  {
    *pNumFound = count;
  }
  
  if (0 == count)
    return SKYRESULT_FAIL;
  
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyArchive::GetInfo
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::GetInfo(unsigned int iNameIndex, char** pNameFound, SkyArchiveTypeCode* pTypeCode, unsigned int* pNumFound)
 * @brief Returns information about the key at the specified index.
 * 
 * PARAMETERS
 * @param nameIndex Key index to look up.
 * @param pNameFound Key name is returned here.
 * @param pTypeCode Key type is returned here.
 * @param pNumFound Number of fields held under key name is returned here.
 */ 
SKYRESULT SkyArchive::GetInfo(unsigned int        iNameIndex, 
                              char**              pNameFound, 
                              SkyArchiveTypeCode* pTypeCode,
                              unsigned int*       pNumFound)
{ 
  assert( pNameFound != NULL);
  
  if (!pNameFound)
    return SKYRESULT_FAIL;

  unsigned int      iCurrentKeyIndex = 0;
  SkyMMapConstIter  iter;
  const char*       pLastKey = "";
  
  for (iter = _dataTable.begin(); iter != _dataTable.end(); iter++)
  {
    const char* pKey = (*iter).first;
    if (::strcmp( pLastKey, pKey))
    {
      if (iCurrentKeyIndex == iNameIndex)
      {
        *pNameFound = new char[::strlen(pKey) + 1];
        ::strcpy(*pNameFound, pKey);
        
        if (pTypeCode)
        {
          const SkyArchiveEntry* pEntry = (*iter).second;
          *pTypeCode = (SkyArchiveTypeCode)pEntry->type;
        }
        
        if (pNumFound)
        {
          return GetInfo( *pNameFound, *pTypeCode, pNumFound);
        }
        return SKYRESULT_OK;
      }
      
      pLastKey = pKey;
      ++iCurrentKeyIndex;
    }
  }
  return SKYRESULT_FAIL;
}


//------------------------------------------------------------------------------
// Function     	  : SkyArchive::GetNumUniqueNames
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyArchive::GetNumUniqueNames() const
* @brief Computes the number of unique key names in _dataTable.
*/ 
unsigned int SkyArchive::GetNumUniqueNames() const
{
  // duh!
  if (IsEmpty())
    return 0;
  
  unsigned int      iNumKeys = 0;
  SkyMMapConstIter  iter;
  const char*       pLastKey = "";
  
  for (iter = _dataTable.begin(); iter != _dataTable.end(); iter++)
  {
    const char* pKey = (*iter).first;
    if (::strcmp( pLastKey, pKey))
    {
      ++iNumKeys;
      pLastKey = pKey;
    }
  }
  return iNumKeys;
}


//=============================================================================
//  Removing Contents of SkyArchive
//=============================================================================

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::MakeEmpty
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyArchive::MakeEmpty()
* @brief Remove all the contents of the database. 
*/ 
SKYRESULT SkyArchive::MakeEmpty()
{
  SkyMMapIter iter;
  
  for (iter = _dataTable.begin(); iter != _dataTable.end(); iter++)
  {
    SkyArchiveEntry* pEntry = (*iter).second;
    char* pName = (*iter).first;
    SAFE_DELETE_ARRAY(pName);
    
    if (ARCHIVE_TYPE == pEntry->type)
    {
      SkyArchive* pArchive = (SkyArchive*)(pEntry->pData);
      SAFE_DELETE(pArchive);
    }
    else
    {
      SAFE_DELETE_ARRAY(pEntry->pData);
    }
    SAFE_DELETE(pEntry);
  }
  
  _dataTable.clear();
  
  return SKYRESULT_OK;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::IsEmpty
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyArchive::IsEmpty() const
* @brief returns true if the archive is empty, false if it contains any data.
*/ 
bool SkyArchive::IsEmpty() const
{
  return (0 == _dataTable.size());
}


//------------------------------------------------------------------------------
// Function     	  : SkyArchive::Load
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::Load(const char* pFileName)
 * @brief Load the contents of a SkyArchive from file storage.
 */ 
SKYRESULT SkyArchive::Load(const char* pFileName)
{
  if (!pFileName)
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::Load(): file name is NULL.");
  
  FILE* pSrcFile = NULL;
  
  if (NULL == (pSrcFile = fopen(pFileName, "rb"))) // file opened successfully   
  {
    SkyTrace("Error: SkyArchive::Load(): failed to open file for reading.");
    return SKYRESULT_FAIL;
  }
    
  SKYRESULT retVal = _Load(pSrcFile);
  fclose(pSrcFile);

  FAIL_RETURN(retVal);
  return SKYRESULT_OK;
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::Commit
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::Save(const char* pFilename) const
 * @brief Commit Contents of SkyArchive to file storage.                 
 */ 
SKYRESULT SkyArchive::Save(const char* pFileName) const
{
  if (!pFileName)
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::Save(): file name is NULL.");

  FILE* pDestFile = NULL;
  
  if (NULL == (pDestFile = fopen(pFileName, "wb"))) // file opened successfully   
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::Save(): failed to open file for writing."); 
  
  SKYRESULT retVal = _Save(pDestFile);
  fflush(pDestFile);
  fclose(pDestFile);

  FAIL_RETURN(retVal);
  return SKYRESULT_OK;
}


//=============================================================================
// Private helper functions
//=============================================================================

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::_FindEntry
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::_FindEntry(const char* pName, unsigned int index, SkyArchiveTypeCode  eType) const
 * @brief Locates and returns the SkyArchiveEntry with the specified name, index, and type.                                                                
 * 
 * PARAMETERS
 * @param pName Entry name to locate (this is used as the database key)
 * @param index Entry index to locate (in case of multiple entries)
 * @param type Entry must have this type (@see SkyArchiveTypeCode)
 *
 * Returns a pointer to the entry or NULL if no matching entry could be located.
 */ 
const SkyArchiveEntry* SkyArchive::_FindEntry(const char*         pName, 
                                              unsigned int        index, 
                                              SkyArchiveTypeCode  eType) const
{
  //
  // Find the range of entries in the mmap with the key matching /name/
  //
  std::pair< SkyMMapConstIter, SkyMMapConstIter > b = _dataTable.equal_range((char*)pName);
  
  unsigned int count = 0;
  for (SkyMMapConstIter i = b.first; (i != b.second) && (count <= index); ++i)
  {
    //
    // The entry's type and index must match...
    //
    const SkyArchiveEntry* pEntry = (*i).second;
    if (pEntry->type == eType)
    {
      if (count == index)
      {
        return pEntry;
      }
      
      // only increment the count when the type matches
      ++count;
    }
  }
  return NULL;
}


//.---------------------------------------------------------------------------.
//|   Function   : SkyArchive::_CopyDataTable                               |
//|   Description:                                                            |
//.---------------------------------------------------------------------------.
void SkyArchive::_CopyDataTable( const SkyArchiveMMap& src)
{
  SkyMMapConstIter iter;
  
  for (iter = src.begin(); iter != src.end(); iter++)
  {
    const SkyArchiveEntry* pSrcEntry  = (*iter).second;
    const char* pSrcName              = (*iter).first;
    
    if (ARCHIVE_TYPE == pSrcEntry->type)
    {
      SkyArchive* pSrcArchive = (SkyArchive*)pSrcEntry->pData;
      AddArchive(*pSrcArchive);
    }
    else
    {
      SkyArchiveEntry* pNewEntry    = new SkyArchiveEntry;
      pNewEntry->type               = pSrcEntry->type;
      pNewEntry->iDataSize          = pSrcEntry->iDataSize;
      pNewEntry->pData              = new unsigned char[pNewEntry->iDataSize];
      ::memcpy( pNewEntry->pData, pSrcEntry->pData, pNewEntry->iDataSize);
      
      char* pName = new char[::strlen(pSrcName)+1];
      ::strcpy( pName, pSrcName );
      _dataTable.insert(std::make_pair(pName, pNewEntry));
    }
  }
}

//------------------------------------------------------------------------------
// Function     	  : SkyArchive::_Save
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyArchive::_Save(FILE* pDestFile) const
 * @brief Saves data to a file (possibly recursively in the case of subarchives).
 */ 
SKYRESULT SkyArchive::_Save(FILE* pDestFile) const
{
  // fill out a record for this archive & write it
  SkyArchiveFileEntry me;
  me.type       = ARCHIVE_TYPE;
  ::strncpy( me.pName, _pName, 32);
  me.iDataSize  = _dataTable.size();
  
  size_t iNumItemsWritten = fwrite((const void*)&me, sizeof(SkyArchiveFileEntry), 1, pDestFile);
  
  if (1 > iNumItemsWritten)
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::_Save(): failed to write Archive header.");
  
  SkyMMapConstIter iter;
  for (iter = _dataTable.begin(); iter != _dataTable.end(); iter++)
  {
    // fill out a record for each item in _dataTable & write it
    const SkyArchiveEntry* pEntry = (*iter).second;
    switch(pEntry->type)
    {
    case ARCHIVE_TYPE:
      {
        ((SkyArchive*)(pEntry->pData))->_Save(pDestFile);
        break;
      }
      
    default:
      {
        SkyArchiveFileEntry item;
        item.type       = pEntry->type;
        ::strncpy( item.pName, (*iter).first, 32);
        item.iDataSize  = pEntry->iDataSize;
        
        iNumItemsWritten = fwrite((const void*)&item, sizeof(SkyArchiveFileEntry), 1, pDestFile);
        if (1 > iNumItemsWritten)
          FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::_Save(): failed to write Archive Entry header.");
        iNumItemsWritten = fwrite((const void*)pEntry->pData, pEntry->iDataSize, 1, pDestFile);
        if (1 > iNumItemsWritten)
          FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::_Save(): failed to write Archive Entry data.");
        break;
      }
    }
  }
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyArchive::_Load
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyArchive::_Load( FILE* pSrcFile)
* @brief Loads data from a file (possibly recursively in the case of subarchives).
*/ 
SKYRESULT SkyArchive::_Load( FILE* pSrcFile)
{
  // first make sure the file is open and readable.
  
  // load the first record
  SkyArchiveFileEntry thisItem;
  size_t iNumItemsRead = fread((void*)&thisItem, sizeof(SkyArchiveFileEntry), 1, pSrcFile);
  if (1 > iNumItemsRead)
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::_Load(): failed to read Archive header.");
  
  _pName = new char[::strlen(thisItem.pName)+1];
  ::strcpy( _pName, thisItem.pName);
    
  for (unsigned int iNumItems = 0; iNumItems < thisItem.iDataSize; ++iNumItems)
  {
    SkyArchiveFileEntry embeddedItem;
    long iFileLoc = ftell(pSrcFile);  // store location before the read
    iNumItemsRead = fread((void*)&embeddedItem, sizeof(SkyArchiveFileEntry), 1, pSrcFile);
    if (1 > iNumItemsRead)
      FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::_Load(): failed to read embedded archive item.");
   
    switch( embeddedItem.type)
    {
    case ARCHIVE_TYPE:
      {
        if (0 != fseek(pSrcFile, iFileLoc, SEEK_SET))
          FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::_Load(): failed to set the file position.");
        SkyArchive newArchive;
        newArchive._Load(pSrcFile); // recursively load the subarchive
        AddArchive(newArchive);     // add the loaded archive to the database in memory.
      }
      break;
    default:
      {
        void* pData = new unsigned char[embeddedItem.iDataSize];
        iNumItemsRead = fread((void*)pData, embeddedItem.iDataSize, 1, pSrcFile);
        if (1 > iNumItemsRead)
          FAIL_RETURN_MSG(SKYRESULT_FAIL, "Error: SkyArchive::_Load(): failed to read item data.");
        AddData( embeddedItem.pName, 
                (SkyArchiveTypeCode)embeddedItem.type, 
                pData, 
                embeddedItem.iDataSize);
        delete[] pData;
        break;
      }      
    }
  }
  return SKYRESULT_OK;
}
