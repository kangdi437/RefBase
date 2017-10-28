//
//  main.cpp
//  RefBase
//
//  Created by 金康 on 2017/10/27.
//  Copyright © 2017年 kangdi. All rights reserved.
//

#include <iostream>
#include "StrongPointer.h"
#include "RefBase.hpp"
using namespace std;

class WeightClass : public RefBase
{
public:
    void printRefCount()
    {
        int32_t strong = getStrongCount();
        weakref_type* ref = getWeakRefs();
        printf("-----------------------\n");
        printf("Strong Ref Count: %d.\n", strong); //打印强引用计数
        printf("Weak Ref Count: %d.\n", ref->getWeakCount()); //打印弱引用计数
        printf("-----------------------\n");
    }
};

class StrongClass : public WeightClass
{
public:
    StrongClass()
    {
        printf("Construct StrongClass Object.\n");
    }
    
    virtual ~StrongClass()
    {
        printf("Destory StrongClass Object.\n");
    }
};

class WeakClass : public WeightClass
{
public:
    WeakClass()
    {
        extendObjectLifetime(OBJECT_LIFETIME_WEAK); //受弱引用控制
        printf("Construct WeakClass Object.\n");
    }
    
    virtual ~WeakClass()
    {
        printf("Destory WeakClass Object.\n");
    }
};

void TestStrongClass(StrongClass* pStrongClass)
{
    wp<StrongClass> wpOut = pStrongClass; //弱引用计数 +1
    pStrongClass->printRefCount();
    
    {
        sp<StrongClass> spInner = pStrongClass; //强引用计数+1, 弱引用计数+1
        pStrongClass->printRefCount();
    }//调用sp的析构函数, 强引用 = 0, 弱引用 = 1, 因受强引用控制, 所以对象被删除
    
    sp<StrongClass> spOut = wpOut.promote(); //升级失败
    printf("spOut: %p.\n", spOut.get());
}

void TestWeakClass(WeakClass* pWeakClass)
{
    wp<WeakClass> wpOut = pWeakClass; //弱引用计数 +1
    pWeakClass->printRefCount();
    
    {
        sp<WeakClass> spInner = pWeakClass; //强引用计数+1, 弱引用计数+1
        pWeakClass->printRefCount();
    }//调用sp的析构函数, 强引用 = 0, 弱引用 = 1, 因受弱引用控制, 所以对象不会被删除
    
    pWeakClass->printRefCount();
    sp<WeakClass> spOut = wpOut.promote(); //升级成功
    printf("spOut: %p.\n", spOut.get());
}

int main(int argc, const char * argv[]) {
    
    printf("Test Strong Class: \n");
    StrongClass* pStrongClass = new StrongClass();
    TestStrongClass(pStrongClass);
    
    printf("\nTest Weak Class: \n");
    WeakClass* pWeakClass = new WeakClass();
    TestWeakClass(pWeakClass);
    
    
    return 0;
}
