#ifndef HELP_H
#define HELP_H

#include "msxgl.h"

typedef const c8* (*HelpSummaryFn)(u8 index);

void Help_Init(void);
void Help_Show(const c8* topic, u8 topicLen, bool screen,
	u8 summaryCount, HelpSummaryFn summaryAt);

#endif