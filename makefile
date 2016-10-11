test: cJSON2_Interface.c cJSON2_Test.c cJSON2_Parse.c cJSON2_Serialize.c cJSON2_Utils.c
	gcc cJSON2_Interface.c cJSON2_Test.c cJSON2_Parse.c cJSON2_Serialize.c cJSON2_Utils.c -D_GNU_SOURCE -Wall -o test
