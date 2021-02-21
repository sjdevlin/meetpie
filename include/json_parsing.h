//
//  json_parsing.h
//  
//
//  Created by Stephen Devlin on 01/02/2021.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <json-c/json.h>

#ifdef  __cplusplus
extern "C" {
#endif

void json_parse(char *, odas_data * );
void json_parse_item(json_object *, odas_data *, const int);

#ifdef  __cplusplus
}
#endif


