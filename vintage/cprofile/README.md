# CPROFILE

很久以前写的一个性能评测工具，当你觉得没法使用 gprof 之类工具链自带的 profile 或者，工具链自带的 profile 输出太过复杂，当你只想对具体某部分代码进行 profile的时候，可以试试这个工具。

## C 语言使用

加入 PROFILE 宏，在各个需要评测的模块前面加上 CPROFILE_START(name), CPROFILE_STOP，即可：

```c
#include <windows.h>

#define PROFILE
#include "cprofile.h"


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

```

最后 cprofile_result_printall() 输出评测结果：

```text
+(2017-07-12 12:41:34)  /  (ordered by: default)
name    ncalls tottime percent cumtime percent filename:line
main         1   0.382 100.00%   0.100  26.29% test_c.c:10
work1        1   0.201  52.54%   0.201  52.54% test_c.c:13
work2        1   0.081  21.17%   0.000   0.00% test_c.c:18
work2.2      1   0.050  13.13%   0.050  13.13% test_c.c:20
work2.3      1   0.031   8.04%   0.031   8.04% test_c.c:25
```


## C++ 使用

C++版本更精简一点：

```cpp
#include <windows.h>

#define PROFILE
#include "cprofile.h"


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
```

最后输出：

```text
+(2017-07-12 14:44:25)  /  (ordered by: default)
name    ncalls tottime percent cumtime percent filename:line
main         1   0.382 100.00%   0.100  26.25% test.cpp:10
work1        1   0.201  52.57%   0.201  52.57% test.cpp:13
work2        1   0.081  21.18%   0.000   0.00% test.cpp:17
work2.2      1   0.050  13.10%   0.050  13.10% test.cpp:19
work2.3      1   0.031   8.08%   0.031   8.08% test.cpp:23
```

