#include "tinytext.h"
#include "tinyxml.h"

const char* getTinyTextByTag(const char* tag, const TiXmlElement* element)
{
	while (element)
	{
		if (::strcmp(element->Value(), tag) == 0)
		{
			return element->GetText();
		}

		element = element->NextSiblingElement();
	}

	return nullptr;
}
