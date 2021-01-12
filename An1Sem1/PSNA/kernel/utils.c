#include "utils.h"

BYTE strcmp_custom(char* string1, char* string2)
{

    while (*string1 == *string2 && *string1 && *string2)
    {
        string1++;
        string2++;
    }
    return *string1 - *string2;
}
