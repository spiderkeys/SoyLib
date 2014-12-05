#include "SoyString.h"
#include <regex>
#include <sstream>
#include "array.hpp"
#include "bufferarray.hpp"
#include "heaparray.hpp"
#include <SoyDebug.h>
#include <regex>


void Soy::StringToLower(std::string& String)
{
	std::transform( String.begin(), String.end(), String.begin(), ::tolower );
}

std::string Soy::StringToLower(const std::string& String)
{
	std::string LowerString = String;
	StringToLower( LowerString );
	return LowerString;
}


bool Soy::StringContains(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	if (CaseSensitive)
	{
		//	gr: may need to see WHY this doesn't return npos....
		if ( Needle.empty() )
			return false;
		return (Haystack.find(Needle) != std::string::npos);
	}
	else
	{
		std::string HaystackLow = Haystack;
		std::string NeedleLow = Needle;
		Soy::StringToLower( HaystackLow );
		Soy::StringToLower( NeedleLow );
		return StringContains(HaystackLow, NeedleLow, true);
	}
}


bool Soy::StringBeginsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	if (CaseSensitive)
	{
		return (Haystack.find(Needle) == 0 );
	}
	else
	{
		std::string HaystackLow = Haystack;
		std::string NeedleLow = Needle;
		Soy::StringToLower( HaystackLow );
		Soy::StringToLower( NeedleLow );
		return StringBeginsWith(HaystackLow, NeedleLow, true);
	}
}


bool Soy::StringEndsWith(const std::string& Haystack, const std::string& Needle, bool CaseSensitive)
{
	std::regex_constants::syntax_option_type Flags = std::regex_constants::ECMAScript;
	if ( CaseSensitive )
		Flags |= std::regex::icase;
	
	//	escape some chars...
	std::stringstream Pattern;
	Pattern << ".*";
	for ( auto it=Needle.begin();	it!=Needle.end();	it++ )
	{
		auto Char = *it;
		if ( Char == '.' )
			Pattern << "\\";
		Pattern << Char;
	}
	Pattern << "$";
	
	std::smatch Match;
	std::string HaystackStr = Haystack;
	std::regex Regex( Pattern.str(), Flags );
	if ( std::regex_match( HaystackStr, Match, Regex ) )
		return true;

	return false;
}

std::string	Soy::Join(const std::vector<std::string>& Strings,const std::string& Glue)
{
	//	gr: consider a lambda here?
	std::stringstream Stream;
	for ( auto it=Strings.begin();	it!=Strings.end();	it++ )
	{
		Stream << (*it);
		auto itLast = Strings.end();
		itLast--;
		if ( it != itLast )
			Stream << Glue;
	}
	return Stream.str();
}



std::string Soy::ArrayToString(const ArrayBridge<char>& Array)
{
	std::stringstream Stream;
	ArrayToString( Array, Stream );
	return Stream.str();
}

void Soy::ArrayToString(const ArrayBridge<char>& Array,std::stringstream& String)
{
	for ( int i=0;	i<Array.GetSize();	i++ )
	{
		if ( Array[i] == '\0' )
			continue;
		String << Array[i];
	}
}

void Soy::StringToArray(std::string String,ArrayBridge<char>& Array)
{
	const int Size = String.length();
	auto CommandStrArray = GetRemoteArray( String.c_str(), Size, Size );
	Array.PushBackArray( CommandStrArray );
}

void Soy::StringSplitByString(ArrayBridge<std::string>& Parts,std::string String,std::string Delim,bool IncludeEmpty)
{
	std::string::size_type Start = 0;
	while ( Start < String.length() )
	{
		auto End = String.find( Delim, Start );
		if ( End == std::string::npos )
			End = String.length();
		
		auto Part = String.substr( Start, End-Start );
		//std::Debug << "found [" << Part << "] in [" << String << "]" << std::endl;

		if ( IncludeEmpty || !Part.empty() )
			Parts.PushBack( Part );
	
		Start = End+1;
	}
}

void Soy::StringSplitByString(ArrayBridge<std::string>&& Parts,std::string String,std::string Delim,bool IncludeEmpty)
{
	StringSplitByString( Parts, String, Delim, IncludeEmpty );
}

std::string Soy::StreamToString(std::stringstream&& Stream)
{
	return Stream.str();
}

std::string Soy::StreamToString(std::ostream& Stream)
{
	std::stringstream TempStream;
	TempStream << Stream.rdbuf();
	return TempStream.str();
}


void Soy::StringSplitByMatches(ArrayBridge<std::string>& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty)
{
	std::stringstream PatternString;
	PatternString << "^(.*)[";
	//	match any of these characters and EOF
	PatternString << MatchingChars << "$" << "]";
	//	only match one case if including emptys, else 1+
	PatternString <<  (IncludeEmpty ? "{1}" : "+");
	
	//	split at cross-platform line feed (or EOF so we don't need to end with a linefeed)
	std::regex Pattern( PatternString.str(), std::regex::icase );
	std::smatch Match;
	std::string PendingLines = String;
	while ( std::regex_search( PendingLines, Match, Pattern ) )
	{
		//	match line
		auto Line = Match[1].str();
		for ( int i=2;	i<Match.size();	i++ )
		{
			std::Debug << "Unexpected OTHER regex matches when splitting file lines. Part#" << i << ": " << Match[i].str() << std::endl;
		}
		
		if ( !Line.empty() )
			Parts.PushBack( Line );
		
		//	next
		PendingLines = Match.suffix().str();
	}
		
	//	gr: regex wont match $ (end of string) argh... check for trailing command
	if ( !Soy::Assert( PendingLines.empty(), "Remove the need for this by fixing the regex pattern!" ) )
	{
		auto& Line = PendingLines;
		if ( !Line.empty() )
			Parts.PushBack( Line );
	}
}

void Soy::StringSplitByMatches(ArrayBridge<std::string>&& Parts,const std::string& String,const std::string& MatchingChars,bool IncludeEmpty)
{
	StringSplitByMatches( Parts, String, MatchingChars, IncludeEmpty );
}
	
void Soy::SplitStringLines(ArrayBridge<std::string>& StringLines,const std::string& String)
{
	StringSplitByMatches( StringLines, String, "\n\r", false );
}
	
void Soy::SplitStringLines(ArrayBridge<std::string>&& StringLines,const std::string& String)
{
	SplitStringLines( StringLines, String );
}
