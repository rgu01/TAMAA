#ifndef TRACE_H
#define TRACE_H

#include "common.h"
#include "agent.h"

int test(istream& file, vector<Agent>& agents);
int parse(int argc, char *argv[]);
void loadIF(istream& file);
void loadTrace(istream& file);
void clear();

#endif