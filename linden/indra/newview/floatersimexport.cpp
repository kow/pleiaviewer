
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


FloaterSimExport* FloaterSimExport::sInstance = 0;

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
	mRegionId=gAgent.getRegion()->getHandle();
	gIdleCallbacks.addFunction(statsupdate, NULL);
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

void FloaterSimExport::startexport()
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
		gIdleCallbacks.deleteFunction(statsupdate);
	
		//JCExportTracker::export_tga = sInstance->getChild<LLCheckBoxCtrl>("export_textures")->get();
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

	ExportTrackerFloater::RemoteStart(root_objects,total);
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

	LLViewerRegion *regionp = gAgent.getRegion();
	LLVLComposition *compp = regionp->getComposition();

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
}

void ExportTrackerFloater::RemoteStart(	LLDynamicArray<LLViewerObject*> catfayse,int primcount)
{
	ExportTrackerFloater::getInstance()->show();
	objectselection=catfayse;
	total_linksets=objectselection.count();
	total_objects=primcount;
	JCExportTracker::selection_size = LLVector3(256,256,256);
	JCExportTracker::selection_center = LLVector3(128,128,128);
	JCExportTracker::serialize(objectselection);
}
