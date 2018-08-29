#include <windows.h>

#define PROFILE
#include "cprofile.c"


int main(void)
{
	{
		CPROFILE_START("main");
		Sleep(100);
		{
			CPROFILE_START("work1");
			Sleep(200);
			CPROFILE_STOP();
		}
		{
			CPROFILE_START("work2");
			{
				CPROFILE_START("work2.2");
				Sleep(50);
				CPROFILE_STOP();
			}
			{
				CPROFILE_START("work2.3");
				Sleep(30);
				CPROFILE_STOP();
			}
			CPROFILE_STOP();
		}
		CPROFILE_STOP();
	}
	cprofile_result_printall();
	return 0;
}



