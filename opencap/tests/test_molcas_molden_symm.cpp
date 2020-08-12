/*
 * test_molcas_molden_symm.cpp
 *
 *  Created on: Aug 8, 2020
 *      Author: JG
 */
#include <iostream>
#include <vector>
#include "System.h"
#include <chrono>
#include <map>
#include <iomanip>
#include <algorithm>
#include <string>
#include <cmath>
#include <limits>
#include "InputParser.h"
#include "Atom.h"
#include "opencap_exception.h"
#include "CAP.h"
#include "keywords.h"

int main()
{
	std::string input_filename = "../tests/openmolcas/test_molcas_molden_symm.in";
	try
	{
		std::tuple<System,std::map<std::string,std::string>> inp_data = parse_input(input_filename);
		std::map<std::string,std::string> params = std::get<1>(inp_data);
		if(params["jobtype"] == "perturb_cap")
		{
			CAP pc(std::get<0>(inp_data),get_params_for_field(params,"perturb_cap"));
			pc.run();
		}
	}
	catch (exception& e)
	{
		opencap_handle_exception(e);
		return 1;
	}
	return 0;
}
