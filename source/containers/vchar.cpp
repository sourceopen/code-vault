/*
Copyright c1997-2005 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 2.3.2
http://www.bombaydigital.com/
*/

/** @file */

#include "vchar.h"

VChar::VChar() :
mValue(0)
    {
    }

VChar::VChar(char c) :
mValue(c)
    {
    }

VChar::VChar(int i) :
mValue(static_cast<char>(i))
    {
    }

VChar& VChar::operator=(char c)
    {
    mValue = c;
    return *this;
    }

VChar& VChar::operator=(int i)
    {
    mValue = static_cast<char> (i);
    return *this;
    }

bool VChar::isLowerCase() const
    {
    return (islower(mValue) != 0);    // Note: global scope operator removed to compile against HPUX headers which use a macro
    }

VChar VChar::lowerCase() const
    {
    return VChar(::tolower(mValue));
    }

void VChar::toLowerCase()
    {
    mValue = static_cast<char>(::tolower(mValue));
    }

bool VChar::isUpperCase() const
    {
    return (isupper(mValue) != 0);    // Note: global scope operator removed to compile against HPUX headers which use a macro
    }

VChar VChar::upperCase() const
    {
    return VChar(::toupper(mValue));
    }

void VChar::toUpperCase()
    {
    mValue = static_cast<char>(::toupper(mValue));
    }

char VChar::charValue() const
    {
    return mValue;
    }

int VChar::intValue() const
    {
    // Need to make sure return value is positive even for mValue > 0x7F.
    Vu8    unsignedValue = static_cast<Vu8>(mValue);
    return unsignedValue;
    }

void VChar::set(char c)
    {
    mValue = c;
    }

void VChar::set(int i)
    {
    mValue = static_cast<char>(i);
    }

VChar::operator char() const
    {
    return mValue;
    }
        
bool VChar::isAlpha() const
    {
    return ((mValue >= 'a') && (mValue <= 'z')) ||
        ((mValue >= 'A') && (mValue <= 'Z'));
    }

bool VChar::isNumeric() const
    {
    return (mValue >= '0') && (mValue <= '9');
    }

bool VChar::isAlphaNumeric() const
    {
    return this->isAlpha() || this->isNumeric();
    }

bool VChar::isWhitespace() const
    {
    // Need to be careful about signage for values > 0x7F.
    int    value = this->intValue();
    return (value <= 0x20) || (value == 0x7F);
    }