#pragma once


//	see ofConstants
#define WIN32_LEAN_AND_MEAN

#define NOMINMAX
//http://stackoverflow.com/questions/1904635/warning-c4003-and-errors-c2589-and-c2059-on-x-stdnumeric-limitsintmax

#include <windows.h>
#include <process.h>
#include <vector>
#include <mmsystem.h>
#include <direct.h>
#pragma comment(lib,"winmm.lib")


#define __func__	__FUNCTION__
#define __thread	__declspec( thread )
// Attribute to make function be exported from a plugin
#define __export	extern "C" __declspec(dllexport)

#elif defined(TARGET_OSX)

#include <sys/time.h>
#include <math.h>



typedef signed __int32		int32;
typedef unsigned __int32	uint32;
typedef signed __int64		int64;
typedef unsigned __int64	uint64;
typedef SSIZE_T				ssize_t;




//	this is for directx really but so many things have a Release() it can probably move
template<typename TYPE>
class AutoReleasePtr
{
public:
	AutoReleasePtr() :
	mObject	( nullptr )
	{
	}
	AutoReleasePtr(TYPE* Object,bool AddRef) :
	mObject	( nullptr )
	{
		Set( Object, AddRef );
	}
	AutoReleasePtr(const TAutoRelease& that) :
	mObject	( nullptr )
	{
		Set( that.mObject, true );
	}
	~AutoReleasePtr()		{	Release();	}
	
	TYPE*		operator ->()	{	return mObject;	}
	operator	TYPE*()			{	return mObject;	}
	
	void		Set(TYPE* Object,bool AddRef)
	{
		Release();
		mObject = Object;
		if ( AddRef && mObject )
			mObject->AddRef();
	}
	
	void		Release()
	{
		if ( mObject )
		{
			mObject->Release();
			mObject = nullptr;
		}
	}
	
public:
	TYPE*	mObject;
};