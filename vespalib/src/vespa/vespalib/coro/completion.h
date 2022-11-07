// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "lazy.h"
#include "detached.h"
#include "received.h"

#include <coroutine>
#include <exception>
#include <future>
#include <type_traits>

namespace vespalib::coro {

// Resume/start the coroutine responsible for calculating the result
// and signal the receiver when it completes or fails. Note that the
// detached coroutine will own both the coroutine calculating the
// result and the receiver that is later notified of the result. The
// detached coroutine will automatically self-destroy when it returns,
// thereby also destroying the value and receiver. This is the
// fundamental building block used to adapt the asynchronous result of
// a coroutine with external code. This also closely models abstract
// execution where the coroutine represented by Lazy<T> is the
// sender. Execution parameters can be encapsulated inside Lazy<T>
// using composition (for example which executor should run the
// coroutine).

template <typename T, typename R>
Detached connect_resume(Lazy<T> value, R receiver) {
    try {
        receiver.set_value(co_await std::move(value));
    } catch (...) {
        receiver.set_error(std::current_exception());
    }
}

// replace Lazy<T> with std::future<T> to be able to synchronously
// wait for its completion. Implemented in terms of connect_resume.

template <typename T>
std::future<T> make_future(Lazy<T> value) {
    struct receiver {
        std::promise<T> promise;
        receiver() : promise() {}
        void set_value(T value) {
            promise.set_value(std::move(value));
        }
        void set_error(std::exception_ptr error) {
            promise.set_exception(error);
        }
    };
    receiver my_receiver;
    auto future = my_receiver.promise.get_future();
    connect_resume(std::move(value), std::move(my_receiver));
    return future;
}

// Create a receiver from a function object (typically a lambda
// closure) that takes a received value (stored receiver result) as
// its only parameter.

template <typename T, typename F>
auto make_receiver(F &&f) {
    struct receiver {
        Received<T> result;
        std::decay_t<F> fun;
        receiver(F &&f)
          : result(), fun(std::forward<F>(f)) {}
        void set_value(T value) {
            result.set_value(std::move(value));
            fun(std::move(result));
        }
        void set_error(std::exception_ptr why) {
            result.set_error(why);
            fun(std::move(result));
        }
    };
    return receiver(std::forward<F>(f));
}

/**
 * Wait for a lazy value to be calculated synchronously. Make sure the
 * thread waiting is not needed in the calculation of the value, or
 * you will end up with a deadlock.
 **/
template <typename T>
T sync_wait(Lazy<T> value) {
    return make_future(std::move(value)).get();
}

/**
 * Wait for a lazy value to be calculated asynchronously; the provided
 * callback will be called with a Received<T> when the Lazy<T> is
 * done. Both the callback itself and the Lazy<T> will be destructed
 * afterwards; cleaning up the coroutine tree representing the
 * calculation.
 **/
template <typename T, typename F>
void async_wait(Lazy<T> value, F &&f) {
    connect_resume(std::move(value), make_receiver<T>(std::forward<F>(f)));
}

}
