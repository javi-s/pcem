#ifndef _NVR_MM58174_H_
#define _NVR_MM58174_H_
void nvr_mm58174_init();

extern int enable_sync;

extern int nvr_dosave;

void mm58174_loadnvr();
void mm58174_savenvr();

void mm58174_nvr_recalc();

FILE *nvrfopen(char *fn, char *mode);

#endif /* _NVR_MM58174_H_ */
