/*
 * swir_json.cpp
 *
 *	Helper class to deal with JSON payload serialization and deserialization
 *  getValue() method is generic (not specific to AV)... could be placed in a separate class
 *
 *  Created on: June 2015
 *      Author: Nhon Chu
 */
 
#ifndef _SWIR_JSON_H_
#define _SWIR_JSON_H_


#include "arduino.h"


class SwirJson
{
	public:
		SwirJson();

		char*		serialize(char* szKey, char* szValue, unsigned long ulTimestamp = 0);
		char*		serialize(char* szKey, float fValue, unsigned long ulTimestamp = 0);
		char*		serialize(char* szKey, int nValue, unsigned long ulTimestamp = 0);
		char* 		serialize(char* szKey1, char* szValue1, char* szKey2, char* szValue2, unsigned long ulTimestamp = 0);
		//char*		serialize(char* szKey, char** pszValueList, int nValueCount, unsigned long* pulTimestampList = NULL);
		char *		getValue(char* szJson, int nKeyIndex, char* szSearchKey = NULL);

	protected:
		//char* 		buildJsonList(char* szBuffer, char** pszValueList, int nValueCount, unsigned long* pulTimestampList = NULL);
};

#endif	//_SWIR_JSON_H_
