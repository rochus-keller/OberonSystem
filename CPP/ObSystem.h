#ifndef __ObSystem_H__
#define __ObSystem_H__

// Generated by OberonViewer 0.7.5 on 2020-01-27T01:00:45
// Then further developed manually by RK

#include <OberonSystem/Ob_Global.h>
#include <OberonSystem/Ob_SYSTEM.h>
#include <OberonSystem/ObKernel.h>
#include <OberonSystem/ObFileDir.h>
#include <OberonSystem/ObFiles.h>
#include <OberonSystem/ObModules.h>
#include <OberonSystem/ObInput.h>
#include <OberonSystem/ObDisplay.h>
#include <OberonSystem/ObViewers.h>
#include <OberonSystem/ObFonts.h>
#include <OberonSystem/ObTexts.h>
#include <OberonSystem/ObOberon.h>
#include <OberonSystem/ObMenuViewers.h>
#include <OberonSystem/ObTextFrames.h>

namespace Ob
{
	class System : public _Root
	{
	public:
		/* JG 3.10.90 / NW 12.10.93 / NW 20.6.2016 */
		static System* _inst();
		System();
        void _init();
		~System();

		// CONST
        static const _ValArray<char> StandardMenu;
        static const _ValArray<char> LogMenu;

		// VAR
		Texts::Writer W;
		_FxArray<char,32> pat;

		// PROC
		static void GetArg(Texts::Scanner& S);
		static void EndLine();
		static void SetUser();
		static void SetFont();
		static void SetColor();
		static void SetOffset();
		static void Date();
		static void Collect();
		static void Open();
		static void Clear();
		static void Close();
		static void CloseTrack();
		static void Recall();
		static void Copy();
		static void Grow();
		static void Free1(Texts::Scanner& S);
		static void Free();
		static void FreeFonts();
		static void List(_ValArray<char> name, int adr, bool& cont);
		static void Directory();
		static void CopyFiles();
		static void RenameFiles();
		static void DeleteFiles();
		static void Watch();
		static void ShowModules();
		static void ShowCommands();
		static void ShowFonts();
		static void OpenViewers();
		static void ExtendDisplay();
		static void Trap(int& a, int b);
		static void Abort();

	};
}

#endif // __ObSystem_H__