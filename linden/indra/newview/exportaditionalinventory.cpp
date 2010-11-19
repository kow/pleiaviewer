/* Copyright (c) 2009
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
//HPA export and more added -Patrick Sapinski (Wednesday, November 25, 2009)

#include "llviewerprecompiledheaders.h"

#include "exportaditionalinventory.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "lldirpicker.h"
#include "llfilepicker.h"
#include "llviewerregion.h"
#include "llwindow.h"
#include "lltransfersourceasset.h"
#include "lltooldraganddrop.h"
#include "llviewernetwork.h"
#include "llcurl.h"
#include "llviewerimagelist.h"
#include "llscrolllistctrl.h"

#include "llimagej2c.h"

#include "llviewertexteditor.h"
#include "lllogchat.h" //for timestamp
#include "lluictrlfactory.h"
#include "llcheckboxctrl.h"
#include "llcallbacklist.h"

#include <fstream>
using namespace std;

#ifdef LL_STANDALONE
# include <zlib.h>
#else
# include "zlib/zlib.h"
#endif

ExportInvTracker* ExportInvTracker::sInstance;
U32 ExportInvTracker::status;
std::string ExportInvTracker::destination;
LLXMLNodePtr ExportInvTracker::root;
std::string ExportInvTracker::asset_dir;
void cmdline_printchat(std::string chat);
std::list<InventoryRequest_t*> ExportInvTracker::requested_inventory;

ExportInvTrackerFloater* ExportInvTrackerFloater::sInstance = 0;

LLDynamicArray<LLViewerObject*> ExportInvTrackerFloater::objectsNeeded;

std::map<LLUUID,LLSD *> ExportInvTracker::received_inventory;

int ExportInvTrackerFloater::inventories_received = 0;
int ExportInvTrackerFloater::inventories_needed = 0;
int ExportInvTrackerFloater::inventory_assets_received = 0;
int ExportInvTrackerFloater::inventory_assets_needed = 0;

void ExportInvTrackerFloater::draw()
{
	LLFloater::draw();

	std::string status_text;

	if (ExportInvTracker::status == ExportInvTracker::IDLE)
		status_text = "idle";
	else
		status_text = "exporting";

	//is this a bad place for this function? -Patrick Sapinski (Friday, November 13, 2009)
	sInstance->getChild<LLTextBox>("status label")->setValue(
		"Status: " + status_text
		+  llformat("\nInventories Received: %u/%u",inventories_received,inventories_needed)
		+  llformat("\nInventory Items: %u/%u",inventory_assets_received,inventory_assets_needed)
		);

	// Draw the progress bar.

	LLRect rec  = getChild<LLPanel>("progress_bar")->getRect();

	S32 bar_width = rec.getWidth();
	S32 left = rec.mLeft, right = rec.mRight;
	S32 top = rec.mTop;
	S32 bottom = rec.mBottom;

	gGL.color4f(0.f, 0.f, 0.f, 0.75f);
	gl_rect_2d(rec);

	F32 data_progress = ((F32)inventory_assets_received/inventory_assets_needed);

	if (data_progress > 0.0f)
	{
		// Downloaded bytes
		right = left + llfloor(data_progress * (F32)bar_width);
		if (right > left)
		{
			gGL.color4f(0.f, 0.f, 1.f, 0.75f);
			gl_rect_2d(left, top, right, bottom);
		}
	}

	
}


ExportInvTrackerFloater::ExportInvTrackerFloater()
:	LLFloater( std::string("Inventory Export Floater") )
{
	LLUICtrlFactory::getInstance()->buildFloater( this, "floater_inv_export.xml" );

	childSetAction("export", onClickStart, this);
	//childSetAction("close", onClickClose, this);
	childSetEnabled("export",true);
	childSetAction("stop", onClickStop, this);

	ExportInvTracker::init();

	gIdleCallbacks.deleteFunction(ExportInvTracker::exportworker);
	inventories_received = 0;
	inventories_needed = 0;
	inventory_assets_received = 0;
	inventory_assets_needed = 0;
}

ExportInvTrackerFloater* ExportInvTrackerFloater::getInstance()
{
    if ( ! sInstance )
        sInstance = new ExportInvTrackerFloater();

	return sInstance;
}

ExportInvTrackerFloater::~ExportInvTrackerFloater()
{	
	ExportInvTracker::sInstance->close();
	ExportInvTrackerFloater::sInstance = NULL;
	sInstance = NULL;
}

void ExportInvTrackerFloater::close()
{
	if(sInstance)
	{
		ExportInvTracker::finalize();
		delete sInstance;
		sInstance = NULL;
	}
}

void ExportInvTrackerFloater::show()
{
	if(sInstance)
	{
		ExportInvTracker::sInstance->close();
		delete sInstance;
		sInstance = NULL;
	}

	if(NULL==sInstance) 
	{
		sInstance = new ExportInvTrackerFloater();
		llwarns << "sInstance assigned. sInstance=" << sInstance << llendl;
	}
	
	sInstance->open();	/*Flawfinder: ignore*/

}

// static
void ExportInvTrackerFloater::onClickStart(void* data)
{
	ExportInvTracker::serialize();
}

// static
void ExportInvTrackerFloater::onClickClose(void* data)
{
	sInstance->close();
	ExportInvTracker::sInstance->close();
}
// static
void ExportInvTrackerFloater::onClickStop(void* data)
{
	sInstance->close();
	ExportInvTracker::sInstance->close();
}

ExportInvTracker::ExportInvTracker()
{
	llassert_always(sInstance == NULL);
	sInstance = this;
}

ExportInvTracker::~ExportInvTracker()
{
	ExportInvTracker::cleanup();
	sInstance = NULL;
}

void ExportInvTracker::close()
{
	if(sInstance)
	{
		delete sInstance;
		sInstance = NULL;
	}
}

void ExportInvTracker::init()
{
	if(!sInstance)
	{
		sInstance = new ExportInvTracker();
	}
	status = IDLE;
	destination = "";
	asset_dir = "";
}

struct JCAssetInfo
{
	std::string path;
	std::string name;
	LLUUID assetid;
};


//getRegisterInventoryListenerAndAddtoDB
bool ExportInvTracker::prepareToGetInventory(LLViewerObject * obj)
{
	InventoryRequest_t * ireq = new InventoryRequest_t();
	ireq->object=obj;
	ireq->request_time = 0;	
	ireq->num_retries = 0;	
	requested_inventory.push_back(ireq);
	obj->mInventoryRecieved=false;
	obj->registerInventoryListener(sInstance,(void*)ireq);
	
	return true;
}



//This function accepts the HPA <group> object and sends them to the object parser
void ExportInvTracker::parse_hpa_group(LLXMLNodePtr group)
{
	for (LLXMLNodePtr child = group->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("center")||child->hasName("max")||child->hasName("min"))
		{
			//who cares, we aren't even importing, 
			//excluding them here to make it easy to find object types in the else
		}
		else if (child->hasName("group"))
		{
			parse_hpa_group(child);			
		}
		else if (child->hasName("linkset"))
		{
			for (LLXMLNodePtr innerchild = child->getFirstChild(); innerchild.notNull(); innerchild = innerchild->getNextSibling())
			{
				parse_hpa_object(innerchild);
			}
		}
		else
		{
			parse_hpa_object(child);
		}
	}
}



//This function accepts a <box>,<cylinder>,<etc> XML object and builds a list of objects who we havnt scanned inventory of
void ExportInvTracker::parse_hpa_object(LLXMLNodePtr prim)
{
	BOOL inventoryRecieved=FALSE;
	LLUUID objectUUID=LLUUID::null;
	LLSD invToCheck=0;
	LLXMLNodePtr inventoryNode;
	for (LLXMLNodePtr param = prim->getFirstChild(); param.notNull(); param = param->getNextSibling())
	{			
		if (param->hasName("inventory"))
		{
			inventoryNode=param;

			for (LLXMLNodePtr item = param->getFirstChild(); item.notNull(); item = item->getNextSibling())
			{
				if(item->hasName("received"))
				{
					inventoryRecieved=TRUE;
				}else if(item->hasName("item"))
				{
					LLSD sd;
					for (LLXMLNodePtr info = item->getFirstChild(); info.notNull(); info = info->getNextSibling())
					{
						if (info->hasName("item_id"))
							sd["item_id"] = info->getTextContents();
						else if (info->hasName("type"))
							sd["type"] = info->getTextContents();
					}
					invToCheck.append(sd);
				}
			}
		}else if(param->hasName("uuid"))
		{
			objectUUID=LLUUID(param->getTextContents());
		}
	}

	if(!objectUUID.notNull())
	{
		cmdline_printchat("This hpa file is too old to support dynamic inventory retreival");
		return;
	}
	//lets see if we have the same object this session, else we are SOL
	LLViewerObject * object = gObjectList.findObject(objectUUID);
	if(object)
	{
		//ok, we got it :D add it to the list and get its inventory soul
		if(!inventoryRecieved)
		{
			//gasp! this is what we are looking for!
			//first, see if we miraculously already have the inventory, in which case we are saving
			LLSD * inventory = received_inventory[objectUUID];
			if(inventory !=NULL)
			{
				//llinfos << "we are saving.. and now have inv for "<<objectUUID.asString().c_str()<<llendl;
				//delete existing childen (items) and replace with new found ones
				LLXMLNodePtr child = inventoryNode->getFirstChild();
				while(child.notNull())
				{
					LLXMLNodePtr toDelete = child;
					child = child->getNextSibling();
					inventoryNode->deleteChild(toDelete.get());
				}

				if(object->mInventoryRecieved)
				{
					inventoryNode->createChild("received", FALSE)->setValue("true");
				}
				//for each inventory item
				for (LLSD::array_iterator inv = (*inventory).beginArray(); inv != (*inventory).endArray(); ++inv)
				{
					LLSD item = (*inv);
					LLXMLNodePtr field_xml = inventoryNode->createChild("item", FALSE);
					field_xml->createChild("description", FALSE)->setValue(item["desc"].asString());
					field_xml->createChild("item_id", FALSE)->setValue(item["item_id"].asString());
					field_xml->createChild("name", FALSE)->setValue(item["name"].asString());
					field_xml->createChild("type", FALSE)->setValue(item["type"].asString());
					//llinfos << "injecting inventory for "<<objectUUID.asString().c_str()<<" named "<<item["name"].asString().c_str()<<llendl;

				}
				delete(inventory);
				received_inventory.erase(objectUUID);

			}
			else
			{
				//llinfos << "no inventory found for "<<objectUUID.asString().c_str()<<" grabbing it..."<<llendl;
				ExportInvTrackerFloater::objectsNeeded.put(object);
				ExportInvTrackerFloater::inventories_needed++;
			}
		}
		else
		{
			checkInventory(object,invToCheck);
		}
	}
	else
	{
		//llinfos << "unable  to find object "<<objectUUID.asString().c_str()<<llendl;
		//cmdline_printchat("uh oh, object "+objectUUID.asString());
	}
	//check if object exists and if inv needed
}

void ExportInvTracker::checkInventory(LLViewerObject* item,LLSD invs)
{
	for(int i =0;i<invs.size();i++)
	{
		LLSD inv = invs[i];
		std::string filename(inv["item_id"].asString()+"."+inv["type"].asString());
		std::string full_filename(asset_dir+gDirUtilp->getDirDelimiter()+"inventory"+gDirUtilp->getDirDelimiter()+filename);
		if(!gDirUtilp->fileExists(full_filename))
		{
			//llinfos << "Found missing inventory item " <<full_filename.c_str() <<llendl;
			ExportInvTrackerFloater::objectsNeeded.put(item);
			ExportInvTrackerFloater::inventories_needed++;
		}
	}
}

bool ExportInvTracker::serialize()
{
	LLSelectMgr::getInstance()->deselectAll();

	LLFilePicker& file_picker = LLFilePicker::instance();
		
	if (!file_picker.getOpenFile(LLFilePicker::FFLOAD_HPA))
		return false; // User canceled save.

	init();

	status = EXPORTING;

	ExportInvTrackerFloater::sInstance->childSetEnabled("export",false);
	destination = file_picker.getFirstFile();
	asset_dir = gDirUtilp->getDirName(destination);

	//scan hpa file quickly
	LLXMLNodePtr newRoot;
	if (!LLXMLNode::parseFile(destination,newRoot,NULL))
	{
		cmdline_printchat("Problem reading HPA file: " + destination);
		return FALSE;
	}
	root=newRoot;
	for (LLXMLNodePtr child = root->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("group"))
		{
			parse_hpa_group(child);
		}
	}

	gIdleCallbacks.addFunction(exportworker, NULL);

	return true;

}

void ExportInvTracker::exportworker(void *userdata)
{
	//CHECK IF WE'RE DONE
	if(requested_inventory.empty()
		&& (ExportInvTrackerFloater::inventory_assets_needed - ExportInvTrackerFloater::inventory_assets_received ==0)
		&& (ExportInvTrackerFloater::inventories_needed - ExportInvTrackerFloater::inventories_received ==0)
		)
	{
		finalize();
		return;
	}

	int kick_count=0;
	int total=requested_inventory.size();
	// Check for inventory properties that are taking too long
	std::list<InventoryRequest_t*> still_going;
	still_going.clear();
	while(total!=0)
	{
		std::list<InventoryRequest_t*>::iterator iter;
		time_t tnow=time(NULL);
		
		InventoryRequest_t * req;
		iter=requested_inventory.begin();
		req=(*iter);
		requested_inventory.pop_front();
		
		if( (req->request_time+INV_REQUEST_KICK)< tnow)
		{
			req->request_time=time(NULL);
			if(ExportInvTrackerFloater::objectsNeeded.empty())//dont count inv retrys till things calm down a bit
			{
				req->num_retries++;
				//cmdline_printchat("Re-trying inventory request for " + req->object->mID.asString()+
				//	", try number "+llformat("%u",req->num_retries));
			}

			//LLViewerObject* obj = gObjectList.findObject(req->object->mID);
			LLViewerObject* obj = req->object;

			//cmdline_printchat("requesting inventory: " + obj->mID.asString());

			if(obj->mID.isNull())
				cmdline_printchat("no mID");

			if (!obj)
			{
				cmdline_printchat("export worker: " + req->object->mID.asString() + " not found in object list");
			}
			LLPCode pcode = obj->getPCode();
			if (pcode != LL_PCODE_VOLUME &&
				pcode != LL_PCODE_LEGACY_GRASS &&
				pcode != LL_PCODE_LEGACY_TREE)
			{
				cmdline_printchat("exportworker: object has invalid pcode");
				//return NULL;
			}
			obj->dirtyInventory();
			obj->requestInventory();
			//kick_count++;
		}
		kick_count++;

		if (req->num_retries < gSavedSettings.getU32("ExporterNumberRetrys"))
			still_going.push_front(req);
		else
		{
			//req->object->mInventoryRecieved = true;

			cmdline_printchat("failed to retrieve inventory for " + req->object->mID.asString());
			ExportInvTrackerFloater::inventories_received++;
		}
		
		if(kick_count>=gSavedSettings.getS32("ExporterNumberConcurentDownloads"))
			break; // that will do for now				

		total--;
	}
	while(!still_going.empty())
	{
		std::list<InventoryRequest_t*>::iterator iter;
		InventoryRequest_t * req;
		iter=still_going.begin();
		req=(*iter);
		still_going.pop_front();
		requested_inventory.push_front(req);
	}

	int count=0;

	//Keep the requested list topped up
	while(count < 20)
	{
		if(!ExportInvTrackerFloater::objectsNeeded.empty())
		{
			LLViewerObject* obj=ExportInvTrackerFloater::objectsNeeded.get(0);
			ExportInvTrackerFloater::objectsNeeded.remove(0);

			if (!obj)
			{
				cmdline_printchat("export worker invalid obj");
				break;
			}

			prepareToGetInventory(obj);
			count++;
		}
		else
		{
			break;
		}
	}
}

void ExportInvTracker::finalize()
{
	for (LLXMLNodePtr child = root->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("group"))
		{
			parse_hpa_group(child);
		}
	}

	llofstream out(destination+".new.hpa",ios_base::out);
	if (!out.good())
	{
		llwarns << "Unable to open for output." << llendl;
	}
	else
	{
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		root->writeToOstream(out);
		out.close();
	}

	//

	root=NULL;
	ExportInvTrackerFloater::objectsNeeded.clear();
	ExportInvTrackerFloater::getInstance()->childSetEnabled("export",true);
	cleanup();
}

BOOL canDL(LLAssetType::EType type)
{
	switch(type)
	{//things we could plausibly DL anyway atm
	case LLAssetType::AT_TEXTURE:
	case LLAssetType::AT_SCRIPT:
	case LLAssetType::AT_SOUND:
	case LLAssetType::AT_ANIMATION:
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_NOTECARD:
	case LLAssetType::AT_LSL_TEXT:
	case LLAssetType::AT_TEXTURE_TGA:
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_GESTURE:
		return TRUE;
		break;
	default:
		return FALSE;
		break;
	}
}


void ExportInvTracker::inventoryChanged(LLViewerObject* obj,
								 InventoryObjectList* inv,
								 S32 serial_num,
								 void* user_data)
{
	if(status == IDLE)
	{
		obj->removeInventoryListener(sInstance);
		return;
	}

	if(requested_inventory.empty())
		return;

	std::list<InventoryRequest_t*>::iterator iter=requested_inventory.begin();
	for(;iter!=requested_inventory.end();iter++)
	{
		if((*iter)->object->getID()==obj->getID())
		{
			//cmdline_printchat("downloaded inventory for "+obj->getID().asString());
			LLSD * inventory = new LLSD();
			InventoryObjectList::const_iterator it = inv->begin();
			InventoryObjectList::const_iterator end = inv->end();
			U32 num = 0;
			for( ;	it != end;	++it)
			{
				LLInventoryObject* asset = (*it);
				if(asset)
				{
					LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
					LLViewerInventoryItem* new_item = (LLViewerInventoryItem*)item;

					LLPermissions perm;
					llassert(new_item && new_item->getPermissions());

					perm = new_item->getPermissions();

					if(canDL(asset->getType())
						&& perm.allowCopyBy(gAgent.getID())
						&& perm.allowModifyBy(gAgent.getID())
						&& perm.allowTransferTo(LLUUID::null))// && is_asset_id_knowable(asset->getType()))
					{
						LLSD inv_item;
						inv_item["name"] = asset->getName();
						inv_item["type"] = LLAssetType::lookup(asset->getType());
						//cmdline_printchat("requesting asset "+asset->getName());
						inv_item["desc"] = ((LLInventoryItem*)((LLInventoryObject*)(*it)))->getDescription();//god help us all
						inv_item["item_id"] = asset->getUUID().asString();
						if(!LLFile::isdir(asset_dir+gDirUtilp->getDirDelimiter()+"inventory"))
						{
							LLFile::mkdir(asset_dir+gDirUtilp->getDirDelimiter()+"inventory");
						}

						ExportInvTracker::mirror(asset, obj, asset_dir+gDirUtilp->getDirDelimiter()+"inventory", asset->getUUID().asString());//loltest
						//unacceptable
						(*inventory)[num] = inv_item;
						num += 1;
						ExportInvTrackerFloater::inventory_assets_needed++;
					}
				}
			}

			//MEH
			//(*link_itr)["inventory"] = inventory;
			received_inventory[obj->getID()]=inventory;

			//cmdline_printchat(llformat("%d inv queries left",requested_inventory.size()));

			obj->removeInventoryListener(sInstance);
			obj->mInventoryRecieved=true;

			requested_inventory.erase(iter);
			ExportInvTrackerFloater::inventories_received++;
			break;
		}
	}


}

void InvAssetExportCallback(LLVFS *vfs, const LLUUID& uuid, LLAssetType::EType type, void *userdata, S32 result, LLExtStat extstat)
{
	JCAssetInfo* info = (JCAssetInfo*)userdata;
	if(result == LL_ERR_NOERR)
	{
		//cmdline_printchat("Saved file "+info->path+" ("+info->name+")");
		S32 size = vfs->getSize(uuid, type);
		U8* buffer = new U8[size];
		vfs->getData(uuid, type, buffer, 0, size);

		if(type == LLAssetType::AT_NOTECARD)
		{
			LLViewerTextEditor* edit = new LLViewerTextEditor("",LLRect(0,0,0,0),S32_MAX,"");
			if(edit->importBuffer((char*)buffer, (S32)size))
			{
				//cmdline_printchat("Decoded notecard");
				std::string card = edit->getText();
				//delete edit;
				edit->die();
				delete buffer;
				buffer = new U8[card.size()];
				size = card.size();
				strcpy((char*)buffer,card.c_str());
			}else cmdline_printchat("Failed to decode notecard");
		}
		LLAPRFile infile;
		infile.open(info->path.c_str(), LL_APR_WB,LLAPRFile::global);
		apr_file_t *fp = infile.getFileHandle();
		if(fp)infile.write(buffer, size);
		infile.close();

		ExportInvTrackerFloater::inventory_assets_received++;

		//delete buffer;
	} else cmdline_printchat("Failed to save file "+info->path+" ("+info->name+") : "+std::string(LLAssetStorage::getErrorString(result)));

	delete info;
}

BOOL ExportInvTracker::mirror(LLInventoryObject* item, LLViewerObject* container, std::string root, std::string iname)
{
	if(item)
	{
		////cmdline_printchat("item");
		//LLUUID asset_id = item->getAssetUUID();
		//if(asset_id.notNull())
		LLPermissions perm(((LLInventoryItem*)item)->getPermissions());
		if(perm.allowCopyBy(gAgent.getID())
		&& perm.allowModifyBy(gAgent.getID())
		&& perm.allowTransferTo(LLUUID::null))
		{
			////cmdline_printchat("asset_id.notNull()");
			LLDynamicArray<std::string> tree;
			LLViewerInventoryCategory* cat = gInventory.getCategory(item->getParentUUID());
			while(cat)
			{
				std::string folder = cat->getName();
				folder = curl_escape(folder.c_str(), folder.size());
				tree.insert(tree.begin(),folder);
				cat = gInventory.getCategory(cat->getParentUUID());
			}
			if(container)
			{
				//tree.insert(tree.begin(),objectname i guess fuck);
				//wat
			}
			if(root == "")root = gSavedSettings.getString("EmeraldInvMirrorLocation");
			if(!LLFile::isdir(root))
			{
				cmdline_printchat("Error: mirror root \""+root+"\" is nonexistant");
				return FALSE;
			}
			std::string path = gDirUtilp->getDirDelimiter();
			root = root + path;
			for (LLDynamicArray<std::string>::iterator it = tree.begin();
			it != tree.end();
			++it)
			{
				std::string folder = *it;
				root = root + folder;
				if(!LLFile::isdir(root))
				{
					LLFile::mkdir(root);
				}
				root = root + path;
				//cmdline_printchat(root);
			}
			if(iname == "")
			{
				iname = item->getName();
				iname = curl_escape(iname.c_str(), iname.size());
			}
			root = root + iname + "." + LLAssetType::lookup(item->getType());
			//cmdline_printchat(root);

			JCAssetInfo* info = new JCAssetInfo;
			info->path = root;
			info->name = item->getName();
			info->assetid = item->getUUID();
			
			//LLHost host = container != NULL ? container->getRegion()->getHost() : LLHost::invalid;

			switch(item->getType())
			{
			case LLAssetType::AT_TEXTURE:
			case LLAssetType::AT_NOTECARD:
			case LLAssetType::AT_SCRIPT:
			case LLAssetType::AT_LSL_TEXT: // normal script download
			case LLAssetType::AT_LSL_BYTECODE:
				gAssetStorage->getInvItemAsset(container != NULL ? container->getRegion()->getHost() : LLHost::invalid,
					gAgent.getID(),
					gAgent.getSessionID(),
					perm.getOwner(),
					container != NULL ? container->getID() : LLUUID::null,
					item->getUUID(),
					info->assetid,
					item->getType(),
					InvAssetExportCallback,
					info,
					TRUE
				);
				break;
			case LLAssetType::AT_SOUND:
			case LLAssetType::AT_CLOTHING:
			case LLAssetType::AT_BODYPART:
			case LLAssetType::AT_ANIMATION:
			case LLAssetType::AT_GESTURE:
			default:
				gAssetStorage->getAssetData(info->assetid, item->getType(), InvAssetExportCallback, info, TRUE);
				break;
			}

			return TRUE;

			//gAssetStorage->getAssetData(asset_id, item->getType(), JCAssetExportCallback, info,1);
		}
	}
	return FALSE;
}


void ExportInvTracker::cleanup()
{
	gIdleCallbacks.deleteFunction(exportworker);
	
	status=IDLE;

	std::list<InventoryRequest_t*>::iterator iter3=requested_inventory.begin();
	for(;iter3!=requested_inventory.end();iter3++)
	{
		(*iter3)->object->removeInventoryListener(sInstance);
	}

	requested_inventory.clear();
	
	std::map<LLUUID,LLSD *>::iterator iter2=received_inventory.begin();
	for(;iter2!=received_inventory.begin();iter2++)
	{
		free((*iter2).second);
	}
	received_inventory.clear();
}
