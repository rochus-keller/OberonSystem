#ifndef __ObFonts_H__
#define __ObFonts_H__

// Generated by OberonViewer 0.7.5 on 2020-01-27T01:00:45
// Then further developed manually by RK

#include <OberonSystem/Ob_Global.h>
#include <OberonSystem/Ob_SYSTEM.h>
#include <OberonSystem/ObFiles.h>

namespace Ob
{
	class Fonts : public _Root
	{
	public:
		/* JG 18.11.90; PDR 8.6.12; NW 25.3.2013 */
		static Fonts* _inst();
		Fonts();
        void _init();
		~Fonts();

		// CONST
		static const int FontFileId = 0x0DB;

		// TYPE
		struct FontDesc;
		struct LargeFontDesc;

		typedef FontDesc* Font;
		struct FontDesc : public _Root {
            _FxArray<uint8_t*,128> T; // original int
			int height;
			int maxX;
			int maxY;
			int minX;
			int minY;
			_FxArray<char,32> name;
			Font next;
			_FxArray<uint8_t,2360> raster;
		};
		struct LargeFontDesc : public FontDesc {
			_FxArray<uint8_t,2560> ext;
		};
		typedef LargeFontDesc* LargeFont;

		// VAR
		/*  raster sizes: Syntax8 1367, Syntax10 1628, Syntax12 1688, Syntax14 1843, Syntax14b 1983,
      Syntax16 2271, Syntax20 3034, Syntac24 4274, Syntax24b 4302   */
		Font Default;
		Font root;

		// PROC
        static void GetPat(Font fnt, char ch, int& dx, int& x, int& y, int& w, int& h, QByteArray& pattern);
        static void RdInt16(Files::Rider& R, uint8_t& b0);
        Font This(_ValArray<char> name);
		static void Free();

	};
}

#endif // __ObFonts_H__
