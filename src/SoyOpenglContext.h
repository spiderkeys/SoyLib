//	high level opengl stuff
#pragma once

#include "SoyOpengl.h"
#include "SoyEnum.h"
#include "SoyThread.h"


namespace OpenglExtensions
{
	enum Type
	{
		Invalid,
		AppleClientStorage,
		VertexArrayObjects,
	};
	
	DECLARE_SOYENUM( OpenglExtensions );
}

namespace Opengl
{
	class TContext;			//	opengl context abstraction - contexts are not thread safe, but can share objects
	class TRenderTarget;	//	PBO/FBO, but sometimes with additional stuff (buffer flip etc depending on source)
	class TVersion;

	class TRenderTargetFbo;
};

namespace PopWorker
{
	class TContext;
	class TJobQueue;
	class TJob;
	class TJob_Function;	//	simple job for lambda's
};


class Opengl::TVersion
{
public:
	TVersion() :
		mMajor	( 0 ),
		mMinor	( 0 )
	{
	}
	TVersion(size_t Major,size_t Minor) :
		mMajor	( Major ),
		mMinor	( Minor )
	{
	}
	explicit TVersion(const std::string& VersionStr);
	
public:
	size_t	mMajor;
	size_t	mMinor;
};

namespace Opengl
{
	inline std::ostream& operator<<(std::ostream &out,const Opengl::TVersion& in)
	{
		out << in.mMajor << '.' << in.mMinor;
		return out;
	}
}

class PopWorker::TJob
{
public:
	TJob() :
		mSemaphore	( nullptr )
	{
	}
	
	//	returns "im finished"
	//	return false to stop any more tasks being run and re-insert this in the queue
	virtual bool		Run(std::ostream& Error)=0;
	
	//	todo: event, or sempahore? or OS semaphore?
	Soy::TSemaphore*		mSemaphore;
	//SoyEvent<bool>		mOnFinished;
};



class PopWorker::TJob_Function : public TJob
{
public:
	TJob_Function(std::function<bool ()> Function) :
		mFunction	( Function )
	{
	}

	virtual bool		Run(std::ostream& Error)
	{
		return mFunction();
	}
	
	std::function<bool ()>	mFunction;
};


class PopWorker::TContext
{
public:
	virtual bool	Lock()=0;
	virtual void	Unlock()=0;
};

class PopWorker::TJobQueue
{
public:
	void			Flush(TContext& Context);
	bool			HasJobs() const			{	return !mJobs.empty();	}
	void			PushJob(std::function<bool(void)> Lambda);
	void			PushJob(std::function<bool(void)> Lambda,Soy::TSemaphore& Semaphore);
	void			PushJob(std::shared_ptr<TJob>& Job)									{	PushJobImpl( Job, nullptr );	}
	void			PushJob(std::shared_ptr<TJob>& Job,Soy::TSemaphore& Semaphore)		{	PushJobImpl( Job, &Semaphore );	}

protected:
	virtual void	PushJobImpl(std::shared_ptr<TJob>& Job,Soy::TSemaphore* Semaphore);

public:
	SoyEvent<std::shared_ptr<TJob>>		mOnJobPushed;

private:
	std::vector<std::shared_ptr<TJob>>	mJobs;		//	gr: change this to a nice soy ringbuffer
	std::recursive_mutex				mLock;
};


class Opengl::TContext : public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	TContext();
	virtual ~TContext()				{}

	void			Init();
	void			Iteration()			{	Flush(*this);	}
	virtual bool	Lock() override		{	return true;	}
	virtual void	Unlock() override	{	}
	
	bool			IsSupported(OpenglExtensions::Type Extension)	{	return IsSupported(Extension,this);	}
	static bool		IsSupported(OpenglExtensions::Type Extension,TContext* Context);
	

public:
	TVersion		mVersion;
	std::string		mDeviceName;
};


//	render/frame buffer
class Opengl::TRenderTarget
{
public:
	TRenderTarget(const std::string& Name) :
		mName	( Name )
	{
	}
	virtual ~TRenderTarget()	{}
	
	virtual bool				Bind()=0;
	virtual void				Unbind()=0;
	virtual Soy::Rectx<size_t>	GetSize()=0;
	
	std::string		mName;
};


class Opengl::TRenderTargetFbo : public Opengl::TRenderTarget
{
public:
	//	provide context for non-immediate construction
	TRenderTargetFbo(TFboMeta Meta,Opengl::TContext& Context,Opengl::TTexture ExistingTexture=Opengl::TTexture());
	TRenderTargetFbo(TFboMeta Meta,Opengl::TTexture ExistingTexture=Opengl::TTexture());
	~TRenderTargetFbo()
	{
		mFbo.reset();
	}
	
	virtual bool				Bind();
	virtual void				Unbind();
	virtual Soy::Rectx<size_t>	GetSize();
	TTexture					GetTexture();
	
	std::shared_ptr<TFbo>	mFbo;
	TTexture				mTexture;
};
