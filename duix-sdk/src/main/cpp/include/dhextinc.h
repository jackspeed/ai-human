
#ifndef GJ_EXTINC
#define GJ_EXTINC
#include "dhextend.h"

#ifdef __cplusplus
extern "C" {
#endif


ext_model_t*  load_funasrext(char* cfg);
ext_model_t*  load_chatggmlext(char* cfg);
ext_model_t*  load_piperext(char* cfg);
ext_model_t* load_msasrext(char* cfg);

ext_model_t* load_aliasrext(char* cfg);

#ifdef __cplusplus
}
#endif

#endif
