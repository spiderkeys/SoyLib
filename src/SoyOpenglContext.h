//	high level opengl stuff
#pragma once

#include "SoyOpengl.h"
#include "SoyEnum.h"
#include "SoyThread.h"


typedef struct _CGLContextObject       *CGLContextObj;


namespace OpenglExtensions
{
	enum Type
	{
		Invalid,
		AppleClientStorage,
		VertexArrayObjects,
		VertexBuffers,
		DrawBuffers,
		GenFrameBuffers,
		BindFrameBuffer,
		ImageExternal,
		ImageExternalSS3,
	};
	
	DECLARE_SOYENUM( OpenglExtensions );
}

namespace Opengl
{
	class TContext;			//	opengl context abstraction - contexts are not thread safe, but can share objects
	class TRenderTarget;	//	PBO/FBO, but sometimes with additional stuff (buffer flip etc depending on source)

	class TRenderTargetFbo;
	
	
	//	safe-as-possible deffered delete of an opengl object (pass in your members here!)
	template<typename TYPE>
	void	DeferDelete(std::shared_ptr<Opengl::TContext> Context,std::shared_ptr<TYPE>& pObject);

	
	//	extension bindings
	extern std::function<void(GLuint)>					BindVertexArray;
	extern std::function<void(GLsizei,GLuint*)>			GenVertexArrays;
	extern std::function<void(GLsizei,const GLuint*)>	DeleteVertexArrays;
	extern std::function<GLboolean(GLuint)>				IsVertexArray;
	
	extern std::function<void(GLsizei,GLuint*)>			GenFrameBuffers;
	extern std::function<void(GLenum,GLuint)>			BindFrameBuffer;
	extern std::function<void(GLsizei,const GLenum *)>	DrawBuffers;
};



class Opengl::TContext : public PopWorker::TJobQueue, public PopWorker::TContext
{
public:
	TContext();
	virtual ~TContext()				{}

	void			Init();
	void			Iteration()			{	Flush(*this);	}
	virtual bool	Lock() override;
	virtual void	Unlock() override;
	virtual bool	IsLocked(std::thread::id Thread) override;
	virtual std::shared_ptr<Opengl::TContext>	CreateSharedContext()	{	return nullptr;	}
	bool			HasMultithreadAccess() const	{	return false;	}

	bool			IsSupported(OpenglExtensions::Type Extension)	{	return IsSupported(Extension,this);	}
	static bool		IsSupported(OpenglExtensions::Type Extension,TContext* Context);
	
	void			BindVertexArrayObjectsExtension();
	void			BindDrawBuffersExtension();
	void			BindGenFrameBuffersExtension();
	void			BindBindFrameBuffersExtension();

#if defined(TARGET_OSX)
	virtual CGLContextObj	GetPlatformContext()	{	return nullptr;	}
#endif
	
public:
	Soy::TVersion	mVersion;
	Soy::TVersion	mShaderVersion;	//	max version supported
	std::string		mDeviceName;
	
private:
	std::thread::id		mLockedThread;	//	needed in the context as it gets locked in other places than the job queue
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
	
	virtual void				Bind()=0;
	virtual void				Unbind()=0;
	virtual Soy::Rectx<size_t>	GetSize()=0;
	
	void			SetViewportNormalised(Soy::Rectf Viewport);
	
	std::string		mName;
};


class Opengl::TRenderTargetFbo : public Opengl::TRenderTarget
{
public:
	//	provide context for non-immediate construction
	TRenderTargetFbo(TFboMeta Meta,Opengl::TContext& Context,Opengl::TTexture ExistingTexture=Opengl::TTexture());
	TRenderTargetFbo(TFboMeta Meta,Opengl::TTexture ExistingTexture=Opengl::TTexture());
	explicit TRenderTargetFbo(Opengl::TTexture ExistingTexture);
	~TRenderTargetFbo()
	{
		mFbo.reset();
	}
	
	virtual void				Bind() override;
	virtual void				Unbind() override;
	virtual Soy::Rectx<size_t>	GetSize() override;
	TTexture					GetTexture();
	
public:
	bool					mGenerateMipMaps;
	std::shared_ptr<TFbo>	mFbo;
	TTexture				mTexture;
};



template<typename TYPE>
inline void Opengl::DeferDelete(std::shared_ptr<Opengl::TContext> Context,std::shared_ptr<TYPE>& pObject)
{
	if ( !pObject )
		return;
	
	if ( !Context )
	{
		std::Debug << __func__ << " Warning; missing opengl context, objects will free immediately in wrong thread" << std::endl;
		try
		{
			pObject.reset();
		}
		catch(std::exception& e)
		{
			std::Debug << __func__ << " non deffered delete of geo; " << e.what() << std::endl;
		}
		return;
	}
	
	//	push deffered delete with copy of object and release local one in this thread
	//	note: because of race conditions we capture and release the local one first
	std::shared_ptr<TYPE> LocalpObject = pObject;
	pObject.reset();
	std::shared_ptr<PopWorker::TJob> Job( new TDefferedDeleteJob<TYPE>( LocalpObject ) );
	Context->PushJob( Job );
	LocalpObject.reset();
}

