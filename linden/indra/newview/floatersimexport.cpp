
#include "llviewerprecompiledheaders.h"

// system library includes
#include <iostream>
#include <fstream>
#include <sstream>

// linden library includes
#include "indra_constants.h"
#include "llcallbacklist.h"

// newview includes
#include "llagent.h"
#include "llselectmgr.h"
#include "llviewercontrol.h"	// gSavedSettings
#include "llcheckboxctrl.h"
#include "llspinctrl.h"
#include "llviewergenericmessage.h"
#include "llviewerimage.h"
#include "llviewerimagelist.h"

#include "lluictrlfactory.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llappviewer.h"

#include "llparcelselection.h"
#include "llviewerparcelmgr.h"
#include "llvlcomposition.h"

#include "llfilepicker.h"

#include "lllogchat.h"
#include "floatersimexport.h" 
#include "exporttracker.h"

using namespace std;

void cmdline_printchat(std::string chat);
FloaterSimExport* FloaterSimExport::sInstance = 0;
S32 cur_x = 1;
S32 cur_y = 1;
S32 cur_z = 0;

struct JCAssetInfo
{
	std::string path;
	std::string name;
	LLUUID assetid;
};

FloaterSimExport::FloaterSimExport()
:	LLFloater( std::string("Sim Export Floater") )
{
	LLUICtrlFactory::getInstance()->buildFloater( this, "floater_sim_export.xml" );

	childSetAction("export", onClickExport, this);
	childSetAction("close", onClickClose, this);
	childSetAction("saveterrain", onClickSaveTerrain, this);

	// reposition floater from saved settings
//	 LLRect rect = gSavedSettings.getRect( "FloaterSimExport" );
//	 reshape( rect.getWidth(), rect.getHeight(), FALSE );
///	 setRect( rect );

	mRunning=true;
	move_time=0;
	mRegionp = gAgent.getRegion();
	mRegionId=gAgent.getRegion()->getHandle();

	LLFilePicker& file_picker = LLFilePicker::instance();

	if (!file_picker.getSaveFile(LLFilePicker::FFSAVE_HPA, mRegionp->getName()))
		cmdline_printchat("ERROR: no destination specified!");

	target_file = file_picker.getFirstFile();

	gIdleCallbacks.addFunction(statsupdate, NULL);

	gAgent.setFlying(TRUE);
}

FloaterSimExport* FloaterSimExport::getInstance()
{
    if ( ! sInstance )
        sInstance = new FloaterSimExport();

	return sInstance;
}

FloaterSimExport::~FloaterSimExport()
{
	// save position of floater
//	gSavedSettings.setRect( "FloaterSimExport", getRect() );
	mRunning=false;

	//which one??
	FloaterSimExport::sInstance = NULL;
	sInstance = NULL;
}

void FloaterSimExport::close()
{
	if(sInstance)
	{
		gIdleCallbacks.deleteFunction(statsupdate);
		delete sInstance;
		sInstance = NULL;
	}
}

void FloaterSimExport::draw()
{
	LLFloater::draw();
}

void FloaterSimExport::onClose( bool app_quitting )
{
	setVisible( false );
	// HACK for fast XML iteration replace with:
	// destroy();
}

void FloaterSimExport::show()
{
	if(NULL==sInstance) 
	{
		sInstance = new FloaterSimExport();
		llwarns << "sInstance assigned. sInstance=" << sInstance << llendl;
	}
	
	sInstance->open();	/*Flawfinder: ignore*/
}


// static
void FloaterSimExport::onClickExport(void* data)
{
	sInstance->startRemoteExport();
}

void FloaterSimExport::onClickSaveTerrain(void * data)
{
	LLViewerRegion *regionp = gAgent.getRegion();
	//LLVLComposition *compp = regionp->getComposition();

	/*
	compp->getHeightRange(LLVLComposition::SOUTHWEST);
	compp->getHeightRange(LLVLComposition::SOUTHEAST);
	compp->getHeightRange(LLVLComposition::NORTHWEST);
	compp->getHeightRange(LLVLComposition::NORTHEAST);

	compp->getStartHeight(LLVLComposition::SOUTHWEST);
	compp->getStartHeight(LLVLComposition::SOUTHEAST);
	compp->getStartHeight(LLVLComposition::NORTHWEST);
	compp->getStartHeight(LLVLComposition::NORTHEAST);

	compp->getDetailTextureID(SOUTHWEST);
	compp->getDetailTextureID(SOUTHEAST);
	compp->getDetailTextureID(NORTHWEST);
	compp->getDetailTextureID(NORTHEAST);
*/
	F32 water_height=regionp->getWaterHeight();

	U16 terrain[256][256][13];

	LLFilePicker& file_picker = LLFilePicker::instance();
		
	if (!file_picker.getSaveFile(LLFilePicker::FFSAVE_RAW))
		return; // User canceled save.
		
	std::string destination = file_picker.getFirstFile();
	
	llofstream out(destination,llofstream::binary);

	for (int x=255;x>=0;x--)
	{
		for (int y=0;y<256;y++)
		{			
			F32 height=regionp->getLandHeightRegion(LLVector3((float)x,(float)y,0.0));
			
			if(height>256)
			{
				terrain[x][y][0]=(height/2.0);
				terrain[x][y][1]=256;
			}
			if(height>128 && height <=256)
			{
				terrain[x][y][0]=height;
				terrain[x][y][1]=128;
			}

			if(height>64 && height <=128)
			{
				terrain[x][y][0]=(height*2.0);
				terrain[x][y][1]=64;
			}
			if(height<=64)
			{
				terrain[x][y][0]=(height*4.0);
				terrain[x][y][1]=32;
			}

			terrain[x][y][2]=water_height;
			terrain[x][y][3]=0x00;
			terrain[x][y][4]=0x00;
			terrain[x][y][5]=0x00;
			terrain[x][y][6]=0x00;
			terrain[x][y][7]=0xff;
			terrain[x][y][8]=0xff;
			terrain[x][y][9]=0xff;
			terrain[x][y][10]=0xff;
			terrain[x][y][11]=terrain[x][y][0];
			terrain[x][y][12]=terrain[x][y][1];
		}
	}

	for (int x=255;x>=0;x--)
	{
		for (int y=0;y<256;y++)
		{			
			for(int z=0;z<13;z++)
			{
				char ch=terrain[y][x][z];
				out.write(&ch,1);
			}
		}
	}

	out.close();
}

// static
void FloaterSimExport::onClickClose(void* data)
{
	sInstance->close();
}

void FloaterSimExport::startRemoteExport()
{
	gIdleCallbacks.deleteFunction(statsupdate);

	JCExportTracker::export_tga = sInstance->getChild<LLCheckBoxCtrl>("export_textures_tga")->get();
	JCExportTracker::export_j2c = sInstance->getChild<LLCheckBoxCtrl>("export_textures")->get();
	sInstance->mExportTrees=sInstance->getChild<LLCheckBoxCtrl>("export_trees")->get();
	JCExportTracker::export_inventory = sInstance->getChild<LLCheckBoxCtrl>("export_contents")->get();
	JCExportTracker::export_properties = sInstance->getChild<LLCheckBoxCtrl>("export_properties")->get();

	LLParcelSelectionHandle mParcel = LLViewerParcelMgr::getInstance()->selectParcelAt(gAgent.getPositionGlobal());

	int obj_count=gObjectList.getNumObjects();

	LLDynamicArray<LLViewerObject *> root_objects;

	for(int x=0;x<obj_count;x++)
	{
		LLViewerObject * obj=gObjectList.getObject(x);
		if(obj==NULL)
			continue;

		if(!obj->isRoot())
			continue;

		LLViewerRegion *reg=obj->getRegion();
		if(reg!=NULL) //does the object exist in a region
		{
			U64 regid=reg->getHandle();
			if(regid==sInstance->mRegionId) // is it in the export region
			{			
				if(obj->getPCode()==LL_PCODE_VOLUME) // is it a volume
				{						
					if(!obj->isAttachment()) // is it not an attchment
					{
						root_objects.put(obj); //grab it then!
					}
				}				
				else if(sInstance->mExportTrees && obj->getPCode()==LL_PCODE_LEGACY_GRASS)
				{						
					root_objects.put(obj); //grab it then!
				}
				else if(sInstance->mExportTrees && obj->getPCode()==LL_PCODE_LEGACY_TREE)
				{						
					root_objects.put(obj); //grab it then!
				}
			}
		}
	}

	int total=0;
	total=sInstance->mRootPrims+sInstance->mChildPrims;

	if(sInstance->mExportTrees)
		total+=sInstance->mPlants;

	ExportTrackerFloater::RemoteStart(root_objects,total, sInstance->target_file);

	LLVLComposition *compp = sInstance->mRegionp->getComposition();

	//write out some stats
	std::string statdest = gDirUtilp->getDirName(sInstance->target_file) + "\\sim info.txt";

	// Create a file stream and write to it
	llofstream out(statdest,ios_base::out);
	if (!out.good())
	{
		llwarns << "Unable to open for output." << llendl;
	}
	else
	{
		int totalprims = LLViewerStats::getInstance()->mSimObjects.getCurrent();
		out << "Region: " << sInstance->mRegionp->getName() << "\n";
		out << llformat("Simulator prims: %d", totalprims) << "\n";
		out << llformat("Cached prims: %d", sInstance->mRootPrims+sInstance->mChildPrims) << "\n";
		out << llformat("Cached plants: %d",sInstance->mPlants) << "\n";
		out << llformat("Missing prims: %d", totalprims -(sInstance->mRootPrims+sInstance->mChildPrims+sInstance->mPlants)) << "\n";
		out << "\n";
		out << "Ground Textures:\n";

		out << "SW low: " << compp->getStartHeight(LLVLComposition::SOUTHWEST)
			 << " high: " << compp->getHeightRange(LLVLComposition::SOUTHWEST) << "\n";
		out << "NW low: " << compp->getStartHeight(LLVLComposition::NORTHWEST)
			 << " high: " << compp->getHeightRange(LLVLComposition::NORTHWEST) << "\n";
		out << "SE low: " << compp->getStartHeight(LLVLComposition::SOUTHEAST)
			 << " high: " << compp->getHeightRange(LLVLComposition::SOUTHEAST) << "\n";
		out << "NE low: " << compp->getStartHeight(LLVLComposition::NORTHEAST)
			 << " high: " << compp->getHeightRange(LLVLComposition::NORTHEAST) << "\n";

		out.close();
	}

	//Save terrain textures.
	for(S32 i = 0; i < 4; ++i)
	{
		LLUUID tmp_id(compp->getDetailTextureID(i));

		JCAssetInfo* info = new JCAssetInfo;
		info->path = gDirUtilp->getDirName(sInstance->target_file) + "\\" + llformat("terrain_%d", i);
		info->name = llformat("texture_detail_%d", i);

		LLViewerImage* img = gImageList.getImage(tmp_id, MIPMAP_TRUE, FALSE);
		if(img->getDiscardLevel()==0)
		{
			//llinfos << "Already have texture " << asset_id.asString() << " in memory, attemping GL readback" << llendl;
			FloaterSimExport::onFileLoadedForSave(true,img,NULL,NULL,0,true,info);
		}
		else
		{
			img->setBoostLevel(LLViewerImageBoostLevel::BOOST_MAX_LEVEL);
			img->forceToSaveRawImage(0); //this is required for us to receive the full res image.
			//img->setAdditionalDecodePriority(1.0f) ;
			img->setLoadedCallback( FloaterSimExport::onFileLoadedForSave, 
				0, TRUE, FALSE, info );
			//llinfos << "Requesting texture " << asset_id.asString() << llendl;
		}
	}

	//Save terrain heightmap.
	F32 water_height=sInstance->mRegionp->getWaterHeight();

	U16 terrain[256][256][13];
	
	std::string dest = gDirUtilp->getDirName(sInstance->target_file) + "\\" + sInstance->mRegionp->getName() + ".raw";
	llofstream out2(dest,llofstream::binary);

	for (int x=255;x>=0;x--)
	{
		for (int y=0;y<256;y++)
		{			
			F32 height=sInstance->mRegionp->getLandHeightRegion(LLVector3((float)x,(float)y,0.0));
			
			if(height>256)
			{
				terrain[x][y][0]=(height/2.0);
				terrain[x][y][1]=256;
			}
			if(height>128 && height <=256)
			{
				terrain[x][y][0]=height;
				terrain[x][y][1]=128;
			}

			if(height>64 && height <=128)
			{
				terrain[x][y][0]=(height*2.0);
				terrain[x][y][1]=64;
			}
			if(height<=64)
			{
				terrain[x][y][0]=(height*4.0);
				terrain[x][y][1]=32;
			}

			terrain[x][y][2]=water_height;
			terrain[x][y][3]=0x00;
			terrain[x][y][4]=0x00;
			terrain[x][y][5]=0x00;
			terrain[x][y][6]=0x00;
			terrain[x][y][7]=0xff;
			terrain[x][y][8]=0xff;
			terrain[x][y][9]=0xff;
			terrain[x][y][10]=0xff;
			terrain[x][y][11]=terrain[x][y][0];
			terrain[x][y][12]=terrain[x][y][1];
		}
	}

	for (int x=255;x>=0;x--)
	{
		for (int y=0;y<256;y++)
		{			
			for(int z=0;z<13;z++)
			{
				char ch=terrain[y][x][z];
				out2.write(&ch,1);
			}
		}
	}

	out2.close();
}

void FloaterSimExport::statsupdate(void *userdata)
{
	//This is suboptimal as we work out the list everytime
	// and recount, but it makes things easier later on

	LLParcelSelectionHandle mParcel = LLViewerParcelMgr::getInstance()->selectParcelAt(gAgent.getPositionGlobal());

	int count=LLViewerStats::getInstance()->mSimObjects.getCurrent();

	int obj_count=gObjectList.getNumObjects();

	sInstance->mRootPrims=0;
	sInstance->mChildPrims=0;
	sInstance->mPlants=0;

	for(int x=0;x<obj_count;x++)
	{
		LLViewerObject * obj=gObjectList.getObject(x);
		if(obj==NULL)
			continue;

		if(!obj->isRoot())
			continue;

		LLViewerRegion *reg=obj->getRegion();
		if(reg!=NULL)
		{
			U64 regid=reg->getHandle();
			if(regid==sInstance->mRegionId)
			{			
				if(obj->getPCode()==LL_PCODE_VOLUME)
				{						
					if(!obj->isAttachment())
					{
						sInstance->mRootPrims++;						
						LLViewerObject::const_child_list_t clist =obj->getChildren();
						sInstance->mChildPrims+=clist.size();
					}
				}

				else if(obj->getPCode()==LL_PCODE_LEGACY_GRASS)
				{						
					sInstance->mPlants++;
				}
				else if(obj->getPCode()==LL_PCODE_LEGACY_TREE)
				{						
					sInstance->mPlants++;
				}
			}
		}
	}

	LLUICtrl * ctrl=sInstance->getChild<LLUICtrl>("simprims");	
	ctrl->setValue(LLSD("Text")=llformat("Simulator prims:  %d", count));
	
	ctrl=sInstance->getChild<LLUICtrl>("volumes");	
	ctrl->setValue(LLSD("Text")=llformat("Cached prims:  %d", sInstance->mRootPrims+sInstance->mChildPrims));

	ctrl=sInstance->getChild<LLUICtrl>("plants");	
	ctrl->setValue(LLSD("Text")=llformat("Cached plants:  %d",sInstance->mPlants));

	ctrl=sInstance->getChild<LLUICtrl>("missing");	
	ctrl->setValue(LLSD("Text")=llformat("Missing prims:  %d", count-(sInstance->mRootPrims+sInstance->mChildPrims+sInstance->mPlants)));

	LLVLComposition *compp = sInstance->mRegionp->getComposition();

	if(!compp->getParamsReady() || compp->getDetailTexture(LLVLComposition::SOUTHWEST)==NULL ||
	compp->getDetailTexture(LLVLComposition::SOUTHEAST)==NULL ||
	compp->getDetailTexture(LLVLComposition::NORTHWEST)==NULL ||
	compp->getDetailTexture(LLVLComposition::NORTHEAST)==NULL)
	{
		sInstance->childSetEnabled("saveterrain",false);
	}
	else
	{
		sInstance->childSetEnabled("saveterrain",true);
	}
		
	time_t tnow=time(NULL);

	LLSpinCtrl* mCtrlObject = sInstance->getChild<LLSpinCtrl>("object_threshold");
	LLSpinCtrl* mCtrlTime = sInstance->getChild<LLSpinCtrl>("time_threshold");

	//any new objects since we last ran?
	if ((sInstance->mRootPrims+sInstance->mChildPrims) != sInstance->mLastCount)
	{
		sInstance->mLastCount = sInstance->mRootPrims+sInstance->mChildPrims;
		sInstance->threshold_time=time(NULL);
	}

	ctrl=sInstance->getChild<LLUICtrl>("timelast");	
	ctrl->setValue(LLSD("Text")=llformat("%d seconds since last new object received", tnow - sInstance->threshold_time));



	//has enough time passed since we last received objects to start an export?
	if(tnow > ((mCtrlTime->get() * 60) + sInstance->threshold_time))
	{
		//do we have enough objects to start?
		if ((count-(sInstance->mRootPrims+sInstance->mChildPrims+sInstance->mPlants)) < mCtrlObject->get())
		{
			sInstance->startRemoteExport();
		}
	}

	//randomly move us around
	if(sInstance->getChild<LLCheckBoxCtrl>("move")->get() && ((sInstance->move_time+5)< tnow))
	{
		sInstance->move_time=time(NULL);

		F32 height=sInstance->mRegionp->getLandHeightRegion(LLVector3(cur_x * 16.f, cur_y * 16.f,0.0));
		LLVector3 cur_pos(cur_x * 16.f, cur_y * 16.f, cur_z * 16.f + height);

		LLVector3d pos_global;
		pos_global.setVec( cur_pos );
		pos_global += sInstance->mRegionp->getOriginGlobal();

		//tp there
		gAgent.teleportViaLocation( pos_global );

		if (cur_x < 15)
		{
			cur_x++;
		}
		else if (cur_x == 15)
		{
			if (cur_y < 15)
			{
				cur_x = 1;
				cur_y++;
			}
			else if (cur_y == 15)
			{
				cur_y = 1;
				cur_z++;

			}

		}

		height=sInstance->mRegionp->getLandHeightRegion(LLVector3(cur_x * 16.f, cur_y * 16.f,0.0));
		LLVector3 new_pos(cur_x * 16.f, cur_y * 16.f, cur_z * 16.f + height);
		pos_global.setVec( new_pos );
		pos_global += sInstance->mRegionp->getOriginGlobal();

		//fly to the position of a known object via autopilot
		std::vector<std::string> strings;
		std::string val;
		val = llformat("%g", pos_global.mdV[VX]);
		strings.push_back(val);
		val = llformat("%g", pos_global.mdV[VY]);
		strings.push_back(val);
		val = llformat("%g", pos_global.mdV[VZ]);
		strings.push_back(val);
		send_generic_message("autopilot", strings);


		ctrl=sInstance->getChild<LLUICtrl>("curpos");
		ctrl->setValue("moving to " + llformat("%.1f", cur_pos.mV[VX]) + "," + llformat("%.1f", cur_pos.mV[VY]) + "," + llformat("%.1f", cur_pos.mV[VZ]));

		//old random movement code
		/*
		LLSpinCtrl* mCtrlPosX = sInstance->getChild<LLSpinCtrl>("altitude");

		//get random pos
		LLVector3 pos_local( 32.f + ll_frand(192.f), 32.f + ll_frand(192.f), ll_frand(mCtrlPosX->get()) );
		LLVector3d pos_global;
		pos_global.setVec( pos_local );
		pos_global += sInstance->mRegionp->getOriginGlobal();

		//tp there
		gAgent.teleportViaLocation( pos_global );

		int tmp = ll_rand(2);
		LLViewerObject * obj;

		if (tmp == 1)
		{
			//sometimes we fly to a random known object
			obj=gObjectList.getObject(ll_rand(obj_count - 1));
		}
		else
		{
			//other times we fly to the last discovered object
			obj=gObjectList.getObject(obj_count - 1);
		}

		if (!obj)
			return;
		pos_local = obj->getPosition();
		F32 temp = pos_local.mV[VZ];
		pos_local.clamp(32.f, 224.f);
		pos_local.mV[VZ] = temp;
		pos_global = sInstance->mRegionp->getPosGlobalFromRegion(pos_local);

		//fly to the position of a known object via autopilot
		std::vector<std::string> strings;
		std::string val;
		val = llformat("%g", pos_global.mdV[VX]);
		strings.push_back(val);
		val = llformat("%g", pos_global.mdV[VY]);
		strings.push_back(val);
		val = llformat("%g", pos_global.mdV[VZ]);
		strings.push_back(val);
		send_generic_message("autopilot", strings);

		// Snap camera back to behind avatar
		//gAgent.setFocusOnAvatar(TRUE, ANIMATE);
		gAgent.resetView(TRUE, TRUE);
		*/
	}
}

void ExportTrackerFloater::RemoteStart(	LLDynamicArray<LLViewerObject*> objectArray,int primcount, std::string destination_file)
{
	ExportTrackerFloater::getInstance()->show();

	//NOT INITING, EH? YOU'D BETTER!
	JCExportTracker::init();
	
	// For a remote start disable the options, they can't be changed anyway as the export
	// as alrady started
	ExportTrackerFloater::getInstance()->childSetEnabled("export_tga",false);
	ExportTrackerFloater::getInstance()->childSetEnabled("export_j2c",false);
	ExportTrackerFloater::getInstance()->childSetEnabled("export_properties",false);
	ExportTrackerFloater::getInstance()->childSetEnabled("export_contents",false);

	ExportTrackerFloater::mObjectSelection = objectArray;
	JCExportTracker::destination = destination_file;
	JCExportTracker::mTotalLinksets = mObjectSelection.count();
	JCExportTracker::mTotalObjects = primcount;
	JCExportTracker::selection_size = LLVector3(256,256,256);
	JCExportTracker::selection_center = LLVector3(128,128,128);
	JCExportTracker::serialize(mObjectSelection);
}

void FloaterSimExport::onFileLoadedForSave(BOOL success, 
											LLViewerImage *src_vi,
											LLImageRaw* src, 
											LLImageRaw* aux_src, 
											S32 discard_level,
											BOOL final,
											void* userdata)
{
	JCAssetInfo* info = (JCAssetInfo*)userdata;
	if(final)
	{
		if( success )
		{ /*
			LLPointer<LLImageJ2C> image_j2c = new LLImageJ2C();
			if(!image_j2c->encode(src,0.0))
			{
				//errorencode
				llinfos << "Failed to encode " << info->path << llendl;
			}else if(!image_j2c->save( info->path ))
			{
				llinfos << "Failed to write " << info->path << llendl;
				//errorwrite
			}else
			{
				ExportTrackerFloater::mTexturesExported++;
				llinfos << "Saved texture " << info->path << llendl;
				//success
			} */

			//RC
			//If we have a NULL raw image, then read one back from the GL buffer
			bool we_created_raw=false;
			if(src==NULL)
			{
				src = new LLImageRaw();
				we_created_raw=true;

				if(!src_vi->readBackRaw(0,src,false))
				{
					cmdline_printchat("Failed to readback texture");
					src->deleteData(); //check me, is this valid?
					delete info;
					return;
				}
			}
			
			LLPointer<LLImageTGA> image_tga = new LLImageTGA;

			if( !image_tga->encode( src ) )
			{
				llinfos << "Failed to encode " << info->path << llendl;
			}
			else if( !image_tga->save( info->path + ".tga" ) )
			{
				llinfos << "Failed to write " << info->path << llendl;
			}

			LLPointer<LLImageJ2C> image_j2c = new LLImageJ2C();
			if(!image_j2c->encode(src,0.0))
			{
				//errorencode
				llinfos << "Failed to encode " << info->path << llendl;
			}else if(!image_j2c->save( info->path+".j2c" ))
			{
				llinfos << "Failed to write " << info->path << llendl;
				//errorwrite
			}else
			{
				//llinfos << "Saved texture " << info->path << llendl;
				//success
			}
		
			//RC
			//meh if we did a GL readback we created the raw image
			// so we better delete, but the destructor is private
			// so this needs checking for a memory leak that this is correct
			if(we_created_raw)
				src->deleteData();

		}
		delete info;
	}

}