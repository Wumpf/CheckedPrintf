#include "CheckedPrintf.h"

int main()
{
  float f = 1.0f;
  double d = 1.0;
  char c = 1;
  int i = -1;
  unsigned int ui = 1;
  char* string = "blub";
  wchar_t* wstring =  L"blub";
  int* pointer = (int*)0x0001;

  // Basic test with variables.
  printf_checked("%i", i);
  printf_checked("%d", i);
  printf_checked("%o", i);
  printf_checked("%x", i);
  printf_checked("%X", i);
  printf_checked("%f", f);
  printf_checked("%F", f);
  printf_checked("%e", f);
  printf_checked("%E", f);
  printf_checked("%a", f);
  printf_checked("%A", f);
  printf_checked("%c", i);
  printf_checked("%s", string);
  printf_checked("%p", pointer);
  printf_checked("%%");

  // Basic test with literals
  printf_checked("%i", -1);
  printf_checked("%d", 1);
  printf_checked("%o", -1);
  printf_checked("%x", 1);
  printf_checked("%X", 1);
  printf_checked("%f", 1.0f);
  printf_checked("%F", 1.0);
  printf_checked("%e", 1.0f);
  printf_checked("%E", 1.0f);
  printf_checked("%a", 1.0f);
  printf_checked("%A", 1.0f);
  printf_checked("%c", 1);
  printf_checked("%s", "blub");

  // Some formatting stuff.
  printf_checked("%.2f", 1.0f);
  printf_checked("%*d", ui, i);  // Special case for variable width formating.

  // Some larger expressions.
  printf_checked("Hello %s %i some text after %%", "test", 10);
  printf_checked("A float %f a double scientific %e, an integer %i and another %i. And a silly string between %s", f, d, i, ui, string);
}