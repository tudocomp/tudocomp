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

        std::string m_separator;
        size_t m_max_width;
        size_t m_width;

        std::vector<column_t> m_columns;
        std::vector<std::vector<std::string>> m_rows;

        inline void print_row(std::ostream& out, std::string* row) const {
            bool first_wrap_row = true;
            bool empty;
            do {
                empty = true;
                for(size_t i = 0; i < m_columns.size(); i++) {
                    if(i > 0) {
                        if(first_wrap_row) out << m_separator;
                        else out << std::setw(m_separator.length()) << "";
                    }

                    auto w = m_columns[i].width;
                    auto s = wrap(row[i], m_columns[i].width);
                    out << std::setw(w)
                        << (m_columns[i].align_left ? std::left : std::right)
                        << s;
                    empty = empty && row[i].empty();
                }
                out << std::endl;
                first_wrap_row = false;
            } while(!empty);
        }

        inline void print_hline(std::ostream& out) const {
            for(size_t i = 0; i < m_max_width; i++) {
                out << "-";
            }
            out << std::endl;
        }

    public:
        inline ASCIITable(
            size_t max_width = 80, const std::string& separator = " | ")
            : m_separator(separator), m_max_width(max_width), m_width(0) {
        }

        inline void add_column(
            const std::string& title,
            size_t width,
            bool align_left = false) {

            if(width == 0) {
                width = m_max_width - m_width - m_separator.length();
            }

            if(title.length() > width) {
                throw std::runtime_error("column title too long");
            }

            if(m_width + m_separator.length() <= m_max_width) {
                m_width += width + m_separator.length();
                m_columns.emplace_back(column_t{title, width, align_left});
            } else {
                throw std::runtime_error("column too wide");
            }
        }

        inline void add_row(std::initializer_list<std::string> cols) {
            m_rows.emplace_back(cols);
        }

        inline void print(std::ostream& out,
            bool print_header = true,
            bool print_head_line = true,
            bool print_foot_line = true) const {

            const size_t num_cols = m_columns.size();
            std::string* row = new std::string[num_cols];

            if(print_header) {
                // header
                for(size_t i = 0; i < num_cols; i++) {
                    row[i] = m_columns[i].title;
                }
                print_row(out, row);
            }

            if(print_head_line) print_hline(out);

            // rows
            for(auto& r : m_rows) {
                for(size_t i = 0; i < num_cols; i++) {
                    if(i < r.size()) row[i] = r[i];
                }
                print_row(out, row);
            }

            if(print_foot_line) print_hline(out);

            // clean up
            delete[] row;
        }
    };
}
