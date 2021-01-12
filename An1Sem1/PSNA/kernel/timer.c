#include "timer.h"
#include "screen.h"
#include "logging.h"

QWORD gTimeSinceStart;
#define TIME_DIGITS 5
void TimerDriverInit()
{
    gTimeSinceStart = 0;
}

void TimerDriver()
{
    gTimeSinceStart += 1;

    char TimeDigits[TIME_DIGITS + 1];

    if (gTimeSinceStart % 20 == 0)
    {
        QWORD aux = gTimeSinceStart / 20;

        for (int i = 0; i < TIME_DIGITS; i++)
        {
            TimeDigits[TIME_DIGITS - i - 1] = ((aux % 10) + '0');
            aux /= 10;
        }
        TimeDigits[TIME_DIGITS] = 0;

        PutStringAtPosition(TimeDigits, 0);
    }

}
