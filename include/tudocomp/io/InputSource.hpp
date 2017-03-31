#pragma once

#include <iostream>
#include <string>
#include <tudocomp/util/View.hpp>

namespace tdc {namespace io {
    class InputSource {
    public:
        enum class Content {
            View,
            File,
            Stream
        };
    private:
        Content       m_content;

        View          m_view = ""_v;
        std::string   m_path = "";
        std::istream* m_stream = nullptr;
    public:
        friend inline bool operator==(const InputSource&, const InputSource&);

        inline InputSource(const std::string& path):
            m_content(Content::File),
            m_path(path) {}
        inline InputSource(const View& view):
            m_content(Content::View),
            m_view(view) {}
        inline InputSource(std::istream* stream):
            m_content(Content::Stream),
            m_stream(stream) {}

        inline bool is_view() const { return m_content == Content::View; }
        inline bool is_stream() const { return m_content == Content::Stream; }
        inline bool is_file() const { return m_content == Content::File; }

        inline const View& view()        const { return m_view; }
        inline std::istream* stream()    const { return m_stream; }
        inline const std::string& file() const { return m_path; }
    };

    inline bool operator==(const InputSource& lhs, const InputSource& rhs) {
        return lhs.m_content == rhs.m_content
            && lhs.m_view.data() == rhs.m_view.data()
            && lhs.m_view.size() == rhs.m_view.size()
            && lhs.m_path == rhs.m_path
            && lhs.m_stream == rhs.m_stream;
    };

    inline std::ostream& operator<<(std::ostream& o, const InputSource& v) {
        if (v.is_view()) {
            return o << "{ view:   " << std::hex << size_t(v.view().data()) << std::dec << " }";
        }
        if (v.is_stream()) {
            return o << "{ stream: " << std::hex << size_t(v.stream()) << std::dec << " }";
        }
        if (v.is_file()) {
            return o << "{ file:   " << v.file() << " }";
        }
        return o;
    }
}}
