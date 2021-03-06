//
//  RefBase.hpp
//  RefBase
//
//  Created by 金康 on 2017/10/27.
//  Copyright © 2017年 kangdi. All rights reserved.
//

#ifndef RefBase_hpp
#define RefBase_hpp

#include <stdio.h>
#include "StrongPointer.h"


class RefBase
{
public:
    void            incStrong(const void* id) const;
    void            decStrong(const void* id) const;
    
    void            forceIncStrong(const void* id) const;
    
    //! DEBUGGING ONLY: Get current strong ref count.
    __int32_t         getStrongCount() const;
    
    class weakref_type
    {
    public:
        RefBase*            refBase() const;
        
        void                incWeak(const void* id);
        void                decWeak(const void* id);
        
        // acquires a strong reference if there is already one.
        bool                attemptIncStrong(const void* id);
        
        // acquires a weak reference if there is already one.
        // This is not always safe. see ProcessState.cpp and BpBinder.cpp
        // for proper use.
        bool                attemptIncWeak(const void* id);
        
        //! DEBUGGING ONLY: Get current weak ref count.
        __int32_t             getWeakCount() const;
        
        //! DEBUGGING ONLY: Print references held on object.
        void                printRefs() const;
        
        //! DEBUGGING ONLY: Enable tracking for this object.
        // enable -- enable/disable tracking
        // retain -- when tracking is enable, if true, then we save a stack trace
        //           for each reference and dereference; when retain == false, we
        //           match up references and dereferences and keep only the
        //           outstanding ones.
        
        void                trackMe(bool enable, bool retain);
    };
    
    weakref_type*   createWeak(const void* id) const;
    
    weakref_type*   getWeakRefs() const;
    
    //! DEBUGGING ONLY: Print references held on object.
    inline  void            printRefs() const { getWeakRefs()->printRefs(); }
    
    //! DEBUGGING ONLY: Enable tracking of object.
    inline  void            trackMe(bool enable, bool retain)
    {
        getWeakRefs()->trackMe(enable, retain);
    }
    
    typedef RefBase basetype;
    
protected:
    RefBase();
    virtual                 ~RefBase();
    
    //! Flags for extendObjectLifetime()
    enum {
        OBJECT_LIFETIME_STRONG  = 0x0000,
        OBJECT_LIFETIME_WEAK    = 0x0001,
        OBJECT_LIFETIME_MASK    = 0x0001
    };
    
    void            extendObjectLifetime(__int32_t mode);
    
    //! Flags for onIncStrongAttempted()
    enum {
        FIRST_INC_STRONG = 0x0001
    };
    
    virtual void            onFirstRef();
    virtual void            onLastStrongRef(const void* id);
    virtual bool            onIncStrongAttempted(__uint32_t flags, const void* id);
    virtual void            onLastWeakRef(const void* id);
    
private:
    friend class weakref_type;
    class weakref_impl;
    
    RefBase(const RefBase& o);
    RefBase&        operator=(const RefBase& o);
    
private:
    
    weakref_impl* const mRefs;
};

template <typename T>
class wp
{
public:
    typedef typename RefBase::weakref_type weakref_type;
    
    inline wp() : m_ptr(0) { }
    
    wp(T* other);
    wp(const wp<T>& other);
    wp(const sp<T>& other);
    template<typename U> wp(U* other);
    template<typename U> wp(const sp<U>& other);
    template<typename U> wp(const wp<U>& other);
    
    ~wp();
    
    // Assignment
    
    wp& operator = (T* other);
    wp& operator = (const wp<T>& other);
    wp& operator = (const sp<T>& other);
    
    template<typename U> wp& operator = (U* other);
    template<typename U> wp& operator = (const wp<U>& other);
    template<typename U> wp& operator = (const sp<U>& other);
    
    void set_object_and_refs(T* other, weakref_type* refs);
    
    // promotion to sp
    
    sp<T> promote() const;
    
    // Reset
    
    void clear();
    
    // Accessors
    
    inline  weakref_type* get_refs() const { return m_refs; }
    
    inline  T* unsafe_get() const { return m_ptr; }
    
    // Operators
    
//    COMPARE_WEAK(==)
//    COMPARE_WEAK(!=)
//    COMPARE_WEAK(>)
//    COMPARE_WEAK(<)
//    COMPARE_WEAK(<=)
//    COMPARE_WEAK(>=)
    
    inline bool operator == (const wp<T>& o) const {
        return (m_ptr == o.m_ptr) && (m_refs == o.m_refs);
    }
    template<typename U>
    inline bool operator == (const wp<U>& o) const {
        return m_ptr == o.m_ptr;
    }
    
    inline bool operator > (const wp<T>& o) const {
        return (m_ptr == o.m_ptr) ? (m_refs > o.m_refs) : (m_ptr > o.m_ptr);
    }
    template<typename U>
    inline bool operator > (const wp<U>& o) const {
        return (m_ptr == o.m_ptr) ? (m_refs > o.m_refs) : (m_ptr > o.m_ptr);
    }
    
    inline bool operator < (const wp<T>& o) const {
        return (m_ptr == o.m_ptr) ? (m_refs < o.m_refs) : (m_ptr < o.m_ptr);
    }
    template<typename U>
    inline bool operator < (const wp<U>& o) const {
        return (m_ptr == o.m_ptr) ? (m_refs < o.m_refs) : (m_ptr < o.m_ptr);
    }
    inline bool operator != (const wp<T>& o) const { return m_refs != o.m_refs; }
    template<typename U> inline bool operator != (const wp<U>& o) const { return !operator == (o); }
    inline bool operator <= (const wp<T>& o) const { return !operator > (o); }
    template<typename U> inline bool operator <= (const wp<U>& o) const { return !operator > (o); }
    inline bool operator >= (const wp<T>& o) const { return !operator < (o); }
    template<typename U> inline bool operator >= (const wp<U>& o) const { return !operator < (o); }
    
private:
    template<typename Y> friend class sp;
    template<typename Y> friend class wp;
    
    T*              m_ptr;
    weakref_type*   m_refs;
};
template<typename T>
wp<T>::wp(T* other)
: m_ptr(other)
{
    if (other) m_refs = other->createWeak(this);
}

template<typename T>
wp<T>::wp(const wp<T>& other)
: m_ptr(other.m_ptr), m_refs(other.m_refs)
{
    if (m_ptr) m_refs->incWeak(this);
}

template<typename T>
wp<T>::wp(const sp<T>& other)
: m_ptr(other.m_ptr)
{
    if (m_ptr) {
        m_refs = m_ptr->createWeak(this);
    }
}

template<typename T> template<typename U>
wp<T>::wp(U* other)
: m_ptr(other)
{
    if (other) m_refs = other->createWeak(this);
}

template<typename T> template<typename U>
wp<T>::wp(const wp<U>& other)
: m_ptr(other.m_ptr)
{
    if (m_ptr) {
        m_refs = other.m_refs;
        m_refs->incWeak(this);
    }
}

template<typename T> template<typename U>
wp<T>::wp(const sp<U>& other)
: m_ptr(other.m_ptr)
{
    if (m_ptr) {
        m_refs = m_ptr->createWeak(this);
    }
}

template<typename T>
wp<T>::~wp()
{
    if (m_ptr) m_refs->decWeak(this);
}

template<typename T>
wp<T>& wp<T>::operator = (T* other)
{
    weakref_type* newRefs =
    other ? other->createWeak(this) : 0;
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = other;
    m_refs = newRefs;
    return *this;
}

template<typename T>
wp<T>& wp<T>::operator = (const wp<T>& other)
{
    weakref_type* otherRefs(other.m_refs);
    T* otherPtr(other.m_ptr);
    if (otherPtr) otherRefs->incWeak(this);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = otherPtr;
    m_refs = otherRefs;
    return *this;
}

template<typename T>
wp<T>& wp<T>::operator = (const sp<T>& other)
{
    weakref_type* newRefs =
    other != NULL ? other->createWeak(this) : 0;
    T* otherPtr(other.m_ptr);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = otherPtr;
    m_refs = newRefs;
    return *this;
}

template<typename T> template<typename U>
wp<T>& wp<T>::operator = (U* other)
{
    weakref_type* newRefs =
    other ? other->createWeak(this) : 0;
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = other;
    m_refs = newRefs;
    return *this;
}

template<typename T> template<typename U>
wp<T>& wp<T>::operator = (const wp<U>& other)
{
    weakref_type* otherRefs(other.m_refs);
    U* otherPtr(other.m_ptr);
    if (otherPtr) otherRefs->incWeak(this);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = otherPtr;
    m_refs = otherRefs;
    return *this;
}

template<typename T> template<typename U>
wp<T>& wp<T>::operator = (const sp<U>& other)
{
    weakref_type* newRefs =
    other != NULL ? other->createWeak(this) : 0;
    U* otherPtr(other.m_ptr);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = otherPtr;
    m_refs = newRefs;
    return *this;
}

template<typename T>
void wp<T>::set_object_and_refs(T* other, weakref_type* refs)
{
    if (other) refs->incWeak(this);
    if (m_ptr) m_refs->decWeak(this);
    m_ptr = other;
    m_refs = refs;
}

template<typename T>
sp<T> wp<T>::promote() const
{
    sp<T> result;
    if (m_ptr && m_refs->attemptIncStrong(&result)) {
        result.set_pointer(m_ptr);
    }
    return result;
}

template<typename T>
void wp<T>::clear()
{
    if (m_ptr) {
        m_refs->decWeak(this);
        m_ptr = 0;
    }
}

#endif /* RefBase_hpp */


