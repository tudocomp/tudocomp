#pragma once

#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

namespace tdc {
    class ASCIITable {
    private:
        static inline std::string wrap(std::string& s, size_t width) {
            size_t end = 0;

            size_t x = s.find_first_of(" \t");
            while(x != std::string::npos && x <= width) {
                end = x;
                x = s.find_first_of(" \t", x+1);
            }

            if(x == std::string::npos) {
                end = std::min(s.length(), width);
            }

            size_t nl = s.find('\n');
            if(nl != std::string::npos) end = std::min(end, nl);

            auto r = (end > 0) ? s.substr(0, end) : std::string();
            s = (end+1 < s.length()) ? s.substr(end+1) : std::string();
            return r;
        }

        struct column_t {
            std::string title;
            size_t      width;
            bool        align_left;
        };

        size_t m_max_width;
        size_t m_width;

        std::vector<column_t> m_columns;
        std::vector<std::vector<std::string>> m_rows;

        inline void print_row(std::ostream& out, std::string* row) const {
            bool empty;
            do {
                empty = true;
                for(size_t i = 0; i < m_columns.size(); i++) {
                    if(i > 0) out << " | ";
                    auto w = m_columns[i].width;
                    auto s = wrap(row[i], m_columns[i].width);
                    out << std::setw(w)
                        << (m_columns[i].align_left ? std::left : std::right)
                        << s;
                    empty = empty && row[i].empty();
                }
                out << std::endl;
            } while(!empty);
        }

    public:
        inline ASCIITable(size_t max_width = 80)
            : m_max_width(max_width), m_width(0) {
        }

        inline void add_column(
            const std::string& title,
            size_t width,
            bool align_left = false) {

            if(width == 0) {
                width = m_max_width - m_width - 3;
            }

            if(title.length() > width) {
                throw std::runtime_error("column title too long");
            }

            if(m_width + 3 <= m_max_width) {
                m_width += width + 3;
                m_columns.emplace_back(column_t{title, width, align_left});
            } else {
                throw std::runtime_error("column too wide");
            }
        }

        inline void add_row(std::initializer_list<std::string> cols) {
            m_rows.emplace_back(cols);
        }

        inline void print(std::ostream& out) const {
            const size_t num_cols = m_columns.size();
            std::string* row = new std::string[num_cols];

            // header
            for(size_t i = 0; i < num_cols; i++) {
                row[i] = m_columns[i].title;
            }
            print_row(out, row);

            // line
            for(size_t i = 0; i < m_max_width; i++) {
                out << "-";
            }
            out << std::endl;

            // rows
            for(auto& r : m_rows) {
                for(size_t i = 0; i < num_cols; i++) {
                    if(i < r.size()) row[i] = r[i];
                }
                print_row(out, row);
            }

            // clean up
            delete[] row;
        }
    };
}
