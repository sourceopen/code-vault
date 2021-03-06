/*
Copyright c1997-2014 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 4.1
http://www.bombaydigital.com/
License: MIT. See LICENSE.md in the Vault top level directory.
*/

/** @file */

#include "vchar.h"

/*
The "null character" constant constructs to char value zero.
This is used by VString to return a null char when an accessor
reads the zero byte of an empty string.
*/
// static
const VChar& VChar::NULL_CHAR() {
    static const VChar kNullChar;
    return kNullChar;
}

VChar::VChar()
    : mValue(VCHAR_NULL_TERMINATOR)
    {
}

VChar::VChar(char c)
    : mValue(c)
    {
}

VChar::VChar(int i)
    : mValue(static_cast<char>(i))
    {
}

VChar& VChar::operator=(char c) {
    mValue = c;
    return *this;
}

VChar& VChar::operator=(int i) {
    mValue = static_cast<char>(i);
    return *this;
}

// Note: Use of static_cast<Vu8> here is to force to positive int before calling int-based library APIs.
// Alternative would be to call this->intValue(), but that might incur one extra copy of the char.

bool VChar::isLowerCase() const {
    return (islower(static_cast<Vu8>(mValue)) != 0);    // Note: global scope operator removed to compile against HPUX headers which use a macro
}

VChar VChar::lowerCase() const {
    return VChar(::tolower(static_cast<Vu8>(mValue)));
}

void VChar::toLowerCase() {
    mValue = static_cast<char>(::tolower(static_cast<Vu8>(mValue)));
}

bool VChar::isUpperCase() const {
    return (isupper(static_cast<Vu8>(mValue)) != 0);    // Note: global scope operator removed to compile against HPUX headers which use a macro
}

VChar VChar::upperCase() const {
    return VChar(::toupper(static_cast<Vu8>(mValue)));
}

void VChar::toUpperCase() {
    mValue = static_cast<char>(::toupper(static_cast<Vu8>(mValue)));
}

char VChar::charValue() const {
    return mValue;
}

int VChar::intValue() const {
    // Need to make sure return value is positive even for mValue > 0x7F.
    Vu8 unsignedValue = static_cast<Vu8>(mValue);
    return unsignedValue;
}

void VChar::set(char c) {
    mValue = c;
}

void VChar::set(int i) {
    mValue = static_cast<char>(i);
}

VChar::operator char() const {
    return mValue;
}

bool VChar::isAlpha() const {
    return ((mValue >= 'a') && (mValue <= 'z')) ||
           ((mValue >= 'A') && (mValue <= 'Z'));
}

bool VChar::isNumeric() const {
    return (mValue >= '0') && (mValue <= '9');
}

bool VChar::isAlphaNumeric() const {
    return this->isAlpha() || this->isNumeric();
}

bool VChar::isWhitespace() const {
    // Need to be careful about signage for values > 0x7F.
    int value = this->intValue();
    return (value <= 0x20) || (value == 0x7F);
}

bool VChar::isHexadecimal() const {
    return ((mValue >= '0') && (mValue <= '9')) ||
           ((mValue >= 'a') && (mValue <= 'f')) ||
           ((mValue >= 'A') && (mValue <= 'F'));
}

// static
bool VChar::equalsIgnoreCase(const VChar& c1, const VChar& c2) {
    return (c1 == c2) ||
           (c1.upperCase() == c2) ||
           (c2.upperCase() == c1);
}

// static
bool VChar::equalsIgnoreCase(const VChar& c1, char c2) {
    return VChar::equalsIgnoreCase(c1, VChar(c2));
}

// static
bool VChar::equalsIgnoreCase(char c1, const VChar& c2) {
    return VChar::equalsIgnoreCase(VChar(c1), c2);
}

// static
bool VChar::equalsIgnoreCase(char c1, char c2) {
    return VChar::equalsIgnoreCase(VChar(c1), VChar(c2));
}

