
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
	void show();
	
	//Export button
	static void onClickExport(void* data);

	static void onClickSaveTerrain(void * data);
	
	//Close button
	static void onClickClose(void* data);

	//start export function
	static void startRemoteExport();

	static void statsupdate(void *userdata);

	static void			onFileLoadedForSave( 
		BOOL success,
		LLViewerImage *src_vi,
		LLImageRaw* src, 
		LLImageRaw* aux_src,
		S32 discard_level, 
		BOOL final,
		void* userdata );

	U64  getRegion() { return mRegionId;};
	bool isRunning() { return mRunning; };

	//meh, this should be static
	//but i want to know its state in viewermessage
	static FloaterSimExport* sInstance;
	time_t	move_time;
	time_t	threshold_time;
	std::string target_file;


private:

	//Static singleton stuff
	FloaterSimExport();	

	bool mExportTrees;
	bool mRunning;
	int mRootPrims;
	int mChildPrims;
	int mPlants;
	int mLastCount;

	LLViewerRegion *mRegionp;

	// are we active flag
	U64 mRegionId;

};
