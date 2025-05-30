set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(qp_cpp_posix_lib 
		STATIC 
		qpcpp/src/qf/qep_hsm.cpp
		qpcpp/src/qf/qep_msm.cpp
		qpcpp/src/qf/qf_act.cpp
		qpcpp/src/qf/qf_actq.cpp
		qpcpp/src/qf/qf_defer.cpp
		qpcpp/src/qf/qf_dyn.cpp
		qpcpp/src/qf/qf_mem.cpp
		qpcpp/src/qf/qf_ps.cpp
		qpcpp/src/qf/qf_qact.cpp
		qpcpp/src/qf/qf_qeq.cpp
		qpcpp/src/qf/qf_qmact.cpp
		qpcpp/src/qf/qf_time.cpp

		qpcpp/ports/posix/qf_port.cpp
		qpcpp/include/qstamp.cpp
)

target_compile_options(
	qp_cpp_posix_lib
	PUBLIC
		-c
		-g
		-O
	#	-fno-pie # TODO We do not know what kind of optimizations will be needed, these flags will remain just in case.
				# If the node meets the requirements, delete them in the release.
		-pedantic
		-Wall
		-Wextra
	#	-fno-rtti # we need rtti
	#	-fno-exceptions
		-fPIC # is it necessary
)

target_compile_definitions(qp_cpp_posix_lib PUBLIC QP_API_VERSION=9999)

target_include_directories(
	qp_cpp_posix_lib
	PUBLIC
	qpcpp/include
	qpcpp/src
	qpcpp/ports/posix
)

target_sources(
	qp_cpp_posix_lib
	PUBLIC
	
)

target_link_libraries(
	qp_cpp_posix_lib
	PUBLIC
	Threads::Threads
)