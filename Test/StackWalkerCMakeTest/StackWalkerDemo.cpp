/**
 * @file StackWalkerDemo.cpp
 * @author Derek Huang
 * @brief C++ program testing external StackWalker usage
 *
 * @copyright
 *
 * LICENSE (http://www.opensource.org/licenses/bsd-license.php)
 *
 *   Copyright (c) 2005-2025, Jochen Kalmbach
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without modification,
 *   are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *   Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *   Neither the name of Jochen Kalmbach nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Windows.h>

#include <cstdint>
#include <iostream>
#include <string>

#include <StackWalker.h>

// __cplusplus macro that works for MSVC without /Zc:__cplusplus
#if defined(_MSC_VER)
#define SW_CPLUSPLUS _MSVC_LANG
#else
#define SW_CPLUSPLUS __cplusplus
#endif  // !defined(_MSC_VER)

// indicate C++11 support
#if SW_CPLUSPLUS >= 201103L
#define HAVE_CXX_11 1
#else
#define HAVE_CXX_11 0
#endif // SW_CPLUSPLUS < 201103L

// override specifier only available in C++11 or later
#if HAVE_CXX_11
#define CXX11_OVERRIDE override
#else
#define CXX11_OVERRIDE
#endif  // !HAVE_CXX_11

// don't inline so even with optimization we can get a stack trace
#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE
#endif  // !defined(_MSC_VER)

namespace
{

// simple StackWalker class that prints to stdout
class PrintWalker : public StackWalker
{
public:
// enable testing different C++ standards
#if HAVE_CXX_11
  using StackWalker::StackWalker;
#else
  PrintWalker() : StackWalker() {}
#endif  // !HAVE_CXX_11

protected:
// enable testing different C++ standards
#if !HAVE_CXX_11
  virtual
#endif  // !HAVE_CXX_11
  void OnOutput(LPCSTR text) CXX11_OVERRIDE
  {
    std::cout << text << std::flush;
    StackWalker::OnOutput(text);
  }
};

// functions for nesting
// note: some calls may get optimized out and inlined with optimization on

namespace inner3
{

NOINLINE
void Func3(const std::string& /*s*/)
{
  PrintWalker sw;
  sw.ShowCallstack();
}

}  // namespace inner3

namespace inner2
{
namespace detail
{

NOINLINE
void Func2(double /*x*/)
{
  inner3::Func3("hello");
}

}  // namespace detail
}  // namespace inner2

namespace inner1
{

NOINLINE
void Func1(const void* /*data*/)
{
  inner2::detail::Func2(1.4);
}

}  // namespace inner1

}  // namespace

int main()
{
  inner1::Func1(nullptr);
  return EXIT_SUCCESS;
}
