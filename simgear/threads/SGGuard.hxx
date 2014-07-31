#ifndef SGGUARD_HXX_INCLUDED
#define SGGUARD_HXX_INCLUDED 1

/**
 * A scoped locking utility.
 * An SGGuard object locks its synchronization object during creation and
 * automatically unlocks it when it goes out of scope.
 */
template<class SGLOCK>
class SGGuard
{
public:

    /**
     * Create an SGGuard object and lock the passed lockable object.
     * @param l A lockable object.
     */
    inline SGGuard( SGLOCK& l ) : lock(l) { lock.lock(); }

    /**
     * Destroy this object and unlock the lockable object.
     */
    inline ~SGGuard() { lock.unlock(); }

private:

    SGLOCK& lock; //!< A lockable object

private:
    // Disable copying.
    SGGuard(const SGLOCK&);
    SGLOCK& operator= (const SGLOCK&);
};

#endif // SGGUARD_HXX_INCLUDED
