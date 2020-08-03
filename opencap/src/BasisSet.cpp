#include <vector>
#include "BasisSet.h"
#include "Atom.h"
#include <math.h>
#include <algorithm>
#include <string>
#include <map>
#include <list>
#include "Shell.h"
#include "utils.h"
#include <array>
#include <limits>
#include <iostream>
#include "BasisSetParser.h"
#include "opencap_exception.h"
#include "gto_ordering.h"


bool shell_id::operator==(const shell_id& other)
{
	return l == other.l && shell_num == other.shell_num && ctr==other.ctr;
}

bool bf_id::operator==(const bf_id& other)
{
	return l == other.l && shell_num == other.shell_num && ctr==other.ctr && m==other.m;
}

void shell_id::print()
{
	std::cout << ctr << "," << shell_num << "," << l << std::endl;
}

void bf_id::print()
{
	std::cout << ctr << "," << shell_num << "," << l << "," << m << std::endl;
}

BasisSet::BasisSet(std::vector<Atom> geometry,std::map<std::string, std::string> parameters)
{
	std::string basis_name = parameters["basis_file"];
	std::string cart_bf = parameters["cart_bf"];
	for(auto atm:geometry)
		centers.push_back(atm.coords);
	BasisSetParser parser (parameters);
	build_basis_set(geometry,parser.read_basis());
	Nshells = basis.size();
	Nbasis = calc_basis_size();
}

BasisSet::BasisSet()
{
	Nshells=0;
	Nbasis=0;
}

long BasisSet::get_index_of_shell_id(shell_id id)
{
	for(size_t i=0;i<shell_ids.size();i++)
	{
		shell_id cur_id = shell_ids[i];
		if(id.ctr==cur_id.ctr && id.shell_num==cur_id.shell_num && id.l==cur_id.l)
			return i;
	}
	return -1;
}

void BasisSet::add_shell(Shell new_shell)
{
	//first figure out which atom it belongs to
	size_t atm_idx = 0;
	while(new_shell.origin!=centers[atm_idx]&&atm_idx<centers.size())
		atm_idx++;
	if(atm_idx>=centers.size())
		opencap_throw("Error: Invalid center.");
	shell_id id(atm_idx+1,1,new_shell.l);
	if(!new_shell.pure)
		id.l*=-1;
	while(get_index_of_shell_id(id)!=-1)
		id.shell_num++;
	basis.push_back(new_shell);
	shell_ids.push_back(id);
	if(new_shell.pure)
	{
		std::vector<int> harmonic_order = opencap_harmonic_ordering(new_shell);
		for (auto m:harmonic_order)
			bf_ids.push_back(bf_id(id,m));
	}
	else
	{
		std::vector<std::array<size_t,3>> carts_order = opencap_carts_ordering(new_shell);
		size_t counter = 0;
		int idx = -1*carts_order.size()/2;
		while(counter<carts_order.size())
		{
			bf_ids.push_back(bf_id(id,idx));
			counter++;
			idx++;
		}
	}
}

size_t BasisSet::calc_basis_size()
{
	size_t num_functions = 0;
	for(size_t i=0;i<Nshells;i++)
	{
		num_functions+=basis[i].num_bf();
	}
	return num_functions;
}

size_t BasisSet::num_carts()
{
	size_t num_carts = 0;
	for(auto&shell:basis)
	{
		num_carts += shell.num_carts();
	}
	return num_carts;
}

void BasisSet::build_basis_set(std::vector<Atom> geometry,map<string,std::vector<Shell>> all_shells)
{

	for (Atom atm: geometry)
	{
		std::vector<Shell> my_shells = all_shells[atm.symbol];
		for(const auto&shell:my_shells)
		{
			auto new_shell = shell;
			new_shell.update_coords(atm.coords);
			add_shell(new_shell);
		}
	}
	Nshells = basis.size();
	Nbasis = calc_basis_size();
}

int BasisSet::max_L()
{
	int l_max = 0;
	for(const auto&shell:basis)
	{
		if(shell.l > l_max)
			l_max = shell.l;
	}
	return l_max;
}

double BasisSet::alpha_max(Atom &atm)
{
	double max_val = 0;
	std::vector<Shell> shells = shells_on_center(atm);
	for(const auto&shell:shells)
	{
		for(const double exp: shell.exps)
		{
			if(exp>max_val)
				max_val=exp;
		}
	}
	return max_val;
}

std::vector<double> BasisSet::alpha_min(Atom &atm)
{
	std::vector<Shell> shells = shells_on_center(atm);
	std::vector<double> min_alpha(max_L()+1,0.0);
	for(const auto&shell:shells)
	{
		for(const double exp: shell.exps)
		{
			if(exp<min_alpha[shell.l] || min_alpha[shell.l]==0.0)
				min_alpha[shell.l]=exp;
		}
	}
	return min_alpha;
}

std::vector<Shell> BasisSet::shells_on_center(Atom &atm)
{
	std::vector<Shell> shells;
	for(const auto&shell:basis)
	{
		if(shell.origin[0]==atm.coords[0] && shell.origin[1]==atm.coords[1] && shell.origin[2]==atm.coords[2])
			shells.push_back(shell);
	}
	return shells;
}

void BasisSet::normalize()
{
	Nshells = basis.size();
	Nbasis = calc_basis_size();
	for(size_t i=0;i<basis.size();i++)
		basis[i].normalize();
}

void BasisSet::print_basis()
{
	for(auto id:bf_ids)
		id.print();
}


