# This cmake build is for Windows 64-bit only.
#
# Prerequisites:
#     You must have Visual Studio 2013 Update 4 installed. Start the Developer Command Prompt window that is a part of Visual Studio installation.
#     Run the build commands from within the Developer Command Prompt window to have paths to the compiler and runtime libraries set.
#     You must have git.exe in your %PATH% environment variable.
#
# To build VidarDB for Windows is as easy as 1-2-3-4-5:
#
# 1. Update paths to third-party libraries in thirdparty.inc file
# 2. Create a new directory for build artifacts
#        mkdir build
#        cd build
# 3. Run cmake to generate project files for Windows, add more options to enable required third-party libraries.
#    See thirdparty.inc for more information.
#        sample command: cmake -G "Visual Studio 12 Win64" -DGFLAGS=1 -DSNAPPY=1 -DJEMALLOC=1 -DJNI=1 ..
#        OR for VS Studio 15 cmake -G "Visual Studio 14 Win64" -DGFLAGS=1 -DSNAPPY=1 -DJEMALLOC=1 -DJNI=1 ..
# 4. Then build the project in debug mode (you may want to add /m[:<N>] flag to run msbuild in <N> parallel threads
#                                          or simply /m ot use all avail cores)
#        msbuild vidardb.sln
#
#        vidardb.sln build features exclusions of test only code in Release. If you build ALL_BUILD then everything
#        will be attempted but test only code does not build in Release mode.
#
# 5. And release mode (/m[:<N>] is also supported)
#        msbuild vidardb.sln /p:Configuration=Release
#

cmake_minimum_required(VERSION 2.6)
project(vidardb)

include(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty.inc)
execute_process(COMMAND powershell -Command "Get-Date -format MM_dd_yyyy" OUTPUT_VARIABLE DATE)
execute_process(COMMAND powershell -Command "Get-Date -format HH:mm:ss" OUTPUT_VARIABLE TIME)
string(REGEX REPLACE "(..)_(..)_..(..).*" "\\1/\\2/\\3" DATE ${DATE})
string(REGEX REPLACE "(..):(.....).*" " \\1:\\2" TIME ${TIME})
string(CONCAT GIT_DATE_TIME ${DATE} ${TIME})

find_package(Git)

if (GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(COMMAND $ENV{COMSPEC} /C ${GIT_EXECUTABLE} -C ${CMAKE_CURRENT_SOURCE_DIR} rev-parse HEAD OUTPUT_VARIABLE GIT_SHA)
else()
    set(GIT_SHA 0)
endif()

string(REGEX REPLACE "[^0-9a-f]+" "" GIT_SHA ${GIT_SHA})

set(BUILD_VERSION_CC ${CMAKE_CURRENT_SOURCE_DIR}/util/build_version.cc)

add_custom_command(OUTPUT ${BUILD_VERSION_CC}
    COMMAND echo "#include \"build_version.h\"" > ${BUILD_VERSION_CC}
    COMMAND echo "const char* vidardb_build_git_sha = \"vidardb_build_git_sha:${GIT_SHA}\";" >> ${BUILD_VERSION_CC}
    COMMAND echo "const char* vidardb_build_git_datetime = \"vidardb_build_git_datetime:${GIT_DATE_TIME}\";" >> ${BUILD_VERSION_CC}
    COMMAND echo const char* vidardb_build_compile_date = __DATE__\; >> ${BUILD_VERSION_CC}
)

add_custom_target(GenerateBuildVersion DEPENDS ${BUILD_VERSION_CC})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi /nologo  /EHsc /GS /Gd /GR /GF /fp:precise /Zc:wchar_t /Zc:forScope /errorReport:queue")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FC /d2Zi+ /W3 /WX /wd4127 /wd4800 /wd4996 /wd4351")

# Used to run CI build and tests so we can run faster
set(OPTIMIZE_DEBUG_DEFAULT 0)        # Debug build is unoptimized by default use -DOPTDBG=1 to optimize

if(DEFINED OPTDBG)
   set(OPTIMIZE_DEBUG ${OPTDBG})
else()
   set(OPTIMIZE_DEBUG ${OPTIMIZE_DEBUG_DEFAULT})
endif()

if((${OPTIMIZE_DEBUG} EQUAL 1))
   message(STATUS "Debug optimization is enabled")
   set(CMAKE_CXX_FLAGS_DEBUG "/Oxt /MDd")
else()
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /RTC1 /Gm /MDd")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oxt /Zp8 /Gm- /Gy /MD")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")

add_definitions(-DWIN32 -DOS_WIN -D_MBCS -DWIN64 -DNOMINMAX)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/third-party/gtest-1.7.0/fused-src)

set(VIDARDB_LIBS vidardblib${ARTIFACT_SUFFIX})
set(THIRDPARTY_LIBS ${THIRDPARTY_LIBS} gtest)
set(SYSTEM_LIBS ${SYSTEM_LIBS} Shlwapi.lib Rpcrt4.lib)

set(LIBS ${VIDARDB_LIBS} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})

add_subdirectory(third-party/gtest-1.7.0/fused-src/gtest)

# Main library source code
set(SOURCES
        db/auto_roll_logger.cc
        db/builder.cc
        db/c.cc
        db/column_family.cc
        db/compacted_db_impl.cc
        db/compaction.cc
        db/compaction_iterator.cc
        db/compaction_job.cc
        db/compaction_picker.cc
        db/convenience.cc
        db/dbformat.cc
        db/db_filesnapshot.cc
        db/db_impl.cc
        db/db_impl_debug.cc
        db/db_impl_experimental.cc
        db/db_impl_readonly.cc
        db/db_info_dumper.cc
        db/db_iter.cc
        db/event_helpers.cc
        db/experimental.cc
        db/filename.cc
        db/file_indexer.cc
        db/flush_job.cc
        db/flush_scheduler.cc
        db/forward_iterator.cc
        db/internal_stats.cc
        db/log_reader.cc
        db/log_writer.cc
        db/managed_iterator.cc
        memtable/memtable.cc
        memtable/memtable_allocator.cc
        memtable/memtable_list.cc
        db/merge_helper.cc
        db/merge_operator.cc
        db/repair.cc
        db/slice.cc
        db/snapshot_impl.cc
        db/table_cache.cc
        db/table_properties_collector.cc
        db/transaction_log_impl.cc
        db/version_builder.cc
        db/version_edit.cc
        db/version_set.cc
        db/wal_manager.cc
        db/write_batch.cc
        db/write_batch_base.cc
        db/write_controller.cc
        db/write_thread.cc
        db/xfunc_test_points.cc
        memtable/hash_cuckoo_rep.cc
        memtable/hash_linklist_rep.cc
        memtable/hash_skiplist_rep.cc
        memtable/skiplistrep.cc
        memtable/vectorrep.cc
        port/stack_trace.cc
        port/win/io_win.cc
        port/win/env_win.cc
        port/win/env_default.cc
        port/win/port_win.cc
        port/win/win_logger.cc
        port/win/xpress_win.cc
        table/adaptive_table_factory.cc
        table/block.cc
        table/block_based_filter_block.cc
        table/block_based_table_builder.cc
        table/block_based_table_factory.cc
        table/block_based_table_reader.cc
        table/block_builder.cc
        table/block_prefix_index.cc
        table/bloom_block.cc
        table/cuckoo_table_builder.cc
        table/cuckoo_table_factory.cc
        table/cuckoo_table_reader.cc
        table/flush_block_policy.cc
        table/format.cc
        table/full_filter_block.cc
        table/get_context.cc
        table/iterator.cc
        table/merger.cc
        table/sst_file_writer.cc
        table/meta_blocks.cc
        table/plain_table_builder.cc
        table/plain_table_factory.cc
        table/plain_table_index.cc
        table/plain_table_key_coding.cc
        table/plain_table_reader.cc
        table/persistent_cache_helper.cc
        table/table_properties.cc
        table/two_level_iterator.cc
        tools/sst_dump_tool.cc
        tools/db_bench_tool.cc
        tools/dump/db_dump_tool.cc
        util/arena.cc
        util/bloom.cc
        util/build_version.cc
        util/cache.cc
        util/coding.cc
        util/compaction_job_stats_impl.cc
        util/comparator.cc
        util/splitter.cc
        util/concurrent_arena.cc
        util/crc32c.cc
        util/delete_scheduler.cc
        util/dynamic_bloom.cc
        util/env.cc
        util/env_chroot.cc
        util/env_hdfs.cc
        util/event_logger.cc
        util/file_util.cc
        util/file_reader_writer.cc
        util/sst_file_manager_impl.cc
        util/filter_policy.cc
        util/hash.cc
        util/histogram.cc
        util/histogram_windowing.cc
        util/instrumented_mutex.cc
        util/iostats_context.cc
        tools/ldb_cmd.cc
        tools/ldb_tool.cc
        util/logging.cc
        util/log_buffer.cc
        util/memenv.cc
        util/murmurhash.cc
        util/mutable_cf_options.cc
        util/options.cc
        util/options_helper.cc
        util/options_parser.cc
        util/options_sanity_check.cc
        util/perf_context.cc
        util/perf_level.cc
        util/random.cc
        util/rate_limiter.cc
        util/slice.cc
        util/statistics.cc
        util/status.cc
        util/status_message.cc
        util/string_util.cc
        util/sync_point.cc
        util/testharness.cc
        util/testutil.cc
        util/thread_local.cc
        util/threadpool.cc
        util/thread_status_impl.cc
        util/thread_status_updater.cc
        util/thread_status_util.cc
        util/thread_status_util_debug.cc
        util/transaction_test_util.cc
        util/xfunc.cc
        util/xxhash.cc
        utilities/backupable/backupable_db.cc
        utilities/checkpoint/checkpoint.cc
        utilities/compaction_filters/remove_emptyvalue_compactionfilter.cc
        utilities/document/document_db.cc
        utilities/document/json_document.cc
        utilities/document/json_document_builder.cc
        utilities/env_mirror.cc
        utilities/env_registry.cc
        utilities/flashcache/flashcache.cc
        utilities/geodb/geodb_impl.cc
        utilities/leveldb_options/leveldb_options.cc
        utilities/memory/memory_util.cc
        utilities/merge_operators/string_append/stringappend.cc
        utilities/merge_operators/string_append/stringappend2.cc
        utilities/merge_operators/put.cc
        utilities/merge_operators/max.cc
        utilities/merge_operators/uint64add.cc
        utilities/options/options_util.cc
        utilities/persistent_cache/persistent_cache_tier.cc
        utilities/persistent_cache/volatile_tier_impl.cc
        utilities/redis/redis_lists.cc
        utilities/spatialdb/spatial_db.cc
        utilities/table_properties_collectors/compact_on_deletion_collector.cc
        utilities/transactions/optimistic_transaction_impl.cc
        utilities/transactions/optimistic_transaction_db_impl.cc
        utilities/transactions/transaction_base.cc
        utilities/transactions/transaction_impl.cc
        utilities/transactions/transaction_db_impl.cc
        utilities/transactions/transaction_db_mutex_impl.cc
        utilities/transactions/transaction_lock_mgr.cc
        utilities/transactions/transaction_util.cc
        utilities/ttl/db_ttl_impl.cc
        utilities/write_batch_with_index/write_batch_with_index.cc
        utilities/write_batch_with_index/write_batch_with_index_internal.cc
)

# For test util library that is build only in DEBUG mode
# and linked to tests. Add test only code that is not #ifdefed for Release here.
set(TESTUTIL_SOURCE
    db/db_test_util.cc
    table/mock_table.cc
    util/mock_env.cc
    util/fault_injection_test_env.cc
    util/thread_status_updater_debug.cc
)

add_library(vidardblib${ARTIFACT_SUFFIX} ${SOURCES})
set_target_properties(vidardblib${ARTIFACT_SUFFIX} PROPERTIES COMPILE_FLAGS "/Fd${CMAKE_CFG_INTDIR}/vidardblib${ARTIFACT_SUFFIX}.pdb")
add_dependencies(vidardblib${ARTIFACT_SUFFIX} GenerateBuildVersion)

add_library(vidardb${ARTIFACT_SUFFIX} SHARED ${SOURCES})
set_target_properties(vidardb${ARTIFACT_SUFFIX} PROPERTIES COMPILE_FLAGS "-DVIDARDB_DLL -DVIDARDB_LIBRARY_EXPORTS /Fd${CMAKE_CFG_INTDIR}/vidardb${ARTIFACT_SUFFIX}.pdb")
add_dependencies(vidardb${ARTIFACT_SUFFIX} GenerateBuildVersion)
target_link_libraries(vidardb${ARTIFACT_SUFFIX} ${LIBS})

if (DEFINED JNI)
  if (${JNI} EQUAL 1)
    message(STATUS "JNI library is enabled")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/java)
  else()
    message(STATUS "JNI library is disabled")
  endif()
else()
  message(STATUS "JNI library is disabled")
endif()

set(APPS
        tools/db_bench.cc
        memtable/memtablerep_bench.cc
        table/table_reader_bench.cc
        tools/db_stress.cc
        tools/write_stress.cc
        tools/db_repl_stress.cc
        tools/ldb.cc
        tools/sst_dump.cc
        tools/dump/vidardb_dump.cc
        tools/dump/vidardb_undump.cc
        util/cache_bench.cc
        utilities/persistent_cache/hash_table_bench.cc
)

set(C_TESTS db/c_test.c)

set(TESTS
        test/db/auto_roll_logger_test.cc
        test/db/column_family_test.cc
        test/db/compact_files_test.cc
        test/db/compaction_iterator_test.cc
        test/db/compaction_job_test.cc
        test/db/compaction_job_stats_test.cc
        test/db/compaction_picker_test.cc
        test/db/comparator_db_test.cc
        test/db/corruption_test.cc
        test/db/cuckoo_table_db_test.cc
        test/db/db_compaction_filter_test.cc
        test/db/db_compaction_test.cc
        test/db/db_dynamic_level_test.cc
        test/db/db_inplace_update_test.cc
        test/db/db_iter_test.cc
        test/db/db_log_iter_test.cc
        test/db/db_properties_test.cc
        test/db/db_table_properties_test.cc
        test/db/db_tailing_iter_test.cc
        test/db/db_test.cc
        test/db/db_test2.cc
        test/db/db_block_cache_test.cc
        test/db/db_bloom_filter_test.cc
        test/db/db_iterator_test.cc
        test/db/db_sst_test.cc
        test/db/db_universal_compaction_test.cc
        test/db/db_wal_test.cc
        test/db/dbformat_test.cc
        test/db/deletefile_test.cc
        test/db/fault_injection_test.cc
        test/db/file_indexer_test.cc
        test/db/filename_test.cc
        test/db/flush_job_test.cc
        test/db/inlineskiplist_test.cc
        test/db/listener_test.cc
        test/db/log_test.cc
        test/db/manual_compaction_test.cc
        test/memtable/memtable_list_test.cc
        test/db/merge_test.cc
        test/db/merge_helper_test.cc
        test/db/options_file_test.cc
        test/db/perf_context_test.cc
        test/db/plain_table_db_test.cc
        test/db/prefix_test.cc
        test/db/repair_test.cc
        test/db/skiplist_test.cc
        test/db/table_properties_collector_test.cc
        test/db/version_builder_test.cc
        test/db/version_edit_test.cc
        test/db/version_set_test.cc
        test/db/wal_manager_test.cc
        test/db/write_batch_test.cc
        test/db/write_callback_test.cc
        test/db/write_controller_test.cc
        test/db/db_io_failure_test.cc
        test/table/block_based_filter_block_test.cc
        test/table/block_test.cc
        test/table/cuckoo_table_builder_test.cc
        test/table/cuckoo_table_reader_test.cc
        test/table/full_filter_block_test.cc
        test/table/merger_test.cc
        test/table/table_test.cc
        test/tools/db_sanity_test.cc
        test/tools/ldb_cmd_test.cc
        test/tools/reduce_levels_test.cc
        test/tools/sst_dump_test.cc
        test/util/arena_test.cc
        test/util/autovector_test.cc
        test/util/bloom_test.cc
        test/util/cache_test.cc
        test/util/coding_test.cc
        test/util/crc32c_test.cc
        test/util/delete_scheduler_test.cc
        test/util/dynamic_bloom_test.cc
        test/util/env_basic_test.cc
        test/util/env_test.cc
        test/util/event_logger_test.cc
        test/util/filelock_test.cc
        test/util/file_reader_writer_test.cc
        test/util/heap_test.cc
        test/util/histogram_test.cc
        test/util/iostats_context_test.cc
        test/util/mock_env_test.cc
        test/util/options_settable_test.cc
        test/util/options_test.cc
        test/util/rate_limiter_test.cc
        test/util/slice_transform_test.cc
        test/util/thread_list_test.cc
        test/util/thread_local_test.cc
)

set(EXES ${APPS})

foreach(sourcefile ${EXES})
    string(REPLACE ".cc" "" exename ${sourcefile})
    string(REGEX REPLACE "^((.+)/)+" "" exename ${exename})
    add_executable(${exename}${ARTIFACT_SUFFIX} ${sourcefile})
    target_link_libraries(${exename}${ARTIFACT_SUFFIX} ${LIBS})
endforeach(sourcefile ${EXES})

# test utilities are only build in debug
set(TESTUTILLIB testutillib${ARTIFACT_SUFFIX})
add_library(${TESTUTILLIB} STATIC ${TESTUTIL_SOURCE})
set_target_properties(${TESTUTILLIB} PROPERTIES COMPILE_FLAGS "/Fd${CMAKE_CFG_INTDIR}/testutillib${ARTIFACT_SUFFIX}.pdb")
set_target_properties(${TESTUTILLIB}
      PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD_RELEASE 1
      EXCLUDE_FROM_DEFAULT_BUILD_MINRELEASE 1
      EXCLUDE_FROM_DEFAULT_BUILD_RELWITHDEBINFO 1
      )

# Tests are excluded from Release builds
set(TEST_EXES ${TESTS})

foreach(sourcefile ${TEST_EXES})
    string(REPLACE ".cc" "" exename ${sourcefile})
    string(REGEX REPLACE "^((.+)/)+" "" exename ${exename})
    add_executable(${exename}${ARTIFACT_SUFFIX} ${sourcefile})
    set_target_properties(${exename}${ARTIFACT_SUFFIX}
      PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD_RELEASE 1
      EXCLUDE_FROM_DEFAULT_BUILD_MINRELEASE 1
      EXCLUDE_FROM_DEFAULT_BUILD_RELWITHDEBINFO 1
      )
    target_link_libraries(${exename}${ARTIFACT_SUFFIX} ${LIBS} testutillib${ARTIFACT_SUFFIX})
endforeach(sourcefile ${TEST_EXES})

# C executables must link to a shared object
set(C_TEST_EXES ${C_TESTS})

foreach(sourcefile ${C_TEST_EXES})
    string(REPLACE ".c" "" exename ${sourcefile})
    string(REGEX REPLACE "^((.+)/)+" "" exename ${exename})
    add_executable(${exename}${ARTIFACT_SUFFIX} ${sourcefile})
    set_target_properties(${exename}${ARTIFACT_SUFFIX}
      PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD_RELEASE 1
      EXCLUDE_FROM_DEFAULT_BUILD_MINRELEASE 1
      EXCLUDE_FROM_DEFAULT_BUILD_RELWITHDEBINFO 1
      )
    target_link_libraries(${exename}${ARTIFACT_SUFFIX} vidardb${ARTIFACT_SUFFIX} testutillib${ARTIFACT_SUFFIX})
endforeach(sourcefile ${C_TEST_EXES})
