#pragma once

#include <iostream>

namespace CheckedPrintf
{
  // Possible errors.
  enum class ErrorCode
  {
    SUCCESS,
    TOO_FEW_ARGS,
    TOO_MANY_ARGS,
    WRONG_ARG,
    INVALID_FORMATSTRING,
  };

  namespace Details
  {
    // Possible formats.
    enum class FormatType
    {
      STRING,
      REAL,
      INT,
      POINTER
    };

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
      //template<> struct ParamCheck<wchar_t*, FormatType::STRING> { typedef std::true_type Result; };
      template<int N> struct ParamCheck<char[N], FormatType::STRING> { typedef std::true_type Result; };
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

    // "Eats" a paramter type and checks if it matches the expected format type.
    // If yes, goes back to CheckPrintfFormat (with one parameter less).
    template<FormatType ExpectedFormat, typename int FormatLen, typename Param0, typename ...Param>
    constexpr ErrorCode CheckPrintfArgumentAndContinue(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: One of your arguments does not match the format string!
      // Check the value of the parameter "pos" to find out which format string was missmatched.
      // --------------------------------------------------------------------------------------------------

      // Removing const from param to keep number of ParamChecks a bit lower.
      // If ParamCheck says it is valid, keep going but jump over %x (thus pos+2).
      return ParamCheck<std::remove_cv<Param0>::type, ExpectedFormat>::Result::value ? CheckPrintfFormat<FormatLen, Param...>(format, pos + 1) : throw ErrorCode::WRONG_ARG;
    }

    // Similar to CheckPrintfArgumentAndContinue, but for the special case of something like "%*d" which requires an extra integer.
    template<typename int FormatLen, typename Param0, typename ...Param>
    constexpr  ErrorCode VariableWidthSpecialCase(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: You put a %* in your format string and another integer was expected but not provided.
      // --------------------------------------------------------------------------------------------------

      return ParamCheck<std::remove_cv<Param0>::type, FormatType::INT>::Result::value ? ParseSymbol<FormatLen, Param...>(format, pos + 1) : throw ErrorCode::WRONG_ARG;
    }

    // The symbol parsing engine. Invoked if there are still params and % was found (but without trailing %)
    template<typename int FormatLen, typename Param0, typename ...Param>
    constexpr ErrorCode ParseSymbol(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: Can't parse format string, unknown specifier.
      // --------------------------------------------------------------------------------------------------

      // See http://www.cplusplus.com/reference/cstdio/printf/

      return format[pos] == 'i' || // Signed decimal integer
             format[pos] == 'd' || // Signed decimal integer
             format[pos] == 'o' || // Unsigned octal
             format[pos] == 'x' || // Unsigned hexadecimal integer
             format[pos] == 'X' || // Unsigned hexadecimal integer, uppper case
             format[pos] == 'c' ?  // Character
                CheckPrintfArgumentAndContinue<FormatType::INT, FormatLen, Param0, Param...>(format, pos) : 

             format[pos] == 'f' || // Decimal floating point, lowercase
             format[pos] == 'F' || // Decimal floating point, uppercase
             format[pos] == 'e' || // Scientific notation (mantissa/exponent), lowercase
             format[pos] == 'E' || // Scientific notation (mantissa/exponent), uppercase
             format[pos] == 'a' || // Hexadecimal floating point, lowercase 
             format[pos] == 'A' ?  // Hexadecimal floating point, uppercase
                CheckPrintfArgumentAndContinue<FormatType::REAL, FormatLen, Param0, Param...>(format, pos) : 

             format[pos] == 's' ? CheckPrintfArgumentAndContinue<FormatType::STRING, FormatLen, Param0, Param...>(format, pos) : // String of characters

             format[pos] == 'p' ? CheckPrintfArgumentAndContinue<FormatType::POINTER, FormatLen, Param0, Param...>(format, pos) : // Pointer address


             // There are various symboles that describe how stuff should be formatted which are always between '%' and the specifier.

             // The one special case is a single * character right after %.
             // It means that a integer is required to give the width of the upcoming thing to print.
             format[pos-1] == '%' && format[pos] == '*' ? VariableWidthSpecialCase<FormatLen, Param0, Param...>(format, pos) :

             // Format specifier
             format[pos] == '-' || format[pos] == '+' || format[pos] == ' ' || format[pos] == '#' || format[pos] == '0' ||
             // Width & precision
             (format[pos] >= '0' && format[pos] <= '9') || format[pos] == '.' || format[pos] == '*' 
               // Then skip this letter.
               ? ParseSymbol<FormatLen, Param0, Param...>(format, pos+1) :

             // Nothing we know!
             throw ErrorCode::INVALID_FORMATSTRING;
    }

    // Call to symbol parsing engine without paramters left is always an error.
    // Need to provide this function to make VariableWidthSpecialCase compile.
    template<typename int FormatLen>
    constexpr ErrorCode ParseSymbol(const char(&format)[FormatLen], int pos)
    {
      throw ErrorCode::TOO_FEW_ARGS;
    }

    // Entry if there are no parameters (can be end of variadic recursion)
    template<typename int FormatLen>
    constexpr ErrorCode CheckPrintfFormat(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: You didn't provide enough arguments!
      // --------------------------------------------------------------------------------------------------

      // Check if we are at the end - if yes, success!
      return pos + 1 >= FormatLen ? ErrorCode::SUCCESS :
        // A new % would mean we have too many args... unless there is another one right after it.
        format[pos] == '%' ?
        format[pos + 1] == '%' ? CheckPrintfFormat(format, pos + 2) : throw ErrorCode::TOO_FEW_ARGS :
        // Otherwise keep parsing (recurse)...
        CheckPrintfFormat(format, pos + 1);
    }

    // Entry for parsing when there are (still) parameters.
    template<typename int FormatLen, typename Param0, typename ...Param>
    constexpr ErrorCode CheckPrintfFormat(const char(&format)[FormatLen], int pos)
    {
      // --------------------------------------------------------------------------------------------------
      // Compile time error in this function: You passed too many arguments!
      // --------------------------------------------------------------------------------------------------

      // Check if we are at the end - if yes, too many args!
      return pos + 1 >= FormatLen ? throw ErrorCode::TOO_MANY_ARGS :
          format[pos] == '%' && format[pos + 1] != '%' ?      //	A % followed by another % character will write a single % to the stream.
            // If there has been a %, do type checks.
            ParseSymbol<FormatLen, Param0, Param...>(format, pos + 1) : // Signed decimal integer
            // No arg symbol, check next char ...
            CheckPrintfFormat<FormatLen, Param0, Param...>(format, pos + 1);
    }
  }

  // Helper to start checking with parameters.
  // Parameters are not actually passed further, they just fill up the variadic parameter pack automatically.
  template<typename int FormatLen, typename ...Param>
  constexpr ErrorCode CheckPrintfFormat(const char(&format)[FormatLen], int pos, const Param&... params)
  {
    return Details::CheckPrintfFormat<FormatLen, Param...>(format, pos);
  }
}

// ------------------------------------------------------------------
// Limitations:
// - No distinction between signed and unsigned int.
// - Missing from the list: %g, %G
// - Evaluation of format specifier very limited
// ------------------------------------------------------------------

// "Front end"

#define printf_checked(format, ...) \
  static_assert(CheckedPrintf::CheckPrintfFormat(format, 0, __VA_ARGS__) == CheckedPrintf::ErrorCode::SUCCESS, "This should never happen."); \
  printf(format, __VA_ARGS__);
