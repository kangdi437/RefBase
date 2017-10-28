//
//  StrongPointer.h
//  RefBase
//
//  Created by 金康 on 2017/10/27.
//  Copyright © 2017年 kangdi. All rights reserved.
//

#ifndef StrongPointer_h
#define StrongPointer_h

template<typename T>
class sp {
public:
    inline sp() : m_ptr(0) { }
    
    sp(T* other);
    sp(const sp<T>& other);
    sp(sp<T>&& other);
    template<typename U> sp(U* other);
    template<typename U> sp(const sp<U>& other);
    template<typename U> sp(sp<U>&& other);
    
    ~sp();
    
    // Assignment
    
    sp& operator = (T* other);
    sp& operator = (const sp<T>& other);
    sp& operator = (sp<T>&& other);
    
    template<typename U> sp& operator = (const sp<U>& other);
    template<typename U> sp& operator = (sp<U>&& other);
    template<typename U> sp& operator = (U* other);
    
    //! Special optimization for use by ProcessState (and nobody else).
    void force_set(T* other);
    
    // Reset
    
    void clear();
    
    // Accessors
    
    inline  T&      operator* () const  { return *m_ptr; }
    inline  T*      operator-> () const { return m_ptr;  }
    inline  T*      get() const         { return m_ptr; }
    
    // Operators
    
    
    
private:
    template<typename Y> friend class sp;
    template<typename Y> friend class wp;
    void set_pointer(T* ptr);
    T* m_ptr;
};

template<typename T>
sp<T>::sp(T* other)
: m_ptr(other) {
    //printf("调用sp的有参构造函数\n");
    if (other)
        other->incStrong(this);
}

template<typename T>
sp<T>::sp(const sp<T>& other)
: m_ptr(other.m_ptr) {
    if (m_ptr)
        m_ptr->incStrong(this);
}

template<typename T>
sp<T>::sp(sp<T>&& other)
: m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
}

template<typename T> template<typename U>
sp<T>::sp(U* other)
: m_ptr(other) {
    if (other)
        ((T*) other)->incStrong(this);
}

template<typename T> template<typename U>
sp<T>::sp(const sp<U>& other)
: m_ptr(other.m_ptr) {
    if (m_ptr)
        m_ptr->incStrong(this);
}

template<typename T> template<typename U>
sp<T>::sp(sp<U>&& other)
: m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
}

template<typename T>
sp<T>::~sp() {
    //printf("调用sp的析构函数\n");
    if (m_ptr)
        m_ptr->decStrong(this);
}

template<typename T>
sp<T>& sp<T>::operator =(const sp<T>& other) {
    T* otherPtr(other.m_ptr);
    if (otherPtr)
        otherPtr->incStrong(this);
    if (m_ptr)
        m_ptr->decStrong(this);
    m_ptr = otherPtr;
    return *this;
}

template<typename T>
sp<T>& sp<T>::operator =(sp<T>&& other) {
    if (m_ptr)
        m_ptr->decStrong(this);
    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
    return *this;
}

template<typename T>
sp<T>& sp<T>::operator =(T* other) {
    if (other)
        other->incStrong(this);
    if (m_ptr)
        m_ptr->decStrong(this);
    m_ptr = other;
    return *this;
}

template<typename T> template<typename U>
sp<T>& sp<T>::operator =(const sp<U>& other) {
    T* otherPtr(other.m_ptr);
    if (otherPtr)
        otherPtr->incStrong(this);
    if (m_ptr)
        m_ptr->decStrong(this);
    m_ptr = otherPtr;
    return *this;
}

template<typename T> template<typename U>
sp<T>& sp<T>::operator =(sp<U>&& other) {
    if (m_ptr)
        m_ptr->decStrong(this);
    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
    return *this;
}

template<typename T> template<typename U>
sp<T>& sp<T>::operator =(U* other) {
    if (other)
        ((T*) other)->incStrong(this);
    if (m_ptr)
        m_ptr->decStrong(this);
    m_ptr = other;
    return *this;
}

template<typename T>
void sp<T>::force_set(T* other) {
    other->forceIncStrong(this);
    m_ptr = other;
}

template<typename T>
void sp<T>::clear() {
    if (m_ptr) {
        m_ptr->decStrong(this);
        m_ptr = 0;
    }
}

template<typename T>
void sp<T>::set_pointer(T* ptr) {
    m_ptr = ptr;
}



#endif /* StrongPointer_h */
