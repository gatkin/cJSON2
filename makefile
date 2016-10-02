cjson2: cJSON2_test.c cJSON2.c
	gcc cJSON2.c cJSON2_test.c -D_GNU_SOURCE=1 -Wall -o testcjson2
