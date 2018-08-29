#include <windows.h>

#define PROFILE
#include "cprofile.c"


int main(void)
{
	{
		CPROFILE("main");
		Sleep(100);
		{
			CPROFILE("work1");
			Sleep(200);
		}
		{
			CPROFILE("work2");
			{
				CPROFILE("work2.2");
				Sleep(50);
			}
			{
				CPROFILE("work2.3");
				Sleep(30);
			}
		}
	}
	cprofile_result_printall();
	return 0;
}



