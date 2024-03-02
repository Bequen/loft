#pragma once

template<typename TValue, typename TError>
class result {
private:
    bool m_isSuccess;

    union {
        TValue m_value;
        TError m_error;
    } m_value;

public:
    result<TValue, TError>() : m_isSuccess(false) {

    }

    void set_ok(TValue value) {
        m_value.m_value = value;
        m_isSuccess = true;
    }

    void set_fail(TError err) {
        m_value.m_error = err;
        m_isSuccess = false;
    }
};

template<typename TSuccess, typename TError>
result<TSuccess, TError> fail(TError failure) {
    auto r = result<TSuccess, TError>();
    r.set_fail(failure);
    return r;
}

template<typename TSuccess, typename TError>
result<TSuccess, TError> ok(TSuccess value) {
    auto r = result<TSuccess, TError>();
    r.set_ok(value);
    return r;
}
