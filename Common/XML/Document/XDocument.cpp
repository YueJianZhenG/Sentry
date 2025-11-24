//
// Created by yy on 2024/6/26.
//

#include "XDocument.h"

namespace xml
{
	XElement::XElement(tinyxml2::XMLElement* element, tinyxml2::XMLDocument & doc)
		: mElement(element), mDocument(doc)
	{

	}

	bool XElement::Get(const char* key, int& value) const
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		tinyxml2::XMLElement * element = this->mElement->FirstChildElement(key);
		if(element == nullptr)
		{
			return false;
		}
		return element->QueryIntText(&value) == tinyxml2::XML_SUCCESS;
	}

	bool XElement::Get(const char* key, double& value) const
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		tinyxml2::XMLElement * element = this->mElement->FirstChildElement(key);
		if(element == nullptr)
		{
			return false;
		}
		return element->QueryDoubleText(&value) == tinyxml2::XML_SUCCESS;
	}

	bool XElement::Get(const char* key, std::string& value) const
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		tinyxml2::XMLElement * element = this->mElement->FirstChildElement(key);
		if(element == nullptr)
		{
			return false;
		}
		const char * text = element->GetText();
		if(text == nullptr)
		{
			return false;
		}
		value.assign(text);
		return true;
	}

	bool XElement::Next(const char* key, std::unique_ptr<xml::XElement>& value) const
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		tinyxml2::XMLElement * element = this->mElement->NextSiblingElement(key);
		if(element == nullptr)
		{
			return false;
		}
		value = std::make_unique<XElement>(element, this->mDocument);
		return true;
	}

	bool XElement::Get(const char* key, std::unique_ptr<XElement>& value) const
	{
		if(this->mElement == nullptr)
		{
			return false;
		}

		tinyxml2::XMLElement * element = this->mElement->FirstChildElement(key);
		if(element == nullptr)
		{
			return false;
		}
		value = std::make_unique<XElement>(element, this->mDocument);
		return true;
	}

	bool XElement::Get(const char* key, std::vector<std::unique_ptr<xml::XElement>>& value) const
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		tinyxml2::XMLElement * xmlElement = this->mElement->FirstChildElement(key);
		if(xmlElement == nullptr)
		{
			return false;
		}
		while(xmlElement != nullptr)
		{
			value.emplace_back(std::make_unique<XElement>(xmlElement, this->mDocument));
			xmlElement = xmlElement->NextSiblingElement(key);
		}
		return true;
	}
}

namespace xml
{
	bool XElement::Add(const char* key, int value)
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		tinyxml2::XMLElement * xmlElement = this->mDocument.NewElement(key);
		{
			xmlElement->SetText(value);
		}
		this->mElement->InsertEndChild(xmlElement);
		return true;
	}

	bool XElement::Add(const char* key, const std::string& value)
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		tinyxml2::XMLElement * xmlElement = this->mDocument.NewElement(key);
		{
			xmlElement->SetText(value.c_str());
		}
		this->mElement->InsertEndChild(xmlElement);
		return true;
	}

	std::unique_ptr<xml::XElement> XElement::AddElement(const char* key)
	{
		tinyxml2::XMLElement* element = this->mDocument.NewElement(key);
		this->mElement->InsertEndChild(element);
		return std::make_unique<xml::XElement>(element, this->mDocument);
	}

	bool XElement::AddAttribute(const char* key, int value)
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		this->mElement->SetAttribute(key, value);
		return true;
	}

	bool XElement::AddAttribute(const char* key, const std::string& value)
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		this->mElement->SetAttribute(key, value.c_str());
		return true;
	}

	bool XElement::GetAttribute(const char* key, int& value)
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		value = this->mElement->IntAttribute(key);
		return true;
	}

	bool XElement::GetAttribute(const char* key, std::string& value)
	{
		if(this->mElement == nullptr)
		{
			return false;
		}
		value = this->mElement->Attribute(key);
		return true;
	}
}

namespace xml
{
	XDocument::XDocument()
		: XElement(nullptr, this->mDocument)
	{

	}

	bool XDocument::Decode(const std::string& xml)
	{
		if(this->mDocument.Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS)
		{
			return false;
		}
		this->mElement = this->mDocument.RootElement();
		return true;
	}

	bool XDocument::Encode(std::string& xml)
	{
		tinyxml2::XMLPrinter xmlPrinter;
		this->mDocument.Print(&xmlPrinter);
		xml.assign(xmlPrinter.CStr(), xmlPrinter.CStrSize());
		return true;
	}

	std::unique_ptr<xml::XElement> XDocument::AddElement(const char* key)
	{
		tinyxml2::XMLElement * element = this->mDocument.NewElement(key);
		if(this->mElement == nullptr)
		{
			tinyxml2::XMLDeclaration * declaration = this->mDocument.NewDeclaration();
			{
				this->mElement = element;
				this->mDocument.InsertFirstChild(declaration);
			}
		}
		this->mDocument.InsertEndChild(element);
		return std::make_unique<xml::XElement>(element, this->mDocument);
	}
}