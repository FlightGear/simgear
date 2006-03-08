/**************************************************************************
 * callback.hxx -- Wrapper classes to treat function and method pointers
 * as objects.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 **************************************************************************/

#ifndef _SG_CALLBACK_HXX
#define _SG_CALLBACK_HXX

/**
 * Abstract base class for all callbacks.
 */
class SGCallback
{
public:

    /**
     * 
     */
    virtual ~SGCallback() {}

    /**
     * 
     */
    virtual SGCallback* clone() const = 0;

    /**
     * Execute the callback function.
     */
    virtual void operator()() = 0;

protected:
    /**
     * 
     */
    SGCallback() {}

private:
    // Not implemented.
    void operator=( const SGCallback& );
};

/**
 * Callback for invoking a file scope function.
 */
template< typename Fun >
class SGFunctionCallback : public SGCallback
{
public:
    /**
     * 
     */
    SGFunctionCallback( const Fun& fun )
	: SGCallback(), f_(fun) {}

    SGCallback* clone() const
    {
	return new SGFunctionCallback( *this );
    }

    void operator()() { f_(); }

private:
    // Not defined.
    SGFunctionCallback();

private:
    Fun f_;
};

/**
 * Callback for invoking a member function.
 */
template< class ObjPtr, typename MemFn >
class SGMethodCallback : public SGCallback
{
public:

    /**
     * 
     */
    SGMethodCallback( const ObjPtr& pObj, MemFn pMemFn )
	: SGCallback(),
	  pObj_(pObj),
	  pMemFn_(pMemFn)
    {
    }

    /**
     * 
     */
    SGCallback* clone() const
    {
	return new SGMethodCallback( *this );
    }

    /**
     * 
     */
    void operator()()
    {
	((*pObj_).*pMemFn_)();
    }

private:
    // Not defined.
    SGMethodCallback();

private:
    ObjPtr pObj_;
    MemFn pMemFn_;
};

/**
 * Helper template functions.
 */

template< typename Fun >
SGCallback*
make_callback( const Fun& fun )
{
    return new SGFunctionCallback<Fun>(fun);
}

template< class ObjPtr, typename MemFn >
SGCallback*
make_callback( const ObjPtr& pObj, MemFn pMemFn )
{
    return new SGMethodCallback<ObjPtr,MemFn>(pObj, pMemFn );
}

#endif // _SG_CALLBACK_HXX

