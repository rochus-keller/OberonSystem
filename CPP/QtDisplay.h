#ifndef QTDISPLAY_H
#define QTDISPLAY_H

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

#include <QWidget>
#include <QImage>
#include <OberonSystem/Ob_Global.h>

class QtDisplay : public QWidget
{
    Q_OBJECT
public:
    enum { Width = 1024, Height = 768 };

    QImage d_img;
    int d_xOb, d_yOb;
    Ob::_Set d_keys;
    int d_lock;

    static QtDisplay* inst();

    static int mapToQt(int yOb);
    static int mapToOb(int yQt);

    typedef void (*MouseHandler)(const Ob::_Set& keys, int x, int y);
    typedef void (*CharHandler)(char ch);
    typedef void (*IdleHandler)();
    static void registerMouseHandler(MouseHandler h);
    static void registerCharHandler(CharHandler h);
    static void registerIdleHandler(IdleHandler h);

protected:
    void paintEvent(QPaintEvent *);
    void timerEvent(QTimerEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void mapOb( QMouseEvent* );
private:
    explicit QtDisplay();
};

#endif // QTDISPLAY_H
