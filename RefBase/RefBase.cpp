//
//  RefBase.cpp
//  RefBase
//
//  Created by 金康 on 2017/10/27.
//  Copyright © 2017年 kangdi. All rights reserved.
//

#include "RefBase.hpp"
#include <atomic>

#define INITIAL_STRONG_VALUE (1<<28)

class RefBase::weakref_impl : public RefBase::weakref_type
{
public:
    std::atomic<int32_t>    mStrong;
    std::atomic<int32_t>    mWeak;
    RefBase* const          mBase;
    std::atomic<int32_t>    mFlags;
    
#if !DEBUG_REFS
    
    weakref_impl(RefBase* base)
    : mStrong(INITIAL_STRONG_VALUE)
    , mWeak(0)
    , mBase(base)
    , mFlags(0)
    {
        //printf("weakref_impl的无参构造函数\n");
    }
    
    void addStrongRef(const void* /*id*/) { }
    void removeStrongRef(const void* /*id*/) { }
    void renameStrongRefId(const void* /*old_id*/, const void* /*new_id*/) { }
    void addWeakRef(const void* /*id*/) { }
    void removeWeakRef(const void* /*id*/) { }
    void renameWeakRefId(const void* /*old_id*/, const void* /*new_id*/) { }
    void printRefs() const { }
    void trackMe(bool, bool) { }
    
#else
    
    weakref_impl(RefBase* base)
    : mStrong(INITIAL_STRONG_VALUE)
    , mWeak(0)
    , mBase(base)
    , mFlags(0)
    , mStrongRefs(NULL)
    , mWeakRefs(NULL)
    , mTrackEnabled(!!DEBUG_REFS_ENABLED_BY_DEFAULT)
    , mRetain(false)
    {
    }
    
    ~weakref_impl()
    {
        bool dumpStack = false;
        if (!mRetain && mStrongRefs != NULL) {
            dumpStack = true;
            ALOGE("Strong references remain:");
            ref_entry* refs = mStrongRefs;
            while (refs) {
                char inc = refs->ref >= 0 ? '+' : '-';
                ALOGD("\t%c ID %p (ref %d):", inc, refs->id, refs->ref);
#if DEBUG_REFS_CALLSTACK_ENABLED
                refs->stack.log(LOG_TAG);
#endif
                refs = refs->next;
            }
        }
        
        if (!mRetain && mWeakRefs != NULL) {
            dumpStack = true;
            ALOGE("Weak references remain!");
            ref_entry* refs = mWeakRefs;
            while (refs) {
                char inc = refs->ref >= 0 ? '+' : '-';
                ALOGD("\t%c ID %p (ref %d):", inc, refs->id, refs->ref);
#if DEBUG_REFS_CALLSTACK_ENABLED
                refs->stack.log(LOG_TAG);
#endif
                refs = refs->next;
            }
        }
        if (dumpStack) {
            ALOGE("above errors at:");
            CallStack stack(LOG_TAG);
        }
    }
    
    void addStrongRef(const void* id) {
        //ALOGD_IF(mTrackEnabled,
        //        "addStrongRef: RefBase=%p, id=%p", mBase, id);
        addRef(&mStrongRefs, id, mStrong.load(std::memory_order_relaxed));
    }
    
    void removeStrongRef(const void* id) {
        //ALOGD_IF(mTrackEnabled,
        //        "removeStrongRef: RefBase=%p, id=%p", mBase, id);
        if (!mRetain) {
            removeRef(&mStrongRefs, id);
        } else {
            addRef(&mStrongRefs, id, -mStrong.load(std::memory_order_relaxed));
        }
    }
    
    void renameStrongRefId(const void* old_id, const void* new_id) {
        //ALOGD_IF(mTrackEnabled,
        //        "renameStrongRefId: RefBase=%p, oid=%p, nid=%p",
        //        mBase, old_id, new_id);
        renameRefsId(mStrongRefs, old_id, new_id);
    }
    
    void addWeakRef(const void* id) {
        addRef(&mWeakRefs, id, mWeak.load(std::memory_order_relaxed));
    }
    
    void removeWeakRef(const void* id) {
        if (!mRetain) {
            removeRef(&mWeakRefs, id);
        } else {
            addRef(&mWeakRefs, id, -mWeak.load(std::memory_order_relaxed));
        }
    }
    
    void renameWeakRefId(const void* old_id, const void* new_id) {
        renameRefsId(mWeakRefs, old_id, new_id);
    }
    
    void trackMe(bool track, bool retain)
    {
        mTrackEnabled = track;
        mRetain = retain;
    }
    
    void printRefs() const
    {
        String8 text;
        
        {
            Mutex::Autolock _l(mMutex);
            char buf[128];
            s//printf(buf, "Strong references on RefBase %p (weakref_type %p):\n", mBase, this);
            text.append(buf);
            printRefsLocked(&text, mStrongRefs);
            s//printf(buf, "Weak references on RefBase %p (weakref_type %p):\n", mBase, this);
            text.append(buf);
            printRefsLocked(&text, mWeakRefs);
        }
        
        {
            char name[100];
            sn//printf(name, 100, DEBUG_REFS_CALLSTACK_PATH "/%p.stack", this);
            int rc = open(name, O_RDWR | O_CREAT | O_APPEND, 644);
            if (rc >= 0) {
                write(rc, text.string(), text.length());
                close(rc);
                ALOGD("STACK TRACE for %p saved in %s", this, name);
            }
            else ALOGE("FAILED TO PRINT STACK TRACE for %p in %s: %s", this,
                       name, strerror(errno));
        }
    }
    
private:
    struct ref_entry
    {
        ref_entry* next;
        const void* id;
#if DEBUG_REFS_CALLSTACK_ENABLED
        CallStack stack;
#endif
        int32_t ref;
    };
    
    void addRef(ref_entry** refs, const void* id, int32_t mRef)
    {
        if (mTrackEnabled) {
            AutoMutex _l(mMutex);
            
            ref_entry* ref = new ref_entry;
            // Reference count at the time of the snapshot, but before the
            // update.  Positive value means we increment, negative--we
            // decrement the reference count.
            ref->ref = mRef;
            ref->id = id;
#if DEBUG_REFS_CALLSTACK_ENABLED
            ref->stack.update(2);
#endif
            ref->next = *refs;
            *refs = ref;
        }
    }
    
    void removeRef(ref_entry** refs, const void* id)
    {
        if (mTrackEnabled) {
            AutoMutex _l(mMutex);
            
            ref_entry* const head = *refs;
            ref_entry* ref = head;
            while (ref != NULL) {
                if (ref->id == id) {
                    *refs = ref->next;
                    delete ref;
                    return;
                }
                refs = &ref->next;
                ref = *refs;
            }
            
            ALOGE("RefBase: removing id %p on RefBase %p"
                  "(weakref_type %p) that doesn't exist!",
                  id, mBase, this);
            
            ref = head;
            while (ref) {
                char inc = ref->ref >= 0 ? '+' : '-';
                ALOGD("\t%c ID %p (ref %d):", inc, ref->id, ref->ref);
                ref = ref->next;
            }
            
            CallStack stack(LOG_TAG);
        }
    }
    
    void renameRefsId(ref_entry* r, const void* old_id, const void* new_id)
    {
        if (mTrackEnabled) {
            AutoMutex _l(mMutex);
            ref_entry* ref = r;
            while (ref != NULL) {
                if (ref->id == old_id) {
                    ref->id = new_id;
                }
                ref = ref->next;
            }
        }
    }
    
    void printRefsLocked(String8* out, const ref_entry* refs) const
    {
        char buf[128];
        while (refs) {
            char inc = refs->ref >= 0 ? '+' : '-';
            s//printf(buf, "\t%c ID %p (ref %d):\n",
                    inc, refs->id, refs->ref);
            out->append(buf);
#if DEBUG_REFS_CALLSTACK_ENABLED
            out->append(refs->stack.toString("\t\t"));
#else
            out->append("\t\t(call stacks disabled)");
#endif
            refs = refs->next;
        }
    }
    
    mutable Mutex mMutex;
    ref_entry* mStrongRefs;
    ref_entry* mWeakRefs;
    
    bool mTrackEnabled;
    // Collect stack traces on addref and removeref, instead of deleting the stack references
    // on removeref that match the address ones.
    bool mRetain;
    
#endif
};

RefBase* RefBase::weakref_type::refBase() const
{
    return static_cast<const weakref_impl*>(this)->mBase;
}

void RefBase::weakref_type::incWeak(const void* id)
{
    //printf("弱引用计数+1\n");
    weakref_impl* const impl = static_cast<weakref_impl*>(this);
    impl->addWeakRef(id);
    const __int32_t c __unused = impl->mWeak.fetch_add(1,
                                                     std::memory_order_relaxed);
    
}


void RefBase::weakref_type::decWeak(const void* id)
{
    //printf("开始调用decWeak\n");
    weakref_impl* const impl = static_cast<weakref_impl*>(this);
    impl->removeWeakRef(id);
    //printf("弱引用减1? 减1前 = ");
    //printf("%d\n", impl->mWeak.load(std::memory_order_relaxed));
    const __int32_t c = impl->mWeak.fetch_sub(1, std::memory_order_release);
    
    if (c != 1) return;
    atomic_thread_fence(std::memory_order_acquire);
    
    __int32_t flags = impl->mFlags.load(std::memory_order_relaxed);
    if ((flags&OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
        // This is the regular lifetime case. The object is destroyed
        // when the last strong reference goes away. Since weakref_impl
        // outlive the object, it is not destroyed in the dtor, and
        // we'll have to do it here.
        if (impl->mStrong.load(std::memory_order_relaxed)
            == INITIAL_STRONG_VALUE) {
            // Special case: we never had a strong reference, so we need to
            // destroy the object now.
            delete impl->mBase;
        } else {
            //printf("受强引用控制并且强引用计数初始化过现在为0, RefBase也删掉了, 需要删掉weakref_impl\n");
            // ALOGV("Freeing refs %p of old RefBase %p\n", this, impl->mBase);
            delete impl;
        }
    } else {
        // This is the OBJECT_LIFETIME_WEAK case. The last weak-reference
        // is gone, we can destroy the object.
        impl->mBase->onLastWeakRef(id);
        delete impl->mBase;
    }
}

RefBase::RefBase()
: mRefs(new weakref_impl(this))
{
    //printf("RefBase的无参构造函数\n");
}

RefBase::~RefBase()
{
    //printf("开始RefBase析构函数\n");
    if (mRefs->mStrong.load(std::memory_order_relaxed)
        == INITIAL_STRONG_VALUE) {
        //printf("强引用计数根本没有+1过\n");
        // we never acquired a strong (and/or weak) reference on this object.
        delete mRefs;
    } else {
        // life-time of this object is extended to WEAK, in
        // which case weakref_impl doesn't out-live the object and we
        // can free it now.
        int32_t flags = mRefs->mFlags.load(std::memory_order_relaxed);
        if ((flags & OBJECT_LIFETIME_MASK) != OBJECT_LIFETIME_STRONG) {
            // It's possible that the weak count is not 0 if the object
            // re-acquired a weak reference in its destructor
            //printf("RefBase析构函数, 如果被弱引用计数控制\n");
            if (mRefs->mWeak.load(std::memory_order_relaxed) == 0) {
                //printf("弱引用计数为0, 删除mRef 也就是 weakref_impl\n");
                delete mRefs;
            }
        }
    }
    // for debugging purposes, clear this.
    //printf("将RefBase中的mRef指向为空\n");
    const_cast<weakref_impl*&>(mRefs) = NULL;
}

void RefBase::extendObjectLifetime(int32_t mode)
{
    // Must be happens-before ordered with respect to construction or any
    // operation that could destroy the object.
    mRefs->mFlags.fetch_or(mode, std::memory_order_relaxed);
}

void RefBase::onFirstRef()
{
    //printf("onFirstRef\n");
}

void RefBase::onLastStrongRef(const void* /*id*/)
{
    //printf("onLastStrongRef\n");
}

bool RefBase::onIncStrongAttempted(uint32_t flags, const void* /*id*/)
{
    return (flags&FIRST_INC_STRONG) ? true : false;
}

void RefBase::onLastWeakRef(const void* /*id*/)
{
    //printf("onLastWeakRef\n");
}
int32_t RefBase::getStrongCount() const
{
    // Debugging only; No memory ordering guarantees.
    return mRefs->mStrong.load(std::memory_order_relaxed);
    
}
void RefBase::incStrong(const void* id) const
{
    //printf("调用RefBase->incStrong\n");
    weakref_impl* const refs = mRefs;
    refs->incWeak(id);
    
    refs->addStrongRef(id);
    //printf("强引用计数+1\n");
    const int32_t c = refs->mStrong.fetch_add(1, std::memory_order_relaxed);
//    ALOG_ASSERT(c > 0, "incStrong() called on %p after last strong ref", refs);
#if PRINT_REFS
    ALOGD("incStrong of %p from %p: cnt=%d\n", this, id, c);
#endif
    if (c != INITIAL_STRONG_VALUE)  {
        return;
    }
    
    //printf("强引用加上初始值\n");
    int32_t old = refs->mStrong.fetch_sub(INITIAL_STRONG_VALUE,
                                          std::memory_order_relaxed);
    // A decStrong() must still happen after us.
//    ALOG_ASSERT(old > INITIAL_STRONG_VALUE, "0x%x too small", old);
    refs->mBase->onFirstRef();
}
RefBase::weakref_type* RefBase::getWeakRefs() const
{
    return mRefs;
}
int32_t RefBase::weakref_type::getWeakCount() const
{
    // Debug only!
    return static_cast<const weakref_impl*>(this)->mWeak
    .load(std::memory_order_relaxed);
}
void RefBase::decStrong(const void* id) const
{
    weakref_impl* const refs = mRefs;
    refs->removeStrongRef(id);
    //printf("强引用计数 - 1\n");
    const int32_t c = refs->mStrong.fetch_sub(1, std::memory_order_release);
#if PRINT_REFS
    ALOGD("decStrong of %p from %p: cnt=%d\n", this, id, c);
#endif
//    ALOG_ASSERT(c >= 1, "decStrong() called on %p too many times", refs);
    if (c == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        refs->mBase->onLastStrongRef(id);
        int32_t flags = refs->mFlags.load(std::memory_order_relaxed);
        if ((flags&OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
            //printf("默认受强引用控制\n");
            //printf("删除RefBase自身\n");
            delete this;
            // Since mStrong had been incremented, the destructor did not
            // delete refs.
        }
    }
    // Note that even with only strong reference operations, the thread
    // deallocating this may not be the same as the thread deallocating refs.
    // That's OK: all accesses to this happen before its deletion here,
    // and all accesses to refs happen before its deletion in the final decWeak.
    // The destructor can safely access mRefs because either it's deleting
    // mRefs itself, or it's running entirely before the final mWeak decrement.
    //printf("删除掉RefBase后, 接着走RefBase_decStrong\n");
    refs->decWeak(id);
}
bool RefBase::weakref_type::attemptIncStrong(const void* id)
{
    incWeak(id);
    
    weakref_impl* const impl = static_cast<weakref_impl*>(this);
    int32_t curCount = impl->mStrong.load(std::memory_order_relaxed);
    
//    ALOG_ASSERT(curCount >= 0,
//                "attemptIncStrong called on %p after underflow", this);
    
    while (curCount > 0 && curCount != INITIAL_STRONG_VALUE) {
        // we're in the easy/common case of promoting a weak-reference
        // from an existing strong reference.
        if (impl->mStrong.compare_exchange_weak(curCount, curCount+1,
                                                std::memory_order_relaxed)) {
            break;
        }
        // the strong count has changed on us, we need to re-assert our
        // situation. curCount was updated by compare_exchange_weak.
    }
    
    if (curCount <= 0 || curCount == INITIAL_STRONG_VALUE) {
        // we're now in the harder case of either:
        // - there never was a strong reference on us
        // - or, all strong references have been released
        int32_t flags = impl->mFlags.load(std::memory_order_relaxed);
        if ((flags&OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
            // this object has a "normal" life-time, i.e.: it gets destroyed
            // when the last strong reference goes away
            if (curCount <= 0) {
                // the last strong-reference got released, the object cannot
                // be revived.
                decWeak(id);
                return false;
            }
            
            // here, curCount == INITIAL_STRONG_VALUE, which means
            // there never was a strong-reference, so we can try to
            // promote this object; we need to do that atomically.
            while (curCount > 0) {
                if (impl->mStrong.compare_exchange_weak(curCount, curCount+1,
                                                        std::memory_order_relaxed)) {
                    break;
                }
                // the strong count has changed on us, we need to re-assert our
                // situation (e.g.: another thread has inc/decStrong'ed us)
                // curCount has been updated.
            }
            
            if (curCount <= 0) {
                // promote() failed, some other thread destroyed us in the
                // meantime (i.e.: strong count reached zero).
                decWeak(id);
                return false;
            }
        } else {
            // this object has an "extended" life-time, i.e.: it can be
            // revived from a weak-reference only.
            // Ask the object's implementation if it agrees to be revived
            if (!impl->mBase->onIncStrongAttempted(FIRST_INC_STRONG, id)) {
                // it didn't so give-up.
                decWeak(id);
                return false;
            }
            // grab a strong-reference, which is always safe due to the
            // extended life-time.
            curCount = impl->mStrong.fetch_add(1, std::memory_order_relaxed);
            // If the strong reference count has already been incremented by
            // someone else, the implementor of onIncStrongAttempted() is holding
            // an unneeded reference.  So call onLastStrongRef() here to remove it.
            // (No, this is not pretty.)  Note that we MUST NOT do this if we
            // are in fact acquiring the first reference.
            if (curCount != 0 && curCount != INITIAL_STRONG_VALUE) {
                impl->mBase->onLastStrongRef(id);
            }
        }
    }
    
    impl->addStrongRef(id);
    
#if PRINT_REFS
    ALOGD("attemptIncStrong of %p from %p: cnt=%d\n", this, id, curCount);
#endif
    
    // curCount is the value of mStrong before we incremented it.
    // Now we need to fix-up the count if it was INITIAL_STRONG_VALUE.
    // This must be done safely, i.e.: handle the case where several threads
    // were here in attemptIncStrong().
    // curCount > INITIAL_STRONG_VALUE is OK, and can happen if we're doing
    // this in the middle of another incStrong.  The subtraction is handled
    // by the thread that started with INITIAL_STRONG_VALUE.
    if (curCount == INITIAL_STRONG_VALUE) {
        impl->mStrong.fetch_sub(INITIAL_STRONG_VALUE,
                                std::memory_order_relaxed);
    }
    
    return true;
}
RefBase::weakref_type* RefBase::createWeak(const void* id) const
{
    mRefs->incWeak(id);
    return mRefs;
}
