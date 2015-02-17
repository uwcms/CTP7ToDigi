#include <iostream>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <fstream>

using namespace std;

#include "RunNumberFactory.hh"

/*
 * This is a simple parser to get to first run number listed on the runsummary.html page
 * Eventually can be integrated into somewhere else?
 *
 */

int RunNumberFactory::RunSummary()
{
	string str1 ("?RUN=");
	string str2 ("RunSummary.html");
	ifstream inFile("RunSummary.html");
	if (!inFile) {
		cerr << "File RunSummary.html not found." << endl;
		return -1;
	}
	// Using getline() to read one line at a time.
	string line;
	while (getline(inFile, line)) {
		if (line.empty()) continue;
		size_t found = line.rfind(str1);
		if (found!=string::npos){
			unsigned found_begin = line.find("RUN=");
			string str_a=line.substr(found_begin+4);
			unsigned found_end = str_a.find("&DB");
			string value_str = str_a.substr(0,found_end);
			const char *cstr = value_str.c_str();
			int value =atol(cstr);
			return value;
		}
	}
	return -1;
}

