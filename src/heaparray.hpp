#pragma once


#include <cassert>
#include <cstddef>
#include "SoyTypes.h"	//	gr: not sure why I have to include this, when it's included earlier in Soy.hpp...
#include "array.hpp"
#include "memheap.hpp"
#include "SoyDebug.h"
#include "SoyArray.h"



//	gr: this is exactly the same as an Array type, but uses a prmem::Heap to allocate from.
//		As long as it doesn't have any repurcussions, the original Array type should
//		be able to be replaced by this, but with a default Heap
//	gr: This should always allocate from a heap now, by default the "pr-global heap"
//		with the exception of heaparrays that are allocated globally before the prcore::Heap
template <typename T,prmem::Heap& HEAP=prcore::Heap>
class Array // : public ArrayInterface<T>
{
public:
	typedef T TYPE;	//	in case you ever need to get to T in a template function/class, you can use ARRAYPARAM::TYPE (sometimes need typename ARRAYPARAM::TYPE)

public:

	Array()
	: mdata(nullptr),mmaxsize(0),moffset(0),mHeap(nullptr)
	{
		SetHeap( HEAP );
	}

	explicit Array(prmem::Heap& Heap)
	: mdata(nullptr),mmaxsize(0),moffset(0),mHeap(nullptr)
	{
		SetHeap( Heap );
	}

	Array(const size_t size)
	: mdata(nullptr),mmaxsize(0),moffset(0),mHeap(nullptr)
	{
		SetHeap( HEAP );
		SetSize(size);
	}
	
	Array(const int size)
	: mdata(nullptr),mmaxsize(0),moffset(0),mHeap(nullptr)
	{
		SetHeap( HEAP );
		SetSize(size);
	}
	
	Array(unsigned int size)
	: mdata(nullptr),mmaxsize(0),moffset(0),mHeap(nullptr)
	{
		SetHeap( HEAP );
		SetSize(size);
	}
	
	//	need an explicit constructor of self-type
	Array(const Array& v)
	: mdata(nullptr),mmaxsize(0),moffset(0),mHeap(nullptr)
	{
		SetHeap( HEAP );
		Copy( v );
	}

	//	explicit to avoid accidental implicit array-conversions (eg. when passing BufferArray to Array param)
	template<typename ARRAYTYPE>
	explicit Array(const ARRAYTYPE& v)
	: mdata(nullptr),mmaxsize(0),moffset(0),mHeap(nullptr)
	{
		SetHeap( HEAP );
		Copy( v );
	}

	//	construct from a C-array like int Hello[2]={0,1}; this automatically sets the size
	template<size_t CARRAYSIZE>
	explicit Array(const T(&CArray)[CARRAYSIZE])
	: mdata(nullptr),mmaxsize(0),moffset(0),mHeap(nullptr)
	{
		PushBackArray( CArray );
	}

	~Array()
	{
		if ( mdata )
		{
			auto& Heap = GetHeap();
			Heap.FreeArray( mdata, mmaxsize );
		}
	}

	template<typename ARRAYTYPE>
	void operator = (const ARRAYTYPE& v)
	{
		Copy(v);
	}

	//	need an explicit = operator for self-type
	void operator = (const Array& v)
	{
		Copy(v);
	}

	template<typename ARRAYTYPE>
	bool Copy(const ARRAYTYPE& v)
	{
		return GetArrayBridge(*this).Copy(v);
	}

	T& GetAt(size_t index)
	{
		SoyArray::CheckBounds( index, *this );
		return mdata[index];
	}

	const T& GetAtConst(size_t index) const
	{
		SoyArray::CheckBounds( index, *this );
		return mdata[index];
	}
	T&			operator [] (size_t index)				{	return GetAt(index);	}
	const T&	operator [] (size_t index) const		{	return GetAtConst(index);	}

	T&			GetBack()				{	return (*this)[GetSize()-1];	}
	const T&	GetBack() const			{	return (*this)[GetSize()-1];	}
	const T*	GetArray() const		{	return mdata;	}
	T*			GetArray()				{	return mdata;	}
	bool		IsEmpty() const			{	return GetSize() == 0;		}
	bool		IsFull() const			{	return GetSize() >= MaxSize();		}
	size_t		GetSize() const			{	return moffset;		}
	size_t		GetDataSize() const		{	return GetSize() * sizeof(T);	}	//	size of all the data in bytes
	size_t		GElementSize() const	{	return sizeof(T);	}	//	size of all the data in bytes

	bool SetSize(size_t size, bool preserve=true,bool AllowLess=true)
	{
		if ( size == mmaxsize )
		{
			moffset = size;
			return true;
		}

		if ( AllowLess && size <= mmaxsize )
		{
			moffset = size;
			return true;
		}

		T* array = NULL;

		if ( size )
		{
			auto& Heap = GetHeap();
			array = Heap.template AllocArray<T>(size);
			if ( !array )
				return false;

			if ( mdata && preserve )
			{
				auto size0 = GetSize();
				auto count = (size0 > size) ? size : size0;

				if ( Soy::DoComplexCopy<T,T>() )
				{
					for ( size_t i=0; i<count; ++i )
						array[i] = mdata[i];
				}
				else if ( count > 0 )
				{
					memcpy( array, mdata, count * sizeof(T) );
				}
			}
		}

		if ( mdata )
		{
			//	gr; for safety, if there is no heap, we use normal new
			auto& Heap = GetHeap();
			Heap.FreeArray( mdata, mmaxsize );
		}
		mdata    = array;
		mmaxsize = size;
		moffset  = size;
		return true;
	}

	void Reserve(size_t size,bool clear=false)
	{
		if ( clear )
		{
			SetSize(size,false);
			ResetIndex();
		}
		else
		{
			//	grow the array as neccessary, then restore the current size again
			auto CurrentSize = GetSize();
			SetSize( CurrentSize + size, true, true );
			SetSize( CurrentSize, true, true );
		}
	}

	//	raw push of data as a reinterpret cast. Really only for use on PoD array types...
	template<typename THATTYPE>
	T* PushBackReinterpret(const THATTYPE& OtherData)
	{
		return GetArrayBridge(*this).PushBackReinterpret( OtherData );
	}

	//	raw push of data as a reinterpret cast. Really only for use on PoD array types...
	template<typename THATTYPE>
	T* PushBackReinterpretReverse(const THATTYPE& OtherData)
	{
		return GetArrayBridge(*this).PushBackReinterpretReverse( OtherData );
	}

	T* PushBlock(size_t count)
	{
		auto curoff = moffset;
		auto endoff = moffset + count;

		if ( endoff >= mmaxsize )
		{
			if ( !mmaxsize )
			{
				auto size = count * 2;
				if ( !Alloc(size) )
					return nullptr;
			}
			else
			{
				auto size = mmaxsize * 2 + count;
				SetSize(size,true);
			}
		}

		moffset = endoff;
		
		//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
		if ( Soy::IsComplexType<TYPE>() )
		{
			for ( size_t i=curoff;	i<curoff+count;	i++ )
				mdata[i] = T();
		}
		return mdata + curoff;
	}
		
	T& PushBackUnique(const T& item)
	{
		auto* pExisting = Find( item );
		if ( pExisting )
			return *pExisting;

		return PushBack( item );
	}
		
	T& PushBack(const T& item)
	{
		if ( moffset >= mmaxsize )
		{
			if ( !mmaxsize )
			{
				if ( !Alloc( 4 ) )
					return *mdata;
				moffset = 0;
			}
			else
			{
				auto offset = moffset;
				auto size = mmaxsize * 2;
				SetSize(size,true);
				moffset = offset;
			}
		}

		auto& ref = mdata[moffset++];
		ref = item;
		return ref;
	}

	T& PushBack()
	{
		if ( moffset >= mmaxsize )
		{
			if ( !mmaxsize )
			{
				if ( !Alloc( 4 ) )
					return *mdata;
				moffset = 0;
			}
			else
			{
				auto offset = moffset;
				auto size = mmaxsize * 2;
				SetSize(size,true);
				moffset = offset;
			}
		}
		//	we need to re-initialise an element in the buffer array as the memory (eg. a string) could still have old contents
		auto& ref = mdata[moffset++];
		ref = T();
		return ref;
	}

	template<class ARRAYTYPE>
	void PushBackArray(const ARRAYTYPE& v)
	{
		GetArrayBridge( *this ).PushBackArray(v);
	}

	//	pushback a c-array
	template<size_t CARRAYSIZE>
	void PushBackArray(const T(&CArray)[CARRAYSIZE])
	{
		auto NewDataIndex = GetSize();
		auto* pNewData = PushBlock( CARRAYSIZE );

		if ( Soy::IsComplexType<T>() )
		{
			for ( size_t i=0; i<CARRAYSIZE; ++i )
				(*this)[i+NewDataIndex] = CArray[i];	//	use [] operator for bounds check
		}
		else if ( GetSize() > 0 )
		{
			//	re-fetch address for bounds check. technically unncessary
			pNewData = &((*this)[NewDataIndex]);
			//	note: lack of bounds check for all elements here
			memcpy( pNewData, CArray, CARRAYSIZE * sizeof(T) );
		}
	}

	T& PopBack()
	{
		SoyArray::CheckBounds(moffset,*this);
		return mdata[--moffset];
	}

	T PopAt(size_t Index)
	{
		T Item = GetAt(Index);
		RemoveBlock( Index, 1 );
		return Item;
	}

	template<typename MATCHTYPE>
	bool	Remove(const MATCHTYPE& Match)
	{
		auto Index = FindIndex( Match );
		if ( Index < 0 )
			return false;
		RemoveBlock( Index, 1 );
		return true;
	}

	T* InsertBlock(size_t index,size_t count)
	{
		//	do nothing if nothing to add
		if ( count == 0 )
			return IsEmpty() ? NULL : (mdata + index);

		if ( index >= moffset ) 
			return PushBlock( count );

//			int left = static_cast<int>((mdata+moffset) - (mdata+index));
		auto left = static_cast<ssize_t>(moffset - index);
		PushBlock(count);	// increase array mem 

		if ( Soy::DoComplexCopy<T,T>() )
		{
			T* src = mdata + moffset - count - 1;
			T* dest = mdata + moffset - 1;
			for ( size_t i=0; i<left; ++i )
				*dest-- = *src--;
		}
		else if ( left > 0 )
		{
			memmove( &mdata[index+count], &mdata[index], left * sizeof(T) );
		}

		return mdata + index;

	}

	void RemoveBlock(size_t index,size_t count)
	{
		//	do nothing if nothing to remove
		if ( count == 0 )
			return;

		//	assert if we're trying to remove item[s] from outside the array to avoid memory corruptiopn
		assert( index >= 0 && index < GetSize() && (index+count-1) < GetSize() );

		T* dest = mdata + index;
		T* src = mdata + index + count;
		int left = static_cast<int>((mdata+moffset) - src);

		if ( Soy::DoComplexCopy<T,T>() )
		{				
			for ( size_t i=0; i<left; ++i )
				*dest++ = *src++;
			assert( (moffset-count) == static_cast<int>(dest - mdata) );
			moffset = static_cast<int>(dest - mdata);
		}
		else
		{
			if ( left > 0 )
				memmove( dest, src, left * sizeof(T) );
			moffset -= count;
		}			
	}

	void SetIndex(size_t index)
	{
		assert( index >= 0 && index < mmaxsize );
		moffset = index;
	}

	void ResetIndex()
	{
		moffset = 0;
	}

	void TrimArray()
	{
		int size = GetSize();
		SetSize(size,true);
	}

	void Clear(bool Dealloc=true)
	{
		bool AllowLess = !Dealloc;
		SetSize(0,false,AllowLess);
	}

	size_t	MaxSize() const
	{
		return mmaxsize;
	}

	size_t	MaxAllocSize() const
	{
		return std::numeric_limits<size_t>::max();
	}

	size_t	MaxDataSize() const
	{
		return MaxSize() * sizeof(T);
	}

	//	simple iterator to find index of an element matching via == operator
	template<typename MATCHTYPE>
	ssize_t			FindIndex(const MATCHTYPE& Match) const
	{
		for ( size_t i=0;	i<GetSize();	i++ )
		{
			auto& Element = mdata[i];
			if ( Element == Match )
				return i;
		}
		return -1;
	}

	//	find an element - returns first matching element or NULL
	template<typename MATCH> T*			Find(const MATCH& Match)		{	auto Index = FindIndex( Match );		return (Index < 0) ? NULL : &mdata[Index];	}
	template<typename MATCH> const T*	Find(const MATCH& Match) const	{	auto Index = FindIndex( Match );		return (Index < 0) ? NULL : &mdata[Index];	}

	//	swap two elements
	void	Swap(size_t a,size_t b)
	{
		if ( !SoyArray::CheckBounds( a, *this ) || !SoyArray::CheckBounds(b, *this ) )
			return;
		auto& ElementA = (*this)[a];
		auto& ElementB = (*this)[b];
		T Temp = ElementA;
		ElementA = ElementB;
		ElementB = Temp;
	}

	//	set all elements to a value
	template<typename TYPE>
	void	SetAll(const TYPE& Value)
	{
		for ( size_t i=0;	i<GetSize();	i++ )
			mdata[i] = Value;
	}
	
	void	SetAll(const T& Value)
	{
		//	attempt non-complex memset if element can be broken into a byte
		if ( !Soy::IsComplexType<T>() )
		{
			bool AllSame = true;
			const uint8* pValue = reinterpret_cast<const uint8*>( &Value );
			for ( size_t i=1;	i<sizeof(Value);	i++ )
			{
				if ( pValue[i] == pValue[i-1] )
					continue;
				AllSame = false;
			}
			if ( AllSame )
			{
				memset( mdata, pValue[0], GetDataSize() );
				return;
			}
		}

		for ( size_t i=0;	i<GetSize();	i++ )
			mdata[i] = Value;
	}

	template<class ARRAYTYPE>
	inline bool	operator==(const ARRAYTYPE& Array)
	{
		auto ThisBridge = GetArrayBridge( *this );
		auto ThatBridge = GetArrayBridge( Array );
		return ThisBridge.Matches( ThatBridge );
	}

	template<class ARRAYTYPE>
	inline bool	operator!=(const ARRAYTYPE& Array)
	{
		auto ThisBridge = GetArrayBridge( *this );
		auto ThatBridge = GetArrayBridge( Array );
		return ThisBridge.Matches( ThatBridge )==false;
	}
		
	prmem::Heap&	GetHeap() const
	{
		assert( mHeap );
		if ( !mHeap )
		{
			auto& This = *const_cast<Array<T,HEAP>*>( this );
			This.SetHeap( prcore::Heap );
		}
		return mHeap ? *mHeap : prcore::Heap;	
	}

	bool			SetHeap(prmem::Heap& Heap)
	{
		if ( !Soy::Assert( (&Heap)!=nullptr, "Tried to set to null heap" ) )
			return false;
		
		//	if heap isn't valid yet (eg. if this has been allocated before the global heap), 
		//	throw error, but address should be okay, so in these bad cases.. we MIGHT get away with it.
		//	gr: need a better method, this is virtual again, so more likely to throw an exception (invalid vtable)
		if ( !Soy::Assert( Heap.IsValid(), "Invalid heap" ) )
		{
			std::Debug << "Array<" << Soy::GetTypeName<T>() << "> assigned non-valid heap. Array constructed before heap?" << std::endl;
			return false;
		}

		//	if heap changes and we have allocated data we should re-allocate the data...
		bool ReAlloc = mdata && (&Heap!=mHeap);

		//	currently we don't support re-allocating to another heap. 
		//	It's quite trivial, but usually indicates an unexpected situation so leaving it for now
		if ( !Soy::Assert( !ReAlloc, "Currently don't support re-allocating to a new heap") )
			return false;
			
		mHeap = &Heap;
		return true;
	}
	
	//	copy data to a Buffer[BUFFERSIZE] c-array. (lovely template syntax! :)
	//	returns number of elements copied
	template <typename TYPE,size_t BUFFERSIZE>
	int		CopyToBuffer(TYPE (& Buffer)[BUFFERSIZE]) const
	{
		auto Count = std::min( GetSize(), BUFFERSIZE );
		for ( size_t i=0;	i<Count;	i++ )
		{
			Buffer[i] = mdata[i];
		}
		return Count;
	}
	
	bool Alloc(size_t NewSize)
	{
		auto& Heap = GetHeap();
		mdata = Heap.template AllocArray<T>( NewSize );

		//	failed to alloc - make sure vars are accurate
		if ( !mdata )
		{
			mmaxsize = 0;
			moffset = 0;
			return false;
		}
		mmaxsize = NewSize;
		assert( moffset <= mmaxsize );
		return true;
	}

private:
	prmem::Heap*	mHeap;		//	where to alloc/free from
	T*				mdata;
	size_t			mmaxsize;
	size_t			moffset;
};
