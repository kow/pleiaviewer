/* Copyright (c) 2009
 *
 * Modular Systems All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS LTD AND CONTRIBUTORS "AS IS"
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

#define FOLLOW_PERMS 0
#define PROP_REQUEST_KICK 10
#define INV_REQUEST_KICK 10

struct PropertiesRequest_t
{
	time_t	request_time;
	LLUUID	target_prim;
	U32		localID;
	U32		num_retries;
};

struct InventoryRequest_t
{
	time_t	request_time;
	LLViewerObject * object; // I can't be bothered to itterate the objects list on every kick, so hold the pointer here
	LLViewerObject * real_object; //for when we need to know what object the request is "actually" for (if using surrogates)
	U32		num_retries;
};


class ExportTrackerFloater : public LLFloater
{
public:
	void draw();
	static ExportTrackerFloater* getInstance() { return sInstance; }
	virtual ~ExportTrackerFloater();
	//close me
	static void close();
	void refresh();
	BOOL postBuild(void);

	void show();
	ExportTrackerFloater();	
	static ExportTrackerFloater* sInstance;

	//BOOL handleMouseDown(S32 x,S32 y,MASK mask);
	//BOOL handleMouseUp(S32 x,S32 y,MASK mask);
	//BOOL handleHover(S32 x,S32 y,MASK mask);

	//static void 	onCommitPosition(LLUICtrl* ctrl, void* userdata);

	//Import button
	static void onClickExport(void* data);
	void addAvatarStuff(LLVOAvatar* avatarp);
	void buildExportListFromSelection();
	void buildExportListFromAvatar(LLVOAvatar* avatarp);

	//List handling
	static void onClickSelectAll(void* user_data);
	static void onClickSelectNone(void* user_data);
	static void onClickSelectObjects(void* user_data);
	static void onClickSelectWearables(void* user_data);
	static void onChangeFilter(void* user_data, std::string filter);
	void buildListDisplays();
	void addObjectToList(LLUUID id, BOOL checked, std::string name, std::string type, LLUUID owner_id);
	
	static void RemoteStart(LLDynamicArray<LLViewerObject*> catfayse, int primcount);

	//Close button
	static void onClickClose(void* data);


	static LLDynamicArray<LLViewerObject*> mObjectSelection;
	static LLDynamicArray<LLViewerObject*> mObjectSelectionWaitList;

private:
	enum LIST_COLUMN_ORDER
	{
		LIST_CHECKED,
		LIST_TYPE,
		LIST_NAME,
		LIST_AVATARID
	};
	LLObjectSelectionHandle mSelection;

};

class JCExportTracker : public LLVOInventoryListener
{
public:
	JCExportTracker();
	~JCExportTracker();
	static void close();

	static JCExportTracker* sInstance;
	static void init();
private:
	static LLSD* getprim(LLUUID id);
public:
	static void processObjectProperties(LLMessageSystem* msg, void** user_data);
	void inventoryChanged(LLViewerObject* obj,
								 InventoryObjectList* inv,
								 S32 serial_num,
								 void* data);
	static void			onFileLoadedForSave( 
							BOOL success,
							LLViewerImage *src_vi,
							LLImageRaw* src, 
							LLImageRaw* aux_src,
							S32 discard_level, 
							BOOL final,
							void* userdata );


	static JCExportTracker* getInstance(){ init(); return sInstance; }

	static bool serialize(LLDynamicArray<LLViewerObject*> objects);

	static bool getAsyncData(LLViewerObject * obj);

	static void requestInventory(LLViewerObject * obj, LLViewerObject * surrogate_obj=NULL);

	static void processSurrogate(LLViewerObject * surrogate_object);
	static void createSurrogate(LLViewerObject * object);
	static void removeSurrogates();

	static void error(std::string name, U32 localid, LLVector3 object_pos, std::string error_msg);

	//Export idle callback
	static void exportworker(void *userdata);
	static void propertyworker(void *userdata);

	static bool serializeSelection();
	static void finalize();

	static BOOL mirror(LLInventoryObject* item, LLViewerObject* container = NULL, std::string root = "", std::string iname = "");

private:
	static LLSD* subserialize(LLViewerObject* linkset);
	static void requestPrimProperties(U32 localID);

public:
	enum ExportState { IDLE, EXPORTING };

	static U32 mStatus;

	//enum ExportLevel { DEFAULT, PROPERTIES, INVENTORY };

	static BOOL export_properties;
	static BOOL export_inventory;
	static BOOL export_tga;
	static BOOL export_j2c;

	static BOOL export_is_avatar;

	static BOOL using_surrogates;

	//static U32 level;
	static LLVector3 selection_center;
	static LLVector3 selection_size;

	static U32		propertyqueries;
	static U32		invqueries;
	static U32		mTotalPrims;
	static U32		mLinksetsExported;
	static U32		mPropertiesReceived;
	static U32		mInventoriesReceived;
	static U32		mPropertiesQueries;
	static U32		mAssetsExported;
	static U32		mTexturesExported;
	static U32		mTotalObjects;
	static U32		mTotalLinksets;
	static U32		mTotalAssets;
	static U32		mTotalTextures;
	
	static void cleanup();

	static std::set<LLUUID> mRequestedTextures;

	static std::list<PropertiesRequest_t*> requested_properties;
	static std::list<InventoryRequest_t*> requested_inventory;

	static std::list<LLSD *> processed_prims;
	static std::map<LLUUID,LLSD *>received_inventory;
	static std::map<LLUUID,LLSD *>received_properties;

	static std::map<LLVector3, LLUUID> expected_surrogate_pos;
	static std::list<LLViewerObject *> surrogate_roots;

	static std::string destination;
private:
	static LLSD total;

	static std::string asset_dir;
};

// zip a folder. this doesn't work yet.
BOOL zip_folder(const std::string& srcfile, const std::string& dstfile);
