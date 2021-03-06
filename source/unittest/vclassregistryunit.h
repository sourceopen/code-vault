/*
Copyright c1997-2014 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 4.1
http://www.bombaydigital.com/
License: MIT. See LICENSE.md in the Vault top level directory.
*/

#ifndef vclassregistryunit_h
#define vclassregistryunit_h

/** @file */

#include "vunit.h"

/**
Unit test class for validating VClassRegistry.
*/
class VClassRegistryUnit : public VUnit {
    public:

        /**
        Constructs a unit test object.
        @param    logOnSuccess    true if you want successful tests to be logged
        @param    throwOnError    true if you want an exception thrown for failed tests
        */
        VClassRegistryUnit(bool logOnSuccess, bool throwOnError);
        /**
        Destructor.
        */
        virtual ~VClassRegistryUnit() {}

        /**
        Executes the unit test.
        */
        virtual void run();

};

#endif /* vclassregistryunit_h */
