#ifndef _OBERON_MACHINE_
#define _OBERON_MACHINE_

/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Project Oberon System project.
*
* The following is the license that applies to this copy of the
* file. For a license to use the file under conditions
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

#include <stdint.h>

/* Memory map of Wirth's RISC-5 machine (see PO.Computer chapter 3),
   kept 1:1 so that Kernel.Mod, Display.Mod and Input.Mod are unchanged.
   The I/O words live in the topmost 64 bytes of the address space. */
enum {
    IO_START   = 0xFFFFFFC0u, /* -64 */
    IO_MSTIMER = 0xFFFFFFC0u, /* -64 milliseconds since start */
    IO_LEDS    = 0xFFFFFFC4u, /* -60 switches (read) / LEDs (write) */
    IO_RS232D  = 0xFFFFFFC8u, /* -56 RS232 data */
    IO_RS232S  = 0xFFFFFFCCu, /* -52 RS232 status */
    IO_SPIDATA = 0xFFFFFFD0u, /* -48 SPI data */
    IO_SPICTRL = 0xFFFFFFD4u, /* -44 SPI control */
    IO_MOUSE   = 0xFFFFFFD8u, /* -40 mouse state and keyboard ready flag */
    IO_KBD     = 0xFFFFFFDCu, /* -36 keyboard scan code */
    IO_GPIO    = 0xFFFFFFE0u, /* -32 GPIO data */
    IO_GPIOC   = 0xFFFFFFE4u  /* -28 GPIO tri-state control */
};

enum {
    DISPLAY_BASE   = 0x000E7F00u, /* as in Display.Mod */
    DISPLAY_WIDTH  = 1024,
    DISPLAY_HEIGHT = 768,
    DISPLAY_SPAN   = DISPLAY_WIDTH / 8,
    DISPLAY_LEN    = DISPLAY_SPAN * DISPLAY_HEIGHT
};

void diskOpen(const char* path);
void diskClose(void);
void diskSelect(int on);
uint32_t diskTransfer(uint32_t out);   /* byte transfer, slow mode */
uint32_t diskTransfer32(uint32_t out); /* word transfer, fast mode */

#endif
