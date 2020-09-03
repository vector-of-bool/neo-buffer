#pragma once

#include "../as_buffer.hpp"
#include "../as_dynamic_buffer.hpp"
#include "../buffers_cat.hpp"
#include "./copy.hpp"
#include "./size.hpp"

namespace neo {

// clang-format off
template <as_dynamic_buffer_convertible DynBuf, typename... Args>
    requires ((as_buffer_convertible<Args> || buffer_range<Args>) && ...)
constexpr std::size_t dynbuf_concat(DynBuf&& db, Args&&... args)
    noexcept(noexcept(as_dynamic_buffer(db).grow(1)))
{
    // clang-format on
    auto&& dbuf      = as_dynamic_buffer(db);
    auto&& bufs      = buffers_cat(ensure_buffer_range(args)...);
    auto   init_size = dbuf.size();
    auto   grow_size = buffer_size(bufs);
    dbuf.grow(grow_size);
    return buffer_copy(dbuf.data(init_size, grow_size), bufs);
}

}  // namespace neo
