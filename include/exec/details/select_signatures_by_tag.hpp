#ifndef EXEC_DETAILS_SELECT_SIGNATURE_BY_TAG_HPP
#define EXEC_DETAILS_SELECT_SIGNATURE_BY_TAG_HPP

#include "exec/completion_signatures.hpp"

#include "exec/details/meta_filter.hpp"
#include "exec/details/signature_info.hpp"
#include "exec/details/valid_completion_signatures.hpp"

namespace exec::details {
    template<typename TagT, typename SigT>
    struct has_same_tag : std::is_same<TagT, completion_tag_of_t<completion_signatures<SigT>>> {};

    template<typename TagT, valid_completion_signatures SignatureT>
    using select_signatures_by_tag_t = meta_filter_t<TagT, SignatureT, has_same_tag>;
}

#endif // !EXEC_DETAILS_SELECT_SIGNATURE_BY_TAG_HPP