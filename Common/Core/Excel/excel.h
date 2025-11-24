#pragma once

#include <map>
#include <utility>
#include <vector>
#include <string>
#include <cmath>
#include "miniz.h"
#include <codecvt>
#include "XML/Src/tinyxml2.h"
#include "Util/Tools/String.h"

#ifdef __ENABLE_MI_MALLOC__
#include "mimalloc.h"
#endif
namespace lxlsx
{
    inline bool is_date_ime(uint32_t id)
    {
        return (id >= 14 && id <= 22) || (id >= 27 && id <= 36) || (id >= 45 && id <= 47)
            || (id >= 50 && id <= 58) || (id >= 71 && id <= 81);
    }

    inline bool is_custom(uint32_t id)
    {
        return id > 165;
    }

#ifdef __ENABLE_MI_MALLOC__
	inline void* mi_mz_alloc(void* opaque, size_t items, size_t size)
	{
		(void)opaque;
		return mi_calloc(items, size);
	}

// 释放函数
	inline void mi_mz_free(void* opaque, void* address)
	{
		(void)opaque;
		mi_free(address);
	}

// 重新分配函数
	inline void* mi_mz_realloc(void* opaque, void* address, size_t items, size_t size)
	{
		(void)opaque;
		return mi_realloc(address, items * size);
	}
#endif

    class Cell
    {
    public:
        void _gc()
        {
        }

        std::string type;
        std::string value;
        std::string fmt_code;
        uint32_t fmt_id = 0;

    public:
        Cell() = default;

        Cell* clone() const
        {
            Cell* cl = new Cell();
            cl->type = type;
            cl->value = value;
            cl->fmt_code = fmt_code;
            cl->fmt_id = fmt_id;
            return cl;
        }
    };


    class Sheet
    {
    public:
        ~Sheet()
        {
            for (auto cell : cells)
            {
                delete cell;
            }
        }

        Sheet() = default;

        explicit Sheet(std::string name) : name(std::move(name))
        {
        }

        void _gc()
        {
        }

        Cell* get_cell(uint32_t row, uint32_t col)
        {
            if (row < first_row || row > last_row || col < first_col || col > last_col)
                return nullptr;
            uint32_t index = (row - 1) * (last_col - first_col + 1) + (col - first_col);
            return cells[index];
        }

        inline std::string get_cell_str(uint32_t row, uint32_t col)
        {
            std::string result;
            Cell * cell = this->get_cell(row, col);
            if(cell != nullptr)
            {
                result = cell->value;
            }
            return result;
        }

        void add_cell(uint32_t row, uint32_t col, Cell * co)
        {
            if (row < first_row || row > last_row || col < first_col || col > last_col)
                return;
            uint32_t index = (row - 1) * (last_col - first_col + 1) + (col - first_col);
            cells[index] = co;
        }

        std::string rid;
        std::string name;
        std::string path;
        bool visible = true;
        uint32_t sheet_id = 0;
        uint32_t last_row = 0;
        uint32_t last_col = 0;
        uint32_t first_row = 0;
        uint32_t first_col = 0;
        std::vector<Cell *> cells = {};
    };

    class ExcelFile
    {
    public:
        ~ExcelFile()
        {
            mz_zip_reader_end(&archive);
            for (auto sh : excel_sheets)
            {
                delete sh;
            }
        }


        inline bool Open(const std::string & filename, std::string& error)
        {
            memset(&archive, 0, sizeof(archive));
#ifdef __ENABLE_MI_MALLOC__
			archive.m_pAlloc = mi_mz_alloc;
			archive.m_pFree = mi_mz_free;
			archive.m_pRealloc = mi_mz_realloc;
#endif
            if (!mz_zip_reader_init_file(&archive, filename.c_str(), 0))
            {
#ifdef __OS_WIN__
				std::string newPath = help::text::Utf8ToGB2312(filename);
				if (!mz_zip_reader_init_file(&archive, newPath.c_str(), 0))
				{
					error.assign(mz_zip_get_error_string(archive.m_last_error));
					return false;
				}
#else
				return false;
#endif
            }

            read_work_book("xl/workbook.xml");
            read_shared_strings("xl/sharedStrings.xml");
            read_work_book_rels("xl/_rels/workbook.xml.rels");
            read_styles("xl/styles.xml");
            for (Sheet* s : excel_sheets)
            {
                read_sheet(s);
            }
            return true;
        }

        inline Sheet* GetSheet(const char* name)
        {
            for (auto sh : excel_sheets)
            {
                if (sh->name == name) return sh;
            }
            return nullptr;
        }

        inline std::vector<Sheet *> GetSheets()
        {
            return this->excel_sheets;
        }

    private:
        inline bool open_xml(const char* filename, tinyxml2::XMLDocument& doc)
        {
            uint32_t index = mz_zip_reader_locate_file(&archive, filename, nullptr, 0);
            size_t size = 0;
            auto data = (const char*)mz_zip_reader_extract_to_heap(&archive, index, &size, 0);
            if (data && doc.Parse(data, size) == tinyxml2::XML_SUCCESS)
            {
                delete []data;
                return true;
            }
            return false;
        }

        void read_sheet(Sheet* sh)
        {
            tinyxml2::XMLDocument doc;
            if (!open_xml(sh->path.c_str(), doc)) return;

            tinyxml2::XMLElement* root = doc.FirstChildElement("worksheet");
            tinyxml2::XMLElement* dim = root->FirstChildElement("dimension");
            tinyxml2::XMLElement* shdata = root->FirstChildElement("sheetData");
            parse_range(dim, shdata, sh);
            sh->cells.resize(sh->last_col * sh->last_row);
            tinyxml2::XMLElement* row = shdata->FirstChildElement("row");
            while (row)
            {
                uint32_t row_idx = row->IntAttribute("r");
                tinyxml2::XMLElement* c = row->FirstChildElement("c");
                while (c)
                {
                    uint32_t col_idx = 0;
                    std::unique_ptr<Cell> cel = std::make_unique<Cell>();
                    parse_cell(c->Attribute("r"), row_idx, col_idx);
                    read_cell(cel.get(), c->Attribute("t"), c->Attribute("s"), c->FirstChildElement("v"));
                    sh->add_cell(row_idx, col_idx, cel.release());
                    c = c->NextSiblingElement("c");
                }
                row = row->NextSiblingElement("row");
            }
            tinyxml2::XMLElement* mcell = root->FirstChildElement("mergeCells");
            if (mcell)
            {
                mcell = mcell->FirstChildElement("mergeCell");
                while (mcell)
                {
                    merge_cells(sh, mcell->Attribute("ref"));
                    mcell = mcell->NextSiblingElement("mergeCell");
                }
            }
        }

        inline void read_styles(const char* filename)
        {
            tinyxml2::XMLDocument doc;
            if (!open_xml(filename, doc)) return;

            tinyxml2::XMLElement* styleSheet = doc.FirstChildElement("styleSheet");
            if (styleSheet == nullptr) return;
            tinyxml2::XMLElement* numFmts = styleSheet->FirstChildElement("numFmts");
            if (numFmts == nullptr) return;

            std::map<int, std::string> custom_date_formats;
            for (tinyxml2::XMLElement* numFmt = numFmts->FirstChildElement(); numFmt; numFmt = numFmt->NextSiblingElement())
            {
                uint32_t id = std::atoi(numFmt->Attribute("numFmtId"));
                std::string fmt = numFmt->Attribute("formatCode");
                custom_date_formats.emplace(id, fmt);
            }

            tinyxml2::XMLElement* cellXfs = styleSheet->FirstChildElement("cellXfs");
            if (cellXfs == nullptr) return;

            uint32_t i = 0;
            for (tinyxml2::XMLElement* cellXf = cellXfs->FirstChildElement(); cellXf; cellXf = cellXf->NextSiblingElement())
            {
                const char* fi = cellXf->Attribute("numFmtId");
                if (fi)
                {
                    std::string fmt;
                    uint32_t formatId = std::atoi(fi);
                    auto iter = custom_date_formats.find(formatId);
                    if (iter != custom_date_formats.end())
                    {
                        fmt = iter->second;
                    }
                    form_ids.emplace(i, formatId);
                    fmt_codes.emplace(formatId, fmt);
                }
                ++i;
            }
        }

        inline void read_work_book(const char* filename)
        {
            tinyxml2::XMLDocument doc;
            if (!open_xml(filename, doc)) return;
            tinyxml2::XMLElement* e = doc.FirstChildElement("workbook");
            tinyxml2::XMLElement* sheets = e->FirstChildElement("sheets");
            tinyxml2::XMLElement* sheet = sheets->FirstChildElement("sheet");
            while (sheet != nullptr)
            {
                std::unique_ptr<Sheet> s = std::make_unique<Sheet>();
                {
                    s->rid = sheet->Attribute("r:id");
                    s->name = sheet->Attribute("name");
                    s->sheet_id = sheet->IntAttribute("sheetId");
                    s->visible = (sheet->Attribute("state") && !strcmp(sheet->Attribute("state"), "hidden"));
                    sheet = sheet->NextSiblingElement("sheet");
                }
                excel_sheets.emplace_back(s.release());
            }
        }

        inline void read_shared_strings(const char* filename)
        {
            tinyxml2::XMLDocument doc;
            if (!open_xml(filename, doc)) return;
            tinyxml2::XMLElement* e = doc.FirstChildElement("sst");
            e = e->FirstChildElement("si");
            while (e)
            {
                tinyxml2::XMLElement* t = e->FirstChildElement("t");
                if (t)
                {
                    const char* text = t->GetText();
                    shared_string.emplace_back(text ? text : "");
                    e = e->NextSiblingElement("si");
                    continue;
                }
                std::string value;
                tinyxml2::XMLElement* r = e->FirstChildElement("r");
                while (r)
                {
                    t = r->FirstChildElement("t");
                    const char* text = t->GetText();
                    if (text) value.append(text);
                    r = r->NextSiblingElement("r");
                }
                shared_string.push_back(value);
                e = e->NextSiblingElement("si");
            }
        }

        inline void read_work_book_rels(const char* filename)
        {
            tinyxml2::XMLDocument doc;
            if (!open_xml(filename, doc)) return;
            tinyxml2::XMLElement* e = doc.FirstChildElement("Relationships");
            e = e->FirstChildElement("Relationship");
            while (e)
            {
                const char* rid = e->Attribute("Id");
                for (auto sheet : excel_sheets)
                {
                    if (sheet->rid == rid)
                    {
                        sheet->path = "xl/" + std::string(e->Attribute("Target"));
                        break;
                    }
                }
                e = e->NextSiblingElement("Relationship");
            }
        }

        inline void read_cell(Cell* c, const char* t, const char* s, tinyxml2::XMLElement* v)
        {
            if (!v || !v->GetText())
            {
                c->type = "blank";
                return;
            }
            c->type = "error";
            c->value = v->GetText();
            if (!t || !strcmp(t, "n"))
            {
                c->type = "number";
                if (s)
                {
                    uint32_t idx = std::atoi(s);
                    auto it = form_ids.find(idx);
                    if (it == form_ids.end()) return;
                    uint32_t format_id = it->second;
                    auto it2 = fmt_codes.find(format_id);
                    if (it2 == fmt_codes.end()) return;
                    c->fmt_id = format_id;
                    c->fmt_code = it2->second;
                    if (is_date_ime(format_id))
                    {
                        c->type = "date";
                    }
                    else if (is_custom(format_id))
                    {
                        c->type = "custom";
                    }
                    return;
                }
            }
            if (!t) return;
            if (!strcmp(t, "s"))
            {
                c->type = "string";
                c->value = shared_string[std::atoi(v->GetText())];
            }
            else if (!strcmp(t, "inlineStr"))
            {
                c->type = "string";
            }
            else if (!strcmp(t, "str"))
            {
                c->type = "string";
            }
            else if (!strcmp(t, "b"))
            {
                c->type = "bool";
            }
        }

        inline void parse_cell(const std::string& value, uint32_t& row, uint32_t& col)
        {
            col = 0;
            uint32_t arr[10];
            uint32_t index = 0;
            while (index < value.length())
            {
                if (isdigit(value[index])) break;
                arr[index] = value[index] - 'A' + 1;
                index++;
            }
            for (uint32_t i = 0; i < index; i++)
            {
                col += (arr[i] * pow(26, index - i - 1));
            }
            row = atol(value.c_str() + index);
        }

        inline void merge_cells(Sheet* sh, const std::string& value)
        {
            size_t index = value.find_first_of(':');
            if (index != std::string::npos)
            {
                uint32_t first_row = 0, first_col = 0, last_row = 0, last_col = 0;
                parse_cell(value.substr(0, index), first_row, first_col);
                parse_cell(value.substr(index + 1), last_row, last_col);
                Cell* valc = sh->get_cell(first_row, first_col);
                if (valc)
                {
                    for (uint32_t i = first_row; i <= last_row; ++i)
                    {
                        for (uint32_t j = first_col; j <= last_col; ++j)
                        {
                            if (i != first_row || j != first_col)
                            {
                                sh->add_cell(i, j, valc->clone());
                            }
                        }
                    }
                }
            }
        }

        inline void parse_range(tinyxml2::XMLElement* dim, tinyxml2::XMLElement* shdata, Sheet* sh)
        {
            if (dim)
            {
                std::string value = dim->Attribute("ref");
                size_t index = value.find_first_of(':');
                if (index != std::string::npos)
                {
                    parse_cell(value.substr(0, index), sh->first_row, sh->first_col);
                    parse_cell(value.substr(index + 1), sh->last_row, sh->last_col);
                    return;
                }
            }
            tinyxml2::XMLElement* row = shdata->FirstChildElement("row");
            while (row)
            {
                sh->last_row = row->IntAttribute("r");
                if (sh->first_row == 0) sh->first_row = sh->last_row;
                if (sh->last_col == 0)
                {
                    sh->first_col = 1;
                    tinyxml2::XMLElement* c = row->FirstChildElement("c");
                    while (c)
                    {
                        c = c->NextSiblingElement("c");
                        sh->last_col++;
                    }
                }
                row = row->NextSiblingElement("row");
            }
        }

        mz_zip_archive archive;
        std::vector<Sheet *> excel_sheets;
        std::vector<std::string> shared_string;
        std::map<uint32_t, uint32_t> form_ids;
        std::map<uint32_t, std::string> fmt_codes;
    };
}
