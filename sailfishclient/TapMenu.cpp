#include "TapMenu.h"

#include "interfaces/isimpleaddresslookup.h"
#include "interfaces/irenderer.h"
#include "utils/qthelpers.h"
#include "client/mapdata.h"
#include "client/routinglogic.h"

TapMenu::TapMenu(QObject *parent) : QObject(parent) {

}

TapMenu::~TapMenu() {

}

void TapMenu::searchTextChanged(QString text) {
	ISimpleAddressLookup* simpleAddressLookup = MapData::instance()->simpleAddressLookup();
	if ( simpleAddressLookup == NULL )
		return;

	search_suggestions.clear();
	search_dataIndex.clear();
	QStringList characters;
	QStringList placeNames;
	
	if (text.length() == 0) {
		emit searchResultUpdated(search_suggestions, placeNames);
		return;
	}

	Timer time;
	bool found = simpleAddressLookup->GetStreetSuggestions( 0, text, 10, &search_suggestions, &placeNames, &search_dataIndex, &characters );
	qDebug() << "Street Lookup:" << time.elapsed() << "ms";
	
	emit searchResultUpdated(search_suggestions, placeNames);
}

void TapMenu::searchResultSelected(QString command, int index) {
	ISimpleAddressLookup* simpleAddressLookup = MapData::instance()->simpleAddressLookup();
	if ( simpleAddressLookup == NULL )
		return;
	qDebug() << search_dataIndex[index];
	QVector< int > segmentLength;
	QVector< UnsignedCoordinate > coordinates;
	QString place;
	if ( !simpleAddressLookup->GetStreetData( 0, search_suggestions[index], search_dataIndex[index], &segmentLength, &coordinates, &place ) )
		return;
	
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	if (command == "source") {
		RoutingLogic::instance()->setSource( coordinates[0] );
	} else if (command == "destination") {
		RoutingLogic::instance()->setTarget( coordinates[0] );
	}
}

void TapMenu::setSourceFollowLocation(bool follow) {
	RoutingLogic::instance()->setGPSLink(follow);
}
