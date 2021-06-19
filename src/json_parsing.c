//
//  json_parsing.c
//  
//
//  Created by Stephen Devlin on 01/02/2021.
//

#include "../include/meetpie.h"
#include "../include/json_parsing.h"


void json_parse(char *buffer, odas_data * odas_array)
{
  json_object *jobj;
  json_object *jobj_array;
  json_object *jobj_array_item;

  unsigned int i, time_stamp;
  enum json_type type;
  int arraylen;

//    printf ("got in to json parse function\n");

  jobj = json_tokener_parse(buffer);

//    printf ("got past jobj creation\n");

  json_object_object_foreach(jobj, key, val)
  {

    type = json_object_get_type(val);
    switch (type)
    {
    case json_type_int:
      if (!strcmp(key, "timeStamp"))
        time_stamp = json_object_get_int(val) / 20;
	printf("timestamp: %d\n\n",time_stamp);
      break;
    case json_type_array:
      i = json_object_object_get_ex(jobj, key, &jobj_array);
      arraylen = json_object_array_length(jobj_array);
      for (i = 0; i < arraylen; i++)
      {
        jobj_array_item = json_object_array_get_idx(jobj_array, i);
        json_parse_item(jobj_array_item, odas_array, i);
      }
      break;
    }
  }
return;
}


void json_parse_item(json_object * jobj, odas_data * odas_array, const int index)
{
  enum json_type type;

  json_object_object_foreach(jobj, key, val)
  {

    type = json_object_get_type(val);
    switch (type)
    {
    case json_type_double:
      if (!strcmp(key, "x"))
      {
        odas_array[index].x = json_object_get_double(val);
	printf("x (%d): %d  ",index, odas_array[index].x);
      }
      else if (!strcmp(key, "y"))
      {
          odas_array[index].y = json_object_get_double(val);
	printf("y (%d): %2.2f  ",index, odas_array[index].y);
      }
      else if (!strcmp(key, "activity"))
      {
          odas_array[index].activity = json_object_get_double(val);
	printf("E (%d): %2.2f  ",index, odas_array[index].activity);
      }
      break;
    case json_type_int:
      if (!strcmp(key, "freq"))
      {
        odas_array[index].frequency = json_object_get_int(val);
	printf("freq (%d): %d  \n",index, odas_array[index].frequency);
      }      
      break;
    }
  }
}



