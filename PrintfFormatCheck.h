#pragma once

#include <type_traits>

// ------------------------------------------------------------------
// Compile time checks for printf style formatting.
// See macro at bottom of file on how to use.
//
// Limitations:
// - No distinction between signed and unsigned int.
// - Does not know about following specifier: %g, %G
// - Evaluation of format string specifier very limited
// ------------------------------------------------------------------

namespace PrintfFormatCheck
{
  // Possible errors.
  enum class ErrorCode
  {
    SUCCESS,
    TOO_FEW_ARGS,
    TOO_MANY_ARGS,
    WRONG_ARG,
    INVALID_FORMATSTRING,
    UNREACHABLE_CODE
  };

  // Possible formats.
  enum class FormatType
  {
    STRING,
    REAL,
    INT,
    POINTER
  };

  // Non-specialized structs.
  template<int FormatLen, typename ...Params>
  struct CheckPrintfFormat;
  template<int FormatLen, typename ...Params>
  struct ParseSymbol;
  template<FormatType ExpectedFormat, int FormatLen, typename ...Params>
  struct CheckArgument;

  // Matching parser result (FormatType) with types.
  namespace
  {
    // Default "Fail"
    template<typename Param, FormatType ExpectedFormat>
    struct ParamCheck
    {
      typedef std::false_type Result;
    };

    // A List of all success cases:

    template<> struct ParamCheck<char*, FormatType::STRING> { typedef std::true_type Result; };
    template<> struct ParamCheck<const char*, FormatType::STRING> { typedef std::true_type Result; };
    //template<> struct ParamCheck<wchar_t*, FormatType::STRING> { typedef std::true_type Result; };
    template<int N> struct ParamCheck<char[N], FormatType::STRING> { typedef std::true_type Result; };
    template<int N> struct ParamCheck<const char[N], FormatType::STRING> { typedef std::true_type Result; };
    //template<int N> struct ParamCheck<wchar_t[N], FormatType::STRING> { typedef std::true_type Result; };

    template<> struct ParamCheck<char, FormatType::INT> { typedef std::true_type Result; };
    template<> struct ParamCheck<unsigned char, FormatType::INT> { typedef std::true_type Result; };
    template<> struct ParamCheck<short, FormatType::INT> { typedef std::true_type Result; };
    template<> struct ParamCheck<unsigned short, FormatType::INT> { typedef std::true_type Result; };
    template<> struct ParamCheck<int, FormatType::INT> { typedef std::true_type Result; };
    template<> struct ParamCheck<unsigned int, FormatType::INT> { typedef std::true_type Result; };
    template<> struct ParamCheck<long, FormatType::INT> { typedef std::true_type Result; };
    template<> struct ParamCheck<unsigned long, FormatType::INT> { typedef std::true_type Result; };

    template<> struct ParamCheck<float, FormatType::REAL> { typedef std::true_type Result; };
    template<> struct ParamCheck<double, FormatType::REAL> { typedef std::true_type Result; };

    template<typename Pointer> struct ParamCheck<Pointer*, FormatType::POINTER> { typedef std::true_type Result; };
  }

  template<FormatType ExpectedFormat, int FormatLen, typename Param0, typename ...Param>
  struct CheckArgument<ExpectedFormat, FormatLen, Param0, Param...>
  {
    // "Eats" a paramter type and checks if it matches the expected format type.
    // If yes, goes back to CheckPrintfFormat (with one parameter less).
    static constexpr ErrorCode CheckAndContinue(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: One of your arguments does not match the format string!
      // Check the value of the parameter "pos" to find out which format string was missmatched.
      // --------------------------------------------------------------------------------------------------

      // Removing const from param to keep number of ParamChecks a bit lower.
      // If ParamCheck says it is valid, keep going but jump over %x (thus pos+2).
      return ParamCheck<typename std::remove_reference<Param0>::type, ExpectedFormat>::Result::value ?
                CheckPrintfFormat<FormatLen, Param...>::Recurse(format, pos + 1) : throw ErrorCode::WRONG_ARG;
    }

    // Similar to CheckArgument::CheckAndContinue, but for the special case of something like "%*d" which requires an extra integer.
    static constexpr ErrorCode VariableWidthSpecialCase(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: You put a %* in your format string and another integer was expected but not provided.
      // --------------------------------------------------------------------------------------------------

      return ParamCheck<typename std::remove_reference<Param0>::type, FormatType::INT>::Result::value ?
                ParseSymbol<FormatLen, Param...>::Recurse(format, pos + 1) : throw ErrorCode::WRONG_ARG;
    }
  };
  template<FormatType ExpectedFormat, int FormatLen>
  struct CheckArgument<ExpectedFormat, FormatLen>
  {
    static constexpr ErrorCode CheckAndContinue(const char(&format)[FormatLen], int pos)
    {
      //throw ErrorCode::UNREACHABLE_CODE; // A throw without tertiary operator in constexpr is only allowed in C++14
      return ErrorCode::UNREACHABLE_CODE;
    }
    static constexpr ErrorCode VariableWidthSpecialCase(const char(&format)[FormatLen], int pos)
    {
      //throw ErrorCode::UNREACHABLE_CODE; // A throw without tertiary operator in constexpr is only allowed in C++14
      return ErrorCode::UNREACHABLE_CODE;
    }
  };


  // The symbol parsing engine. Invoked if there are still params and % was found (but without trailing %)
  template<int FormatLen, typename Param0, typename ...Param>
  struct ParseSymbol<FormatLen, Param0, Param...>
  {
    static constexpr ErrorCode Recurse(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: Can't parse format string, unknown specifier.
      //
      // Or, less common: Too few arguments in case of using a variable width specifier.
      // --------------------------------------------------------------------------------------------------

      // See http://www.cplusplus.com/reference/cstdio/printf/

      return format[pos] == 'i' || // Signed decimal integer
              format[pos] == 'd' || // Signed decimal integer
              format[pos] == 'o' || // Unsigned octal
              format[pos] == 'x' || // Unsigned hexadecimal integer
              format[pos] == 'X' || // Unsigned hexadecimal integer, uppper case
              format[pos] == 'c' ?  // Character
                CheckArgument<FormatType::INT, FormatLen, Param0, Param...>::CheckAndContinue(format, pos) :

              format[pos] == 'f' || // Decimal floating point, lowercase
              format[pos] == 'F' || // Decimal floating point, uppercase
              format[pos] == 'e' || // Scientific notation (mantissa/exponent), lowercase
              format[pos] == 'E' || // Scientific notation (mantissa/exponent), uppercase
              format[pos] == 'a' || // Hexadecimal floating point, lowercase 
              format[pos] == 'A' ?  // Hexadecimal floating point, uppercase
                CheckArgument<FormatType::REAL, FormatLen, Param0, Param...>::CheckAndContinue(format, pos) :

              format[pos] == 's' ? CheckArgument<FormatType::STRING, FormatLen, Param0, Param...>::CheckAndContinue(format, pos) : // String of characters

              format[pos] == 'p' ? CheckArgument<FormatType::POINTER, FormatLen, Param0, Param...>::CheckAndContinue(format, pos) : // Pointer address


              // There are various symboles that describe how stuff should be formatted which are always between '%' and the specifier.

              // The one special case is a single * character right after %.
              // It means that a integer is required to give the width of the upcoming thing to print.
              format[pos-1] == '%' && format[pos] == '*' ? CheckArgument<FormatType::INT, FormatLen, Param0, Param...>::VariableWidthSpecialCase(format, pos) :

              // Format specifier
              format[pos] == '-' || format[pos] == '+' || format[pos] == ' ' || format[pos] == '#' || format[pos] == '0' ||
              // Width & precision
              (format[pos] >= '0' && format[pos] <= '9') || format[pos] == '.' || format[pos] == '*' 
                // Then skip this letter.
                ? Recurse(format, pos + 1) :

              // Nothing we know!
              throw ErrorCode::INVALID_FORMATSTRING;
    }
  };

  // Implemnentation for no param. Should never be called (but compiler may want to compile it)
  template<int FormatLen>
  struct ParseSymbol<FormatLen>
  {
    static constexpr ErrorCode Recurse(const char(&format)[FormatLen], int pos)
    {
      //throw ErrorCode::UNREACHABLE_CODE; // A throw without tertiary operator in constexpr is only allowed in C++14
      return ErrorCode::UNREACHABLE_CODE;
    }
  };

  // Entry if there are no parameters (can be end of variadic recursion)
  template<int FormatLen>
  struct CheckPrintfFormat<FormatLen>
  {
    static constexpr ErrorCode Recurse(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: You didn't provide enough arguments!
      // --------------------------------------------------------------------------------------------------

      // Check if we are at the end - if yes, success!
      return pos + 1 >= FormatLen ? ErrorCode::SUCCESS :
        // A new % would mean we have too many args... unless there is another one right after it.
        format[pos] == '%' ?
        format[pos + 1] == '%' ? Recurse(format, pos + 2) : throw ErrorCode::TOO_FEW_ARGS :
        // Otherwise keep parsing (recurse)...
        Recurse(format, pos + 1);
    }
  };

  // Entry for parsing when there are (still) parameters.
  template<int FormatLen, typename Param0, typename ...Param>
  struct CheckPrintfFormat<FormatLen, Param0, Param...>
  {
    static constexpr ErrorCode Recurse(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: You passed too many arguments!
      // --------------------------------------------------------------------------------------------------

      // Check if we are at the end of the format string.
       return pos + 1 >= FormatLen ? throw ErrorCode::TOO_MANY_ARGS :
                // A % followed by another % character will write a single % to the stream.
                format[pos] == '%' ?
                format[pos + 1] == '%' ? Recurse(format, pos + 2) :
                // If there has been a %, do type checks.
                ParseSymbol<FormatLen, Param0, Param...>::Recurse(format, pos + 1) : // Signed decimal integer
              // No arg symbol, check next char ...
              Recurse(format, pos + 1);
    }

    // Entry function eats the first param which needs to be void to support zero arguments.
    static constexpr ErrorCode Entry(const char(&format)[FormatLen], int pos)
    {
      static_assert(std::is_same<Param0, void>::value, "First template argument must be void.");
      return CheckPrintfFormat<FormatLen, Param...>::Recurse(format, pos);
    }
    // Fallback for non const string.
    static constexpr ErrorCode Entry(const char*&, int pos)
    {
      return ErrorCode::SUCCESS;
    }
  };

  // Parameters are not actually passed further, they just fill up the variadic parameter pack automatically.
  template<int FormatLen>
  constexpr int StringLiteralLength(const char(&format)[FormatLen])
  {
    return FormatLen;
  }
  // Fallback for non-compile time string. (doesn't do anything!)
  constexpr int StringLiteralLength(const char*& format)
  {
    return 0;
  }
}

// -------------------------------------------------------------------------------
// Some necessary macro magic to get decltype on multiple arguments.

// Is there a better way to do decltype on all arguments that this piece of macro magic?
// Note that we are not able to pass the arguments on since we want to use the call in a static_assert!

#define LIST_EXPAND_ARGS_1(op, a0)                                         op(a0)
#define LIST_EXPAND_ARGS_2(op, a0, a1)                                     op(a0), op(a1)
#define LIST_EXPAND_ARGS_3(op, a0, a1, a2)                                 op(a0), op(a1), op(a2)
#define LIST_EXPAND_ARGS_4(op, a0, a1, a2, a3)                             op(a0), op(a1), op(a2), op(a3)
#define LIST_EXPAND_ARGS_5(op, a0, a1, a2, a3, a4)                         op(a0), op(a1), op(a2), op(a3), op(a4)
#define LIST_EXPAND_ARGS_6(op, a0, a1, a2, a3, a4, a5)                     op(a0), op(a1), op(a2), op(a3), op(a4), op(a5)
#define LIST_EXPAND_ARGS_7(op, a0, a1, a2, a3, a4, a5, a6)                 op(a0), op(a1), op(a2), op(a3), op(a4), op(a5), op(a6)
#define LIST_EXPAND_ARGS_8(op, a0, a1, a2, a3, a4, a5, a6, a7)             op(a0), op(a1), op(a2), op(a3), op(a4), op(a5), op(a6), op(a7)
#define LIST_EXPAND_ARGS_9(op, a0, a1, a2, a3, a4, a5, a6, a7, a8)         op(a0), op(a1), op(a2), op(a3), op(a4), op(a5), op(a6), op(a7), op(a8)
#define LIST_EXPAND_ARGS_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)    op(a0), op(a1), op(a2), op(a3), op(a4), op(a5), op(a6), op(a7), op(a8), op(a9)

#if _MSC_VER >= 1400
  #define VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...)    N
  #define VA_NUM_ARGS_REVERSE_SEQUENCE            10, 9, 8, 7, 6, 5, 4, 3, 2, 1
  #define LEFT_PARENTHESIS (
  #define RIGHT_PARENTHESIS )
#define VA_NUM_ARGS(...)                          VA_NUM_ARGS_HELPER LEFT_PARENTHESIS __VA_ARGS__, VA_NUM_ARGS_REVERSE_SEQUENCE RIGHT_PARENTHESIS
#else
  #define VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...)    N
  #define VA_NUM_ARGS(...) \
      VA_NUM_ARGS_HELPER(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#endif

#define CALL_MACRO(macro, args) macro args

// Concatenates two strings, even when the strings are macros themselves
#define CONCAT(x,y) CONCAT_HELPER(x,y)
#define CONCAT_HELPER(x,y) CONCAT_HELPER2(x,y)
#define CONCAT_HELPER2(x,y) x##y

// Variadic macro "dispatching" the arguments to the correct macro.
// The number of arguments is found by using EZ_VA_NUM_ARGS(__VA_ARGS__)
#define LIST_EXPAND_ARGS(op, ...) \
      CALL_MACRO(CONCAT(LIST_EXPAND_ARGS_, VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

// -------------------------------------------------------------------------------



// Frontend for easy use with functions.
// You can use it as starting point to define your own macro for specific functions. Example:
// #define printf_checked(format, ...) CHECKED_FORMATSTRING(printf, format, ##__VA_ARGS__)

// Note the use of "##__VA_ARGS__". The ## combined with varargs is a non-ISO extension that happens to work in MSVC, GCC and Clang.
// It will eliminate the preceding comma if __VA_ARGS__ is empty

#define EXPAND(x) x

#define CHECKED_FORMATSTRING(function, format, ...) \
  function((format), ##__VA_ARGS__); \
  static_assert(PrintfFormatCheck::CheckPrintfFormat<PrintfFormatCheck::StringLiteralLength(format), LIST_EXPAND_ARGS(decltype, void(), ##__VA_ARGS__)>::Entry(format, 0) == PrintfFormatCheck::ErrorCode::SUCCESS, "This should never happen.");