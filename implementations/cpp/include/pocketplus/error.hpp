/**
 * @file error.hpp
 * @brief POCKET+ error handling.
 *
 * Provides both exception-based and error-code-based error handling
 * for embedded compatibility (-fno-exceptions).
 *
 * @authors Georges Labreche <georges@tanagraspace.com>
 * @authors Claude Code (Anthropic) <noreply@anthropic.com>
 */

#ifndef POCKETPLUS_ERROR_HPP
#define POCKETPLUS_ERROR_HPP

#include "config.hpp"

#if !POCKET_NO_EXCEPTIONS
#include <stdexcept>
#include <string>
#endif

namespace pocketplus {

/**
 * @brief Error codes for error-code-based error handling.
 *
 * Used when exceptions are disabled (POCKET_NO_EXCEPTIONS=1).
 */
enum class Error {
    Ok = 0,          ///< Success
    InvalidArg = -1, ///< Invalid argument
    Overflow = -2,   ///< Buffer overflow
    Underflow = -3,  ///< Buffer underflow (not enough data)
    InvalidData = -4 ///< Invalid/corrupted data
};

/**
 * @brief Get error message for error code.
 * @param error Error code
 * @return Human-readable error message
 */
inline const char* error_string(Error error) noexcept {
    switch (error) {
    case Error::Ok:
        return "Success";
    case Error::InvalidArg:
        return "Invalid argument";
    case Error::Overflow:
        return "Buffer overflow";
    case Error::Underflow:
        return "Buffer underflow";
    case Error::InvalidData:
        return "Invalid or corrupted data";
    default:
        return "Unknown error";
    }
}

#if !POCKET_NO_EXCEPTIONS

/**
 * @brief Base exception for POCKET+ errors.
 */
class PocketException : public std::runtime_error {
public:
    explicit PocketException(const std::string& message, Error code = Error::InvalidArg)
        : std::runtime_error(message), error_code_(code) {}

    Error code() const noexcept {
        return error_code_;
    }

private:
    Error error_code_;
};

/**
 * @brief Exception for invalid arguments.
 */
class InvalidArgumentException : public PocketException {
public:
    explicit InvalidArgumentException(const std::string& message)
        : PocketException(message, Error::InvalidArg) {}
};

/**
 * @brief Exception for buffer overflow.
 */
class OverflowException : public PocketException {
public:
    explicit OverflowException(const std::string& message)
        : PocketException(message, Error::Overflow) {}
};

/**
 * @brief Exception for buffer underflow.
 */
class UnderflowException : public PocketException {
public:
    explicit UnderflowException(const std::string& message)
        : PocketException(message, Error::Underflow) {}
};

/**
 * @brief Exception for invalid/corrupted data.
 */
class InvalidDataException : public PocketException {
public:
    explicit InvalidDataException(const std::string& message)
        : PocketException(message, Error::InvalidData) {}
};

#endif // !POCKET_NO_EXCEPTIONS

} // namespace pocketplus

#endif // POCKETPLUS_ERROR_HPP
