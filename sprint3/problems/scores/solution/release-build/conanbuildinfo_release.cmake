
#################
###  BOOST
#################
set(CONAN_BOOST_ROOT_RELEASE "/home/donkey/.conan/data/boost/1.78.0/_/_/package/dc8aedd23a0f0a773a5fcdcfe1ae3e89c4205978")
set(CONAN_INCLUDE_DIRS_BOOST_RELEASE "/home/donkey/.conan/data/boost/1.78.0/_/_/package/dc8aedd23a0f0a773a5fcdcfe1ae3e89c4205978/include")
set(CONAN_LIB_DIRS_BOOST_RELEASE "/home/donkey/.conan/data/boost/1.78.0/_/_/package/dc8aedd23a0f0a773a5fcdcfe1ae3e89c4205978/lib")
set(CONAN_BIN_DIRS_BOOST_RELEASE )
set(CONAN_RES_DIRS_BOOST_RELEASE )
set(CONAN_SRC_DIRS_BOOST_RELEASE )
set(CONAN_BUILD_DIRS_BOOST_RELEASE )
set(CONAN_FRAMEWORK_DIRS_BOOST_RELEASE )
set(CONAN_LIBS_BOOST_RELEASE boost_contract boost_coroutine boost_fiber_numa boost_fiber boost_context boost_graph boost_iostreams boost_json boost_locale boost_log_setup boost_log boost_math_c99 boost_math_c99f boost_math_c99l boost_math_tr1 boost_math_tr1f boost_math_tr1l boost_nowide boost_program_options boost_random boost_regex boost_stacktrace_addr2line boost_stacktrace_backtrace boost_stacktrace_basic boost_stacktrace_noop boost_timer boost_type_erasure boost_thread boost_chrono boost_container boost_date_time boost_unit_test_framework boost_prg_exec_monitor boost_test_exec_monitor boost_exception boost_wave boost_filesystem boost_atomic boost_wserialization boost_serialization)
set(CONAN_PKG_LIBS_BOOST_RELEASE boost_contract boost_coroutine boost_fiber_numa boost_fiber boost_context boost_graph boost_iostreams boost_json boost_locale boost_log_setup boost_log boost_math_c99 boost_math_c99f boost_math_c99l boost_math_tr1 boost_math_tr1f boost_math_tr1l boost_nowide boost_program_options boost_random boost_regex boost_stacktrace_addr2line boost_stacktrace_backtrace boost_stacktrace_basic boost_stacktrace_noop boost_timer boost_type_erasure boost_thread boost_chrono boost_container boost_date_time boost_unit_test_framework boost_prg_exec_monitor boost_test_exec_monitor boost_exception boost_wave boost_filesystem boost_atomic boost_wserialization boost_serialization)
set(CONAN_SYSTEM_LIBS_BOOST_RELEASE dl rt pthread)
set(CONAN_FRAMEWORKS_BOOST_RELEASE )
set(CONAN_FRAMEWORKS_FOUND_BOOST_RELEASE "")  # Will be filled later
set(CONAN_DEFINES_BOOST_RELEASE "-DBOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"-DBOOST_STACKTRACE_USE_ADDR2LINE"
			"-DBOOST_STACKTRACE_USE_BACKTRACE"
			"-DBOOST_STACKTRACE_USE_NOOP")
set(CONAN_BUILD_MODULES_PATHS_BOOST_RELEASE )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_BOOST_RELEASE "BOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"BOOST_STACKTRACE_USE_ADDR2LINE"
			"BOOST_STACKTRACE_USE_BACKTRACE"
			"BOOST_STACKTRACE_USE_NOOP")

set(CONAN_C_FLAGS_BOOST_RELEASE "")
set(CONAN_CXX_FLAGS_BOOST_RELEASE "")
set(CONAN_SHARED_LINKER_FLAGS_BOOST_RELEASE "")
set(CONAN_EXE_LINKER_FLAGS_BOOST_RELEASE "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_BOOST_RELEASE_LIST "")
set(CONAN_CXX_FLAGS_BOOST_RELEASE_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_BOOST_RELEASE_LIST "")
set(CONAN_EXE_LINKER_FLAGS_BOOST_RELEASE_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_BOOST_RELEASE "${CONAN_FRAMEWORKS_BOOST_RELEASE}" "_BOOST" "_RELEASE")
# Append to aggregated values variable
set(CONAN_LIBS_BOOST_RELEASE ${CONAN_PKG_LIBS_BOOST_RELEASE} ${CONAN_SYSTEM_LIBS_BOOST_RELEASE} ${CONAN_FRAMEWORKS_FOUND_BOOST_RELEASE})


#################
###  ZLIB
#################
set(CONAN_ZLIB_ROOT_RELEASE "/home/donkey/.conan/data/zlib/1.3.1/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646")
set(CONAN_INCLUDE_DIRS_ZLIB_RELEASE "/home/donkey/.conan/data/zlib/1.3.1/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/include")
set(CONAN_LIB_DIRS_ZLIB_RELEASE "/home/donkey/.conan/data/zlib/1.3.1/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/lib")
set(CONAN_BIN_DIRS_ZLIB_RELEASE )
set(CONAN_RES_DIRS_ZLIB_RELEASE )
set(CONAN_SRC_DIRS_ZLIB_RELEASE )
set(CONAN_BUILD_DIRS_ZLIB_RELEASE "/home/donkey/.conan/data/zlib/1.3.1/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/")
set(CONAN_FRAMEWORK_DIRS_ZLIB_RELEASE )
set(CONAN_LIBS_ZLIB_RELEASE z)
set(CONAN_PKG_LIBS_ZLIB_RELEASE z)
set(CONAN_SYSTEM_LIBS_ZLIB_RELEASE )
set(CONAN_FRAMEWORKS_ZLIB_RELEASE )
set(CONAN_FRAMEWORKS_FOUND_ZLIB_RELEASE "")  # Will be filled later
set(CONAN_DEFINES_ZLIB_RELEASE )
set(CONAN_BUILD_MODULES_PATHS_ZLIB_RELEASE )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_ZLIB_RELEASE )

set(CONAN_C_FLAGS_ZLIB_RELEASE "")
set(CONAN_CXX_FLAGS_ZLIB_RELEASE "")
set(CONAN_SHARED_LINKER_FLAGS_ZLIB_RELEASE "")
set(CONAN_EXE_LINKER_FLAGS_ZLIB_RELEASE "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_ZLIB_RELEASE_LIST "")
set(CONAN_CXX_FLAGS_ZLIB_RELEASE_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_ZLIB_RELEASE_LIST "")
set(CONAN_EXE_LINKER_FLAGS_ZLIB_RELEASE_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_ZLIB_RELEASE "${CONAN_FRAMEWORKS_ZLIB_RELEASE}" "_ZLIB" "_RELEASE")
# Append to aggregated values variable
set(CONAN_LIBS_ZLIB_RELEASE ${CONAN_PKG_LIBS_ZLIB_RELEASE} ${CONAN_SYSTEM_LIBS_ZLIB_RELEASE} ${CONAN_FRAMEWORKS_FOUND_ZLIB_RELEASE})


#################
###  BZIP2
#################
set(CONAN_BZIP2_ROOT_RELEASE "/home/donkey/.conan/data/bzip2/1.0.8/_/_/package/c32092bf4d4bb47cf962af898e02823f499b017e")
set(CONAN_INCLUDE_DIRS_BZIP2_RELEASE "/home/donkey/.conan/data/bzip2/1.0.8/_/_/package/c32092bf4d4bb47cf962af898e02823f499b017e/include")
set(CONAN_LIB_DIRS_BZIP2_RELEASE "/home/donkey/.conan/data/bzip2/1.0.8/_/_/package/c32092bf4d4bb47cf962af898e02823f499b017e/lib")
set(CONAN_BIN_DIRS_BZIP2_RELEASE "/home/donkey/.conan/data/bzip2/1.0.8/_/_/package/c32092bf4d4bb47cf962af898e02823f499b017e/bin")
set(CONAN_RES_DIRS_BZIP2_RELEASE )
set(CONAN_SRC_DIRS_BZIP2_RELEASE )
set(CONAN_BUILD_DIRS_BZIP2_RELEASE "/home/donkey/.conan/data/bzip2/1.0.8/_/_/package/c32092bf4d4bb47cf962af898e02823f499b017e/")
set(CONAN_FRAMEWORK_DIRS_BZIP2_RELEASE )
set(CONAN_LIBS_BZIP2_RELEASE bz2)
set(CONAN_PKG_LIBS_BZIP2_RELEASE bz2)
set(CONAN_SYSTEM_LIBS_BZIP2_RELEASE )
set(CONAN_FRAMEWORKS_BZIP2_RELEASE )
set(CONAN_FRAMEWORKS_FOUND_BZIP2_RELEASE "")  # Will be filled later
set(CONAN_DEFINES_BZIP2_RELEASE )
set(CONAN_BUILD_MODULES_PATHS_BZIP2_RELEASE )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_BZIP2_RELEASE )

set(CONAN_C_FLAGS_BZIP2_RELEASE "")
set(CONAN_CXX_FLAGS_BZIP2_RELEASE "")
set(CONAN_SHARED_LINKER_FLAGS_BZIP2_RELEASE "")
set(CONAN_EXE_LINKER_FLAGS_BZIP2_RELEASE "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_BZIP2_RELEASE_LIST "")
set(CONAN_CXX_FLAGS_BZIP2_RELEASE_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_BZIP2_RELEASE_LIST "")
set(CONAN_EXE_LINKER_FLAGS_BZIP2_RELEASE_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_BZIP2_RELEASE "${CONAN_FRAMEWORKS_BZIP2_RELEASE}" "_BZIP2" "_RELEASE")
# Append to aggregated values variable
set(CONAN_LIBS_BZIP2_RELEASE ${CONAN_PKG_LIBS_BZIP2_RELEASE} ${CONAN_SYSTEM_LIBS_BZIP2_RELEASE} ${CONAN_FRAMEWORKS_FOUND_BZIP2_RELEASE})


#################
###  LIBBACKTRACE
#################
set(CONAN_LIBBACKTRACE_ROOT_RELEASE "/home/donkey/.conan/data/libbacktrace/cci.20210118/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646")
set(CONAN_INCLUDE_DIRS_LIBBACKTRACE_RELEASE "/home/donkey/.conan/data/libbacktrace/cci.20210118/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/include")
set(CONAN_LIB_DIRS_LIBBACKTRACE_RELEASE "/home/donkey/.conan/data/libbacktrace/cci.20210118/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/lib")
set(CONAN_BIN_DIRS_LIBBACKTRACE_RELEASE )
set(CONAN_RES_DIRS_LIBBACKTRACE_RELEASE )
set(CONAN_SRC_DIRS_LIBBACKTRACE_RELEASE )
set(CONAN_BUILD_DIRS_LIBBACKTRACE_RELEASE "/home/donkey/.conan/data/libbacktrace/cci.20210118/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/")
set(CONAN_FRAMEWORK_DIRS_LIBBACKTRACE_RELEASE )
set(CONAN_LIBS_LIBBACKTRACE_RELEASE backtrace)
set(CONAN_PKG_LIBS_LIBBACKTRACE_RELEASE backtrace)
set(CONAN_SYSTEM_LIBS_LIBBACKTRACE_RELEASE )
set(CONAN_FRAMEWORKS_LIBBACKTRACE_RELEASE )
set(CONAN_FRAMEWORKS_FOUND_LIBBACKTRACE_RELEASE "")  # Will be filled later
set(CONAN_DEFINES_LIBBACKTRACE_RELEASE )
set(CONAN_BUILD_MODULES_PATHS_LIBBACKTRACE_RELEASE )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_LIBBACKTRACE_RELEASE )

set(CONAN_C_FLAGS_LIBBACKTRACE_RELEASE "")
set(CONAN_CXX_FLAGS_LIBBACKTRACE_RELEASE "")
set(CONAN_SHARED_LINKER_FLAGS_LIBBACKTRACE_RELEASE "")
set(CONAN_EXE_LINKER_FLAGS_LIBBACKTRACE_RELEASE "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_LIBBACKTRACE_RELEASE_LIST "")
set(CONAN_CXX_FLAGS_LIBBACKTRACE_RELEASE_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_LIBBACKTRACE_RELEASE_LIST "")
set(CONAN_EXE_LINKER_FLAGS_LIBBACKTRACE_RELEASE_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_LIBBACKTRACE_RELEASE "${CONAN_FRAMEWORKS_LIBBACKTRACE_RELEASE}" "_LIBBACKTRACE" "_RELEASE")
# Append to aggregated values variable
set(CONAN_LIBS_LIBBACKTRACE_RELEASE ${CONAN_PKG_LIBS_LIBBACKTRACE_RELEASE} ${CONAN_SYSTEM_LIBS_LIBBACKTRACE_RELEASE} ${CONAN_FRAMEWORKS_FOUND_LIBBACKTRACE_RELEASE})


### Definition of global aggregated variables ###

set(CONAN_DEPENDENCIES_RELEASE boost zlib bzip2 libbacktrace)

set(CONAN_INCLUDE_DIRS_RELEASE "/home/donkey/.conan/data/boost/1.78.0/_/_/package/dc8aedd23a0f0a773a5fcdcfe1ae3e89c4205978/include"
			"/home/donkey/.conan/data/zlib/1.3.1/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/include"
			"/home/donkey/.conan/data/bzip2/1.0.8/_/_/package/c32092bf4d4bb47cf962af898e02823f499b017e/include"
			"/home/donkey/.conan/data/libbacktrace/cci.20210118/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/include" ${CONAN_INCLUDE_DIRS_RELEASE})
set(CONAN_LIB_DIRS_RELEASE "/home/donkey/.conan/data/boost/1.78.0/_/_/package/dc8aedd23a0f0a773a5fcdcfe1ae3e89c4205978/lib"
			"/home/donkey/.conan/data/zlib/1.3.1/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/lib"
			"/home/donkey/.conan/data/bzip2/1.0.8/_/_/package/c32092bf4d4bb47cf962af898e02823f499b017e/lib"
			"/home/donkey/.conan/data/libbacktrace/cci.20210118/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/lib" ${CONAN_LIB_DIRS_RELEASE})
set(CONAN_BIN_DIRS_RELEASE "/home/donkey/.conan/data/bzip2/1.0.8/_/_/package/c32092bf4d4bb47cf962af898e02823f499b017e/bin" ${CONAN_BIN_DIRS_RELEASE})
set(CONAN_RES_DIRS_RELEASE  ${CONAN_RES_DIRS_RELEASE})
set(CONAN_FRAMEWORK_DIRS_RELEASE  ${CONAN_FRAMEWORK_DIRS_RELEASE})
set(CONAN_LIBS_RELEASE boost_contract boost_coroutine boost_fiber_numa boost_fiber boost_context boost_graph boost_iostreams boost_json boost_locale boost_log_setup boost_log boost_math_c99 boost_math_c99f boost_math_c99l boost_math_tr1 boost_math_tr1f boost_math_tr1l boost_nowide boost_program_options boost_random boost_regex boost_stacktrace_addr2line boost_stacktrace_backtrace boost_stacktrace_basic boost_stacktrace_noop boost_timer boost_type_erasure boost_thread boost_chrono boost_container boost_date_time boost_unit_test_framework boost_prg_exec_monitor boost_test_exec_monitor boost_exception boost_wave boost_filesystem boost_atomic boost_wserialization boost_serialization z bz2 backtrace ${CONAN_LIBS_RELEASE})
set(CONAN_PKG_LIBS_RELEASE boost_contract boost_coroutine boost_fiber_numa boost_fiber boost_context boost_graph boost_iostreams boost_json boost_locale boost_log_setup boost_log boost_math_c99 boost_math_c99f boost_math_c99l boost_math_tr1 boost_math_tr1f boost_math_tr1l boost_nowide boost_program_options boost_random boost_regex boost_stacktrace_addr2line boost_stacktrace_backtrace boost_stacktrace_basic boost_stacktrace_noop boost_timer boost_type_erasure boost_thread boost_chrono boost_container boost_date_time boost_unit_test_framework boost_prg_exec_monitor boost_test_exec_monitor boost_exception boost_wave boost_filesystem boost_atomic boost_wserialization boost_serialization z bz2 backtrace ${CONAN_PKG_LIBS_RELEASE})
set(CONAN_SYSTEM_LIBS_RELEASE dl rt pthread ${CONAN_SYSTEM_LIBS_RELEASE})
set(CONAN_FRAMEWORKS_RELEASE  ${CONAN_FRAMEWORKS_RELEASE})
set(CONAN_FRAMEWORKS_FOUND_RELEASE "")  # Will be filled later
set(CONAN_DEFINES_RELEASE "-DBOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"-DBOOST_STACKTRACE_USE_ADDR2LINE"
			"-DBOOST_STACKTRACE_USE_BACKTRACE"
			"-DBOOST_STACKTRACE_USE_NOOP" ${CONAN_DEFINES_RELEASE})
set(CONAN_BUILD_MODULES_PATHS_RELEASE  ${CONAN_BUILD_MODULES_PATHS_RELEASE})
set(CONAN_CMAKE_MODULE_PATH_RELEASE "/home/donkey/.conan/data/zlib/1.3.1/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/"
			"/home/donkey/.conan/data/bzip2/1.0.8/_/_/package/c32092bf4d4bb47cf962af898e02823f499b017e/"
			"/home/donkey/.conan/data/libbacktrace/cci.20210118/_/_/package/dfbe50feef7f3c6223a476cd5aeadb687084a646/" ${CONAN_CMAKE_MODULE_PATH_RELEASE})

set(CONAN_CXX_FLAGS_RELEASE " ${CONAN_CXX_FLAGS_RELEASE}")
set(CONAN_SHARED_LINKER_FLAGS_RELEASE " ${CONAN_SHARED_LINKER_FLAGS_RELEASE}")
set(CONAN_EXE_LINKER_FLAGS_RELEASE " ${CONAN_EXE_LINKER_FLAGS_RELEASE}")
set(CONAN_C_FLAGS_RELEASE " ${CONAN_C_FLAGS_RELEASE}")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_RELEASE "${CONAN_FRAMEWORKS_RELEASE}" "" "_RELEASE")
# Append to aggregated values variable: Use CONAN_LIBS instead of CONAN_PKG_LIBS to include user appended vars
set(CONAN_LIBS_RELEASE ${CONAN_LIBS_RELEASE} ${CONAN_SYSTEM_LIBS_RELEASE} ${CONAN_FRAMEWORKS_FOUND_RELEASE})
