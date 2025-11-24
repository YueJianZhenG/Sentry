//
// Created by 64658 on 2025/6/16.
//

#include "CensorFactory.h"
#include <fstream>
#include <memory>
std::string censor::u32_to_utf8(const std::u32string& str)
{
	std::string result;
	for (char32_t ch: str)
	{
		if (ch < 0x80)
		{
			result.push_back(ch);
		}
		else if (ch < 0x800)
		{
			result.push_back(0xC0 | ((ch >> 6) & 0x1F));
			result.push_back(0x80 | (ch & 0x3F));
		}
		else if (ch < 0x10000)
		{
			result.push_back(0xE0 | ((ch >> 12) & 0x0F));
			result.push_back(0x80 | ((ch >> 6) & 0x3F));
			result.push_back(0x80 | (ch & 0x3F));
		}
		else
		{
			result.push_back(0xF0 | ((ch >> 18) & 0x07));
			result.push_back(0x80 | ((ch >> 12) & 0x3F));
			result.push_back(0x80 | ((ch >> 6) & 0x3F));
			result.push_back(0x80 | (ch & 0x3F));
		}
	}
	return result;
}

std::u32string censor::utf8_to_u32(const std::string& str)
{
	std::u32string result;
	size_t i = 0;
	while (i < str.size())
	{
		char32_t ch = 0;
		unsigned char c = str[i];
		if (c < 0x80)
		{
			ch = c;
			i += 1;
		}
		else if ((c >> 5) == 0x6)
		{
			ch = ((c & 0x1F) << 6) | (str[i + 1] & 0x3F);
			i += 2;
		}
		else if ((c >> 4) == 0xE)
		{
			ch = ((c & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
			i += 3;
		}
		else if ((c >> 3) == 0x1E)
		{
			ch = ((c & 0x07) << 18) | ((str[i + 1] & 0x3F) << 12) |
				 ((str[i + 2] & 0x3F) << 6) | (str[i + 3] & 0x3F);
			i += 4;
		}
		else
		{
			++i;
		}
		result.push_back(ch);
	}
	return result;
}


namespace censor
{
	void Factory::Insert(const std::string& utf8word)
	{
		std::u32string word = utf8_to_u32(utf8word);
		Node* node = root;
		for (char32_t ch: word)
		{
			if (!node->children[ch])
				node->children[ch] = new Node();
			node = node->children[ch];
		}
		node->wordLens.emplace_back(word.size());
	}

	bool Factory::LoadFromFile(const std::string & path)
	{
		std::ifstream fin(path);
		if (!fin)
		{
			return false;
		}
		std::string line;
		while (std::getline(fin, line))
		{
			if (!line.empty())
			{
				this->Insert(line);
			}
		}
		this->Build();
		return true;
	}

	void Factory::Build()
	{
		std::queue<Node*> q;
		root->fail = root;
		for (auto& item: root->children)
		{
			item.second->fail = root;
			q.push(item.second);
		}

		while (!q.empty())
		{
			Node* curr = q.front();
			q.pop();
			for (auto& item: curr->children)
			{
				Node* f = curr->fail;
				auto & ch = item.first;
				auto & child = item.second;
				while (f != root && !f->children.count(ch))
					f = f->fail;
				if (f->children.count(ch) && f->children[ch] != child)
					child->fail = f->children[ch];
				else
					child->fail = root;

				child->wordLens.insert(
						child->wordLens.end(),
						child->fail->wordLens.begin(),
						child->fail->wordLens.end()
				);
				q.push(child);
			}
		}
	}

	unsigned int Factory::Mask(std::string& utf8text, char maskChar)
	{
		std::u32string text = utf8_to_u32(utf8text);
		std::unique_ptr<bool[]> mask = std::make_unique<bool[]>(text.size());
		Node* node = root;

		for (size_t i = 0; i < text.size(); ++i)
		{
			char32_t ch = text[i];
			while (node != root && !node->children.count(ch))
				node = node->fail;
			if (node->children.count(ch))
				node = node->children[ch];

			for (int len: node->wordLens)
			{
				for (int j = i - len + 1; j <= i; ++j)
				{
					if (j >= 0) mask[j] = true;
				}
			}
		}
		unsigned int count = 0;
		for (size_t i = 0; i < text.size(); ++i)
		{
			if (mask[i])
			{
				count++;
				text[i] = maskChar;
			}
		}

		utf8text = u32_to_utf8(text);
		return count;
	}

	bool Factory::Check(const std::string& utf8text)
	{
		std::u32string text = utf8_to_u32(utf8text);
		Node* node = root;

		for (size_t i = 0; i < text.size(); ++i)
		{
			char32_t ch = text[i];
			while (node != root && !node->children.count(ch))
				node = node->fail;
			if (node->children.count(ch))
				node = node->children[ch];

			for (int len: node->wordLens)
			{
				for (int j = i - len + 1; j <= i; ++j)
				{
					if (j >= 0)
					{
						return false;
					}
				}
			}
		}
		return true;
	}

	Factory::~Factory() noexcept
	{
		std::queue<Node*> q;
		q.push(root);
		while (!q.empty())
		{
			Node* node = q.front();
			q.pop();
			for (auto& item: node->children)
				q.push(item.second);
			delete node;
		}
	}
}