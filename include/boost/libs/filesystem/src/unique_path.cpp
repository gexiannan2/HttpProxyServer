//  filesystem unique_path.cpp  --------------------------------------------------------//

//  Copyright Beman Dawes 2010
//  Copyright Andrey Semashev 2020, 2024

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#include "platform_config.hpp"

#include <boost/predef/library/c/cloudabi.h>
#include <boost/predef/os/bsd/open.h>
#include <boost/predef/os/bsd/free.h>

#ifdef BOOST_POSIX_API

#include <cerrno>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

#if !defined(BOOST_FILESYSTEM_DISABLE_ARC4RANDOM)
#if BOOST_OS_BSD_OPEN >= BOOST_VERSION_NUMBER(2, 1, 0) || \
    BOOST_OS_BSD_FREE >= BOOST_VERSION_NUMBER(8, 0, 0) || \
    BOOST_LIB_C_CLOUDABI
#include <stdlib.h>
#define BOOST_FILESYSTEM_HAS_ARC4RANDOM
#endif
#endif // !defined(BOOST_FILESYSTEM_DISABLE_ARC4RANDOM)

#if !defined(BOOST_FILESYSTEM_DISABLE_GETRANDOM)
#if (defined(__linux__) || defined(__linux) || defined(linux)) && \
    (!defined(__ANDROID__) || __ANDROID_API__ >= 28)
#include <sys/syscall.h>
#if defined(SYS_getrandom)
#define BOOST_FILESYSTEM_HAS_GETRANDOM_SYSCALL
#endif // defined(SYS_getrandom)
#if defined(__has_include)
#if __has_include(<sys/random.h>)
#define BOOST_FILESYSTEM_HAS_GETRANDOM
#endif
#elif defined(__GLIBC__)
#if __GLIBC_PREREQ(2, 25)
#define BOOST_FILESYSTEM_HAS_GETRANDOM
#endif
#endif // BOOST_FILESYSTEM_HAS_GETRANDOM definition
#if defined(BOOST_FILESYSTEM_HAS_GETRANDOM)
#include <sys/random.h>
#endif
#endif // (defined(__linux__) || defined(__linux) || defined(linux)) && (!defined(__ANDROID__) || __ANDROID_API__ >= 28)
#endif // !defined(BOOST_FILESYSTEM_DISABLE_GETRANDOM)

#include <boost/scope/unique_fd.hpp>
#include "posix_tools.hpp"

#else  // BOOST_WINDOWS_API

// We use auto-linking below to help users of static builds of Boost.Filesystem to link to whatever Windows SDK library we selected.
// The dependency information is currently not exposed in CMake config files generated by Boost.Build (https://github.com/boostorg/boost_install/issues/18),
// which makes it non-trivial for users to discover the libraries they need. This feature is deprecated and may be removed in the future,
// when the situation with CMake config files improves.
// Note that the library build system is the principal source of linking the library, which must work regardless of auto-linking.
#include <boost/predef/platform.h>
#include <boost/winapi/basic_types.hpp>

#if defined(BOOST_FILESYSTEM_HAS_BCRYPT) // defined on the command line by the project
#include <boost/winapi/error_codes.hpp>
#include <boost/winapi/bcrypt.hpp>
#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED) && defined(_MSC_VER)
#pragma comment(lib, "bcrypt.lib")
#endif // !defined(BOOST_FILESYSTEM_NO_DEPRECATED) && defined(_MSC_VER)
#else  // defined(BOOST_FILESYSTEM_HAS_BCRYPT)
#include <boost/winapi/crypt.hpp>
#include <boost/winapi/get_last_error.hpp>
#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED) && defined(_MSC_VER)
#if !defined(_WIN32_WCE)
#pragma comment(lib, "advapi32.lib")
#else
#pragma comment(lib, "coredll.lib")
#endif
#endif // !defined(BOOST_FILESYSTEM_NO_DEPRECATED) && defined(_MSC_VER)
#endif // defined(BOOST_FILESYSTEM_HAS_BCRYPT)

#endif // BOOST_POSIX_API

#include <cstddef>
#include <boost/filesystem/config.hpp>
#include <boost/filesystem/operations.hpp>
#include "private_config.hpp"
#include "atomic_tools.hpp"
#include "error_handling.hpp"

#include <boost/filesystem/detail/header.hpp> // must be the last #include

#if defined(BOOST_POSIX_API)
// At least Mac OS X 10.6 and older doesn't support O_CLOEXEC
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#endif // defined(BOOST_POSIX_API)

namespace boost {
namespace filesystem {
namespace detail {

namespace {

#if defined(BOOST_POSIX_API) && !defined(BOOST_FILESYSTEM_HAS_ARC4RANDOM)

//! Fills buffer with cryptographically random data obtained from /dev/(u)random
int fill_random_dev_random(void* buf, std::size_t len)
{
    boost::scope::unique_fd file;
    while (true)
    {
        file.reset(::open("/dev/urandom", O_RDONLY | O_CLOEXEC));
        if (!file)
        {
            if (errno == EINTR)
                continue;
        }

        break;
    }

    if (!file)
    {
        while (true)
        {
            file.reset(::open("/dev/random", O_RDONLY | O_CLOEXEC));
            if (!file)
            {
                const int err = errno;
                if (err == EINTR)
                    continue;
                return err;
            }

            break;
        }
    }

    std::size_t bytes_read = 0u;
    while (bytes_read < len)
    {
        ssize_t n = ::read(file.get(), buf, len - bytes_read);
        if (BOOST_UNLIKELY(n < 0))
        {
            const int err = errno;
            if (err == EINTR)
                continue;
            return err;
        }

        bytes_read += n;
        buf = static_cast< char* >(buf) + n;
    }

    return 0;
}

#if defined(BOOST_FILESYSTEM_HAS_GETRANDOM) || defined(BOOST_FILESYSTEM_HAS_GETRANDOM_SYSCALL)

typedef int fill_random_t(void* buf, std::size_t len);

//! Pointer to the implementation of fill_random.
fill_random_t* fill_random = &fill_random_dev_random;

//! Fills buffer with cryptographically random data obtained from getrandom()
int fill_random_getrandom(void* buf, std::size_t len)
{
    std::size_t bytes_read = 0u;
    while (bytes_read < len)
    {
#if defined(BOOST_FILESYSTEM_HAS_GETRANDOM)
        ssize_t n = ::getrandom(buf, len - bytes_read, 0u);
#else
        ssize_t n = ::syscall(SYS_getrandom, buf, len - bytes_read, 0u);
#endif
        if (BOOST_UNLIKELY(n < 0))
        {
            const int err = errno;
            if (err == EINTR)
                continue;

            if (err == ENOSYS && bytes_read == 0u)
            {
                filesystem::detail::atomic_store_relaxed(fill_random, &fill_random_dev_random);
                return fill_random_dev_random(buf, len);
            }

            return err;
        }

        bytes_read += n;
        buf = static_cast< char* >(buf) + n;
    }

    return 0;
}

#endif // defined(BOOST_FILESYSTEM_HAS_GETRANDOM) || defined(BOOST_FILESYSTEM_HAS_GETRANDOM_SYSCALL)

#endif // defined(BOOST_POSIX_API) && !defined(BOOST_FILESYSTEM_HAS_ARC4RANDOM)

void system_crypt_random(void* buf, std::size_t len, boost::system::error_code* ec)
{
#if defined(BOOST_POSIX_API)

#if defined(BOOST_FILESYSTEM_HAS_GETRANDOM) || defined(BOOST_FILESYSTEM_HAS_GETRANDOM_SYSCALL)

    int err = filesystem::detail::atomic_load_relaxed(fill_random)(buf, len);
    if (BOOST_UNLIKELY(err != 0))
        emit_error(err, ec, "boost::filesystem::unique_path");

#elif defined(BOOST_FILESYSTEM_HAS_ARC4RANDOM)

    arc4random_buf(buf, len);

#else

    int err = fill_random_dev_random(buf, len);
    if (BOOST_UNLIKELY(err != 0))
        emit_error(err, ec, "boost::filesystem::unique_path");

#endif

#else // defined(BOOST_POSIX_API)

#if defined(BOOST_FILESYSTEM_HAS_BCRYPT)

    boost::winapi::BCRYPT_ALG_HANDLE_ handle;
    boost::winapi::NTSTATUS_ status = boost::winapi::BCryptOpenAlgorithmProvider(&handle, boost::winapi::BCRYPT_RNG_ALGORITHM_, nullptr, 0);
    if (BOOST_UNLIKELY(status != 0))
    {
    fail:
        emit_error(translate_ntstatus(status), ec, "boost::filesystem::unique_path");
        return;
    }

    status = boost::winapi::BCryptGenRandom(handle, static_cast< boost::winapi::PUCHAR_ >(buf), static_cast< boost::winapi::ULONG_ >(len), 0);

    boost::winapi::BCryptCloseAlgorithmProvider(handle, 0);

    if (BOOST_UNLIKELY(status != 0))
        goto fail;

#else // defined(BOOST_FILESYSTEM_HAS_BCRYPT)

    boost::winapi::HCRYPTPROV_ handle;
    boost::winapi::DWORD_ err = 0u;
    if (BOOST_UNLIKELY(!boost::winapi::CryptAcquireContextW(&handle, nullptr, nullptr, boost::winapi::PROV_RSA_FULL_, boost::winapi::CRYPT_VERIFYCONTEXT_ | boost::winapi::CRYPT_SILENT_)))
    {
        err = boost::winapi::GetLastError();

    fail:
        emit_error(err, ec, "boost::filesystem::unique_path");
        return;
    }

    boost::winapi::BOOL_ gen_ok = boost::winapi::CryptGenRandom(handle, static_cast< boost::winapi::DWORD_ >(len), static_cast< boost::winapi::BYTE_* >(buf));

    if (BOOST_UNLIKELY(!gen_ok))
        err = boost::winapi::GetLastError();

    boost::winapi::CryptReleaseContext(handle, 0);

    if (BOOST_UNLIKELY(!gen_ok))
        goto fail;

#endif // defined(BOOST_FILESYSTEM_HAS_BCRYPT)

#endif // defined(BOOST_POSIX_API)
}

#ifdef BOOST_WINDOWS_API
BOOST_CONSTEXPR_OR_CONST wchar_t hex[] = L"0123456789abcdef";
BOOST_CONSTEXPR_OR_CONST wchar_t percent = L'%';
#else
BOOST_CONSTEXPR_OR_CONST char hex[] = "0123456789abcdef";
BOOST_CONSTEXPR_OR_CONST char percent = '%';
#endif

} // unnamed namespace

#if defined(linux) || defined(__linux) || defined(__linux__)

//! Initializes fill_random implementation pointer
void init_fill_random_impl(unsigned int major_ver, unsigned int minor_ver, unsigned int patch_ver)
{
#if defined(BOOST_FILESYSTEM_HAS_INIT_PRIORITY) && \
    (defined(BOOST_FILESYSTEM_HAS_GETRANDOM) || defined(BOOST_FILESYSTEM_HAS_GETRANDOM_SYSCALL))
    fill_random_t* fr = &fill_random_dev_random;

    if (major_ver > 3u || (major_ver == 3u && minor_ver >= 17u))
        fr = &fill_random_getrandom;

    filesystem::detail::atomic_store_relaxed(fill_random, fr);
#endif
}

#endif // defined(linux) || defined(__linux) || defined(__linux__)

BOOST_FILESYSTEM_DECL
path unique_path(path const& model, system::error_code* ec)
{
    // This function used wstring for fear of misidentifying
    // a part of a multibyte character as a percent sign.
    // However, double byte encodings only have 80-FF as lead
    // bytes and 40-7F as trailing bytes, whereas % is 25.
    // So, use string on POSIX and avoid conversions.

    path::string_type s(model.native());

    char ran[16] = {};                                                    // init to avoid clang static analyzer message
                                                                          // see ticket #8954
    BOOST_CONSTEXPR_OR_CONST unsigned int max_nibbles = 2u * sizeof(ran); // 4-bits per nibble

    unsigned int nibbles_used = max_nibbles;
    for (path::string_type::size_type i = 0, n = s.size(); i < n; ++i)
    {
        if (s[i] == percent) // digit request
        {
            if (nibbles_used == max_nibbles)
            {
                system_crypt_random(ran, sizeof(ran), ec);
                if (ec && *ec)
                    return path();
                nibbles_used = 0;
            }
            unsigned int c = ran[nibbles_used / 2u];
            c >>= 4u * (nibbles_used++ & 1u); // if odd, shift right 1 nibble
            s[i] = hex[c & 0xf];              // convert to hex digit and replace
        }
    }

    if (ec)
        ec->clear();

    return s;
}

} // namespace detail
} // namespace filesystem
} // namespace boost

#include <boost/filesystem/detail/footer.hpp>
