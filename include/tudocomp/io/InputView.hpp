#pragma once

namespace tdc {namespace io {
    /// \cond INTERNAL
    class InputViewInternal {
        InputAllocChunkHandle m_handle;

        friend class InputView;
        friend class Input;

        inline InputViewInternal(const InputViewInternal& other) = delete;
        inline InputViewInternal() = delete;

        inline InputViewInternal(InputViewInternal&& s):
            m_handle(std::move(s.m_handle)) {}

        inline InputViewInternal(const InputAllocChunkHandle& handle):
            m_handle(handle) {}
    };
    /// \endcond

    /// \brief Provides a view on the input that allows for random access.
    ///
    /// \sa View.
    class InputView: InputViewInternal, public View {
        friend class Input;

        inline InputView(InputViewInternal&& mem):
            InputViewInternal(std::move(mem)),
            View(m_handle->view()) {}
    public:
        /// Move constructor.
        inline InputView(InputView&& mem):
            InputViewInternal(std::move(mem)),
            View(std::move(mem)) {}

        /// Copy constructor (deleted).
        inline InputView(const InputView& other) = delete;

        /// Default constructor (deleted).
        inline InputView() = delete;
    };

    inline InputView Input::Variant::as_view() const {
        return InputView {
            alloc().find_or_construct(source(), from(), to(), restrictions())
        };
    }

    inline std::ostream& operator<<(std::ostream& o, const InputView& iv) {
        return o << (static_cast<const View&>(iv));
    }

}}
