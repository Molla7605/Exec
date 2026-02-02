#ifndef EXEC_DETAILS_META_MERGE_HPP
#define EXEC_DETAILS_META_MERGE_HPP

#include "exec/details/meta_add.hpp"
#include "exec/details/type_list.hpp"
#include "exec/details/unique_template.hpp"

namespace exec::details {
    template<valid_type_list... Ts>
    using meta_merge_t = meta_unique_t<meta_add_t<Ts...>>;
}

#endif // !EXEC_DETAILS_META_MERGE_HPP