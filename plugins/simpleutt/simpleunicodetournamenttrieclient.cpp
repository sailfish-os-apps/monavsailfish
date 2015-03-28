/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "simpleunicodetournamenttrieclient.h"
#include "utils/qthelpers.h"
#include <QtDebug>
#include <algorithm>
#ifndef NOGUI
 #include <QMessageBox>
#endif

SimpleUnicodeTournamentTrieClient::SimpleUnicodeTournamentTrieClient()
{
	trieFile = NULL;
	subTrieFile = NULL;
	dataFile = NULL;
	trieData = NULL;
	subTrieData = NULL;
}

SimpleUnicodeTournamentTrieClient::~SimpleUnicodeTournamentTrieClient()
{

}

QString SimpleUnicodeTournamentTrieClient::GetName()
{
	return "Simple Unicode Tournament Trie";
}

void SimpleUnicodeTournamentTrieClient::SetInputDirectory( const QString& dir )
{
	directory = dir;
}

void SimpleUnicodeTournamentTrieClient::ShowSettings()
{
#ifndef NOGUI
	QMessageBox::information( NULL, "Settings", "No settings available" );
#endif
}

bool SimpleUnicodeTournamentTrieClient::IsCompatible( int fileFormatVersion )
{
	if ( fileFormatVersion == 1 )
		return true;
	return false;
}

bool SimpleUnicodeTournamentTrieClient::LoadData()
{
	UnloadData();
	QString filename = fileInDirectory( directory, "Unicode Tournament Trie" );
	subTrieFile = new QFile( filename + "_sub" );
	dataFile = new QFile( filename + "_ways" );

	if ( !openQFile( subTrieFile, QIODevice::ReadOnly ) )
		return false;
	if ( !openQFile( dataFile, QIODevice::ReadOnly ) )
		return false;

	subTrieData = ( char* ) subTrieFile->map( 0, subTrieFile->size() );

	if ( subTrieData == NULL ) {
		qDebug( "Failed to Memory Map sub trie data" );
		return false;
	}

	return true;
}

bool SimpleUnicodeTournamentTrieClient::UnloadData()
{
	if ( trieFile != NULL )
		delete trieFile;
	trieFile = NULL;
	if ( subTrieFile != NULL )
		delete subTrieFile;
	subTrieFile = NULL;
	if ( dataFile != NULL )
		delete dataFile;
	dataFile = NULL;

	return true;
}

bool SimpleUnicodeTournamentTrieClient::find( const char* trie, unsigned* resultNode, QString* missingPrefix, QString prefix )
{
	qDebug() << "find";
	unsigned node = *resultNode;
	for ( int i = 0; i < ( int ) prefix.length(); ) {
		sutt::Node element;
		element.Read( trie + node );
		bool found = false;
		for ( int c = 0; c < ( int ) element.labelList.size(); c++ ) {
			const sutt::Label& label = element.labelList[c];
			bool equal = true;
			for ( int subIndex = 0; subIndex < ( int ) label.string.length(); ++subIndex ) {
				if ( i + subIndex >= ( int ) prefix.length() ) {
					*missingPrefix = label.string.mid( subIndex );
					break;
				}
				if ( label.string[subIndex] != prefix[i + subIndex] ) {
					equal = false;
					break;
				}
			}
			if ( !equal )
				continue;

			i += label.string.length();
			node = label.index;
			found = true;
			break;

		}
		if ( !found )
			return false;
	}

	*resultNode = node;
	return true;
}

int SimpleUnicodeTournamentTrieClient::getSuggestion( const char* trie, QStringList* resultNames, QStringList* placeNames, QVector<unsigned> *dataIndex, unsigned node, int count, const QString prefix )
{
	std::vector< Suggestion > candidates( 1 );
	candidates[0].index = node;
	candidates[0].prefix = prefix;
	candidates[0].importance = std::numeric_limits< unsigned >::max();

	while( count > 0 && candidates.size() > 0 ) {
		const Suggestion next = candidates[0];
		candidates[0] = candidates.back();
		candidates.pop_back();

		sutt::Node element;
		element.Read( trie + next.index );
		if ( element.dataList.size() > 0 ) {
			assert( next.prefix.length() > 0 );
			QString suggestion = next.prefix[0].toUpper();
			for ( int i = 1; i < ( int ) next.prefix.length(); ++i ) {
				if ( suggestion[i - 1] == ' ' || suggestion[i - 1] == '-' )
					suggestion += next.prefix[i].toUpper();
				else
					suggestion += next.prefix[i];
			}
			for (unsigned i = 0; i < element.dataList.size(); i++) {
				resultNames->push_back( suggestion );
				dataIndex->push_back(i);
				placeNames->push_back(element.placeDataList[i].name);
			}
			count--;
		}
		for ( std::vector< sutt::Label >::const_iterator c = element.labelList.begin(), e = element.labelList.end(); c != e; ++c ) {
			Suggestion nextEntry;
			nextEntry.prefix = next.prefix + c->string;
			nextEntry.index = c->index;
			nextEntry.importance = c->importance;
			candidates.push_back( nextEntry );
		}
	}

	return count;
}

bool SimpleUnicodeTournamentTrieClient::GetPlaceSuggestions( const QString& input, int amount, QStringList* suggestions, QStringList* inputSuggestions )
{
	unsigned node = 0;
	QString prefix;
	QString name = input.toLower();

	if ( !find( trieData, &node, &prefix, name ) )
		return false;

	if ( prefix.length() == 0 ) {
		sutt::Node element;
		element.Read( trieData + node );
		for ( std::vector< sutt::Label >::const_iterator c = element.labelList.begin(), e = element.labelList.end(); c != e; ++c )
			inputSuggestions->push_back( input + c->string );
	}
	else {
		inputSuggestions->push_back( input + prefix );
	}
	std::sort( inputSuggestions->begin(), inputSuggestions->end() );
	return true;
}

bool SimpleUnicodeTournamentTrieClient::GetStreetSuggestions( int placeID, const QString& input, int amount, QStringList* suggestions, QStringList* placeNames, QVector<unsigned> *dataIndex, QStringList* inputSuggestions )
{
	if ( placeID < 0 )
		return false;
	unsigned node = 0;
	QString prefix;
	QString name = input.toLower();

	if ( !find( subTrieData + placeID, &node, &prefix, name ) )
		return false;

	if ( prefix.length() == 0 ) {
		sutt::Node element;
		element.Read( subTrieData + placeID + node );
		for ( std::vector< sutt::Label >::const_iterator c = element.labelList.begin(), e = element.labelList.end(); c != e; ++c )
			inputSuggestions->push_back( input + c->string );
	}
	else {
		inputSuggestions->push_back( input + prefix );
	}
	getSuggestion( subTrieData + placeID, suggestions, placeNames, dataIndex, node, amount, name + prefix );
	std::sort( inputSuggestions->begin(), inputSuggestions->end() );
	return true;
}

bool SimpleUnicodeTournamentTrieClient::GetPlaceData( QString input, QVector< int >* placeIDs, QVector< UnsignedCoordinate >* placeCoordinates )
{
	unsigned node = 0;
	QString prefix;
	QString name = input.toLower();
	if ( !find( trieData, &node, &prefix, name ) )
		return false;

	sutt::Node element;
	element.Read( trieData + node );

	for ( std::vector< sutt::Data >::const_iterator i = element.dataList.begin(), e = element.dataList.end(); i != e; ++i ) {
		sutt::CityData data;
		data.Read( subTrieData + i->start );
		placeCoordinates->push_back( data.coordinate );
		placeIDs->push_back( i->start + data.GetSize() );
	}

	return placeIDs->size() != 0;
}

bool SimpleUnicodeTournamentTrieClient::GetStreetData( int placeID, QString input, unsigned dataIndex, QVector< int >* segmentLength, QVector< UnsignedCoordinate >* coordinates, QString *place )
{
	if ( placeID < 0 )
		return false;
	unsigned node = 0;
	QString prefix;
	QString name = input.toLower();
	if ( !find( subTrieData + placeID, &node, &prefix, name ) )
		return false;

	sutt::Node element;
	element.Read( subTrieData + placeID + node );

	sutt::Data data = element.dataList[dataIndex];
	unsigned* buffer = new unsigned[data.length * 2];
	dataFile->seek( data.start * sizeof( unsigned ) * 2 );
	dataFile->read( ( char* ) buffer, data.length * 2 * sizeof( unsigned ) );
	for ( unsigned start = 0; start < data.length; ++start ) {
		UnsignedCoordinate temp;
		temp.x = buffer[start * 2];
		temp.y = buffer[start * 2 + 1];
		coordinates->push_back( temp );
	}
	delete[] buffer;
	segmentLength->push_back( coordinates->size() );
	
	sutt::PlaceData placeData = element.placeDataList[dataIndex];
	*place = placeData.name;

	return segmentLength->size() != 0;
}
