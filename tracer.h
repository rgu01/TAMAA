#ifndef TRACE_H
#define TRACE_H

#include "common.h"
#include "agent.h"

int parseTrace(istream& file, vector<Agent>& agents);
int parse(int argc, char *argv[]);
void loadIF(istream& file);
int loadTrace(istream& file);
void clear();

#endif