#pragma once

#include <SoyEvent.h>
#include <SoyMath.h>
#include <SoyPixels.h>

//	opengl stuff
#if defined(TARGET_ANDROID)

#define OPENGL_ES_3	//	need 3 for FBO's
//#define GL_NONE				GL_NO_ERROR	//	declared in GLES3

#if defined(OPENGL_ES_3)
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES/glext.h>	//	need for EOS
#endif


#if defined(OPENGL_ES_2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES/glext.h>	//	need for EOS
#endif

#endif

#if defined(TARGET_OSX)
#include <Opengl/gl3.h>
#include <Opengl/gl3ext.h>

#endif

#define GL_ASSET_INVALID	0
#define GL_UNIFORM_INVALID	-1



namespace Opengl
{
	class TFboMeta;
	class TUniform;
	class TAsset;
	class GlProgram;
	class TShaderState;
	class TTexture;
	class TFbo;
	class TGeoQuad;
	class TShaderEosBlit;
	class TGeometry;
	class TGeometryVertex;
	class TGeometryVertexElement;
	
	
	// It probably isn't worth keeping these shared here, each user
	// should just duplicate them.
	extern const char * externalFragmentShaderSource;
	extern const char * textureFragmentShaderSource;
	extern const char * identityVertexShaderSource;
	extern const char * untexturedFragmentShaderSource;
	
	extern const char * VertexColorVertexShaderSrc;
	extern const char * VertexColorSkinned1VertexShaderSrc;
	extern const char * VertexColorFragmentShaderSrc;
	
	extern const char * SingleTextureVertexShaderSrc;
	extern const char * SingleTextureSkinned1VertexShaderSrc;
	extern const char * SingleTextureFragmentShaderSrc;
	
	extern const char * LightMappedVertexShaderSrc;
	extern const char * LightMappedSkinned1VertexShaderSrc;
	extern const char * LightMappedFragmentShaderSrc;
	
	extern const char * ReflectionMappedVertexShaderSrc;
	extern const char * ReflectionMappedSkinned1VertexShaderSrc;
	extern const char * ReflectionMappedFragmentShaderSrc;
	
	// Will abort() after logging an error if either compiles or the link status
	// fails, but not if uniforms are missing.
	GlProgram	BuildProgram(std::string vertexSrc,std::string fragmentSrc,const TGeometryVertex& GeometryVertex);
	

	
	struct VertexAttribs
	{
		std::vector<vec3f> position;
		std::vector<vec3f> normal;
		std::vector<vec3f> tangent;
		std::vector<vec3f> binormal;
		std::vector<vec4f> color;
		std::vector<vec2f> uv0;
		std::vector<vec2f> uv1;
		//Array< Vector4i > jointIndices;
		//Array< Vector4f > jointWeights;
	};
	
	typedef unsigned short TriangleIndex;
	//typedef unsigned int TriangleIndex;
	
	static const int MAX_GEOMETRY_VERTICES	= 1 << ( sizeof( TriangleIndex ) * 8 );
	static const int MAX_GEOMETRY_INDICES	= 1024 * 1024 * 3;
	
	TGeometry BuildTesselatedQuad( const int horizontal, const int vertical );
	TGeometry	CreateGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<GLshort>&& Indexes,const TGeometryVertex& Vertex);

#define Opengl_IsInitialised()	Opengl::IsInitialised(__func__,true)
#define Opengl_IsOkay()			Opengl::IsOkay(__func__)

	bool	IsInitialised(const std::string& Context,bool ThrowException);	//	throws exception
	bool	IsOkay(const std::string& Context);	//	throws exception
	bool	IsSupported(const char* ExtensionName);
	
	SoyPixelsFormat::Type	GetPixelFormat(GLuint glFormat);
	GLuint					GetPixelFormat(SoyPixelsFormat::Type Format);


	//	helpers
	void	ClearColour(Soy::TRgb Colour);
	void	ClearDepth();
	void	SetViewport(Soy::Rectf Viewport);
};





class Opengl::TFboMeta
{
public:
	TFboMeta()	{}
	TFboMeta(const std::string& Name,size_t Width,size_t Height) :
		mName	( Name ),
		mSize	( Width, Height )
	{
	}
	
	std::string		mName;
	vec2x<size_t>	mSize;
};




class Opengl::TUniform
{
public:
	TUniform() :
		mIndex		( GL_UNIFORM_INVALID ),
		mType		( GL_ASSET_INVALID ),
		mArraySize	( 0 )
	{
	}
	
	bool		IsValid() const	{	return mIndex != GL_UNIFORM_INVALID;	}
	bool		operator==(const std::string& Name) const	{	return mName == Name;	}
	
	std::string	mName;
	GLenum		mType;
	GLsizei		mArraySize;	//	for arrays of mType
	GLint		mIndex;		//	attrib index
};
namespace Opengl
{
std::ostream& operator<<(std::ostream &out,const Opengl::TUniform& in);
}


class Opengl::TGeometryVertexElement : public TUniform
{
public:
	size_t		mElementDataSize;
	bool		mNormalised;
};

class Opengl::TGeometryVertex
{
public:
	size_t							GetDataSize() const;	//	size of vertex struct
	size_t							GetOffset(size_t ElementIndex) const;
	size_t							GetStride(size_t ElementIndex) const;

	void							EnableAttribs(bool Enable=true) const;
	void							DisableAttribs() const				{	EnableAttribs(false);	}
	
public:
	Array<TGeometryVertexElement>	mElements;
};


//	gr: for now, POD, no virtuals...
class Opengl::TAsset
{
public:
	TAsset() :
	mName	( GL_ASSET_INVALID )
	{
	}
	explicit TAsset(GLuint Name) :
	mName	( Name )
	{
	}
	
	bool	IsValid() const		{	return mName != GL_ASSET_INVALID;	}
	
	GLuint	mName;
};

//	clever class which does the binding, auto texture mapping, and unbinding
//	why? so we can use const GlPrograms and share them across threads
class Opengl::TShaderState
{
public:
	TShaderState(const GlProgram& Shader);
	~TShaderState();
	
	void	SetUniform(const char* Name,const vec4f& v);
	void	SetUniform(const char* Name,const TTexture& Texture);	//	special case which tracks how many textures are bound

	void	BindTexture(size_t TextureIndex,TTexture Texture);	//	use to unbind too
	
public:
	const GlProgram&	mShader;
	size_t				mTextureBindCount;
};

class Opengl::GlProgram
{
public:
	bool			IsValid() const;
	void			Destroy();
	
	TShaderState	Bind();	//	let this go out of scope to unbind
	TUniform		GetUniform(const char* Name) const
	{
		auto* Uniform = mUniforms.Find( Name );
		return Uniform ? *Uniform : TUniform();
	}
	TUniform		GetAttribute(const char* Name) const
	{
		auto* Uniform = mAttributes.Find( Name );
		return Uniform ? *Uniform : TUniform();
	}

public:
	TAsset			program;
	TAsset			vertexShader;
	TAsset			fragmentShader;
	
	Array<TUniform>	mUniforms;
	Array<TUniform>	mAttributes;
};



class Opengl::TGeometry
{
public:
	TGeometry() :
	vertexBuffer( GL_ASSET_INVALID ),
	indexBuffer( GL_ASSET_INVALID ),
	vertexArrayObject( GL_ASSET_INVALID ),
	vertexCount( GL_ASSET_INVALID ),
	indexCount( GL_ASSET_INVALID ),
	mIndexType(GL_ASSET_INVALID)
	{
	}
	/*
	TGeometry( const VertexAttribs & attribs, const std::vector< TriangleIndex > & indices ) :
	vertexBuffer( GL_ASSET_INVALID ),
	indexBuffer( GL_ASSET_INVALID ),
	vertexArrayObject( GL_ASSET_INVALID ),
	vertexCount( GL_ASSET_INVALID ),
	indexCount( GL_ASSET_INVALID )
	{
		Create( attribs, indices );
	}
	 */
	
	// Create the VAO and vertex and index buffers from arrays of data.
//	void	Create( const VertexAttribs & attribs, const std::vector< TriangleIndex > & indices );
//	void	Update( const VertexAttribs & attribs );
	
	// Assumes the correct program, uniforms, textures, etc, are all bound.
	// Leaves the VAO bound for efficiency, be careful not to inadvertently
	// modify any of the state.
	// You need to manually bind and draw if you want to use GL_LINES / GL_POINTS / GL_TRISTRIPS,
	// or only draw a subset of the indices.
	void	Draw() const;
	
	// Free the buffers and VAO, assuming that they are strictly for this geometry.
	// We could save some overhead by packing an entire model into a single buffer, but
	// it would add more coupling to the structures.
	// This is not in the destructor to allow objects of this class to be passed by value.
	void	Free();
	
	bool	IsValid() const;
	
public:
	TGeometryVertex	mVertexDescription;
	GLuint		vertexArrayObject;
	GLuint		vertexBuffer;
	GLsizei		vertexCount;
	
	GLuint		indexBuffer;
	GLsizei 	indexCount;
	GLenum		mIndexType;		//	short/int etc data stored in index buffer
};


class Opengl::TTexture
{
public:
	//	invalid
	explicit TTexture() :
		mAutoRelease	( false )
	{
	}
	~TTexture()
	{
		if ( mAutoRelease )
			Delete();
	}
	
	
	
	TTexture(const TTexture& Weak) :
		mAutoRelease	( false ),
		mTexture		( Weak.mTexture ),
		mMeta			( Weak.mMeta )
	{
	}
	
	TTexture& operator=(const TTexture& Weak)
	{
		if ( this != &Weak )
		{
			mAutoRelease = false;
			mTexture = Weak.mTexture;
			mMeta = Weak.mMeta;
		}
		return *this;
	}
	
	//	alloc
	explicit TTexture(SoyPixelsMetaFull Meta);
	
	//	reference
	TTexture(void* TexturePtr,const SoyPixelsMetaFull& Meta) :
		mMeta			( Meta ),
		mAutoRelease	( false ),
#if defined(TARGET_ANDROID)
		mTexture		( reinterpret_cast<GLuint>(TexturePtr) )
#elif defined(TARGET_OSX)
		mTexture		( static_cast<GLuint>(reinterpret_cast<GLuint64>(TexturePtr)) )
#endif
	{
	}

	
	bool				Bind();
	static void			Unbind();
	bool				IsValid() const;
	void				Delete();
	void				Copy(const SoyPixels& Pixels,bool Blocking,bool Stretch);
	
public:
	bool				mAutoRelease;
	TAsset				mTexture;
	SoyPixelsMetaFull	mMeta;
};

class Opengl::TFbo
{
public:
	TFbo(TTexture Texture);
	~TFbo();
	
	bool		IsValid() const	{	return mFbo.IsValid() && mTarget.IsValid();	}
	bool		Bind();
	void		Unbind();
	
	TAsset		mFbo;
	TTexture	mTarget;
};
/*
class Opengl::TGeoQuad
{
public:
	TGeoQuad();
	~TGeoQuad();
	
	bool		IsValid() const	{	return mGeometry.IsValid();	}
	
	TGeometry	mGeometry;
	
	bool		Render();
};


class Opengl::TShaderEosBlit
{
public:
	TShaderEosBlit();
	~TShaderEosBlit();
	
	bool		IsValid() const	{	return mProgram.IsValid();	}
	
	Opengl::GlProgram	mProgram;
};
*/

