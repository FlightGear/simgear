// ----------------------------------------------------------------------------
//  Description      : Garbage collection
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_GARBAGE
#define IXLIB_GARBAGE




#include <memory>
#include <ixlib_exgen.hh>
#include <ixlib_base.hh>




namespace ixion {
  template<class T>
  class delete_deallocator {
    public:
      void operator()(T const *instance) {
        delete instance;
	}
    };
  template<class T>
  class delete_array_deallocator {
    public:
      void operator()(T const *instance) {
        delete[] instance;
	}
    };

  template<class T,class Deallocator = delete_deallocator<T> >
  class reference_manager;

  template<class T>
  class ref_base {
    protected:
      T					*Instance;
    
    public:
      ref_base(T *instance = NULL)
        : Instance(instance) {
	}
      ref_base(ref_base const &src) 
        : Instance(src.Instance) {
	}

      // comparison
      bool operator==(ref_base const &op2) const {
        return Instance == op2.Instance;
        }
      
      // smart pointer nitty-gritty
      T &operator*() const {
        return *Instance;
	}
      T *operator->() const {
        return Instance;
	}
      T *operator+(TIndex index) const {
        return Instance + index;
	}
      T &operator[](TIndex index) const {
        return Instance[index];
	}

      // methods
      T *get() const {
        return Instance;
	}
    };




  template<class T,class T_Managed = T>
  class ref;
  template<class T,class T_Managed = T>
  class no_free_ref;




  template<class T_Managed>
  class reference_manager_keeper {
    public:
    // *** FIXME should be private, but cannot be
    // (partial specializations cannot be declared friends)
      static reference_manager<T_Managed> 	Manager;
    };





  /**
  An object that acts like a reference-counted pointer to an object.
  The corresponding reference_manager is identified implicitly through
  static references.
  
  Example:
  <code>
    IXLIB_GARBAGE_DECLARE_MANAGER(int)
    
    int main() {
      ref<int> my_int = new int(5);
      *my_int = 17;
      ref<int> another_int = my_int;
      *another_int = 12;
      
      *my_int == 12; // true
      }
    </code>
  */
  template<class T,class T_Managed>
  class ref : public ref_base<T> {
    public:
      // we have to have an explicit copy constructor, otherwise the
      // compiler generates one, which is *ahem* - fatal
      ref(ref const &src)
        : ref_base<T>(src) {
	reference_manager_keeper<T_Managed>::Manager.addReference(Instance);
	}
      template<class T2>
      ref(ref<T2,T_Managed> const &src)
        : ref_base<T>(src.get()) {
	reference_manager_keeper<T_Managed>::Manager.addReference(Instance);
	}
      template<class T2>
      ref(no_free_ref<T2,T_Managed> const &src)
        : ref_base<T>(src.get()) {
	reference_manager_keeper<T_Managed>::Manager.addReference(Instance);
	}
      ref(T *instance = NULL)
        : ref_base<T>(instance) {
	reference_manager_keeper<T_Managed>::Manager.addReference(Instance);
	}
      ~ref() {
        reference_manager_keeper<T_Managed>::Manager.freeReference(Instance);
	}
	
      ref &operator=(ref const &src) {
        set(src.get());
        return *this;
        }
      ref &operator=(T *ptr) {
        set(ptr);
        return *this;
        }
      
      // methods
      void release() {
	reference_manager_keeper<T_Managed>::Manager.freeReference(Instance);
	Instance = NULL;
	}
      void set(T *instance) {
        if (instance == Instance) return;
	
	reference_manager_keeper<T_Managed>::Manager.freeReference(Instance);
	Instance = instance;
	reference_manager_keeper<T_Managed>::Manager.addReference(Instance);
        }
      T *releaseFromGCArena() {
        T *oldinst = Instance;
	reference_manager_keeper<T_Managed>::Manager.forgetReference(Instance);
	Instance = NULL;
	return oldinst;
        }
    };




  /**
  An object that acts like a reference-counted pointer to an object.
  However, the referenced object is not freed if the no_free_ref
  is the last reference to the object to go out of scope. 
  
  This is useful to pass objects allocated e.g. on the stack along 
  inside ref's, while making sure they aren't freed. 
  (which would most probably lead to disaster)
  
  no_free_ref's are mostly a hack, but there are situations where you cannot
  avoid them. But even so, you should try not to use them where possible.
  
  The corresponding reference_manager is identified implicitly through
  static references.
  */
  template<class T,class T_Managed>
  class no_free_ref : public ref_base<T>{
    public:
      // we have to have an explicit copy constructor, otherwise the
      // compiler generates one, which is *ahem* - fatal
      no_free_ref(no_free_ref const &src)
        : ref_base<T>(src) {
	reference_manager_keeper<T_Managed>::Manager.addNoFreeReference(Instance);
	}
      template<class T2>
      no_free_ref(ref<T2,T_Managed> const &src)
        : ref_base<T>(src.get()) {
	reference_manager_keeper<T_Managed>::Manager.addNoFreeReference(Instance);
	}
      template<class T2>
      no_free_ref(no_free_ref<T2,T_Managed> const &src)
        : ref_base<T>(src.get()) {
	reference_manager_keeper<T_Managed>::Manager.addNoFreeReference(Instance);
	}
      no_free_ref(T *instance = NULL)
        : ref_base<T>(instance) {
	reference_manager_keeper<T_Managed>::Manager.addNoFreeReference(Instance);
	}
      ~no_free_ref() {
        reference_manager_keeper<T_Managed>::Manager.removeNoFreeReference(Instance);
	}
	
      // assignment
      no_free_ref &operator=(no_free_ref const &src) {
        set(src.get());
        return *this;
        }
      no_free_ref &operator=(T *ptr) {
        set(ptr);
        return *this;
        }
      
      // methods
      void release() {
	reference_manager_keeper<T_Managed>::Manager.removeNoFreeReference(Instance);
	Instance = NULL;
	}
      void set(T *instance) {
        if (instance == Instance) return;
	
	reference_manager_keeper<T_Managed>::Manager.removeNoFreeReference(Instance);
	Instance = instance;
	reference_manager_keeper<T_Managed>::Manager.addNoFreeReference(Instance);
        }
      T *releaseFromGCArena() {
        T *oldinst = Instance;
	reference_manager_keeper<T_Managed>::Manager.forgetReference(Instance);
	Instance = NULL;
	return oldinst;
        }
    };




  /**
  An object that acts like a reference-counted pointer to an object.
  The corresponding reference_manager is identified explicitly.
  */
  template<class T>
  class dynamic_ref : public ref_base<T> {
    protected:
      reference_manager<T> 	&Manager;
      
    public:
      dynamic_ref(dynamic_ref const &src)
        : ref_base<T>(src),Manager(src.Manager) {
	Manager.addReference(Instance);
	}
      dynamic_ref(reference_manager<T> &mgr,T *instance = NULL)
        : ref_base<T>(instance),Manager(mgr) {
	Manager.addReference(Instance);
	}
      ~dynamic_ref() {
        Manager.freeReference(Instance);
	}
      
      // assignment
      dynamic_ref &operator=(dynamic_ref const &src) {
        set(src.get());
        return *this;
        }
      dynamic_ref &operator=(T *ptr) {
        set(ptr);
        return *this;
        }

      // methods
      void release() {
	Manager.freeReference(Instance);
	Instance = NULL;
	}
      void set(T *instance) {
        if (instance == Instance) return;
	
	Manager.freeReference(Instance);
	Instance = instance;
	Manager.addReference(Instance);
        }
      T *releaseFromGCArena() {
        T *oldinst = Instance;
	Manager.forgetReference(Instance);
	Instance = NULL;
	return oldinst;
        }
    };
  
  
  
  
  /**
  An object that acts like a reference-counted pointer to an object.
  However, the referenced object is not freed if the no_free_ref
  is the last reference to the object to go out of scope. 
  
  This is useful to pass objects allocated e.g. on the stack along 
  inside ref's, while making sure they aren't freed. 
  (which would most probably lead to disaster)
  
  no_free_ref's are mostly a hack, but there are situations where you cannot
  avoid them. But even so, you should try not to use them where possible.
  
  The corresponding reference_manager is identified explicitly.
  */
  template<class T>
  class no_free_dynamic_ref : public ref_base<T> {
    protected:
      reference_manager<T> 	&Manager;
      
    public:
      no_free_dynamic_ref(no_free_dynamic_ref const &src)
        : ref_base<T>(src),Manager(src.Manager) {
	Manager.addNoFreeReference(Instance);
	}
      no_free_dynamic_ref(reference_manager<T> &mgr,T *instance = NULL)
        : ref_base<T>(instance),Manager(mgr) {
	Manager.addNoFreeReference(Instance);
	}
      ~no_free_dynamic_ref() {
        Manager.removeNoFreeReference(Instance);
	}
      
      // assignment
      no_free_dynamic_ref &operator=(no_free_dynamic_ref const &src) {
        set(src.get());
        return *this;
        }
      no_free_dynamic_ref &operator=(T *ptr) {
        set(ptr);
        return *this;
        }

      // methods
      void release() {
	Manager.removeNoFreeReference(Instance);
	Instance = NULL;
	}
      void set(T *instance) {
        if (instance == Instance) return;
	
	Manager.removeNoFreeReference(Instance);
	Instance = instance;
	Manager.addNoFreeReference(Instance);
        }
      T *releaseFromGCArena() {
        T *oldinst = Instance;
	Manager.forgetReference(Instance);
	Instance = NULL;
	return oldinst;
        }
    };
  
  
  
  
  template<class T,class Deallocator>
  class reference_manager {
    protected:
    
      struct instance_data {
	T const		*Instance;
        TSize 		ReferenceCount,NoFreeReferenceCount;
	instance_data	*Next,*Previous;
	};
      
      class pointer_hash {
        public:
	};

      typedef unsigned hash_value;
      static hash_value const HASH_MAX = 0x3ff;
      
      instance_data					*Instances[HASH_MAX+1];
      Deallocator					Dealloc;
      
    public:
      reference_manager(Deallocator const &dealloc = Deallocator())
        : Dealloc(dealloc) {
	for (hash_value hv = 0;hv <= HASH_MAX;hv++)
	  Instances[hv] = NULL;
	}
    
    // *** FIXME should be
    // protected:
    // but cannot because partial specializations cannot be declared friends
      void addReference(T const *instance) {
        if (!instance) return;
	instance_data *data = getHashEntry(instance);
        data->ReferenceCount++;
        }
      void freeReference(T const *instance) {
        if (!instance) return;
	instance_data *data = getHashEntry(instance);
	if (--data->ReferenceCount == 0 && data->NoFreeReferenceCount == 0) {
	  removeHashEntry(data);
	  Dealloc(instance);
	  }
        }
      void addNoFreeReference(T const *instance) {
        if (!instance) return;
	instance_data *data = getHashEntry(instance);
        data->NoFreeReferenceCount++;
        }
      void removeNoFreeReference(T const *instance) {
        if (!instance) return;
	instance_data *data = getHashEntry(instance);
	if (--data->NoFreeReferenceCount == 0) {
	  if (data->ReferenceCount != 0)
            EXGEN_THROW(EC_REMAININGREF)
          removeHashEntry(data);
	  }
        }
      void forgetReference(T const *instance) {
        if (!instance) return;
	instance_data *data = getHashEntry(instance);
	if (data->ReferenceCount != 1) 
	  EXGEN_THROW(EC_CANNOTREMOVEFROMGC)
        removeHashEntry(data);
        }
	
    private:
      hash_value hash(T const *ptr) const {
        unsigned u = reinterpret_cast<unsigned>(ptr);
        return (u ^ (u >> 8) ^ (u >> 16) ^ (u >> 24)) & HASH_MAX;
        }
      instance_data *getHashEntry(T const *instance) {
        instance_data *data = Instances[hash(instance)];
	while (data) {
	  if (data->Instance == instance) return data;
	  data = data->Next;
	  }
	
	// not found, add new at front
        instance_data *link = Instances[hash(instance)];
	data = new instance_data;
	
	data->Instance = instance;
	data->ReferenceCount = 0;
	data->NoFreeReferenceCount = 0;
	data->Previous = NULL;
	data->Next = link;
	if (link) link->Previous = data;
	Instances[hash(instance)] = data;
	return data;
        }
      void removeHashEntry(instance_data *data) {
        instance_data *prev = data->Previous;
        if (prev) {
	  prev->Next = data->Next;
	  if (data->Next) data->Next->Previous = prev;
	  delete data;
	  }
	else {
	  Instances[hash(data->Instance)] = data->Next;
	  if (data->Next) data->Next->Previous = NULL;
	  delete data;
	  }
        }
    };




  #define IXLIB_GARBAGE_DECLARE_MANAGER(TYPE) \
    ixion::reference_manager<TYPE> ixion::reference_manager_keeper<TYPE>::Manager;
  }




#endif
