#include "std_inc.h"

struct quarantine {
	char* old_path;
	int old_perm;
	char* filename;
} quarantine = {
	old_path = NULL;
	old_perm = 0;
	filename = NULL;
};
