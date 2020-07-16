 /*! \file InputParser.h
     \brief Class for parsing OpenCAP input files.
 */
#ifndef INPUTPARSER_H_
#define INPUTPARSER_H_
#include <map>
#include <tuple>
#include <string>
#include "System.h"


/** Parses an input file.
 * \param input_file: OpenCAP input file
 * \return A tuple, 1st element is the geometry, second element is the parameters map
*/
std::tuple<System,std::map<std::string,std::string>> parse_input(std::string input_file);
/** Parses the geometry section.
 * \param input_file: OpenCAP input file
 * \return Geometry of molecular system
 */
std::vector<Atom> parse_geometry(std::string input_file);
/** Parses a section of the input file.
 * \param input_file: OpenCAP input file
 * \param parameters: Parameters map
 * \param section_name: Name of section
 */
void parse_section(std::string input_file,std::map<std::string,std::string> &parameters,
		std::string section_name);

#endif /* INPUTPARSER_H_ */
