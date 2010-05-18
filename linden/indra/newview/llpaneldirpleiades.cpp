/**
 * @file llpaneldirfind.cpp
 * @brief The "All" panel in the Search directory.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpaneldirpleiades.h"

// linden library includes
#include "llclassifiedflags.h"
#include "llfontgl.h"
#include "llparcel.h"
#include "llqueryflags.h"
#include "message.h"

// viewer project includes
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lllineeditor.h"
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llmenucommands.h"
#include "llmenugl.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llviewerimagelist.h"
#include "llviewermessage.h"
#include "llfloateravatarinfo.h"
#include "lldir.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"		// for region name for search urls
#include "lluictrlfactory.h"
#include "llfloaterdirectory.h"
#include "llpaneldirbrowser.h"
#include "llpaneldirfind.h" //we're based on LL's all search tab, include classes from there

#include <boost/tokenizer.hpp>
#if LL_WINDOWS
	// Disable warning for unreachable code in boost/lexical_cast.hpp for Boost 1.36 -- McCabe
	#pragma warning(disable : 4702)
#include "boost/lexical_cast.hpp"
	#pragma warning(default : 4702)
#else
#include "boost/lexical_cast.hpp"
#endif

//---------------------------------------------------------------------------
// LLPanelDirPleiades - Google search appliance based search
//---------------------------------------------------------------------------

class LLPanelDirPleiades
:	public LLPanelDirFind
{
public:
	LLPanelDirPleiades(const std::string& name, LLFloaterDirectory* floater);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);
	/*virtual*/ void search(const std::string& search_text);
};

LLPanelDirPleiades::LLPanelDirPleiades(const std::string& name, LLFloaterDirectory* floater)
:	LLPanelDirFind(name, floater, "find_browser")
{
}

// virtual
void LLPanelDirPleiades::reshape(S32 width, S32 height, BOOL called_from_parent = TRUE)
{
	if ( mWebBrowser )
		mWebBrowser->navigateTo( mWebBrowser->getCurrentNavUrl() );

	LLUICtrl::reshape( width, height, called_from_parent );
}

void LLPanelDirPleiades::search(const std::string& search_text)
{
	BOOL inc_pg = childGetValue("incpg").asBoolean();
	BOOL inc_mature = childGetValue("incmature").asBoolean();
	BOOL inc_adult = childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotifications::instance().add("NoContentToSearch");
		return;
	}
	
	if (!search_text.empty())
	{
		std::string selected_collection = childGetValue( "Category" ).asString();
		std::string url = buildSearchURL(search_text, selected_collection, inc_pg, inc_mature, inc_adult);
		if (mWebBrowser)
		{
			mWebBrowser->navigateTo(url);
		}
	}
	else
	{
		// empty search text
		navigateToDefaultPage();
	}

	childSetText("search_editor", search_text);
}

//---------------------------------------------------------------------------
// LLPanelDirPleiadesInterface
//---------------------------------------------------------------------------

// static
LLPanelDirPleiades* LLPanelDirPleiadesInterface::create(LLFloaterDirectory* floater)
{
	return new LLPanelDirPleiades("pleiades_search_panel", floater);
}

// static
void LLPanelDirPleiadesInterface::search(LLPanelDirPleiades* panel,
										const std::string& search_text)
{
	panel->search(search_text);
}

// static
void LLPanelDirPleiadesInterface::focus(LLPanelDirPleiades* panel)
{
	panel->focus();
}

