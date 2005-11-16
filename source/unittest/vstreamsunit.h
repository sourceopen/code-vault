/*
Copyright c1997-2005 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 2.3.2
http://www.bombaydigital.com/
*/

#ifndef vstreamsunit_h
#define vstreamsunit_h

/** @file */

#include "vunit.h"

/**
Unit test class for validating several of the VStream-related classes.
*/
class VStreamsUnit : public VUnit
    {
    public:
    
        /**
        Constructs a unit test object.
        @param    logOnSuccess    true if you want successful tests to be logged
        @param    throwOnError    true if you want an exception thrown for failed tests
        */
        VStreamsUnit(bool logOnSuccess, bool throwOnError);
        /**
        Destructor.
        */
        virtual ~VStreamsUnit() {}
        
        /**
        Executes the unit test.
        */
        virtual void run();

    };

#endif /* vstreamsunit_h */