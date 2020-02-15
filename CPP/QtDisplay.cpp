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

#include "QtDisplay.h"
#include "ObDisplay.h"
#include <QMouseEvent>
#include <QPainter>
using namespace Ob;

static QtDisplay* s_disp = 0;
static QtDisplay::MouseHandler s_mh = 0;
static QtDisplay::CharHandler s_ch = 0;
static QtDisplay::IdleHandler s_ih = 0;

QtDisplay*QtDisplay::inst()
{
    if( s_disp == 0 )
        s_disp = new QtDisplay();
    return s_disp;
}

int QtDisplay::mapToQt(int yOb)
{
    return Height - yOb - 1;
}

int QtDisplay::mapToOb(int yQt)
{
    return Height - yQt - 1; // yQt runs from zero to Height - 1; yOb zero is therefore at Height - 1
}

void QtDisplay::registerMouseHandler(QtDisplay::MouseHandler h)
{
    s_mh = h;
}

void QtDisplay::registerCharHandler(QtDisplay::CharHandler h)
{
    s_ch = h;
}

void QtDisplay::registerIdleHandler(QtDisplay::IdleHandler h)
{
    s_ih = h;
}

void QtDisplay::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.drawImage(0,0,d_img);
}

void QtDisplay::timerEvent(QTimerEvent*)
{
    if( s_ih )
        ; // TODO s_ih();
}

void QtDisplay::mouseMoveEvent(QMouseEvent* e)
{
    mapOb(e);
    if( s_mh && d_lock == 0 )
        s_mh(d_keys, d_xOb, d_yOb );
}

void QtDisplay::mousePressEvent(QMouseEvent* e)
{

    mapOb(e);
    if( s_mh && d_lock == 0 )
        s_mh(d_keys, d_xOb, d_yOb );
}

void QtDisplay::mouseReleaseEvent(QMouseEvent* e)
{
    mapOb(e);
    d_keys.reset();
    if( s_mh && d_lock == 0 )
        s_mh(d_keys, d_xOb, d_yOb );
}

void QtDisplay::keyPressEvent(QKeyEvent* e)
{
    if( s_ch )
        s_ch( e->text().toLatin1()[0] );
}

void QtDisplay::keyReleaseEvent(QKeyEvent*)
{
    // NOP
}

void QtDisplay::mapOb(QMouseEvent* p)
{
    if( !rect().contains(p->pos()) )
        return;
    d_xOb = p->pos().x();
    d_yOb = mapToOb(p->pos().y());
    d_keys.reset();
    d_keys.set( 2, p->buttons() & Qt::LeftButton );
    d_keys.set( 1, p->buttons() & Qt::MidButton );
    if( p->modifiers() == Qt::ControlModifier && ( p->buttons() & Qt::LeftButton ) )
    {
        d_keys.set( 1, true ); // mid button == CTRL + left button
        d_keys.set( 2, false );
    }
    d_keys.set( 0, p->buttons() & Qt::RightButton );
}

QtDisplay::QtDisplay():d_img(Width,Height,QImage::Format_Mono),d_lock(0)
{
    d_img.fill(0);
    setFixedSize(Width,Height);
    show();
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::BlankCursor);
    startTimer(0); // idle timer
}

