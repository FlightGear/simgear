#ifndef SGGUARD_HXX_INCLUDED
#define SGGUARD_HXX_INCLUDED 1

/**
 * A scoped locking utility.
 * An SGGuard object locks its synchronization object during creation and
 * automatically unlocks it when it goes out of scope.
 */
template<class LOCK>
class SGGuard
{
public:

    /**
     * Create an SGGuard object and lock the passed lockable object.
     * @param LOCK A lockable object.
     */
    inline SGGuard( LOCK& l ) : lock(l) { lock.lock(); }

    /**
     * Destroy this object and unlock the locakable object.
     */
    inline ~SGGuard() { lock.unlock(); }

private:

    /**
     * A lockable object.
     */
    LOCK& lock;

private:
    // Disable copying.
    SGGuard(const LOCK&);
    LOCK& operator= (const LOCK&);
};

#endif // SGGUARD_HXX_INCLUDED
