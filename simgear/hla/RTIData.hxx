// Copyright (C) 2009 - 2010  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef RTIData_hxx
#define RTIData_hxx

#include <cstring>
#include <list>
#include <simgear/misc/stdint.hxx>

namespace simgear {

/// Sigh, this is std::vector<char>, except that
/// you could feed that with external pointers without copying ...
/// Note on alignment: the c++ standard garantees (5.3.4.10) that
/// new (unsigned) char returns sufficiently aligned memory
/// for all relevant cases
class RTIData {
public:
    RTIData() :
        _data(0),
        _size(0),
        _capacity(0)
    { }
    RTIData(unsigned size) :
        _data(0),
        _size(0),
        _capacity(0)
    { resize(size); }
    RTIData(char* data, unsigned size) :
        _data(data),
        _size(size),
        _capacity(0)
    { }
    RTIData(const char* data, unsigned size) :
        _data(0),
        _size(0),
        _capacity(0)
    { setData(data, size); }
    RTIData(const char* data) :
        _data(0),
        _size(0),
        _capacity(0)
    { setData(data); }
    RTIData(const RTIData& data) :
        _data(0),
        _size(0),
        _capacity(0)
    {
        unsigned size = data.size();
        if (size) {
            resize(size);
            memcpy(_data, data.data(), size);
        }
    }
    ~RTIData()
    {
        if (_capacity)
            delete [] _data;
        _data = 0;
    }

    const char* data() const
    { return _data; }
    char* data()
    { return _data; }

    unsigned size() const
    { return _size; }

    bool empty() const
    { return _size == 0; }

    void clear()
    {
        if (_capacity == 0) {
            _data = 0;
            _size = 0;
        } else
            resize(0);
    }

    void resize(unsigned size)
    {
        if (size == _size)
            return;
        if (_capacity < size) {
            unsigned capacity = 2*_capacity;
            if (size < capacity)
                ensureCapacity(capacity);
            else
                ensureCapacity(size);
        }
        _size = size;
    }

    void reserve(unsigned capacity)
    {
        if (capacity <= _capacity)
            return;
        ensureCapacity(capacity);
    }

    void swap(RTIData& data)
    {
        std::swap(_data, data._data);
        std::swap(_size, data._size);
        std::swap(_capacity, data._capacity);
    }

    void setData(char* data, unsigned size)
    {
        if (_capacity)
            delete [] _data;
        _data = data;
        _size = size;
        _capacity = 0;
    }
    void setData(const char* data, unsigned size)
    {
        resize(size);
        if (!size)
            return;
        memcpy(_data, data, size);
    }
    void setData(const char* data)
    {
        if (!data) {
            setData("", 1);
        } else {
            size_t size = strlen(data) + 1;
            setData(data, size);
        }
    }

    RTIData& operator=(const RTIData& data)
    {
        unsigned size = data.size();
        if (size) {
            resize(size);
            memcpy(_data, data.data(), size);
        }
        return *this;
    }

    void getData8(char data[1], unsigned offset = 0) const
    {
        data[0] = _data[offset];
    }

    void setData8(const char data[1], unsigned offset = 0)
    {
        _data[offset] = data[0];
    }

    void getData16LE(char data[2], unsigned offset = 0) const
    {
        if (hostIsLittleEndian()) {
            data[0] = _data[offset];
            data[1] = _data[offset+1];
        } else {
            data[1] = _data[offset];
            data[0] = _data[offset+1];
        }
    }
    void setData16LE(const char data[2], unsigned offset = 0)
    {
        if (hostIsLittleEndian()) {
            _data[offset] = data[0];
            _data[offset+1] = data[1];
        } else {
            _data[offset] = data[1];
            _data[offset+1] = data[0];
        }
    }

    void getData16BE(char data[2], unsigned offset = 0) const
    {
        if (hostIsLittleEndian()) {
            data[1] = _data[offset];
            data[0] = _data[offset+1];
        } else {
            data[0] = _data[offset];
            data[1] = _data[offset+1];
        }
    }
    void setData16BE(const char data[2], unsigned offset = 0)
    {
        if (hostIsLittleEndian()) {
            _data[offset] = data[1];
            _data[offset+1] = data[0];
        } else {
            _data[offset] = data[0];
            _data[offset+1] = data[1];
        }
    }


    void getData32LE(char data[4], unsigned offset = 0) const
    {
        if (hostIsLittleEndian()) {
            data[0] = _data[offset];
            data[1] = _data[offset+1];
            data[2] = _data[offset+2];
            data[3] = _data[offset+3];
        } else {
            data[3] = _data[offset];
            data[2] = _data[offset+1];
            data[1] = _data[offset+2];
            data[0] = _data[offset+3];
        }
    }
    void setData32LE(const char data[4], unsigned offset = 0)
    {
        if (hostIsLittleEndian()) {
            _data[offset] = data[0];
            _data[offset+1] = data[1];
            _data[offset+2] = data[2];
            _data[offset+3] = data[3];
        } else {
            _data[offset] = data[3];
            _data[offset+1] = data[2];
            _data[offset+2] = data[1];
            _data[offset+3] = data[0];
        }
    }

    void getData32BE(char data[4], unsigned offset = 0) const
    {
        if (hostIsLittleEndian()) {
            data[3] = _data[offset];
            data[2] = _data[offset+1];
            data[1] = _data[offset+2];
            data[0] = _data[offset+3];
        } else {
            data[0] = _data[offset];
            data[1] = _data[offset+1];
            data[2] = _data[offset+2];
            data[3] = _data[offset+3];
        }
    }
    void setData32BE(const char data[4], unsigned offset = 0)
    {
        if (hostIsLittleEndian()) {
            _data[offset] = data[3];
            _data[offset+1] = data[2];
            _data[offset+2] = data[1];
            _data[offset+3] = data[0];
        } else {
            _data[offset] = data[0];
            _data[offset+1] = data[1];
            _data[offset+2] = data[2];
            _data[offset+3] = data[3];
        }
    }


    void getData64LE(char data[8], unsigned offset = 0) const
    {
        if (hostIsLittleEndian()) {
            data[0] = _data[offset];
            data[1] = _data[offset+1];
            data[2] = _data[offset+2];
            data[3] = _data[offset+3];
            data[4] = _data[offset+4];
            data[5] = _data[offset+5];
            data[6] = _data[offset+6];
            data[7] = _data[offset+7];
        } else {
            data[7] = _data[offset];
            data[6] = _data[offset+1];
            data[5] = _data[offset+2];
            data[4] = _data[offset+3];
            data[3] = _data[offset+4];
            data[2] = _data[offset+5];
            data[1] = _data[offset+6];
            data[0] = _data[offset+7];
        }
    }
    void setData64LE(const char data[8], unsigned offset = 0)
    {
        if (hostIsLittleEndian()) {
            _data[offset] = data[0];
            _data[offset+1] = data[1];
            _data[offset+2] = data[2];
            _data[offset+3] = data[3];
            _data[offset+4] = data[4];
            _data[offset+5] = data[5];
            _data[offset+6] = data[6];
            _data[offset+7] = data[7];
        } else {
            _data[offset] = data[7];
            _data[offset+1] = data[6];
            _data[offset+2] = data[5];
            _data[offset+3] = data[4];
            _data[offset+4] = data[3];
            _data[offset+5] = data[2];
            _data[offset+6] = data[1];
            _data[offset+7] = data[0];
        }
    }

    void getData64BE(char data[8], unsigned offset = 0) const
    {
        if (hostIsLittleEndian()) {
            data[7] = _data[offset];
            data[6] = _data[offset+1];
            data[5] = _data[offset+2];
            data[4] = _data[offset+3];
            data[3] = _data[offset+4];
            data[2] = _data[offset+5];
            data[1] = _data[offset+6];
            data[0] = _data[offset+7];
        } else {
            data[0] = _data[offset];
            data[1] = _data[offset+1];
            data[2] = _data[offset+2];
            data[3] = _data[offset+3];
            data[4] = _data[offset+4];
            data[5] = _data[offset+5];
            data[6] = _data[offset+6];
            data[7] = _data[offset+7];
        }
    }
    void setData64BE(const char data[8], unsigned offset = 0)
    {
        if (hostIsLittleEndian()) {
            _data[offset] = data[7];
            _data[offset+1] = data[6];
            _data[offset+2] = data[5];
            _data[offset+3] = data[4];
            _data[offset+4] = data[3];
            _data[offset+5] = data[2];
            _data[offset+6] = data[1];
            _data[offset+7] = data[0];
        } else {
            _data[offset] = data[0];
            _data[offset+1] = data[1];
            _data[offset+2] = data[2];
            _data[offset+3] = data[3];
            _data[offset+4] = data[4];
            _data[offset+5] = data[5];
            _data[offset+6] = data[6];
            _data[offset+7] = data[7];
        }
    }


#define TYPED_GETSET_IMPLEMENTATION(type, base, suffix)         \
    type get##base##suffix(unsigned offset = 0) const           \
    {                                                           \
        union {                                                 \
            type t;                                             \
            char u8[sizeof(type)];                              \
        } u;                                                    \
        getData##suffix(u.u8, offset);                          \
        return u.t;                                             \
    }                                                           \
    void set##base##suffix(type value, unsigned offset = 0)     \
    {                                                           \
        union {                                                 \
            type t;                                             \
            char u8[sizeof(type)];                              \
        } u;                                                    \
        u.t = value;                                            \
        setData##suffix(u.u8, offset);                          \
    }

    TYPED_GETSET_IMPLEMENTATION(uint8_t, UInt, 8)
    TYPED_GETSET_IMPLEMENTATION(int8_t, Int, 8)
    TYPED_GETSET_IMPLEMENTATION(uint16_t, UInt, 16LE)
    TYPED_GETSET_IMPLEMENTATION(uint16_t, UInt, 16BE)
    TYPED_GETSET_IMPLEMENTATION(int16_t, Int, 16LE)
    TYPED_GETSET_IMPLEMENTATION(int16_t, Int, 16BE)
    TYPED_GETSET_IMPLEMENTATION(uint32_t, UInt, 32LE)
    TYPED_GETSET_IMPLEMENTATION(uint32_t, UInt, 32BE)
    TYPED_GETSET_IMPLEMENTATION(int32_t, Int, 32LE)
    TYPED_GETSET_IMPLEMENTATION(int32_t, Int, 32BE)
    TYPED_GETSET_IMPLEMENTATION(uint64_t, UInt, 64LE)
    TYPED_GETSET_IMPLEMENTATION(uint64_t, UInt, 64BE)
    TYPED_GETSET_IMPLEMENTATION(int64_t, Int, 64LE)
    TYPED_GETSET_IMPLEMENTATION(int64_t, Int, 64BE)

    TYPED_GETSET_IMPLEMENTATION(float, Float, 32LE)
    TYPED_GETSET_IMPLEMENTATION(float, Float, 32BE)
    TYPED_GETSET_IMPLEMENTATION(double, Float, 64LE)
    TYPED_GETSET_IMPLEMENTATION(double, Float, 64BE)

#undef TYPED_GETSET_IMPLEMENTATION

private:
    static inline bool hostIsLittleEndian()
    {
        union {
            uint16_t u16;
            uint8_t u8[2];
        } u;
        u.u16 = 1;
        return u.u8[0] == 1;
    }

    void ensureCapacity(unsigned capacity)
    {
        if (capacity < 32)
            capacity = 32;
        char* data = new char[capacity];
        if (_size)
            memcpy(data, _data, _size);
        if (_capacity)
            delete [] _data;
        _data = data;
        _capacity = capacity;
    }

    char* _data;
    unsigned _size;
    unsigned _capacity;
};

/// Gets an own header at some time

class RTIBasicDataStream {
public:
    RTIBasicDataStream() : _offset(0) {}

    /// Get aligned offset that aligns to a multiple of size
    static inline unsigned getAlignedOffset(unsigned offset, unsigned size)
    {
        return ((offset + size - 1)/size) * size;
    }

protected:
    unsigned _offset;
};

class HLADecodeStream : public RTIBasicDataStream {
public:
    HLADecodeStream(const RTIData& value) :
        _value(value)
    { }

    bool alignOffsetForSize(unsigned size)
    {
        _offset = getAlignedOffset(_offset, size);
        return _offset <= _value.size();
    }

    bool skip(unsigned size)
    {
        _offset += size;
        return _offset <= _value.size();
    }

    bool eof() const
    { return _value.size() <= _offset; }

    const RTIData& getData() const
    { return _value; }

#define TYPED_READ_IMPLEMENTATION(type, base, suffix)           \
    bool decode##base##suffix(type& value)                      \
    {                                                           \
        if (_value.size() < _offset + sizeof(type))             \
            return false;                                       \
        value = _value.get##base##suffix(_offset);              \
        _offset += sizeof(type);                                \
        return true;                                            \
    }

    TYPED_READ_IMPLEMENTATION(uint8_t, UInt, 8)
    TYPED_READ_IMPLEMENTATION(int8_t, Int, 8)
    TYPED_READ_IMPLEMENTATION(uint16_t, UInt, 16LE)
    TYPED_READ_IMPLEMENTATION(uint16_t, UInt, 16BE)
    TYPED_READ_IMPLEMENTATION(int16_t, Int, 16LE)
    TYPED_READ_IMPLEMENTATION(int16_t, Int, 16BE)
    TYPED_READ_IMPLEMENTATION(uint32_t, UInt, 32LE)
    TYPED_READ_IMPLEMENTATION(uint32_t, UInt, 32BE)
    TYPED_READ_IMPLEMENTATION(int32_t, Int, 32LE)
    TYPED_READ_IMPLEMENTATION(int32_t, Int, 32BE)
    TYPED_READ_IMPLEMENTATION(uint64_t, UInt, 64LE)
    TYPED_READ_IMPLEMENTATION(uint64_t, UInt, 64BE)
    TYPED_READ_IMPLEMENTATION(int64_t, Int, 64LE)
    TYPED_READ_IMPLEMENTATION(int64_t, Int, 64BE)

    TYPED_READ_IMPLEMENTATION(float, Float, 32LE)
    TYPED_READ_IMPLEMENTATION(float, Float, 32BE)
    TYPED_READ_IMPLEMENTATION(double, Float, 64LE)
    TYPED_READ_IMPLEMENTATION(double, Float, 64BE)

#undef TYPED_READ_IMPLEMENTATION

private:
    const RTIData& _value;
};

class HLAEncodeStream : public RTIBasicDataStream {
public:
    HLAEncodeStream(RTIData& value) :
        _value(value)
    { }

    bool alignOffsetForSize(unsigned size)
    {
        _offset = getAlignedOffset(_offset, size);
        _value.resize(_offset);
        return true;
    }

    bool skip(unsigned size)
    {
        _offset += size;
        _value.resize(_offset);
        return true;
    }

    bool eof() const
    { return _value.size() <= _offset; }

    void setData(const RTIData& data)
    { _value = data; }

#define TYPED_WRITE_IMPLEMENTATION(type, base, suffix)                  \
    bool encode##base##suffix(type value)                               \
    {                                                                   \
        unsigned nextOffset = _offset + sizeof(type);                   \
        _value.resize(nextOffset);                                      \
        _value.set##base##suffix(value, _offset);                       \
        _offset = nextOffset;                                           \
        return true;                                                    \
    }

    TYPED_WRITE_IMPLEMENTATION(uint8_t, UInt, 8)
    TYPED_WRITE_IMPLEMENTATION(int8_t, Int, 8)
    TYPED_WRITE_IMPLEMENTATION(uint16_t, UInt, 16LE)
    TYPED_WRITE_IMPLEMENTATION(uint16_t, UInt, 16BE)
    TYPED_WRITE_IMPLEMENTATION(int16_t, Int, 16LE)
    TYPED_WRITE_IMPLEMENTATION(int16_t, Int, 16BE)
    TYPED_WRITE_IMPLEMENTATION(uint32_t, UInt, 32LE)
    TYPED_WRITE_IMPLEMENTATION(uint32_t, UInt, 32BE)
    TYPED_WRITE_IMPLEMENTATION(int32_t, Int, 32LE)
    TYPED_WRITE_IMPLEMENTATION(int32_t, Int, 32BE)
    TYPED_WRITE_IMPLEMENTATION(uint64_t, UInt, 64LE)
    TYPED_WRITE_IMPLEMENTATION(uint64_t, UInt, 64BE)
    TYPED_WRITE_IMPLEMENTATION(int64_t, Int, 64LE)
    TYPED_WRITE_IMPLEMENTATION(int64_t, Int, 64BE)

    TYPED_WRITE_IMPLEMENTATION(float, Float, 32LE)
    TYPED_WRITE_IMPLEMENTATION(float, Float, 32BE)
    TYPED_WRITE_IMPLEMENTATION(double, Float, 64LE)
    TYPED_WRITE_IMPLEMENTATION(double, Float, 64BE)

#undef TYPED_WRITE_IMPLEMENTATION

private:
    RTIData& _value;
};

} // namespace simgear

#endif
