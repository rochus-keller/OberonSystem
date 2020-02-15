#ifndef OB_GLOBAL_H
#define OB_GLOBAL_H

/*
* Copyright 2019 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Oberon parser/code model library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <bitset>
#include <cassert>
#include <cstring>
#include <math.h>
#include <typeinfo>
#include <iostream>
#include <stdint.h>
#include <QtDebug>

namespace Ob
{
    struct _Root
    {
        _Root(){}
        virtual ~_Root(){}

        void *operator new(size_t n);

        template<typename T>
        T _to()
        {
            T res = dynamic_cast<T>(this);
            if( res == 0 )
            {
                std::cout << typeName() << "_to" << typeid(T).name() << std::endl;
                throw "type guard error";
            }
            // assert( res != 0 );
            return res;
        }
        virtual const char* typeName() const { return "_Root"; }

        static void deleteArena();

        static int MOD(int a, int b) { return ( a % b + b ) % b; }
        static int DIV(int a, int b) { return ( a - MOD(a, b) ) / b; }
        // from the languate report: x = q*y + r and 0<= r < y
        // x = q*y + r -> x - r = q * y -> ( x - r ) / y = q
        //      therefore q = x DIV y = ( x - r ) / y
        // x = q*y + r -> x - q*y = r
        //      therefore r = x MOD y = x - q*y
        // Example:
        // q = ( -1 - r ) / 16
        //  r = 15 -> q = ( -1 - 15 ) / 16 = -16 / 16 = -1

        static int LSL(int a, int b) { return ((int64_t)a << b); }
        static int ASR(int x, int n) { return ((int64_t)(x)>>(n)); }
        static int ROR(int x, int n) { return ((int64_t)(x)>>(n)); } // TODO: stimmt das?
        static int ABS( int a ) { return a < 0 ? -a : a; }
        static float FLT( int a ) { return a; }
        static void LED(int x ){}
        static int FLOOR(float x ) { return ::floor(x); }
    };

    template<typename T>
    class _ValArray
    {
    public:
        _ValArray():d_val(0),d_len(0) {}
        template<typename T2>
        _ValArray( const T2& rhs ):d_len(rhs.size()),d_val(rhs.data()) { }
        const T& operator[]( int n ) const { assert( n < size() ); return d_val[n]; }
        int size() const { return ( d_len < 0 ? -d_len : d_len ); }
        const T* data() const { return d_val; }
    protected:
        const T* d_val;
        int d_len;
    };

    template<>
    class _ValArray<char>
    {
    public:
        _ValArray():d_val(0),d_len(0) {}
        template<typename T2>
        _ValArray(const T2& rhs):d_len(rhs.size()),d_val(rhs.data()) { }
        _ValArray(const char* str):d_val(str),d_len( ::strlen( str ) + 1 ){}
        _ValArray(int ch):d_val(0),d_len( -2 ){ assert( ch < 255 ); d_str[0] = ch; } // wegen Konstanten der form 0x09
        _ValArray(char ch):d_val(0),d_len( -2 ){ d_str[0] = ch; }
        char operator[]( int n ) const { assert( n < size() ); if( d_len < 0 ) return d_str[n]; else return d_val[n]; }
        int size() const { return ( d_len < 0 ? -d_len : d_len ); }
        const char* data() const { return ( d_len < 0 ? d_str : d_val ); }
    protected:
        union { const char* d_val; char d_str[sizeof(int)]; };
        int d_len;
    };

    template<typename T>
    class _VarArray
    {
    public:
        _VarArray():d_val(0),d_len(0) {}
        template<typename T2>
        _VarArray( T2& rhs ):d_len(rhs.size()),d_val(rhs.data()) { }
        T& operator[](int n) { assert( n < d_len ); return d_val[n]; }
        const T& operator[]( int n ) const { assert( n < d_len ); return d_val[n]; }
        int size() const { return d_len; }
        const T* data() const { return d_val; }
        T* data() { return d_val; }
        bool operator==( const char* rhs ) const { return ::strcmp( d_val, rhs ) == 0; }
        template<typename T2>
        void operator=( const T2& rhs )
        {
            if( d_val == 0 )
                return;
            ::memcpy( d_val, rhs.data(), qMin( d_len, rhs.size() ) );
        }

    protected:
        T* d_val;
        int d_len;
    };

    template<typename T, int LEN>
    class _FxArray
    {
    public:
        _FxArray() { clear(); }

        void operator=( const char* rhs )
            { assert( ::strlen(rhs) < LEN ); clear(); ::strcpy(d_arr,rhs); }
        void operator=( char c )
            { assert( LEN >= 2 ); clear(); d_arr[0] = c; }
        template<typename T2>
        void operator=( const T2& rhs )
            { assert( rhs.size() <= size() ); clear(); for( int i = 0; i < rhs.size(); i++ ) d_arr[i] = rhs[i]; }

        template<typename T2>
        bool operator==( const T2& rhs ) const { if( size() != rhs.size() ) return false; else
                for( int i = 0; i < rhs.size(); i++ ) if( d_arr[i] != rhs[i] ) return false; return true; }
        template<int L2>
        bool operator==( const _FxArray<char,L2>& rhs ) const { return ::strcmp( d_arr, rhs.data() ) == 0; }
        bool operator==( const _ValArray<char>& rhs ) const { return ::strcmp( d_arr, rhs.data() ) == 0; }
        bool operator==( const _VarArray<char>& rhs ) const { return ::strcmp( d_arr, rhs.data() ) == 0; }
        bool operator==( const char* rhs ) const { return ::strcmp( d_arr, rhs ) == 0; }
        template<typename T2>
        bool operator!=( const T2& rhs ) const { return !( *this == rhs ); }

        T& operator[](int n) { assert( n < LEN ); return d_arr[n]; }
        const T& operator[](int n) const { assert( n < LEN ); return d_arr[n]; }

        int size() const { return LEN; }
        const T* data() const { return d_arr; }
        T* data() { return d_arr; }

        void clear() { for( int i = 0; i < LEN; i++ ) d_arr[i] = T(); }
    protected:
        T d_arr[LEN];
    };

    class _Set : public std::bitset<32>
    {
    public:
        _Set() {}
        _Set(int n):bitset<32>(n){}
        _Set( int from, int to )
        {
            for( int i = from; i <= to; i++ )
                set(i);
        }
        _Set& operator+( int rhs ) { set(rhs); return *this; }
        _Set& operator+( const _Set& rhs ) { *this |= rhs; return *this; }
        _Set& operator-( const _Set& rhs ) { *this |= rhs; return *this; } // TODO difference
        _Set& operator*( const _Set& rhs ) { *this &= rhs; return *this; }
        _Set& operator/( const _Set& rhs ) { *this &= rhs; return *this; } // symmetric set difference
        bool contains(int rhs) const { return test(rhs); }
    };

    template<typename T>
    class owned_ptr
    {
    public:
        owned_ptr():d_ptr(0) {}
        ~owned_ptr(){ if( d_ptr ) delete d_ptr; }
        void reset( T* t ) { d_ptr = t; }
        void release() { d_ptr = 0; }
        T* get() const { return d_ptr; }
        T* operator->() const { return d_ptr; }
        T* d_ptr;
    };
}

#endif // OB_GLOBAL_H
