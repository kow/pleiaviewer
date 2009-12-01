
#include "llviewerinventory.h"

class FloaterSimExport
: public LLFloater
{
	public:

	//close me
	static void close();

	//Static accessor
	static FloaterSimExport* getInstance();

	virtual ~FloaterSimExport();
	
	//Floater stuff
	virtual void draw();
	virtual void onClose( bool app_quitting );
	
	//Export entry point
	void startexport();
	
	//Export button
	static void onClickExport(void* data);

	static void onClickSaveTerrain(void * data);
	
	//Close button
	static void onClickClose(void* data);

	static void statsupdate(void *userdata);
	
	U64  getRegion() { return mRegionId;};
	bool isRunning() { return mRunning; };

	//meh, this should be static
	//but i want to know its state in viewermessage
	static FloaterSimExport* sInstance;

private:

	//Static singleton stuff
	FloaterSimExport();	

	bool mExportTrees;
	bool mExportTextures;
	bool mRunning;
	int mRootPrims;
	int mChildPrims;
	int mPlants;

	// are we active flag
	U64 mRegionId;

};
