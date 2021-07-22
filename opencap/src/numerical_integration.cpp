#include "numerical_integration.h"
#include "gto_ordering.h"
#include <iostream>
#include "BasisSet.h"
#include <Eigen/Dense>
#include "opencap_exception.h"
#include "cap_types.h"
#include "grid_radial.h"
#include "bragg.h"
#include <numgrid.h>
#include <omp.h>
#include <iomanip>


void compute_cap_on_grid(Eigen::MatrixXd &cap_mat,BasisSet bs,double* x, double* y, double* z, 
double *grid_w, double *cap_values, int num_points, 
std::function<std::vector<double>(std::vector<double> &, std::vector<double> &, 
		std::vector<double> &, std::vector<double> &, int)> &cap_func)
{
	Eigen::VectorXd cap_vals; Eigen::MatrixXd bf_values;
	bf_values = Eigen::MatrixXd::Zero(num_points,bs.num_carts());
    std::vector<double> x_vec(x,x+num_points);
    std::vector<double> y_vec(y,y+num_points);
    std::vector<double> z_vec(z,z+num_points);
    std::vector<double> w_vec(grid_w,grid_w+num_points);
    std::vector<double> cap_vec = cap_func(x_vec,y_vec,z_vec,w_vec,num_points);
    //std::cout << "CAP vec:" << cap_vec << std::endl;
	size_t bf_idx = 0;
	for(size_t i=0;i<bs.basis.size();i++)
	{
		Shell my_shell = bs.basis[i];
		std::vector<std::array<size_t,3>> order = opencap_carts_ordering(my_shell.l);
		for(size_t j=0;j<my_shell.num_carts();j++)
		{
			std::array<size_t,3> cart = order[j];
			my_shell.evaluate_on_grid(x,y,z,num_points,cart[0],cart[1],cart[2],bf_values.col(bf_idx));
			bf_idx++;
		}
	}
    cap_vals = Eigen::Map<Eigen::VectorXd>(cap_vec.data(),cap_vec.size());
	Eigen::MatrixXd bf_prime;
	bf_prime =  Eigen::MatrixXd::Zero(num_points,bs.num_carts());
	for(size_t i=0;i<bf_prime.cols();i++)
		bf_prime.col(i) = bf_values.col(i).array()*cap_vals.array();
	cap_mat+=bf_prime.transpose()*bf_values;
}

void integrate_cap_numerical(Eigen::MatrixXd &cap_mat, BasisSet bs, std::vector<Atom> atoms, 
double radial_precision, size_t angular_points,
std::function<std::vector<double>(std::vector<double> &, std::vector<double> &, 
std::vector<double> &, std::vector<double> &, int)> &cap_func)
{
    std::cout << "Calculating CAP matrix in AO basis using " << std::to_string(omp_get_max_threads()) << " threads." << std::endl;
    std::cout << std::setprecision(2) << std::scientific  << "Radial precision: " << radial_precision
              << " Angular points: " << angular_points << std::endl;
	size_t num_atoms = atoms.size();
    double x_coords_bohr[num_atoms];
	double y_coords_bohr[num_atoms];
	double z_coords_bohr[num_atoms];
	int nuc_charges[num_atoms];
    #pragma omp parallel for
	for(size_t i=0;i<num_atoms;i++)
	{
		x_coords_bohr[i]=atoms[i].coords[0];
		y_coords_bohr[i]=atoms[i].coords[1];
		z_coords_bohr[i]=atoms[i].coords[2];
		nuc_charges[i]=atoms[i].Z;
		if (atoms[i].Z==0)
			nuc_charges[i]=1; //choose bragg radius for H for ghost atoms
	}
    int min_num_angular_points = angular_points;
    int max_num_angular_points = angular_points;
	for(size_t i=0;i<num_atoms;i++)
	{
        // check parameters
        double alpha_max = bs.alpha_max(atoms[i]);
        std::vector<double> alpha_min = bs.alpha_min(atoms[i]);
        double r_inner = get_r_inner(radial_precision,
                                     alpha_max * 2.0); // factor 2.0 to match DIRAC
        double h = std::numeric_limits<float>::max();
        double r_outer = 0.0;
        for (int l = 0; l <= bs.max_L(); l++)
        {
            if (alpha_min[l] > 0.0)
            {
                r_outer =
                std::max(r_outer,
                         get_r_outer(radial_precision,
                                     alpha_min[l],
                                     l,
                                     4.0 * get_bragg_angstrom(nuc_charges[i])));
                if(r_outer < r_inner)
                {
                    opencap_throw("Error: r_outer < r_inner, grid cannot be allocated for this basis.");
                    //std::cout << "Setting alpha min[l] to 0.01" << std::endl; 
                    //alpha_min[l]=0.01;
                }
                else
                {
                    h = std::min(h,
                                 get_h(radial_precision, l, 0.1 * (r_outer - r_inner)));
                    if(r_outer < h)
                    {
                        opencap_throw("Error: r_outer < h, grid cannot be allocated for this basis.");
                    }
                }
            }
        }
        context_t *context = numgrid_new_atom_grid(radial_precision,
		                                 min_num_angular_points,
		                                 max_num_angular_points,
		                                 nuc_charges[i],
		                                 bs.alpha_max(atoms[i]),
		                                 bs.max_L(),
		                                 alpha_min.data());
		int num_points = numgrid_get_num_grid_points(context);
        double *grid_x_bohr = new double[num_points];
        double *grid_y_bohr = new double[num_points];
        double *grid_z_bohr = new double[num_points];
        double *grid_w = new double[num_points];
		double *cap_values = new double[num_points];
        numgrid_get_grid(  context,
                           num_atoms,
                           i,
                           x_coords_bohr,
                           y_coords_bohr,
                           z_coords_bohr,
                           nuc_charges,
                           grid_x_bohr,
                           grid_y_bohr,
                           grid_z_bohr,
                           grid_w);
        int num_radial_points = numgrid_get_num_radial_grid_points(context);
		compute_cap_on_grid(cap_mat,bs,grid_x_bohr,grid_y_bohr,grid_z_bohr,grid_w,cap_values,
		num_points,cap_func);
	}
}