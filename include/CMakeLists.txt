set(EXEC_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/exec)
set(EXEC_DETAILS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/exec)

target_sources(Exec
INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
        ${EXEC_DETAILS_HEADER_DIR}/as_tuple.hpp
        ${EXEC_DETAILS_HEADER_DIR}/conditional_meta_apply.hpp
        ${EXEC_DETAILS_HEADER_DIR}/decayed_tuple.hpp
        ${EXEC_DETAILS_HEADER_DIR}/default_completion_signatures.hpp
        ${EXEC_DETAILS_HEADER_DIR}/gather_signatures.hpp
        ${EXEC_DETAILS_HEADER_DIR}/indirect_meta_apply.hpp
        ${EXEC_DETAILS_HEADER_DIR}/meta_add.hpp
        ${EXEC_DETAILS_HEADER_DIR}/meta_append.hpp
        ${EXEC_DETAILS_HEADER_DIR}/meta_bind.hpp
        ${EXEC_DETAILS_HEADER_DIR}/meta_filter.hpp
        ${EXEC_DETAILS_HEADER_DIR}/meta_index.hpp
        ${EXEC_DETAILS_HEADER_DIR}/meta_merge.hpp
        ${EXEC_DETAILS_HEADER_DIR}/meta_reverse.hpp
        ${EXEC_DETAILS_HEADER_DIR}/select_signatures_by_tag.hpp
        ${EXEC_DETAILS_HEADER_DIR}/signature_info.hpp
        ${EXEC_DETAILS_HEADER_DIR}/sync_wait_state.hpp
        ${EXEC_DETAILS_HEADER_DIR}/type_holder.hpp
        ${EXEC_DETAILS_HEADER_DIR}/type_list.hpp
        ${EXEC_DETAILS_HEADER_DIR}/unique_template.hpp
        ${EXEC_DETAILS_HEADER_DIR}/valid_completion_signatures.hpp
        ${EXEC_DETAILS_HEADER_DIR}/variant_or_empty.hpp

        ${EXEC_HEADER_DIR}/completion_signatures.hpp
        ${EXEC_HEADER_DIR}/completions.hpp
        ${EXEC_HEADER_DIR}/continues_on.hpp
        ${EXEC_HEADER_DIR}/env.hpp
        ${EXEC_HEADER_DIR}/just.hpp
        ${EXEC_HEADER_DIR}/let.hpp
        ${EXEC_HEADER_DIR}/operation_state.hpp
        ${EXEC_HEADER_DIR}/pipe_adapter.hpp
        ${EXEC_HEADER_DIR}/query.hpp
        ${EXEC_HEADER_DIR}/receiver.hpp
        ${EXEC_HEADER_DIR}/run_loop.hpp
        ${EXEC_HEADER_DIR}/schedule_from.hpp
        ${EXEC_HEADER_DIR}/scheduler.hpp
        ${EXEC_HEADER_DIR}/sender.hpp
        ${EXEC_HEADER_DIR}/starts_on.hpp
        ${EXEC_HEADER_DIR}/stop_token.hpp
        ${EXEC_HEADER_DIR}/sync_wait.hpp
        ${EXEC_HEADER_DIR}/then.hpp
        ${EXEC_HEADER_DIR}/transform_completion_signatures.hpp

        ${CMAKE_CURRENT_SOURCE_DIR}/exec.hpp
)