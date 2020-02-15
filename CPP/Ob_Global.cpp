/*
* Copyright 2020 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Oberon parser/compiler library.
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

#include "Ob_Global.h"
#include <list>
using namespace Ob;

static std::list<_Root*> s_objs;


void*_Root::operator new(size_t n)
{
    void *p = ::operator new(n);
    // TODO s_objs.push_back((_Root*)p);
    return p;
}

void _Root::deleteArena()
{
    std::list<_Root*>::iterator i;
    for( i = s_objs.begin(); i != s_objs.end(); ++i )
    {
        delete (*i);
    }
    s_objs.clear();
}

