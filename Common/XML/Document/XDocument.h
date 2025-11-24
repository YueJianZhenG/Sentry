//
// Created by yy on 2024/6/26.
//

#ifndef APP_XDOCUMENT_H
#define APP_XDOCUMENT_H
#include <string>
#include <memory>
#include <vector>
#include "XML/Src/tinyxml2.h"

namespace xml
{
	class XElement
	{
	public:
		XElement(tinyxml2::XMLElement * element, tinyxml2::XMLDocument & doc);
	public:;
		bool Get(const char * key, int & value) const;
		bool Get(const char * key, double & value) const;
		bool Get(const char * key, std::string & value) const;
		bool Get(const char * key, std::unique_ptr<xml::XElement> & value) const;
		bool Get(const char * key, std::vector<std::unique_ptr<xml::XElement>> & value) const;
	public:
		bool Next(const char * key, std::unique_ptr<xml::XElement> & value) const;
	public:
		bool GetAttribute(const char * key, int & value);
		bool GetAttribute(const char * key, std::string & value);
	public:
		bool Add(const char * key, int value);
		bool Add(const char * key, const std::string & value);
	public:
		bool AddAttribute(const char * key, int value);
		bool AddAttribute(const char * key, const std::string & value);
		virtual std::unique_ptr<xml::XElement> AddElement(const char * key);
	protected:
		tinyxml2::XMLElement * mElement;
		tinyxml2::XMLDocument & mDocument;
	};

	class XDocument : public XElement
	{
	public:
		XDocument();
	public:
		bool Encode(std::string & xml);
		bool Decode(const std::string & xml);
	public:
		std::unique_ptr<xml::XElement> AddElement(const char *key) final;
	private:
		tinyxml2::XMLDocument mDocument;
	};
}


#endif //APP_XDOCUMENT_H
