/* Copyright (c) 2009
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "llagent.h"
#include "llfloater.h"

#define INV_REQUEST_KICK 10

struct InventoryRequest_t
{
	time_t	request_time;
	LLViewerObject * object; // I can't be bothered to itterate the objects list on every kick, so hold the pointer here
	U32		num_retries;
};

class ExportInvTrackerFloater : public LLFloater
{
public:
	void draw();
	static ExportInvTrackerFloater* getInstance();
	virtual ~ExportInvTrackerFloater();
	//close me
	static void close();
	void show();
	ExportInvTrackerFloater();	
	static ExportInvTrackerFloater* sInstance;

	static LLDynamicArray<LLViewerObject*> objectsNeeded;


	//Import button
	static void onClickStart(void* data);
	static void onClickStop(void* data);
	
	//Close button
	static void onClickClose(void* data);

	static int		inventories_received;
	static int		inventories_needed;
	static int		inventory_assets_received;
	static int		inventory_assets_needed;
};

class ExportInvTracker : public LLVOInventoryListener
{
public:
	ExportInvTracker();
	~ExportInvTracker();
	static void close();

	static ExportInvTracker* sInstance;
	static void init();
public:
	void inventoryChanged(LLViewerObject* obj,
								 InventoryObjectList* inv,
								 S32 serial_num,
								 void* data);

	static ExportInvTracker* getInstance(){ init(); return sInstance; }

	static bool prepareToGetInventory(LLViewerObject * obj);
	//Export idle callback
	static void exportworker(void *userdata);

	static bool serialize();

	static void parse_hpa_group(LLXMLNodePtr group);
	static void parse_hpa_object(LLXMLNodePtr prim);
	static void checkInventory(LLViewerObject* item,LLSD invs);

	static void finalize();

	static BOOL mirror(LLInventoryObject* item, LLViewerObject* container = NULL, std::string root = "", std::string iname = "");

	enum ExportState { IDLE, EXPORTING };

	static U32 status;
	static U32 totalinvneeded;
	static U32 totalassetsneeded;

	static void cleanup();

	static std::list<InventoryRequest_t*> requested_inventory;

	static std::map<LLUUID,LLSD *>received_inventory;

private:
	static std::string destination;
	static std::string asset_dir;
	static LLXMLNodePtr root;

	
};
