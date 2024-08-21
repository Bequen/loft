#pragma once

#include <signal.h>
#include <utility>

#define EXPECT(expression,msg) if(!(expression)) { throw std::runtime_error(msg); }

enum Result {
	RESULT_OK,
	RESULT_INVALID_ARGUMENT,
	RESULT_DRW_FAILURE,
	RESULT_GPU_ALLOCATION_FAIL,
	RESULT_GPU_RESOURCE_FAIL,
	RESULT_GPU_IMAGE_MEMORY_FAIL,
    RESULT_DRW_FAILED_CREATE_SHADER,
	RESULT_GPU_DEVICE_CREATION_FAILED,
    RESULT_GPU_COMMAND_POOL_CREATION_FAILED,
    RESULT_GPU_DESCRIPTOR_POOL_CREATION_FAILED,
	RESULT_NO_AVAILABLE_GPU,
};

/**
 * Result. Returns either TResult or TError
 */
template<typename TResult, typename TError>
union result {
private:
	bool m_isOk;

	struct {
		bool m_isOk;
		TResult m_result;
	} m_result;

	struct {
		bool m_isOk;
		TError m_error;
	} m_error;



public:

    result(TResult result) :
            m_result({
                             .m_isOk = true,
                             .m_result = result
                     }) {

    }

    ~result() {

    }

	static result ok(TResult value) { return std::move(result(value)); }

	static result err(TError err) { return { .m_error = err }; }

	void expect(const char* msg) { raise(SIGINT); }

	bool is_ok() const { return m_isOk; }
};
